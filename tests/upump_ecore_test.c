/*
 * Copyright (C) 2012 OpenHeadend S.A.R.L.
 *
 * Authors: Christophe Massiot
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/** @file
 * @short unit tests for upump manager with ecore event loop
 */

#undef NDEBUG

#include <upipe/upump.h>
#include <upump-ecore/upump_ecore.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include <Ecore.h>

static uint64_t timeout = UINT64_C(27000000); /* 1 s */
static const char *padding = "This is an initialized bit of space used to pad sufficiently !";
/* This is an arbitrarily large number that is just supposed to be bigger than
 * the buffer space of a pipe. */
#define MIN_READ (128*1024)

static int pipefd[2];
static struct upump_mgr *mgr;
static struct upump *write_idler;
static struct upump *read_timer;
static struct upump *write_watcher;
static struct upump *read_watcher;
static ssize_t bytes_written = 0, bytes_read = 0;

static void write_idler_cb(struct upump *unused)
{
    ssize_t ret = write(pipefd[1], padding, strlen(padding) + 1);
    if (ret == -1 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
        printf("write idler blocked\n");
        upump_mgr_sink_block(mgr);
        assert(upump_start(write_watcher));
        assert(upump_start(read_timer));
    } else {
        assert(ret != -1);
        bytes_written += ret;
    }
}

static void write_watcher_cb(struct upump *unused)
{
    printf("write watcher passed\n");
    upump_mgr_sink_unblock(mgr);
    assert(upump_stop(write_watcher));
}

static void read_timer_cb(struct upump *unused)
{
    printf("read timer passed\n");
    assert(upump_start(read_watcher));
    /* The timer is automatically stopped */
}

static void read_watcher_cb(struct upump *unused)
{
    char buffer[strlen(padding) + 1];
    ssize_t ret = read(pipefd[0], buffer, strlen(padding) + 1);
    assert(ret != -1);
    bytes_read += ret;
    if (bytes_read > MIN_READ) {
        printf("read watcher passed\n");
        upump_stop(write_idler);
        upump_stop(read_watcher);
    }
}

int main(int argc, char **argv)
{
    long flags;
    assert(ecore_init());
    mgr = upump_ecore_mgr_alloc();
    assert(mgr != NULL);

    /* Create a pipe with non-blocking write */
    assert(pipe(pipefd) != -1);
    flags = fcntl(pipefd[1], F_GETFL);
    assert(flags != -1);
    flags |= O_NONBLOCK;
    assert(fcntl(pipefd[1], F_SETFL, flags) != -1);

    /* Create watchers */
    write_idler = upump_alloc_idler(mgr, write_idler_cb, NULL, true);
    assert(write_idler != NULL);
    write_watcher = upump_alloc_fd_write(mgr, write_watcher_cb, NULL, false,
                                         pipefd[1]);
    assert(write_watcher != NULL);
    read_timer = upump_alloc_timer(mgr, read_timer_cb, NULL, false, timeout, 0);
    assert(read_timer != NULL);
    read_watcher = upump_alloc_fd_read(mgr, read_watcher_cb, NULL, false,
                                       pipefd[0]);
    assert(read_watcher != NULL);

    /* Start tests */
    assert(upump_start(write_idler));

    ecore_main_loop_begin();
    assert(bytes_read);
    assert(bytes_read == bytes_written);

    /* Clean up */
    upump_free(write_idler);
    upump_free(write_watcher);
    upump_free(read_timer);
    upump_free(read_watcher);
    upump_mgr_release(mgr);

    ecore_shutdown();

    return 0;
}