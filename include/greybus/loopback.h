/*
 * Copyright (c) 2014-2015 Google Inc.
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

#ifndef __LOOPBACK__H__
#define __LOOPBACK__H__

#include <nuttx/list.h>
#include <nuttx/greybus/types.h>

/* Greybus loopback request types */
#define GB_LOOPBACK_TYPE_NONE                           0x00
#define GB_LOOPBACK_TYPE_PROTOCOL_VERSION               0x01
#define GB_LOOPBACK_TYPE_PING                           0x02
#define GB_LOOPBACK_TYPE_TRANSFER                       0x03
#define GB_LOOPBACK_TYPE_SINK                           0x04

/* version request has no payload */
struct gb_loopback_proto_version_response {
	__u8	major;
	__u8	minor;
};

struct gb_loopback_transfer_request {
	__le32	len;
	__le32	reserved0;
	__le32	reserved1;
	__u8    data[0];
};

struct gb_loopback_transfer_response {
	__le32	len;
	__le32	reserved0;
	__le32	reserved1;
	__u8    data[0];
};

struct gb_loopback_sync_transfer {
	__le32	len;
	__le32	chksum;
	__u8    data[0];
};

struct gb_loopback_statistics {
    unsigned recv;
    unsigned recv_err;

    unsigned throughput_min;
    unsigned throughput_max;
    unsigned throughput_avg;

    unsigned latency_min;
    unsigned latency_max;
    unsigned latency_avg;

    unsigned reqs_per_sec_min;
    unsigned reqs_per_sec_max;
    unsigned reqs_per_sec_avg;
};

typedef int (*gb_loopback_cport_cb)(int, void *);

int gb_loopback_get_cports(gb_loopback_cport_cb cb, void *data);
int gb_loopback_send_req(int cport, size_t size, uint8_t type);
int gb_loopback_get_stats(int cport, struct gb_loopback_statistics *stats);
void gb_loopback_reset(int cport);
int gb_loopback_cport_valid(int cport);

#endif
