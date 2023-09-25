/*
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

#include "internal.h"
#include "libavutil/opt.h"
#include "libavformat/avformat.h"

/**
 * @think3r context 结构体代码详解
 * @brief 典型的 context 的使用方式
 *
 * @param avctx    [out]    : 内部 alloc, init, 最后赋值
 * @param iformat  [in/out] : 可外部指定/内部自定义(NULL)
 * @param format   [in]     : 格式名称
 * @return int
 */
int ff_alloc_input_device_context(AVFormatContext **avctx, AVInputFormat *iformat, const char *format)
{
    AVFormatContext *s;
    int ret = 0;

    *avctx = NULL;
    if (!iformat && !format)
        return AVERROR(EINVAL);
    if (!(s = avformat_alloc_context()))    // context 的 alloc 和赋值
        return AVERROR(ENOMEM);

    if (!iformat)   // iformat 未指定, find
        iformat = av_find_input_format(format);
    if (!iformat || !iformat->priv_class || !AV_IS_INPUT_DEVICE(iformat->priv_class->category)) {
        ret = AVERROR(EINVAL);
        goto error;
    }
    s->iformat = iformat;   // 找到对应 format 后进行赋值, NOTE: `iformat` 都是 `const` 的.
    if (s->iformat->priv_data_size > 0) {   // 申请 `s->iformat` 的私有数据 xxxContext
        s->priv_data = av_mallocz(s->iformat->priv_data_size);
        if (!s->priv_data) {
            ret = AVERROR(ENOMEM);
            goto error;
        }
        if (s->iformat->priv_class) {   // 通过 `s->iformat->priv_class` 对 `s->priv_data` 赋初值
            *(const AVClass**)s->priv_data= s->iformat->priv_class;     // xxxContext 的第一个变量必须为 `AVClass`
            av_opt_set_defaults(s->priv_data);  // 赋初值
        }
    } else
        s->priv_data = NULL;

    *avctx = s;
    return 0;
  error:
    avformat_free_context(s);
    return ret;
}
