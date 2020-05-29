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
 */

#ifndef __AUDIO_GB_H__
#define __AUDIO_GB_H__

#include <greybus/types.h>

#define GB_AUDIO_TYPE_PROTOCOL_VERSION          0x01
#define GB_AUDIO_TYPE_GET_TOPOLOGY_SIZE         0x02
#define GB_AUDIO_TYPE_GET_TOPOLOGY              0x03
#define GB_AUDIO_TYPE_GET_CONTROL               0x04
#define GB_AUDIO_TYPE_SET_CONTROL               0x05
#define GB_AUDIO_TYPE_ENABLE_WIDGET             0x06
#define GB_AUDIO_TYPE_DISABLE_WIDGET            0x07
#define GB_AUDIO_TYPE_GET_PCM                   0x08
#define GB_AUDIO_TYPE_SET_PCM                   0x09
#define GB_AUDIO_TYPE_SET_TX_DATA_SIZE          0x0a
#define GB_AUDIO_TYPE_GET_TX_DELAY              0x0b
#define GB_AUDIO_TYPE_ACTIVATE_TX               0x0c
#define GB_AUDIO_TYPE_DEACTIVATE_TX             0x0d
#define GB_AUDIO_TYPE_SET_RX_DATA_SIZE          0x0e
#define GB_AUDIO_TYPE_GET_RX_DELAY              0x0f
#define GB_AUDIO_TYPE_ACTIVATE_RX               0x10
#define GB_AUDIO_TYPE_DEACTIVATE_RX             0x11
#define GB_AUDIO_TYPE_JACK_EVENT                0x12
#define GB_AUDIO_TYPE_BUTTON_EVENT              0x13
#define GB_AUDIO_TYPE_STREAMING_EVENT           0x14
#define GB_AUDIO_TYPE_SEND_DATA                 0x15

/* Module must be able to buffer 10ms of audio data, minimum */
#define GB_AUDIO_SAMPLE_BUFFER_MIN_US           10000

#define GB_AUDIO_PCM_NAME_MAX                   32
#define AUDIO_DAI_NAME_MAX                      32
#define AUDIO_CONTROL_NAME_MAX                  32
#define AUDIO_CTL_ELEM_NAME_MAX                 44
#define AUDIO_ENUM_NAME_MAX                     64
#define AUDIO_WIDGET_NAME_MAX                   32

#define GB_AUDIO_PCM_FMT_S8                     BIT(0)
#define GB_AUDIO_PCM_FMT_U8                     BIT(1)
#define GB_AUDIO_PCM_FMT_S16_LE                 BIT(2)
#define GB_AUDIO_PCM_FMT_S16_BE                 BIT(3)
#define GB_AUDIO_PCM_FMT_U16_LE                 BIT(4)
#define GB_AUDIO_PCM_FMT_U16_BE                 BIT(5)
#define GB_AUDIO_PCM_FMT_S24_LE                 BIT(6)
#define GB_AUDIO_PCM_FMT_S24_BE                 BIT(7)
#define GB_AUDIO_PCM_FMT_U24_LE                 BIT(8)
#define GB_AUDIO_PCM_FMT_U24_BE                 BIT(9)
#define GB_AUDIO_PCM_FMT_S32_LE                 BIT(10)
#define GB_AUDIO_PCM_FMT_S32_BE                 BIT(11)
#define GB_AUDIO_PCM_FMT_U32_LE                 BIT(12)
#define GB_AUDIO_PCM_FMT_U32_BE                 BIT(13)

#define GB_AUDIO_PCM_RATE_5512                  BIT(0)
#define GB_AUDIO_PCM_RATE_8000                  BIT(1)
#define GB_AUDIO_PCM_RATE_11025                 BIT(2)
#define GB_AUDIO_PCM_RATE_16000                 BIT(3)
#define GB_AUDIO_PCM_RATE_22050                 BIT(4)
#define GB_AUDIO_PCM_RATE_32000                 BIT(5)
#define GB_AUDIO_PCM_RATE_44100                 BIT(6)
#define GB_AUDIO_PCM_RATE_48000                 BIT(7)
#define GB_AUDIO_PCM_RATE_64000                 BIT(8)
#define GB_AUDIO_PCM_RATE_88200                 BIT(9)
#define GB_AUDIO_PCM_RATE_96000                 BIT(10)
#define GB_AUDIO_PCM_RATE_176400                BIT(11)
#define GB_AUDIO_PCM_RATE_192000                BIT(12)

