/*
 * Copyright (C) 2013 OpenHeadend S.A.R.L.
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
 * @short probe catching need_upump_mgr events and providing a given upump
 * manager
 */

#include <upipe/ubase.h>
#include <upipe/upump.h>
#include <upipe/uprobe.h>
#include <upipe/uprobe_helper_uprobe.h>
#include <upipe/uprobe_upump_mgr.h>
#include <upipe/upipe.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/** @This is a super-set of the uprobe structure with additional local
 * members. */
struct uprobe_upump_mgr {
    /** pointer to upump_mgr to provide */
    struct upump_mgr *upump_mgr;

    /** structure exported to modules */
    struct uprobe uprobe;
};

UPROBE_HELPER_UPROBE(uprobe_upump_mgr, uprobe);

/** @internal @This catches events thrown by pipes.
 *
 * @param uprobe pointer to probe
 * @param upipe pointer to pipe throwing the event
 * @param event event thrown
 * @param args optional event-specific parameters
 * @return true if the event was caught and handled
 */
static bool uprobe_upump_mgr_throw(struct uprobe *uprobe, struct upipe *upipe,
                                   enum uprobe_event event, va_list args)
{
    struct uprobe_upump_mgr *uprobe_upump_mgr =
        uprobe_upump_mgr_from_uprobe(uprobe);

    if (event == UPROBE_NEED_UPUMP_MGR && uprobe_upump_mgr->upump_mgr != NULL) {
        if (unlikely(!upipe_set_upump_mgr(upipe,
                                          uprobe_upump_mgr->upump_mgr))) {
            upipe_warn(upipe, "probe couldn't set upump manager");
            return false;
        }
        return true;
    }

    return false;
}

/** @This allocates a new uprobe_upump_mgr structure.
 *
 * @param next next probe to test if this one doesn't catch the event
 * @param upump_mgr upump manager to provide to pipes
 * @return pointer to uprobe, or NULL in case of error
 */
struct uprobe *uprobe_upump_mgr_alloc(struct uprobe *next,
                                      struct upump_mgr *upump_mgr)
{
    struct uprobe_upump_mgr *uprobe_upump_mgr =
        malloc(sizeof(struct uprobe_upump_mgr));
    if (unlikely(uprobe_upump_mgr == NULL))
        return NULL;
    struct uprobe *uprobe = uprobe_upump_mgr_to_uprobe(uprobe_upump_mgr);
    uprobe_upump_mgr->upump_mgr = upump_mgr;
    if (upump_mgr != NULL)
        upump_mgr_use(upump_mgr);
    uprobe_init(uprobe, uprobe_upump_mgr_throw, next);
    return uprobe;
}

/** @This frees a uprobe_upump_mgr structure.
 *
 * @param uprobe structure to free
 * @return next probe
 */
struct uprobe *uprobe_upump_mgr_free(struct uprobe *uprobe)
{
    struct uprobe *next = uprobe->next;
    struct uprobe_upump_mgr *uprobe_upump_mgr =
        uprobe_upump_mgr_from_uprobe(uprobe);
    upump_mgr_release(uprobe_upump_mgr->upump_mgr);
    free(uprobe_upump_mgr);
    return next;
}

/** @This changes the upump_mgr set by this probe.
 *
 * @param uprobe pointer to probe
 * @param upump_mgr new upump manager to provide to pipes
 */
void uprobe_upump_mgr_set(struct uprobe *uprobe, struct upump_mgr *upump_mgr)
{
    struct uprobe_upump_mgr *uprobe_upump_mgr =
        uprobe_upump_mgr_from_uprobe(uprobe);
    if (uprobe_upump_mgr->upump_mgr != NULL)
        upump_mgr_release(uprobe_upump_mgr->upump_mgr);
    uprobe_upump_mgr->upump_mgr = upump_mgr;
    if (upump_mgr != NULL)
        upump_mgr_use(upump_mgr);
}