/**
 * Copyright (c) 2015 Google, Inc.
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

#ifndef _GREYBUS_CAMERA_GB_H_
#define _GREYBUS_CAMERA_GB_H_

#include <greybus/types.h>

/* Greybus Camera request types */
#define GB_CAMERA_TYPE_INVALID                  GB_INVALID_TYPE
#define GB_CAMERA_TYPE_PROTOCOL_VERSION         0x01
#define GB_CAMERA_TYPE_CAPABILITIES             0x02
#define GB_CAMERA_TYPE_CONFIGURE_STREAMS        0x03
#define GB_CAMERA_TYPE_CAPTURE                  0x04
#define GB_CAMERA_TYPE_FLUSH                    0x05
#define GB_CAMERA_TYPE_METADATA                 0x06

/* Stream configuratoin */
#define MAX_STREAMS_NUM             4

/**
 * Stream configuration for Request
 */
struct gb_stream_config_req {
    /** Image width in pixels */
    __le16    width;
    /** Image height in pixels */
    __le16    height;
    /** Image format */
    __le16    format;
    /** Must be set to zero */
    __le16    padding;
} __packed;

/**
 * Stream configuration for Response
 */
struct gb_stream_config_resp {
    /** Image width in pixels */
    __le16    width;
    /** Image height in pixels */
    __le16    height;
    /** Image format */
    __le16    format;
    /** Virtual channel number for the stream */
    __u8      virtual_channel;
    /** Data type for the stream */
    __u8      data_type[2];
    /** Padding for 32bits alignment */
    __u8     padding[3];
    /** Maximum frame size in bytes */
    __le32    max_size;
} __packed;

/**
 * Camera Protocol Version Request
 */
struct gb_camera_version_request {
    /** Offered Camera Protocol major version */
    __u8    offer_major;
    /** Offered Camera Protocol minor version */
    __u8    offer_minor;
} __packed;

/**
 * Camera Protocol Version Response
 */
struct gb_camera_version_response {
    /** Camera Protocol major version */
    __u8    major;
    /** Camera Protocol major version */
    __u8    minor;
} __packed;

/**
 * Camera Protocol Capabilities Response
 */
struct gb_camera_capabilities_response {
    /** Capabilities of the Camera Module */
    __u8   capabilities[0];
} __packed;

/**
 * Camera Protocol Configure Streams Request
 */
struct gb_camera_configure_streams_request {
    /** Number of streams, between 0 and 4 inclusive */
    __u8    num_streams;
    /** Flags for configure streams request */
    __u8    flags;
    /** Must be set to zero */
    __le16  padding;
    /** Streams Configure Blocks */
    struct gb_stream_config_req config[0];
} __packed;

/**
 * Camera Protocol Configure Streams Response
 */
struct gb_camera_configure_streams_response {
    /** Number of streams, between 0 and 4 inclusive */
    __u8    num_streams;
    /** Flags for configure streams response */
    __u8    flags;
    /** Number of data lanes used on CSI bus, between 1 and 4 inclusive */
    __u8    num_lanes;
    /** Must be set to zero */
    __u8    padding;
    /** Clock speed of the CSI bus */
    __le32   bus_freq;
    /** Total number of lines sent in a second including blankings */
    __le32   lines_per_second;
    /** Streams Configure Blocks */
    struct gb_stream_config_resp config[0];
} __packed;

/**
 * Camera Protocol Capture Request
 */
struct gb_camera_capture_request {
    /** An incrementing integer to uniquely identify the capture request. */
    __le32  request_id;
    /** Bitmask of the streams included in the capture request */
    __u8    streams;
    /** Must be set to zero */
    __u8    padding;
    /** Number of frames to capture (0 for infinity) */
    __le16  num_frames;
    /** Capture request settings */
    __u8    settings[0];
} __packed;

/**
 * Camera Protocol Flush Response
 */
struct gb_camera_flush_response {
    /**
     * The last request that will be processed before the module stops
     * transmitting frames.
     */
    __le32  request_id;
} __packed;

/**
 * Camera Protocol Meta Data Request
 */
struct gb_camera_meta_data_request {
    /** The ID of the corresponding capture request */
    __le32  request_id;
    /** CSI-2 frame number */
    __le16  frame_number;
    /** The stream number */
    __u8    stream;
    /** Must be set to zero */
    __u8    padding;
    /** Meta-data block */
    __u8    data[0];
} __packed;

#endif /* _GREYBUS_CAMERA_GB_H_ */
