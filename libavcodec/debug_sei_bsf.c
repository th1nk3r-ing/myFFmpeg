/*
 * copyright (c) 2006 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdint.h>
#include <string.h>

#include "bsf.h"
#include "bsf_internal.h"

#include "libavutil/log.h"
#include "libavutil/opt.h"

enum DumpFreq {
    DUMP_FREQ_KEYFRAME,
    DUMP_FREQ_ALL,
};

typedef struct DebugSeiContext {
    uint64_t u64Cnt;
    const AVClass *class;
    AVPacket pkt;
    int freq;
} DebugSeiContext;

static int debug_sei_init(AVBSFContext *ctx)
{
    DebugSeiContext *s = ctx->priv_data;
    s->u64Cnt = 0;

    av_log(NULL, AV_LOG_INFO, "[%s %d] \n", __func__, __LINE__);
    return 0;
}

static int debug_sei(AVBSFContext *ctx, AVPacket *out)
{
    DebugSeiContext *s = ctx->priv_data;
    AVPacket *inPkt = &s->pkt;
    int ret = 0;

    ret = ff_bsf_get_packet_ref(ctx, inPkt);
    if (ret < 0)
        return ret;

    s->u64Cnt++;

    // ff_cbs_sei_find_message

    if (s->u64Cnt % 100 == 0) {
        av_log(NULL, AV_LOG_DEBUG, "think3r --> dts/pts:[%" PRIu64 " - %" PRIu64  "], streanIdx:[%d]\n", inPkt->dts, inPkt->pts, inPkt->stream_index);
        if (inPkt->side_data_elems) {
            av_log(NULL, AV_LOG_INFO, "[%s %d] side_data_elems:[%d], type:[%d]\n", __func__, __LINE__, inPkt->side_data_elems, inPkt->side_data[0].type);
            av_log(NULL, AV_LOG_INFO, "%s", inPkt->side_data[0].data);
        }
    }

#if 1
    if (ctx->par_in->extradata &&
        (s->freq == DUMP_FREQ_ALL ||
         (s->freq == DUMP_FREQ_KEYFRAME && inPkt->flags & AV_PKT_FLAG_KEY)) &&
         (inPkt->size < ctx->par_in->extradata_size ||
          memcmp(inPkt->data, ctx->par_in->extradata, ctx->par_in->extradata_size))) {
        if (inPkt->size >= INT_MAX - ctx->par_in->extradata_size) {
            ret = AVERROR(ERANGE);
            goto fail;
        }

        ret = av_new_packet(out, inPkt->size + ctx->par_in->extradata_size);
        if (ret < 0)
            goto fail;

        ret = av_packet_copy_props(out, inPkt);
        if (ret < 0) {
            av_packet_unref(out);
            goto fail;
        }

        memcpy(out->data, ctx->par_in->extradata, ctx->par_in->extradata_size);
        memcpy(out->data + ctx->par_in->extradata_size, inPkt->data, inPkt->size);
    } else {
        av_packet_move_ref(out, inPkt);
    }
#endif

fail:
    av_packet_unref(inPkt);

    return ret;
}

#define OFFSET(x) offsetof(DebugSeiContext, x)
#define FLAGS (AV_OPT_FLAG_VIDEO_PARAM|AV_OPT_FLAG_BSF_PARAM)
static const AVOption options[] = {
    { "freq", "When to dump extradata", OFFSET(freq), AV_OPT_TYPE_INT,
        { .i64 = DUMP_FREQ_KEYFRAME }, DUMP_FREQ_KEYFRAME, DUMP_FREQ_ALL, FLAGS, "freq" },
        { "k",        NULL, 0, AV_OPT_TYPE_CONST, { .i64 = DUMP_FREQ_KEYFRAME }, .flags = FLAGS, .unit = "freq" },
        { "keyframe", NULL, 0, AV_OPT_TYPE_CONST, { .i64 = DUMP_FREQ_KEYFRAME }, .flags = FLAGS, .unit = "freq" },
        { "e",        NULL, 0, AV_OPT_TYPE_CONST, { .i64 = DUMP_FREQ_ALL      }, .flags = FLAGS, .unit = "freq" },
        { "all",      NULL, 0, AV_OPT_TYPE_CONST, { .i64 = DUMP_FREQ_ALL      }, .flags = FLAGS, .unit = "freq" },
    { NULL },
};

static const AVClass debug_sei_class = {
    .class_name = "debug_sei_bsf",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

const FFBitStreamFilter ff_debug_sei_bsf = {
    .p.name         = "debug_sei",
    .p.priv_class   = &debug_sei_class,
    .priv_data_size = sizeof(DebugSeiContext),
    .init           = debug_sei_init,
    .filter         = debug_sei,
};
