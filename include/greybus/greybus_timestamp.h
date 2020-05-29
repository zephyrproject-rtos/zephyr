/**
 * Copyright (c) 2015 Google Inc.
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

#ifndef __CONFIGS_ARA_BRIDGE_INCLUDE_TIMESTAMPS_H
#define  __CONFIGS_ARA_BRIDGE_INCLUDE_TIMESTAMPS_H

#include <nuttx/time.h>

#define GREYBUS_FW_TIMESTAMP_APBRIDGE 0x01
#define GREYBUS_FW_TIMESTAMP_GPBRDIGE 0x02

struct gb_timestamp {
    bool tag;
    struct timeval entry_time;
    struct timeval exit_time;
};

void gb_timestamp_tag_entry_time(struct gb_timestamp *ts,
                                 unsigned int cportid);
void gb_timestamp_tag_exit_time(struct gb_timestamp *ts,
                                unsigned int cportid);
void gb_timestamp_log(struct gb_timestamp *ts, unsigned int cportid,
                      void *payload, size_t len, int id);
void gb_timestamp_init(void);
#endif