#define GB_AUDIO_STREAM_TYPE_CAPTURE            0x1
#define GB_AUDIO_STREAM_TYPE_PLAYBACK           0x2

#define GB_AUDIO_CTL_ELEM_ACCESS_READ           BIT(0)
#define GB_AUDIO_CTL_ELEM_ACCESS_WRITE          BIT(1)

#define GB_AUDIO_CTL_ELEM_TYPE_BOOLEAN          0x01
#define GB_AUDIO_CTL_ELEM_TYPE_INTEGER          0x02
#define GB_AUDIO_CTL_ELEM_TYPE_ENUMERATED       0x03
#define GB_AUDIO_CTL_ELEM_TYPE_INTEGER64        0x06

#define GB_AUDIO_CTL_ELEM_IFACE_CARD            0x00
#define GB_AUDIO_CTL_ELEM_IFACE_HWDEP           0x01
#define GB_AUDIO_CTL_ELEM_IFACE_MIXER           0x02
#define GB_AUDIO_CTL_ELEM_IFACE_PCM             0x03
#define GB_AUDIO_CTL_ELEM_IFACE_RAWMIDI         0x04
#define GB_AUDIO_CTL_ELEM_IFACE_TIMER           0x05
#define GB_AUDIO_CTL_ELEM_IFACE_SEQUENCER       0x06

#define GB_AUDIO_ACCESS_READ                    BIT(0)
#define GB_AUDIO_ACCESS_WRITE                   BIT(1)
#define GB_AUDIO_ACCESS_VOLATILE                BIT(2)
#define GB_AUDIO_ACCESS_TIMESTAMP               BIT(3)
#define GB_AUDIO_ACCESS_TLV_READ                BIT(4)
#define GB_AUDIO_ACCESS_TLV_WRITE               BIT(5)
#define GB_AUDIO_ACCESS_TLV_COMMAND             BIT(6)
#define GB_AUDIO_ACCESS_INACTIVE                BIT(7)
#define GB_AUDIO_ACCESS_LOCK                    BIT(8)
#define GB_AUDIO_ACCESS_OWNER                   BIT(9)

#define GB_AUDIO_WIDGET_TYPE_INPUT              0x0
#define GB_AUDIO_WIDGET_TYPE_OUTPUT             0x1
#define GB_AUDIO_WIDGET_TYPE_MUX                0x2
#define GB_AUDIO_WIDGET_TYPE_VIRT_MUX           0x3
#define GB_AUDIO_WIDGET_TYPE_VALUE_MUX          0x4
#define GB_AUDIO_WIDGET_TYPE_MIXER              0x5
#define GB_AUDIO_WIDGET_TYPE_MIXER_NAMED_CTL    0x6
#define GB_AUDIO_WIDGET_TYPE_PGA                0x7
#define GB_AUDIO_WIDGET_TYPE_OUT_DRV            0x8
#define GB_AUDIO_WIDGET_TYPE_ADC                0x9
#define GB_AUDIO_WIDGET_TYPE_DAC                0xa
#define GB_AUDIO_WIDGET_TYPE_MICBIAS            0xb
#define GB_AUDIO_WIDGET_TYPE_MIC                0xc
#define GB_AUDIO_WIDGET_TYPE_HP                 0xd
#define GB_AUDIO_WIDGET_TYPE_SPK                0xe
#define GB_AUDIO_WIDGET_TYPE_LINE               0xf
#define GB_AUDIO_WIDGET_TYPE_SWITCH             0x10
#define GB_AUDIO_WIDGET_TYPE_VMID               0x11
#define GB_AUDIO_WIDGET_TYPE_PRE                0x12
#define GB_AUDIO_WIDGET_TYPE_POST               0x13
#define GB_AUDIO_WIDGET_TYPE_SUPPLY             0x14
#define GB_AUDIO_WIDGET_TYPE_REGULATOR_SUPPLY   0x15
#define GB_AUDIO_WIDGET_TYPE_CLOCK_SUPPLY       0x16
#define GB_AUDIO_WIDGET_TYPE_AIF_IN             0x17
#define GB_AUDIO_WIDGET_TYPE_AIF_OUT            0x18
#define GB_AUDIO_WIDGET_TYPE_SIGGEN             0x19
#define GB_AUDIO_WIDGET_TYPE_DAI_IN             0x1a
#define GB_AUDIO_WIDGET_TYPE_DAI_OUT            0x1b
#define GB_AUDIO_WIDGET_TYPE_DAI_LINK           0x1c

