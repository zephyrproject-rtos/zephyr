/**
 * Copyright (c) 2015-2016 Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @author Mark Greer
 * @brief Greybus Audio Device Class Protocol Driver
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

//#include <config.h>
//#include <list.h>
#include <device.h>
//#include <device_audio_board.h>
//#include <device_codec.h>
//#include <device_i2s.h>
//#include <ring_buf.h>
//#include <util.h>
//#include <wdog.h>
//#include <list.h>
#include <greybus/types.h>
#include <greybus/greybus.h>
#include <greybus/debug.h>
#include <unipro/unipro.h>

#include <sys/byteorder.h>

#include "audio-gb.h"

#define GB_AUDIO_VERSION_MAJOR              0
#define GB_AUDIO_VERSION_MINOR              1

#define GB_AUDIO_TX_RING_BUF_PAD            2
#define GB_AUDIO_RX_RING_BUF_PAD            2

#define GB_AUDIO_FLAG_PCM_SET               BIT(0)
#define GB_AUDIO_FLAG_TX_DATA_SIZE_SET      BIT(1)
#define GB_AUDIO_FLAG_TX_ACTIVE             BIT(2)
#define GB_AUDIO_FLAG_TX_STARTED            BIT(3)
#define GB_AUDIO_FLAG_TX_STOPPING           BIT(4)
#define GB_AUDIO_FLAG_RX_DATA_SIZE_SET      BIT(5)
#define GB_AUDIO_FLAG_RX_ACTIVE             BIT(6)
#define GB_AUDIO_FLAG_RX_STARTED            BIT(7)

#define GB_AUDIO_IS_CONFIGURED(_dai, _type)                             \
            (((_dai)->flags & GB_AUDIO_FLAG_PCM_SET) &&                 \
             ((_dai)->flags & GB_AUDIO_FLAG_##_type##_DATA_SIZE_SET))

struct gb_audio_info { /* One per audio Bundle */
    bool                    initialized;
    uint16_t                mgmt_cport;
    struct device           *codec_dev;
    struct list_head        dai_list;   /* list of gb_audio_dai_info structs */
    struct list_head        list;       /* next gb_audio_info struct */
};

struct gb_audio_dai_info {
    uint32_t                flags;
    uint16_t                data_cport;
    unsigned int            dai_idx;
    struct device           *i2s_dev;
    uint32_t                format;
    uint32_t                rate;
    uint8_t                 channels;
    uint8_t                 sig_bits;
    unsigned int            sample_size;
    unsigned int            sample_freq;

    unsigned int            tx_rb_count;

    struct ring_buf         *tx_rb;
    unsigned int            tx_data_size;
    unsigned int            tx_samples_per_msg;
    uint8_t                 *tx_dummy_data;
    sem_t                   tx_stop_sem;

    struct ring_buf         *rx_rb;
    unsigned int            rx_data_size;
    unsigned int            rx_samples_per_msg;

    struct gb_audio_info    *info;      /* parent gb_audio_info struct */
    struct list_head        list;       /* next gb_audio_dai_info struct */
};

static LIST_DECLARE(gb_audio_info_list);

static struct gb_audio_info *gb_audio_find_info(uint16_t mgmt_cport)
{
    struct gb_audio_info *info;
    struct list_head *iter;

    list_foreach(&gb_audio_info_list, iter) {
        info = list_entry(iter, struct gb_audio_info, list);

        if (info->mgmt_cport == mgmt_cport) {
            return info;
        }
    }

    return NULL;
}

static struct gb_audio_dai_info *gb_audio_get_dai(struct gb_audio_info *info,
                                                  uint16_t data_cport)
{
    struct gb_audio_dai_info *dai;
    struct list_head *iter;

    if (!info) {
        return NULL;
    }

    list_foreach(&info->dai_list, iter) {
        dai = list_entry(iter, struct gb_audio_dai_info, list);

        if (dai->data_cport == data_cport) {
            return dai;
        }
    }

    return NULL;
}

static struct gb_audio_dai_info *gb_audio_get_dai_by_idx(
                                                    struct gb_audio_info *info,
                                                    unsigned int dai_idx)
{
    struct gb_audio_dai_info *dai;
    struct list_head *iter;

    if (!info) {
        return NULL;
    }

    list_foreach(&info->dai_list, iter) {
        dai = list_entry(iter, struct gb_audio_dai_info, list);

        if (dai->dai_idx == dai_idx) {
            return dai;
        }
    }

    return NULL;
}

static struct gb_audio_dai_info *gb_audio_find_dai(uint16_t data_cport)
{
    struct gb_audio_info *info;
    struct gb_audio_dai_info *dai;
    struct list_head *iter;

    list_foreach(&gb_audio_info_list, iter) {
        info = list_entry(iter, struct gb_audio_info, list);

        dai = gb_audio_get_dai(info, data_cport);
        if (dai) {
            return dai;
        }
    }

    return NULL;
}

static void gb_audio_report_event(struct gb_audio_dai_info *dai, uint32_t event)
{
    struct gb_operation *operation;
    struct gb_audio_streaming_event_request *request;

    operation = gb_operation_create(dai->info->mgmt_cport,
                                    GB_AUDIO_TYPE_STREAMING_EVENT,
                                    sizeof(*request));
    if (!operation) {
        return;
    }

    request = gb_operation_get_request_payload(operation);
    request->data_cport = dai->data_cport;
    request->event = event;

    /* TODO: What to do when this fails? */
    gb_operation_send_request_nowait(operation, NULL, false);
    gb_operation_destroy(operation);
}

static uint8_t gb_audio_protocol_version_handler(struct gb_operation *operation)
{
    struct gb_audio_version_response *response;

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    response->major = GB_AUDIO_VERSION_MAJOR;
    response->minor = GB_AUDIO_VERSION_MINOR;

    return GB_OP_SUCCESS;
}

static uint8_t gb_audio_get_topology_size_handler(
                                                struct gb_operation *operation)
{
    struct gb_audio_get_topology_size_response *response;
    struct gb_audio_info *info;
    uint16_t size;
    int ret;

    info = gb_audio_find_info(operation->cport);
    if (!info) {
        return GB_OP_INVALID;
    }

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    ret = device_codec_get_topology_size(info->codec_dev, &size);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    response->size = sys_cpu_to_le16(size);

    return GB_OP_SUCCESS;
}

static uint8_t gb_audio_get_topology_handler(struct gb_operation *operation)
{
    struct gb_audio_get_topology_response *response;
    struct gb_audio_info *info;
    uint16_t size;
    int ret;

    info = gb_audio_find_info(operation->cport);
    if (!info) {
        return GB_OP_INVALID;
    }

    ret = device_codec_get_topology_size(info->codec_dev, &size);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    response = gb_operation_alloc_response(operation, size);
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    ret = device_codec_get_topology(info->codec_dev, &response->topology);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    return GB_OP_SUCCESS;
}

static uint8_t gb_audio_get_control_handler(struct gb_operation *operation)
{
    struct gb_audio_get_control_request *request =
                gb_operation_get_request_payload(operation);
    struct gb_audio_get_control_response *response;
    struct gb_audio_info *info;
    int ret;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    info = gb_audio_find_info(operation->cport);
    if (!info) {
        return GB_OP_INVALID;
    }

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    ret = device_codec_get_control(info->codec_dev, request->control_id,
                                   request->index, &response->value);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    return GB_OP_SUCCESS;
}

