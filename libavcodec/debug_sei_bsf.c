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
#include <stdio.h>
#include <time.h>

#include "bsf.h"
#include "bsf_internal.h"
#include "libavcodec/packet.h"

#include "libavutil/log.h"
#include "libavutil/opt.h"

enum DumpFreq {
    DUMP_FREQ_KEYFRAME,
    DUMP_FREQ_ALL,
};

typedef struct DebugSeiContext {
    const AVClass *class;
    AVPacket pkt;
    int freq;

    uint64_t u64Cnt;
    int32_t  needInitCtx;
    char     sei_user_data[1024 * 12];
    uint32_t sei_user_data_len;
    char     infoData[1024];
    FILE *   pOutFile;
} DebugSeiContext;

static int debug_sei_init(AVBSFContext *ctx)
{
    DebugSeiContext *s = ctx->priv_data;
    s->u64Cnt = 0;
    s->needInitCtx = 1;

    av_log(NULL, AV_LOG_INFO, "[%s %d] \n", __func__, __LINE__);

    s->pOutFile = fopen("/tmp/seiOutput.txt", "wb+");
    if (s->pOutFile == NULL) {
        av_log(NULL, AV_LOG_ERROR, "[%s %d] error! open File failed!\n", __func__, __LINE__);
        return -1;
    }

    return 0;
}

static void debug_sei_close(AVBSFContext *ctx)
{
    DebugSeiContext *s = ctx->priv_data;
    if (s && s->pOutFile) {
        fclose(s->pOutFile);
    }

    return;
}

static int findSeiData(DebugSeiContext * pCtx, AVPacket *pkt)
{
    char * pData = pkt->data;
    uint32_t startIdx = 0;
    uint32_t endIdx = 0;
    uint64_t pts = 0, dts = 0;

    if (pkt->size <= 12) {
        return 0;
    }

    memset((void *)pCtx->sei_user_data, 0x00, sizeof(pCtx->sei_user_data));

    for (uint32_t idx = 0; idx < pkt->size; idx++) {
        if (pData[idx] == 'm' && pData[idx + 1] == 'g' && pData[idx + 2] == '-' && pData[idx + 3] == 's') {
            if (!startIdx) {
                startIdx = idx;
                startIdx += strlen("mg-sei-user-data");
            }
        }

        if (pData[idx] == 't' && pData[idx + 1] == 'a' && pData[idx + 2] == '"' && pData[idx + 3] == '}') {
            if (!endIdx) {
                endIdx = idx + 4;
            }
        }
    }

    pCtx->sei_user_data_len = FFMIN(endIdx - startIdx, sizeof(pCtx->sei_user_data));
    memcpy((void *)pCtx->sei_user_data, pData + startIdx, pCtx->sei_user_data_len);

    pts = av_rescale_q(pkt->pts, pkt->time_base, av_make_q(1, 1000));
    dts = av_rescale_q(pkt->dts, pkt->time_base, av_make_q(1, 1000));
    snprintf(pCtx->infoData, sizeof(pCtx->infoData), "%" PRIu64 " dts/pts:[%" PRIu64 " - %" PRIu64  "]  ", pCtx->u64Cnt, pts, dts);

    fwrite(pCtx->infoData, strlen(pCtx->infoData), 1, pCtx->pOutFile);
    fwrite(pCtx->sei_user_data, pCtx->sei_user_data_len, 1, pCtx->pOutFile);
    fwrite("\n", 1, 1, pCtx->pOutFile);

    av_log(NULL, AV_LOG_VERBOSE, "[%s %d] len:[%d], %s\n", __func__, __LINE__, pCtx->sei_user_data_len, pCtx->sei_user_data);

    return 0;
}

static int debug_sei(AVBSFContext *ctx, AVPacket *outPkt)
{
    DebugSeiContext * pBSFCtx = (DebugSeiContext *)ctx->priv_data;
    AVPacket *inPkt = &pBSFCtx->pkt;
    int ret = 0;

    ret = ff_bsf_get_packet_ref(ctx, inPkt);
    if (ret < 0)
        return ret;

    pBSFCtx->u64Cnt++;

    if (pBSFCtx->u64Cnt % 100 == 0) {
        av_log(NULL, AV_LOG_DEBUG, "think3r --> dts/pts:[%" PRIu64 " - %" PRIu64  "], streanIdx:[%d]\n", inPkt->dts, inPkt->pts, inPkt->stream_index);
        if (inPkt->side_data_elems) {
            av_log(NULL, AV_LOG_INFO, "[%s %d] side_data_elems:[%d], type:[%d]\n", __func__, __LINE__, inPkt->side_data_elems, inPkt->side_data[0].type);
            av_log(NULL, AV_LOG_INFO, "%s", inPkt->side_data[0].data);
        }
    }

    findSeiData(pBSFCtx, inPkt);

    ret = av_new_packet(outPkt, inPkt->size);
    if (ret < 0)
        goto fail;

    ret = av_packet_copy_props(outPkt, inPkt);
    if (ret < 0) {
        av_packet_unref(outPkt);
        goto fail;
    }
    memcpy(outPkt->data, inPkt->data, inPkt->size);

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
    .close          = debug_sei_close,
};
