/**
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * The trace point event definitions for the the lttng tracer.
 */

#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER smx_lttng

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./lttng_tp.h"

#if !defined(_LTTNG_TP_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define _LTTNG_TP_H

#include <lttng/tracepoint.h>

TRACEPOINT_EVENT_CLASS(
    smx_lttng,
    smx_msg,
    TP_ARGS(
        int, id_msg,
        const char*, name_net
    ),
    TP_FIELDS(
        ctf_integer(int, id_msg, id_msg)
        ctf_string(name_net, name_net)
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_msg,
    msg_create,
    TP_ARGS(
        int, id_msg,
        const char*, name_net
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_msg,
    msg_copy_start,
    TP_ARGS(
        int, id_msg,
        const char*, name_net
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_msg,
    msg_copy_end,
    TP_ARGS(
        int, id_msg,
        const char*, name_net
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_msg,
    msg_destroy,
    TP_ARGS(
        int, id_msg,
        const char*, name_net
    )
)

TRACEPOINT_EVENT_CLASS(
    smx_lttng,
    smx_ch,
    TP_ARGS(
        int, id_ch,
        int, id_net,
        const char*, name_ch,
        int, id_msg,
        int, count
    ),
    TP_FIELDS(
        ctf_integer(int, id_ch, id_ch)
        ctf_integer(int, id_net, id_net)
        ctf_string(name_ch, name_ch)
        ctf_integer(int, id_msg, id_msg)
        ctf_integer(int, count, count)
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_ch,
    ch_read,
    TP_ARGS(
        int, id_ch,
        int, id_net,
        const char*, name_ch,
        int, id_msg,
        int, count
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_ch,
    ch_read_block,
    TP_ARGS(
        int, id_ch,
        int, id_net,
        const char*, name_ch,
        int, id_msg,
        int, count
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_ch,
    ch_read_collector,
    TP_ARGS(
        int, id_ch,
        int, id_net,
        const char*, name_ch,
        int, id_msg,
        int, count
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_ch,
    ch_read_collector_block,
    TP_ARGS(
        int, id_ch,
        int, id_net,
        const char*, name_ch,
        int, id_msg,
        int, count
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_ch,
    ch_write,
    TP_ARGS(
        int, id_ch,
        int, id_net,
        const char*, name_ch,
        int, id_msg,
        int, count
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_ch,
    ch_write_block,
    TP_ARGS(
        int, id_ch,
        int, id_net,
        const char*, name_ch,
        int, id_msg,
        int, count
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_ch,
    ch_write_collector,
    TP_ARGS(
        int, id_ch,
        int, id_net,
        const char*, name_ch,
        int, id_msg,
        int, count
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_ch,
    ch_overwrite,
    TP_ARGS(
        int, id_ch,
        int, id_net,
        const char*, name_ch,
        int, id_msg,
        int, count
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_ch,
    ch_dismiss,
    TP_ARGS(
        int, id_ch,
        int, id_net,
        const char*, name_ch,
        int, id_msg,
        int, count
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_ch,
    ch_duplicate,
    TP_ARGS(
        int, id_ch,
        int, id_net,
        const char*, name_ch,
        int, id_msg,
        int, count
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_ch,
    ch_dl_miss_src,
    TP_ARGS(
        int, id_ch,
        int, id_net,
        const char*, name_ch,
        int, id_msg,
        int, count
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_ch,
    ch_dl_miss_src_cp,
    TP_ARGS(
        int, id_ch,
        int, id_net,
        const char*, name_ch,
        int, id_msg,
        int, count
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_ch,
    ch_dl_miss_sink,
    TP_ARGS(
        int, id_ch,
        int, id_net,
        const char*, name_ch,
        int, id_msg,
        int, count
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_ch,
    ch_tt_miss_src,
    TP_ARGS(
        int, id_ch,
        int, id_net,
        const char*, name_ch,
        int, id_msg,
        int, count
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_ch,
    ch_tt_miss_src_cp,
    TP_ARGS(
        int, id_ch,
        int, id_net,
        const char*, name_ch,
        int, id_msg,
        int, count
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_ch,
    ch_tt_miss_sink,
    TP_ARGS(
        int, id_ch,
        int, id_net,
        const char*, name_ch,
        int, id_msg,
        int, count
    )
)

TRACEPOINT_EVENT_CLASS(
    smx_lttng,
    smx_net,
    TP_ARGS(
        int, id_net,
        const char*, name_net
    ),
    TP_FIELDS(
        ctf_integer(int, id_net, id_net)
        ctf_string(name_net, name_net)
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_net,
    net_start,
    TP_ARGS(
        int, id_net,
        const char*, name_net
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_net,
    net_start_impl,
    TP_ARGS(
        int, id_net,
        const char*, name_net
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_net,
    net_end_impl,
    TP_ARGS(
        int, id_net,
        const char*, name_net
    )
)

TRACEPOINT_EVENT_INSTANCE(
    smx_lttng,
    smx_net,
    net_end,
    TP_ARGS(
        int, id_net,
        const char*, name_net
    )
)

#endif /* _LTTNG_TP */

#include <lttng/tracepoint-event.h>