static uint8_t gb_audio_set_control_handler(struct gb_operation *operation)
{
    struct gb_audio_set_control_request *request =
                gb_operation_get_request_payload(operation);
    struct gb_audio_info *info;
    int ret;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    info = gb_audio_find_info(operation->cport);
    if (!info) {
        return GB_OP_INVALID;
    }

    ret = device_codec_set_control(info->codec_dev, request->control_id,
                                   request->index, &request->value);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    return GB_OP_SUCCESS;
}

static uint8_t gb_audio_enable_widget_handler(struct gb_operation *operation)
{
    struct gb_audio_enable_widget_request *request =
                gb_operation_get_request_payload(operation);
    struct gb_audio_info *info;
    int ret;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    info = gb_audio_find_info(operation->cport);
    if (!info) {
        return GB_OP_INVALID;
    }

    ret = device_codec_enable_widget(info->codec_dev, request->widget_id);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    return GB_OP_SUCCESS;
}

static uint8_t gb_audio_disable_widget_handler(struct gb_operation *operation)
{
    struct gb_audio_disable_widget_request *request =
                gb_operation_get_request_payload(operation);
    struct gb_audio_info *info;
    int ret;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    info = gb_audio_find_info(operation->cport);
    if (!info) {
        return GB_OP_INVALID;
    }

    ret = device_codec_disable_widget(info->codec_dev, request->widget_id);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    return GB_OP_SUCCESS;
}

static uint8_t gb_audio_get_pcm_handler(struct gb_operation *operation)
{
    struct gb_audio_get_pcm_request *request =
                gb_operation_get_request_payload(operation);
    struct gb_audio_get_pcm_response *response;
    struct gb_audio_info *info;
    struct gb_audio_dai_info *dai;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    info = gb_audio_find_info(operation->cport);
    if (!info) {
        return GB_OP_INVALID;
    }

    dai = gb_audio_get_dai(info, sys_le16_to_cpu(request->data_cport));
    if (!dai) {
        return GB_OP_INVALID;
    }

    if (!(dai->flags & GB_AUDIO_FLAG_PCM_SET)) {
        return GB_OP_PROTOCOL_BAD;
    }

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    response->format = sys_cpu_to_le32(dai->format);
    response->rate = sys_cpu_to_le32(dai->rate);
    response->channels = dai->channels;
    response->sig_bits = dai->sig_bits;

    return GB_OP_SUCCESS;
}

static int gb_audio_gb_to_i2s_format(uint32_t gb_format, uint32_t *i2s_format,
                                     unsigned int *bytes)
{
    int ret = 0;

    switch (gb_format) {
    case GB_AUDIO_PCM_FMT_S8:
    case GB_AUDIO_PCM_FMT_U8:
        //*i2s_format = DEVICE_I2S_PCM_FMT_8;
        *bytes = 1;
        break;
    case GB_AUDIO_PCM_FMT_S16_LE:
    case GB_AUDIO_PCM_FMT_S16_BE:
    case GB_AUDIO_PCM_FMT_U16_LE:
    case GB_AUDIO_PCM_FMT_U16_BE:
        //*i2s_format = DEVICE_I2S_PCM_FMT_16;
        *bytes = 2;
        break;
    case GB_AUDIO_PCM_FMT_S24_LE:
    case GB_AUDIO_PCM_FMT_S24_BE:
    case GB_AUDIO_PCM_FMT_U24_LE:
    case GB_AUDIO_PCM_FMT_U24_BE:
        //*i2s_format = DEVICE_I2S_PCM_FMT_24;
        *bytes = 3;
        break;
    case GB_AUDIO_PCM_FMT_S32_LE:
    case GB_AUDIO_PCM_FMT_S32_BE:
    case GB_AUDIO_PCM_FMT_U32_LE:
    case GB_AUDIO_PCM_FMT_U32_BE:
        //*i2s_format = DEVICE_I2S_PCM_FMT_32;
        *bytes = 4;
        break;
    default:
        ret = -EINVAL;
    }

    return ret;
}