#define GB_AUDIO_WIDGET_STATE_DISABLED          0x01
#define GB_AUDIO_WIDGET_STATE_ENAABLED          0x02

#define GB_AUDIO_JACK_EVENT_INSERTION           0x1
#define GB_AUDIO_JACK_EVENT_REMOVAL             0x2

#define GB_AUDIO_BUTTON_EVENT_PRESS             0x1
#define GB_AUDIO_BUTTON_EVENT_RELEASE           0x2

#define GB_AUDIO_STREAMING_EVENT_UNSPECIFIED    0x1
#define GB_AUDIO_STREAMING_EVENT_HALT           0x2
#define GB_AUDIO_STREAMING_EVENT_INTERNAL_ERROR 0x3
#define GB_AUDIO_STREAMING_EVENT_PROTOCOL_ERROR 0x4
#define GB_AUDIO_STREAMING_EVENT_FAILURE        0x5
#define GB_AUDIO_STREAMING_EVENT_UNDERRUN       0x6
#define GB_AUDIO_STREAMING_EVENT_OVERRUN        0x7
#define GB_AUDIO_STREAMING_EVENT_CLOCKING       0x8
#define GB_AUDIO_STREAMING_EVENT_DATA_LEN       0x9

#define GB_AUDIO_INVALID_INDEX                  0xff

struct gb_audio_pcm {
    __u8    stream_name[GB_AUDIO_PCM_NAME_MAX];
    __le32  formats;    /* GB_AUDIO_PCM_FMT_* */
    __le32  rates;      /* GB_AUDIO_PCM_RATE_* */
    __u8    chan_min;
    __u8    chan_max;
    __u8    sig_bits;   /* number of bits of content */
} __packed;

struct gb_audio_dai {
    __u8            name[AUDIO_DAI_NAME_MAX];
    __le16          data_cport;
    struct gb_audio_pcm capture;
    struct gb_audio_pcm playback;
} __packed;

struct gb_audio_integer {
    __le32  min;
    __le32  max;
    __le32  step;
} __packed;

struct gb_audio_integer64 {
    __le64  min;
    __le64  max;
    __le64  step;
} __packed;

struct gb_audio_enumerated {
    __le32  items;
    __le16  names_length;
    __u8    names[0];
} __packed;

struct gb_audio_ctl_elem_info { /* See snd_ctl_elem_info in Linux source */
    __u8        type;       /* GB_AUDIO_CTL_ELEM_TYPE_* */
    __le16      dimen[4];
    union {
        struct gb_audio_integer     integer;
        struct gb_audio_integer64   integer64;
        struct gb_audio_enumerated  enumerated;
    } value;
} __packed;

struct gb_audio_ctl_elem_value { /* See snd_ctl_elem_value in Linux source */
    __le64  timestamp;
    union {
        __le32  integer_value[2];   /* consider CTL_DOUBLE_xxx */
        __le64  integer64_value[2];
        __le32  enumerated_item[2];
    } value;
} __packed;

struct gb_audio_control {
    __u8    name[AUDIO_CONTROL_NAME_MAX];
    __u8    id;             /* 0-63 */
    __u8    iface;          /* GB_AUDIO_IFACE_* */
    __le16  data_cport;
    __le32  access;         /* GB_AUDIO_ACCESS_* */
    __u8    count;          /* count of same elements */
    __u8    count_values;   /* count of values, max=2 for CTL_DOUBLE_xxx */
    struct gb_audio_ctl_elem_info   info;
} __packed;