static int gb_audio_convert_rate(uint32_t gb_rate, uint32_t *i2s_rate,
                                 unsigned int *freq)
{
    switch (gb_rate) {
    case GB_AUDIO_PCM_RATE_5512:
        //*i2s_rate = DEVICE_I2S_PCM_RATE_5512;
        *freq = 5512;
        break;
    case GB_AUDIO_PCM_RATE_8000:
        //*i2s_rate = DEVICE_I2S_PCM_RATE_8000;
        *freq = 8000;
        break;
    case GB_AUDIO_PCM_RATE_11025:
        //*i2s_rate = DEVICE_I2S_PCM_RATE_11025;
        *freq = 11025;
        break;
    case GB_AUDIO_PCM_RATE_16000:
        //*i2s_rate = DEVICE_I2S_PCM_RATE_16000;
        *freq = 16000;
        break;
    case GB_AUDIO_PCM_RATE_22050:
        //*i2s_rate = DEVICE_I2S_PCM_RATE_22050;
        *freq = 22050;
        break;
    case GB_AUDIO_PCM_RATE_32000:
        //*i2s_rate = DEVICE_I2S_PCM_RATE_32000;
        *freq = 32000;
        break;
    case GB_AUDIO_PCM_RATE_44100:
        //*i2s_rate = DEVICE_I2S_PCM_RATE_44100;
        *freq = 44100;
        break;
    case GB_AUDIO_PCM_RATE_48000:
        //*i2s_rate = DEVICE_I2S_PCM_RATE_48000;
        *freq = 48000;
        break;
    case GB_AUDIO_PCM_RATE_64000:
        //*i2s_rate = DEVICE_I2S_PCM_RATE_64000;
        *freq = 64000;
        break;
    case GB_AUDIO_PCM_RATE_88200:
        //*i2s_rate = DEVICE_I2S_PCM_RATE_88200;
        *freq = 88200;
        break;
    case GB_AUDIO_PCM_RATE_96000:
        //*i2s_rate = DEVICE_I2S_PCM_RATE_96000;
        *freq = 96000;
        break;
    case GB_AUDIO_PCM_RATE_176400:
        //*i2s_rate = DEVICE_I2S_PCM_RATE_176400;
        *freq = 176400;
        break;
    case GB_AUDIO_PCM_RATE_192000:
        //*i2s_rate = DEVICE_I2S_PCM_RATE_192000;
        *freq = 192000;
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static int gb_audio_determine_protocol(struct device_codec_dai *codec_dai,
                                       struct device_i2s_dai *i2s_dai)
{
//    if ((codec_dai->protocol & DEVICE_CODEC_PROTOCOL_I2S) &&
//        (i2s_dai->protocol & DEVICE_I2S_PROTOCOL_I2S)) {
//        codec_dai->protocol = DEVICE_CODEC_PROTOCOL_I2S;
//        i2s_dai->protocol = DEVICE_I2S_PROTOCOL_I2S;
//    } else if ((codec_dai->protocol & DEVICE_CODEC_PROTOCOL_LR_STEREO) &&
//               (i2s_dai->protocol & DEVICE_I2S_PROTOCOL_LR_STEREO)) {
//        codec_dai->protocol = DEVICE_CODEC_PROTOCOL_LR_STEREO;
//        i2s_dai->protocol = DEVICE_I2S_PROTOCOL_LR_STEREO;
//    } else if ((codec_dai->protocol & DEVICE_CODEC_PROTOCOL_PCM) &&
//               (i2s_dai->protocol & DEVICE_I2S_PROTOCOL_PCM)) {
//        codec_dai->protocol = DEVICE_CODEC_PROTOCOL_PCM;
//        i2s_dai->protocol = DEVICE_I2S_PROTOCOL_PCM;
//    } else {
//        return -EINVAL;
//    }

    return 0;
}

static int gb_audio_determine_wclk_polarity(struct device_codec_dai *codec_dai,
                                            struct device_i2s_dai *i2s_dai)
{
//    if ((codec_dai->wclk_polarity & DEVICE_CODEC_POLARITY_NORMAL) &&
//        (i2s_dai->wclk_polarity & DEVICE_I2S_POLARITY_NORMAL)) {
//        codec_dai->wclk_polarity = DEVICE_CODEC_POLARITY_NORMAL;
//        i2s_dai->wclk_polarity = DEVICE_I2S_POLARITY_NORMAL;
//    } else if ((codec_dai->wclk_polarity & DEVICE_CODEC_POLARITY_REVERSED) &&
//               (i2s_dai->wclk_polarity & DEVICE_I2S_POLARITY_REVERSED)) {
//        codec_dai->wclk_polarity = DEVICE_CODEC_POLARITY_REVERSED;
//        i2s_dai->wclk_polarity = DEVICE_I2S_POLARITY_REVERSED;
//    } else {
//        return -EINVAL;
//    }

    return 0;
}

static int gb_audio_determine_wclk_change_edge(uint8_t codec_clk_role,
                                            struct device_codec_dai *codec_dai,
                                            struct device_i2s_dai *i2s_dai)
{
//    if ((codec_clk_role == DEVICE_CODEC_ROLE_MASTER) &&
//        (codec_dai->wclk_change_edge & DEVICE_CODEC_EDGE_FALLING) &&
//        (i2s_dai->wclk_change_edge & DEVICE_I2S_EDGE_RISING)) {
//        codec_dai->wclk_change_edge = DEVICE_CODEC_EDGE_FALLING;
//        i2s_dai->wclk_change_edge = DEVICE_I2S_EDGE_RISING;
//    } if ((codec_clk_role == DEVICE_CODEC_ROLE_SLAVE) &&
//          (codec_dai->wclk_change_edge & DEVICE_CODEC_EDGE_RISING) &&
//          (i2s_dai->wclk_change_edge & DEVICE_I2S_EDGE_FALLING)) {
//          codec_dai->wclk_change_edge = DEVICE_CODEC_EDGE_RISING;
//          i2s_dai->wclk_change_edge = DEVICE_I2S_EDGE_FALLING;
//    } else {
//        return -EINVAL;
//    }

    return 0;
}

static int gb_audio_determine_data_edges(struct device_codec_dai *codec_dai,
                                         struct device_i2s_dai *i2s_dai)
{
//    if ((codec_dai->data_tx_edge & DEVICE_CODEC_EDGE_FALLING) &&
//        (i2s_dai->data_rx_edge & DEVICE_I2S_EDGE_RISING)) {
//        codec_dai->data_tx_edge = DEVICE_CODEC_EDGE_FALLING;
//        i2s_dai->data_rx_edge = DEVICE_I2S_EDGE_RISING;
//    } else if ((codec_dai->data_tx_edge & DEVICE_CODEC_EDGE_RISING) &&
//               (i2s_dai->data_rx_edge & DEVICE_I2S_EDGE_FALLING)) {
//        codec_dai->data_tx_edge = DEVICE_CODEC_EDGE_RISING;
//        i2s_dai->data_rx_edge = DEVICE_I2S_EDGE_FALLING;
//    } else {
//        return -EINVAL;
//    }

//    if ((codec_dai->data_rx_edge & DEVICE_CODEC_EDGE_RISING) &&
//        (i2s_dai->data_tx_edge & DEVICE_I2S_EDGE_FALLING)) {
//        codec_dai->data_rx_edge = DEVICE_CODEC_EDGE_RISING;
//        i2s_dai->data_tx_edge = DEVICE_I2S_EDGE_FALLING;
//    } else if ((codec_dai->data_rx_edge & DEVICE_CODEC_EDGE_FALLING) &&
//               (i2s_dai->data_tx_edge & DEVICE_I2S_EDGE_RISING)) {
//        codec_dai->data_rx_edge = DEVICE_CODEC_EDGE_FALLING;
//        i2s_dai->data_tx_edge = DEVICE_I2S_EDGE_RISING;
//    } else {
//        return -EINVAL;
//    }

    return 0;
}

static int gb_audio_set_config(struct gb_audio_dai_info *dai,
                               uint8_t codec_clk_role,
                               struct device_codec_pcm *codec_pcm,
                               struct device_i2s_pcm *i2s_pcm)
{
//    struct device_codec_dai codec_dai;
//    struct device_i2s_dai i2s_dai;
//    uint8_t i2s_clk_role;
//    int ret;
//
//    if (codec_clk_role == DEVICE_CODEC_ROLE_MASTER) {
//        ret = device_codec_get_caps(dai->info->codec_dev, dai->dai_idx,
//                                    codec_clk_role, codec_pcm, &codec_dai);
//        if (ret) {
//            return ret;
//        }
//
//        /*
//         * When codec_clk_role is DEVICE_CODEC_ROLE_MASTER,
//         * device_codec_get_caps() sets codec_dai.mclk_freq.
//         */
//        i2s_dai.mclk_freq = codec_dai.mclk_freq;
//
//        ret = device_i2s_get_caps(dai->i2s_dev, DEVICE_I2S_ROLE_SLAVE, i2s_pcm,
//                                  &i2s_dai);
//        if (ret) {
//            return ret;
//        }
//
//        i2s_clk_role = DEVICE_I2S_ROLE_SLAVE;
//    } else {
//        ret = device_i2s_get_caps(dai->i2s_dev, DEVICE_I2S_ROLE_MASTER, i2s_pcm,
//                                  &i2s_dai);
//        if (ret) {
//            return ret;
//        }
//
//        /*
//         * When i2s_clk_role is DEVICE_I2S_ROLE_MASTER,
//         * device_i2s_get_caps() sets i2s_dai.mclk_freq.
//         */
//        codec_dai.mclk_freq = i2s_dai.mclk_freq;
//
//        ret = device_codec_get_caps(dai->info->codec_dev, dai->dai_idx,
//                                    codec_clk_role, codec_pcm, &codec_dai);
//        if (ret) {
//            return ret;
//        }
//
//        i2s_clk_role = DEVICE_I2S_ROLE_MASTER;
//    }
//
//    ret = gb_audio_determine_protocol(&codec_dai, &i2s_dai);
//    if (ret) {
//        return ret;
//    }
//
//    ret = gb_audio_determine_wclk_polarity(&codec_dai, &i2s_dai);
//    if (ret) {
//        return ret;
//    }
//
//    ret = gb_audio_determine_wclk_change_edge(codec_clk_role, &codec_dai,
//                                              &i2s_dai);
//    if (ret) {
//        return ret;
//    }
//
//    ret = gb_audio_determine_data_edges(&codec_dai, &i2s_dai);
//    if (ret) {
//        return ret;
//    }
//
//    ret = device_codec_set_config(dai->info->codec_dev, dai->dai_idx,
//                                  codec_clk_role, codec_pcm, &codec_dai);
//    if (ret) {
//        return ret;
//    }
//
//    ret = device_i2s_set_config(dai->i2s_dev, i2s_clk_role, i2s_pcm, &i2s_dai);
//    if (ret) {
//        return ret;
//    }

    return 0;
}

static int gb_audio_config_connection(struct gb_audio_dai_info *dai,
                                      uint32_t format, uint32_t rate,
                                      uint8_t channels, uint8_t sig_bits)
{
//    struct device_codec_pcm codec_pcm;
//    struct device_i2s_pcm i2s_pcm;
//    unsigned int bytes, freq;
//    int ret;
//
//    codec_pcm.format = format;
//    codec_pcm.rate = rate;
//    codec_pcm.channels = channels;
//    codec_pcm.sig_bits = sig_bits;
//
//    ret = gb_audio_gb_to_i2s_format(format, &i2s_pcm.format, &bytes);
//    if (ret) {
//        return ret;
//    }
//
//    ret = gb_audio_convert_rate(rate, &i2s_pcm.rate, &freq);
//    if (ret) {
//        return ret;
//    }
//
//    i2s_pcm.channels = channels;
//
//    ret = gb_audio_set_config(dai, DEVICE_CODEC_ROLE_MASTER, &codec_pcm,
//                              &i2s_pcm);
//    if (ret) {
//        ret = gb_audio_set_config(dai, DEVICE_CODEC_ROLE_SLAVE, &codec_pcm,
//                                  &i2s_pcm);
//        if (ret) {
//            return ret;
//        }
//    }
//
//    dai->sample_size = bytes * channels;
//    dai->sample_freq = freq;

    return 0;
}

static uint8_t gb_audio_set_pcm_handler(struct gb_operation *operation)
{
    struct gb_audio_set_pcm_request *request =
                gb_operation_get_request_payload(operation);
    struct gb_audio_info *info;
    struct gb_audio_dai_info *dai;
    uint32_t format, rate;
    uint8_t channels, sig_bits;
    int ret;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    info = gb_audio_find_info(operation->cport);
    if (!info) {
        return GB_OP_INVALID;
    }

    dai = gb_audio_get_dai(info, sys_le16_to_cpu(request->data_cport));
    if (!dai) {
        return GB_OP_INVALID;
    }

    if ((dai->flags & GB_AUDIO_FLAG_TX_ACTIVE) ||
        (dai->flags & GB_AUDIO_FLAG_RX_ACTIVE)) {
        return GB_OP_PROTOCOL_BAD;
    }

    format = sys_le32_to_cpu(request->format);
    rate = sys_le32_to_cpu(request->rate);
    channels = request->channels;
    sig_bits = request->sig_bits;

    ret = gb_audio_config_connection(dai, format, rate, channels, sig_bits);
    if (ret) {
        dai->flags &= ~GB_AUDIO_FLAG_PCM_SET;
        return gb_errno_to_op_result(ret);
    }

    dai->format = format;
    dai->rate = rate;
    dai->channels = channels;
    dai->sig_bits = sig_bits;

    dai->flags |= GB_AUDIO_FLAG_PCM_SET;

    return GB_OP_SUCCESS;
}

static uint8_t gb_audio_set_tx_data_size_handler(struct gb_operation *operation)
{
    struct gb_audio_set_tx_data_size_request *request =
                gb_operation_get_request_payload(operation);
    struct gb_audio_info *info;
    struct gb_audio_dai_info *dai;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    info = gb_audio_find_info(operation->cport);
    if (!info) {
        return GB_OP_INVALID;
    }

    dai = gb_audio_get_dai(info, sys_le16_to_cpu(request->data_cport));
    if (!dai) {
        return GB_OP_INVALID;
    }

    if (!(dai->flags & GB_AUDIO_FLAG_PCM_SET) ||
        (dai->flags & GB_AUDIO_FLAG_TX_ACTIVE)) {
        return GB_OP_PROTOCOL_BAD;
    }

    dai->tx_data_size = sys_le16_to_cpu(request->size);

    if (dai->tx_data_size % dai->sample_size) {
        return GB_OP_INVALID;
    }

    dai->tx_samples_per_msg = dai->tx_data_size / dai->sample_size;
    dai->flags |= GB_AUDIO_FLAG_TX_DATA_SIZE_SET;

    return GB_OP_SUCCESS;
}

static uint8_t gb_audio_get_tx_delay_handler(struct gb_operation *operation)
{
    struct gb_audio_get_tx_delay_request *request =
                gb_operation_get_request_payload(operation);
    struct gb_audio_get_tx_delay_response *response;
    struct gb_audio_info *info;
    struct gb_audio_dai_info *dai;
    uint32_t codec_delay, i2s_delay;
    int ret;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    info = gb_audio_find_info(operation->cport);
    if (!info) {
        return GB_OP_INVALID;
    }

    dai = gb_audio_get_dai(info, sys_le16_to_cpu(request->data_cport));
    if (!dai) {
        return GB_OP_INVALID;
    }

    if (!GB_AUDIO_IS_CONFIGURED(dai, TX)) {
        return GB_OP_PROTOCOL_BAD;
    }

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    ret = device_codec_get_tx_delay(info->codec_dev, &codec_delay);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    ret = device_i2s_get_delay_transmitter(dai->i2s_dev, &i2s_delay);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    /* TODO: Determine delay from this driver and add in */
    response->delay = sys_cpu_to_le32(codec_delay + i2s_delay);

    return GB_OP_SUCCESS;
}

static void gb_audio_i2s_tx(struct gb_audio_dai_info *dai, uint8_t *data)
{
    ring_buf_reset(dai->tx_rb);

    /*
     * Unfortunately, we have to copy the data because the unipro
     * susbystem reuses the buffer immediately and the data may
     * not be sent out yet.
     */
    memcpy(ring_buf_get_tail(dai->tx_rb), data, dai->tx_data_size);

    ring_buf_put(dai->tx_rb, dai->tx_data_size);
    ring_buf_pass(dai->tx_rb);

    dai->tx_rb = ring_buf_get_next(dai->tx_rb);

    dai->tx_rb_count++;
}

enum device_i2s_event {
	DEVICE_I2S_EVENT_NONE,
	DEVICE_I2S_EVENT_RX_COMPLETE,
	DEVICE_I2S_EVENT_TX_COMPLETE,
	DEVICE_I2S_EVENT_UNDERRUN,
	DEVICE_I2S_EVENT_OVERRUN,
	DEVICE_I2S_EVENT_CLOCKING,
	DEVICE_I2S_EVENT_DATA_LEN,
	DEVICE_I2S_EVENT_UNSPECIFIED,
};

enum device_codec_event {
	DEVICE_CODEC_EVENT_NONE,
	DEVICE_CODEC_EVENT_UNSPECIFIED,
	DEVICE_CODEC_EVENT_UNDERRUN,
	DEVICE_CODEC_EVENT_OVERRUN,
	DEVICE_CODEC_EVENT_CLOCKING,
};

enum device_codec_jack_event {
	DEVICE_CODEC_JACK_EVENT_INSERTION,
	DEVICE_CODEC_JACK_EVENT_REMOVAL,
};

enum device_codec_button_event {
	DEVICE_CODEC_BUTTON_EVENT_PRESS,
	DEVICE_CODEC_BUTTON_EVENT_RELEASE,
};

/* Callback for low-level i2s transmit operations */
static void gb_audio_i2s_tx_cb(struct ring_buf *rb,
                               enum device_i2s_event event, void *arg)
{
    struct gb_audio_dai_info *dai = arg;
    uint32_t gb_event = 0;

    switch (event) {
    case DEVICE_I2S_EVENT_TX_COMPLETE:
        /* TODO: Replace with smarter underrun prevention */
        dai->tx_rb_count--;

        if (dai->tx_rb_count < 2) {
            if (!(dai->flags & GB_AUDIO_FLAG_TX_STOPPING)) {
                gb_audio_i2s_tx(dai, dai->tx_dummy_data);
            } else if (!dai->tx_rb_count) {
                sem_post(&dai->tx_stop_sem);
            }
        }

        break;
    case DEVICE_I2S_EVENT_UNDERRUN:
        gb_event = GB_AUDIO_STREAMING_EVENT_UNDERRUN;
        break;
    case DEVICE_I2S_EVENT_OVERRUN:
        gb_event = GB_AUDIO_STREAMING_EVENT_OVERRUN;
        break;
    case DEVICE_I2S_EVENT_CLOCKING:
        gb_event = GB_AUDIO_STREAMING_EVENT_CLOCKING;
        break;
    case DEVICE_I2S_EVENT_DATA_LEN:
        gb_event = GB_AUDIO_STREAMING_EVENT_DATA_LEN;
        break;
    case DEVICE_I2S_EVENT_UNSPECIFIED:
        gb_event = GB_AUDIO_STREAMING_EVENT_UNSPECIFIED;
        break;
    default:
        gb_event = GB_AUDIO_STREAMING_EVENT_INTERNAL_ERROR;
    }

    if (gb_event) {
        gb_audio_report_event(dai, gb_event);
        /* All driver error events halt streaming right now */
        gb_audio_report_event(dai, GB_AUDIO_STREAMING_EVENT_HALT);
    }
}

static uint8_t gb_audio_activate_tx_handler(struct gb_operation *operation)
{
    struct gb_audio_activate_tx_request *request =
                gb_operation_get_request_payload(operation);
    struct gb_audio_info *info;
    struct gb_audio_dai_info *dai;
    unsigned int entries;
    int ret;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    info = gb_audio_find_info(operation->cport);
    if (!info) {
        return GB_OP_INVALID;
    }

    dai = gb_audio_get_dai(info, sys_le16_to_cpu(request->data_cport));
    if (!dai) {
        return GB_OP_INVALID;
    }

    if (!GB_AUDIO_IS_CONFIGURED(dai, TX) ||
        (dai->flags & GB_AUDIO_FLAG_TX_ACTIVE)) {
        return GB_OP_PROTOCOL_BAD;
    }

    /* (rate / samples_per_msg) * (buffer_amount_us / 1,000,000) */
    entries = ((dai->sample_freq * GB_AUDIO_SAMPLE_BUFFER_MIN_US) /
               (dai->tx_samples_per_msg * 1000000)) + GB_AUDIO_TX_RING_BUF_PAD;

    dai->tx_rb = ring_buf_alloc_ring(entries, 0, dai->tx_data_size, 0, NULL,
                                     NULL, NULL);
    if (!dai->tx_rb) {
        return GB_OP_NO_MEMORY;
    }

    dai->tx_dummy_data = zalloc(dai->tx_data_size);
    if (!dai->tx_dummy_data) {
        ret = -ENOMEM;
        goto err_free_tx_rb;
    }

    /* Greybus i2s message receiver is local i2s transmitter */
    ret = device_i2s_prepare_transmitter(dai->i2s_dev, dai->tx_rb,
                                         gb_audio_i2s_tx_cb, dai);
    if (ret) {
        goto err_free_dummy_data;
    }

    dai->flags |= GB_AUDIO_FLAG_TX_ACTIVE;

    return GB_OP_SUCCESS;

err_free_dummy_data:
    free(dai->tx_dummy_data);
    dai->tx_dummy_data = NULL;
err_free_tx_rb:
    ring_buf_free_ring(dai->tx_rb, NULL, NULL);
    dai->tx_rb = NULL;

    return gb_errno_to_op_result(ret);
}

static uint8_t gb_audio_deactivate_tx_handler(struct gb_operation *operation)
{
    struct gb_audio_deactivate_tx_request *request =
                gb_operation_get_request_payload(operation);
    struct gb_audio_info *info;
    struct gb_audio_dai_info *dai;
    int flags;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    info = gb_audio_find_info(operation->cport);
    if (!info) {
        return GB_OP_INVALID;
    }

    dai = gb_audio_get_dai(info, sys_le16_to_cpu(request->data_cport));
    if (!dai) {
        return GB_OP_INVALID;
    }

    if (!(dai->flags & GB_AUDIO_FLAG_TX_ACTIVE)) {
        return GB_OP_PROTOCOL_BAD;
    }

    flags = irq_lock();

    dai->flags |= GB_AUDIO_FLAG_TX_STOPPING;

    if (dai->flags & GB_AUDIO_FLAG_TX_STARTED) {
        irq_unlock(flags);
        sem_wait(&dai->tx_stop_sem);
        device_i2s_stop_transmitter(dai->i2s_dev);
        dai->flags &= ~GB_AUDIO_FLAG_TX_STARTED;
    } else {
        irq_unlock(flags);
    }

    device_i2s_shutdown_transmitter(dai->i2s_dev);

    ring_buf_free_ring(dai->tx_rb, NULL, NULL);
    dai->tx_rb = NULL;
    free(dai->tx_dummy_data);
    dai->tx_dummy_data = NULL;

    dai->tx_rb_count = 0;

    dai->flags &= ~(GB_AUDIO_FLAG_TX_STOPPING | GB_AUDIO_FLAG_TX_ACTIVE);

    return GB_OP_SUCCESS;
}

static uint8_t gb_audio_set_rx_data_size_handler(struct gb_operation *operation)
{
    struct gb_audio_set_rx_data_size_request *request =
                gb_operation_get_request_payload(operation);
    struct gb_audio_info *info;
    struct gb_audio_dai_info *dai;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    info = gb_audio_find_info(operation->cport);
    if (!info) {
        return GB_OP_INVALID;
    }

    dai = gb_audio_get_dai(info, sys_le16_to_cpu(request->data_cport));
    if (!dai) {
        return GB_OP_INVALID;
    }

    if (!(dai->flags & GB_AUDIO_FLAG_PCM_SET) ||
        (dai->flags & GB_AUDIO_FLAG_RX_ACTIVE)) {
        return GB_OP_PROTOCOL_BAD;
    }

    dai->rx_data_size = sys_le16_to_cpu(request->size);

    if (dai->rx_data_size % dai->sample_size) {
        return GB_OP_INVALID;
    }

    dai->rx_samples_per_msg = dai->rx_data_size / dai->sample_size;
    dai->flags |= GB_AUDIO_FLAG_RX_DATA_SIZE_SET;

    return GB_OP_SUCCESS;
}

static uint8_t gb_audio_get_rx_delay_handler(struct gb_operation *operation)
{
    struct gb_audio_get_rx_delay_request *request =
                gb_operation_get_request_payload(operation);
    struct gb_audio_get_rx_delay_response *response;
    struct gb_audio_info *info;
    struct gb_audio_dai_info *dai;
    uint32_t codec_delay, i2s_delay;
    int ret;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    info = gb_audio_find_info(operation->cport);
    if (!info) {
        return GB_OP_INVALID;
    }

    dai = gb_audio_get_dai(info, sys_le16_to_cpu(request->data_cport));
    if (!dai) {
        return GB_OP_INVALID;
    }

    if (!GB_AUDIO_IS_CONFIGURED(dai, RX)) {
        return GB_OP_PROTOCOL_BAD;
    }

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    ret = device_codec_get_rx_delay(info->codec_dev, &codec_delay);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    ret = device_i2s_get_delay_receiver(dai->i2s_dev, &i2s_delay);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    /* TODO: Determine delay from this driver and add in */
    response->delay = sys_cpu_to_le32(codec_delay + i2s_delay);

    return GB_OP_SUCCESS;
}

static void gb_audio_send_data_cb(struct gb_operation *operation)
{
    struct ring_buf *rb = operation->priv_data;

    ring_buf_reset(rb);
    ring_buf_pass(rb);
}

static int gb_audio_send_data(struct gb_audio_dai_info *dai,
                              struct ring_buf *rb)
{
    struct gb_operation *operation;
    int ret;

    /*
     * TODO: Use a thread to pass buffers to greybus_core so i2s irq
     * isn't blocked too long (and we use the more than one entry in
     * the rx ring buffer.
     */
    operation = ring_buf_get_priv(rb);
    operation->priv_data = rb;

    ret = gb_operation_send_request_nowait(operation, gb_audio_send_data_cb,
                                           false);
    if (ret) {
        return ret;
    }

    return 0;
}

/* Callback for low-level i2s receive operations */
static void gb_audio_i2s_rx_cb(struct ring_buf *rb,
                               enum device_i2s_event event, void *arg)
{
    struct gb_audio_dai_info *dai = arg;
    uint32_t gb_event = 0;
    int ret;

    if (!(dai->flags & GB_AUDIO_FLAG_RX_STARTED)) {
        return;
    }

    switch (event) {
    case DEVICE_I2S_EVENT_NONE:
        return;
    case DEVICE_I2S_EVENT_RX_COMPLETE:
        ret = gb_audio_send_data(dai, rb);
        if (ret) {
            gb_audio_report_event(dai, gb_errno_to_op_result(ret));
            return;
        }
        break;
    case DEVICE_I2S_EVENT_UNDERRUN:
        gb_event = GB_AUDIO_STREAMING_EVENT_UNDERRUN;
        break;
    case DEVICE_I2S_EVENT_OVERRUN:
        gb_event = GB_AUDIO_STREAMING_EVENT_OVERRUN;
        break;
    case DEVICE_I2S_EVENT_CLOCKING:
        gb_event = GB_AUDIO_STREAMING_EVENT_CLOCKING;
        break;
    case DEVICE_I2S_EVENT_DATA_LEN:
        gb_event = GB_AUDIO_STREAMING_EVENT_DATA_LEN;
        break;
    case DEVICE_I2S_EVENT_UNSPECIFIED:
        gb_event = GB_AUDIO_STREAMING_EVENT_UNSPECIFIED;
        break;
    default:
        gb_event = GB_AUDIO_STREAMING_EVENT_INTERNAL_ERROR;
        break;
    }

    if (gb_event) {
        gb_audio_report_event(dai, gb_event);
        /* All driver error events halt streaming right now */
        gb_audio_report_event(dai, GB_AUDIO_STREAMING_EVENT_HALT);
    }
}

static int gb_audio_rb_alloc_gb_op(struct ring_buf *rb, void *arg)
{
    struct gb_audio_dai_info *dai = arg;
    struct gb_operation *operation;
    struct gb_audio_send_data_request *request;

    operation = gb_operation_create(dai->data_cport,
                                    GB_AUDIO_TYPE_SEND_DATA,
                                    sizeof(*request) + dai->rx_data_size);
    if (!operation) {
        return -ENOMEM;
    }

    request = gb_operation_get_request_payload(operation);
    request->timestamp = 0; /* TODO: Implement timestamp support */

    ring_buf_init(rb, request,
                  sizeof(struct gb_operation_hdr) + sizeof(*request),
                  dai->rx_data_size);

    ring_buf_set_priv(rb, operation);

    return 0;
}

static void gb_audio_rb_free_gb_op(struct ring_buf *rb, void *arg)
{
    gb_operation_destroy(ring_buf_get_priv(rb));
}

static uint8_t gb_audio_activate_rx_handler(struct gb_operation *operation)
{
    struct gb_audio_activate_rx_request *request =
                gb_operation_get_request_payload(operation);
    struct gb_audio_info *info;
    struct gb_audio_dai_info *dai;
    unsigned int entries;
    int ret;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    info = gb_audio_find_info(operation->cport);
    if (!info) {
        return GB_OP_INVALID;
    }

    dai = gb_audio_get_dai(info, sys_le16_to_cpu(request->data_cport));
    if (!dai) {
        return GB_OP_INVALID;
    }

    if (!GB_AUDIO_IS_CONFIGURED(dai, RX) ||
        (dai->flags & GB_AUDIO_FLAG_RX_ACTIVE)) {
        return GB_OP_PROTOCOL_BAD;
    }

    /* (sample_freq / samples_per_msg) * (buffer_amount_us / 1,000,000) */
    entries = ((dai->sample_freq * GB_AUDIO_SAMPLE_BUFFER_MIN_US) /
               (dai->rx_samples_per_msg * 1000000)) + GB_AUDIO_RX_RING_BUF_PAD;

    dai->rx_rb = ring_buf_alloc_ring(entries, 0, 0, 0,
                                     gb_audio_rb_alloc_gb_op,
                                     gb_audio_rb_free_gb_op, dai);
    if (!dai->rx_rb) {
        return gb_errno_to_op_result(ENOMEM);
    }

    /* Greybus i2s message transmitter is local i2s receiver */
    ret = device_i2s_prepare_receiver(dai->i2s_dev, dai->rx_rb,
                                      gb_audio_i2s_rx_cb, dai);
    if (ret) {
        goto err_free_rx_rb;
    }

    dai->flags |= (GB_AUDIO_FLAG_RX_ACTIVE | GB_AUDIO_FLAG_RX_STARTED);

    ret = device_i2s_start_receiver(dai->i2s_dev);
    if (ret) {
        goto err_shutdown_receiver;
    }

    return GB_OP_SUCCESS;

err_shutdown_receiver:
    device_i2s_shutdown_receiver(dai->i2s_dev);
err_free_rx_rb:
    ring_buf_free_ring(dai->rx_rb, gb_audio_rb_free_gb_op, dai);
    dai->rx_rb = NULL;

    return gb_errno_to_op_result(ret);
}

static uint8_t gb_audio_deactivate_rx_handler(struct gb_operation *operation)
{
    struct gb_audio_deactivate_rx_request *request =
                gb_operation_get_request_payload(operation);
    struct gb_audio_info *info;
    struct gb_audio_dai_info *dai;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    info = gb_audio_find_info(operation->cport);
    if (!info) {
        return GB_OP_INVALID;
    }

    dai = gb_audio_get_dai(info, sys_le16_to_cpu(request->data_cport));
    if (!dai) {
        return GB_OP_INVALID;
    }

    if (!(dai->flags & GB_AUDIO_FLAG_RX_ACTIVE)) {
        return GB_OP_PROTOCOL_BAD;
    }

    device_i2s_stop_receiver(dai->i2s_dev);
    device_i2s_shutdown_receiver(dai->i2s_dev);

    ring_buf_free_ring(dai->rx_rb, gb_audio_rb_free_gb_op, dai);
    dai->rx_rb = NULL;

    dai->flags &= ~(GB_AUDIO_FLAG_RX_ACTIVE | GB_AUDIO_FLAG_RX_STARTED);

    return GB_OP_SUCCESS;
}

static void gb_audio_codec_cb(unsigned int dai_idx,
                              enum device_codec_event event, void *arg)
{
    struct gb_audio_info *info = arg;
    struct gb_audio_dai_info *dai;
    uint32_t gb_event;

    dai = gb_audio_get_dai_by_idx(info, dai_idx);
    if (!dai) {
        return;
    }

    switch (event) {
    case DEVICE_CODEC_EVENT_NONE:
        return;
    case DEVICE_CODEC_EVENT_UNSPECIFIED: /* Catch-all */
         gb_event = GB_AUDIO_STREAMING_EVENT_UNSPECIFIED;
         break;
    case DEVICE_CODEC_EVENT_UNDERRUN:
         gb_event = GB_AUDIO_STREAMING_EVENT_UNDERRUN;
         break;
    case DEVICE_CODEC_EVENT_OVERRUN:
         gb_event = GB_AUDIO_STREAMING_EVENT_OVERRUN;
         break;
    case DEVICE_CODEC_EVENT_CLOCKING:
         gb_event = GB_AUDIO_STREAMING_EVENT_CLOCKING;
         break;
    default:
         return;
    }

    gb_audio_report_event(dai, gb_event);
}

static void gb_audio_codec_jack_event_cb(uint8_t widget_id, uint8_t widget_type,
                                         enum device_codec_jack_event event,
                                         void *arg)
{
    struct gb_audio_info *info = arg;
    struct gb_operation *operation;
    struct gb_audio_jack_event_request *request;
    uint32_t gb_event;

    operation = gb_operation_create(info->mgmt_cport, GB_AUDIO_TYPE_JACK_EVENT,
                                    sizeof(*request));
    if (!operation) {
        return;
    }

    switch (event) {
    case DEVICE_CODEC_JACK_EVENT_INSERTION:
         gb_event = GB_AUDIO_JACK_EVENT_INSERTION;
         break;
    case DEVICE_CODEC_JACK_EVENT_REMOVAL:
         gb_event = GB_AUDIO_JACK_EVENT_REMOVAL;
         break;
    default:
        return;
    }

    request = gb_operation_get_request_payload(operation);
    request->widget_id = widget_id;
    request->widget_type = widget_type;
    request->event = gb_event;

    /* TODO: What to do when this fails? */
    gb_operation_send_request_nowait(operation, NULL, false);
    gb_operation_destroy(operation);
}

static void gb_audio_codec_button_event_cb(uint8_t widget_id,
                                           uint8_t button_id,
                                           enum device_codec_button_event event,
                                           void *arg)
{
    struct gb_audio_info *info = arg;
    struct gb_operation *operation;
    struct gb_audio_button_event_request *request;
    uint32_t gb_event;

    operation = gb_operation_create(info->mgmt_cport,
                                    GB_AUDIO_TYPE_BUTTON_EVENT,
                                    sizeof(*request));
    if (!operation) {
        return;
    }

    switch (event) {
    case DEVICE_CODEC_BUTTON_EVENT_PRESS:
         gb_event = GB_AUDIO_BUTTON_EVENT_PRESS;
         break;
    case DEVICE_CODEC_BUTTON_EVENT_RELEASE:
         gb_event = GB_AUDIO_BUTTON_EVENT_RELEASE;
         break;
    default:
        return;
    }

    request = gb_operation_get_request_payload(operation);
    request->widget_id = widget_id;
    request->button_id = button_id;
    request->event = gb_event;

    /* TODO: What to do when this fails? */
    gb_operation_send_request_nowait(operation, NULL, false);
    gb_operation_destroy(operation);
}

static void gb_audio_alloc_info_list(void)
{
    struct gb_audio_info *info;
    struct gb_audio_dai_info *dai;
    struct device *dev;
    unsigned int i, j, bundle_count, dai_count;
    unsigned int codec_dev_id, i2s_dev_id;
    int ret;

    if (!list_is_empty(&gb_audio_info_list)) {
        return;
    }

    //dev = device_open(DEVICE_TYPE_AUDIO_BOARD_HW, 0);
    dev = NULL;
    if (!dev) {
        return;
    }

    ret = device_audio_board_get_bundle_count(dev, &bundle_count);
    if (ret) {
        goto err_close;
    }

    for (i = 0; i < bundle_count; i++) {
        info = zalloc(sizeof(*info));
        if (!info) {
            continue;
        }

        list_init(&info->dai_list);
        list_init(&info->list);

        ret = device_audio_board_get_mgmt_cport(dev, i, &info->mgmt_cport);
        if (ret) {
            free(info);
            continue;
        }

        ret = device_audio_board_get_codec_dev_id(dev, i, &codec_dev_id);
        if (ret) {
            free(info);
            continue;
        }

        //info->codec_dev = device_open(DEVICE_TYPE_CODEC_HW, codec_dev_id);
        info->codec_dev = NULL;
        if (!info->codec_dev) {
            free(info);
            continue;
        }

        ret = device_audio_board_get_dai_count(dev, i, &dai_count);
        if (ret) {
            device_close(info->codec_dev);
            free(info);
            continue;
        }

        for (j = 0; j < dai_count; j++) {
            dai = zalloc(sizeof(*dai));
            if (!dai) {
                continue;
            }

            dai->info = info;
            list_init(&dai->list);

            ret = sem_init(&dai->tx_stop_sem, 0, 0);
            if (ret != OK) {
                continue;
            }

            ret = device_audio_board_get_data_cport(dev, i, j,
                                                    &dai->data_cport);
            if (ret) {
                sem_destroy(&dai->tx_stop_sem);
                free(dai);
                continue;
            }

            ret = device_audio_board_get_i2s_dev_id(dev, i, j, &i2s_dev_id);
            if (ret) {
                sem_destroy(&dai->tx_stop_sem);
                free(dai);
                continue;
            }

            //dai->i2s_dev = device_open(DEVICE_TYPE_I2S_HW, i2s_dev_id);
            dai->i2s_dev = NULL;
            if (!dai->i2s_dev) {
                sem_destroy(&dai->tx_stop_sem);
                free(dai);
                continue;
            }

            dai->dai_idx = j;

            list_add(&info->dai_list, &dai->list);
        }

        if (list_is_empty(&info->dai_list)) {
            device_close(info->codec_dev);
            free(info);
        } else {
            list_add(&gb_audio_info_list, &info->list);
        }
    }

err_close:
    device_close(dev);
}

static int gb_audio_init(unsigned int mgmt_cport, struct gb_bundle *bundle)
{
    struct gb_audio_info *info;
    int ret;

    gb_audio_alloc_info_list();

    info = gb_audio_find_info(mgmt_cport);
    if (!info) {
        return -ENODEV;
    }

    if (info->initialized) {
        return -EBUSY;
    }

    ret = device_codec_register_tx_callback(info->codec_dev, gb_audio_codec_cb,
                                            info);
    if (ret) {
        return ret;
    }

    ret = device_codec_register_rx_callback(info->codec_dev, gb_audio_codec_cb,
                                            info);
    if (ret) {
        return ret;
    }

    ret = device_codec_register_jack_event_callback(info->codec_dev,
                                                  gb_audio_codec_jack_event_cb,
                                                  info);
    if (ret) {
        return ret;
    }

    ret = device_codec_register_button_event_callback(info->codec_dev,
                                                gb_audio_codec_button_event_cb,
                                                info);
    if (ret) {
        return ret;
    }

    info->initialized = true;

    return 0;
}

static void gb_audio_exit(unsigned int mgmt_cport, struct gb_bundle *bundle)
{
    struct gb_audio_info *info;

    info = gb_audio_find_info(mgmt_cport);
    if (!info) {
        return;
    }

    if (!info->initialized) {
        return;
    }

    info->initialized = false;

}

static struct gb_operation_handler gb_audio_mgmt_handlers[] = {
    GB_HANDLER(GB_AUDIO_TYPE_PROTOCOL_VERSION,
               gb_audio_protocol_version_handler),
    GB_HANDLER(GB_AUDIO_TYPE_GET_TOPOLOGY_SIZE,
               gb_audio_get_topology_size_handler),
    GB_HANDLER(GB_AUDIO_TYPE_GET_TOPOLOGY, gb_audio_get_topology_handler),
    GB_HANDLER(GB_AUDIO_TYPE_GET_CONTROL, gb_audio_get_control_handler),
    GB_HANDLER(GB_AUDIO_TYPE_SET_CONTROL, gb_audio_set_control_handler),
    GB_HANDLER(GB_AUDIO_TYPE_ENABLE_WIDGET, gb_audio_enable_widget_handler),
    GB_HANDLER(GB_AUDIO_TYPE_DISABLE_WIDGET, gb_audio_disable_widget_handler),
    GB_HANDLER(GB_AUDIO_TYPE_GET_PCM, gb_audio_get_pcm_handler),
    GB_HANDLER(GB_AUDIO_TYPE_SET_PCM, gb_audio_set_pcm_handler),
    GB_HANDLER(GB_AUDIO_TYPE_SET_TX_DATA_SIZE,
               gb_audio_set_tx_data_size_handler),
    GB_HANDLER(GB_AUDIO_TYPE_GET_TX_DELAY, gb_audio_get_tx_delay_handler),
    GB_HANDLER(GB_AUDIO_TYPE_ACTIVATE_TX, gb_audio_activate_tx_handler),
    GB_HANDLER(GB_AUDIO_TYPE_DEACTIVATE_TX, gb_audio_deactivate_tx_handler),
    GB_HANDLER(GB_AUDIO_TYPE_SET_RX_DATA_SIZE,
               gb_audio_set_rx_data_size_handler),
    GB_HANDLER(GB_AUDIO_TYPE_GET_RX_DELAY, gb_audio_get_rx_delay_handler),
    GB_HANDLER(GB_AUDIO_TYPE_ACTIVATE_RX, gb_audio_activate_rx_handler),
    GB_HANDLER(GB_AUDIO_TYPE_DEACTIVATE_RX, gb_audio_deactivate_rx_handler),
    /* GB_AUDIO_TYPE_JACK_EVENT should only be received by the AP */
    /* GB_AUDIO_TYPE_BUTTON_EVENT should only be received by the AP */
    /* GB_AUDIO_TYPE_STREAMING_EVENT should only be received by the AP */
    /* GB_AUDIO_TYPE_SEND_DATA should only be sent on a Data Connection */
};

static struct gb_driver gb_audio_mgmt_driver = {
    .init               = gb_audio_init,
    .exit               = gb_audio_exit,
    .op_handlers        = gb_audio_mgmt_handlers,
    .op_handlers_count  = ARRAY_SIZE(gb_audio_mgmt_handlers),
};

void gb_audio_mgmt_register(int mgmt_cport, int bundle)
{
    gb_register_driver(mgmt_cport, bundle, &gb_audio_mgmt_driver);
}

static uint8_t gb_audio_send_data_handler(struct gb_operation *operation)
{
    struct gb_audio_send_data_request *request =
                gb_operation_get_request_payload(operation);
    struct gb_audio_dai_info *dai;
    int flags;
    int ret;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    dai = gb_audio_find_dai(operation->cport);
    if (!dai) {
        return GB_OP_INVALID;
    }

    flags = irq_lock();

    if (!(dai->flags & GB_AUDIO_FLAG_TX_ACTIVE)) {
        irq_unlock(flags);
        return GB_OP_PROTOCOL_BAD;
    }

    if (dai->flags & GB_AUDIO_FLAG_TX_STOPPING) {
        irq_unlock(flags);
        return GB_OP_SUCCESS;
    }

    if (!ring_buf_is_producers(dai->tx_rb)) {
        irq_unlock(flags);
        gb_audio_report_event(dai, GB_AUDIO_STREAMING_EVENT_OVERRUN);
        return GB_OP_SUCCESS;
    }

    gb_audio_i2s_tx(dai, request->data);

    /*
     * TODO: don't start until there is one buffered.  Even better,
     * don't start until half of the ring buffer is filled up (or add
     * a high watermark macro).  Adjust tx start delay value accordingly.
     */

    dai->flags |= GB_AUDIO_FLAG_TX_STARTED;

    irq_unlock(flags);

    ret = device_i2s_start_transmitter(dai->i2s_dev);
    if (ret) {
        dai->flags &= ~GB_AUDIO_FLAG_TX_STARTED;

        gb_audio_report_event(dai, GB_AUDIO_STREAMING_EVENT_FAILURE);
        gb_audio_report_event(dai, GB_AUDIO_STREAMING_EVENT_HALT);
        return GB_OP_SUCCESS;
    }

    return GB_OP_SUCCESS;
}

static struct gb_operation_handler gb_audio_data_handlers[] = {
    GB_HANDLER(GB_AUDIO_TYPE_PROTOCOL_VERSION,
               gb_audio_protocol_version_handler),
    GB_HANDLER(GB_AUDIO_TYPE_SEND_DATA,
               gb_audio_send_data_handler),
};

static struct gb_driver gb_audio_data_driver = {
    .op_handlers        = gb_audio_data_handlers,
    .op_handlers_count  = ARRAY_SIZE(gb_audio_data_handlers),
};

void gb_audio_data_register(int data_cport, int bundle)
{
    gb_register_driver(data_cport, bundle, &gb_audio_data_driver);
}