struct gb_audio_widget {
    __u8    name[AUDIO_WIDGET_NAME_MAX];
    __u8    same[AUDIO_WIDGET_NAME_MAX];
    __u8    id;
    __u8    type;       /* GB_AUDIO_WIDGET_TYPE_* */
    __u8    state;      /* GB_AUDIO_WIDGET_STATE_* */
    __u8    ncontrols;
    struct gb_audio_control ctl[0];   /* 'ncontrols' entries */
} __packed;

struct gb_audio_route {
    __u8    source_id;      /* widget id */
    __u8    destination_id; /* widget id */
    __u8    control_id;     /* 0-63 */
    __u8    index;          /* Selection within the control */
} __packed;

struct gb_audio_topology {
    __u8    num_dais;
    __u8    num_controls;
    __u8    num_widgets;
    __u8    num_routes;
    __le32  size_dais;
    __le32  size_controls;
    __le32  size_widgets;
    __le32  size_routes;
    /*
     * struct gb_audio_dai      dai[num_dais];
     * struct gb_audio_control  controls[num_controls];
     * struct gb_audio_widget   widgets[num_widgets];
     * struct gb_audio_route    routes[num_routes];
     */
    __u8    data[0];
} __packed;

struct gb_audio_version_response {
    __u8    major;
    __u8    minor;
} __packed;

struct gb_audio_get_topology_size_response {
    __le16  size;
} __packed;

struct gb_audio_get_topology_response {
    struct gb_audio_topology    topology;
} __packed;

struct gb_audio_get_control_request {
    __u8    control_id;
    __u8    index;
} __packed;

struct gb_audio_get_control_response {
    struct gb_audio_ctl_elem_value  value;
} __packed;

struct gb_audio_set_control_request {
    __u8    control_id;
    __u8    index;
    struct gb_audio_ctl_elem_value  value;
} __packed;

struct gb_audio_enable_widget_request {
    __u8    widget_id;
} __packed;

struct gb_audio_disable_widget_request {
    __u8    widget_id;
} __packed;

struct gb_audio_get_pcm_request {
    __le16  data_cport;
} __packed;

struct gb_audio_get_pcm_response {
    __le32  format;
    __le32  rate;
    __u8    channels;
    __u8    sig_bits;
} __packed;

struct gb_audio_set_pcm_request {
    __le16  data_cport;
    __le32  format;
    __le32  rate;
    __u8    channels;
    __u8    sig_bits;
} __packed;

struct gb_audio_set_tx_data_size_request {
    __le16  data_cport;
    __le16  size;
} __packed;

struct gb_audio_get_tx_delay_request {
    __le16  data_cport;
} __packed;

struct gb_audio_get_tx_delay_response {
    __le32  delay;
} __packed;

struct gb_audio_activate_tx_request {
    __le16  data_cport;
} __packed;

struct gb_audio_deactivate_tx_request {
    __le16  data_cport;
} __packed;

struct gb_audio_set_rx_data_size_request {
    __le16  data_cport;
    __le16  size;
} __packed;

struct gb_audio_get_rx_delay_request {
    __le16  data_cport;
} __packed;

struct gb_audio_get_rx_delay_response {
    __le32  delay;
} __packed;

struct gb_audio_activate_rx_request {
    __le16  data_cport;
} __packed;

struct gb_audio_deactivate_rx_request {
    __le16  data_cport;
} __packed;

struct gb_audio_jack_event_request {
    __u8    widget_id;
    __u8    widget_type;
    __u8    event;
} __packed;

struct gb_audio_button_event_request {
    __u8    widget_id;
    __u8    button_id;
    __u8    event;
} __packed;

struct gb_audio_streaming_event_request {
    __le16  data_cport;
    __u8    event;
} __packed;

struct gb_audio_send_data_request {
    __le64  timestamp;
    __u8    data[0];
} __packed;

#endif /* __AUDIO_GB_H__ */
