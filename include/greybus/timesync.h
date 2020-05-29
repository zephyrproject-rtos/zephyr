/*
 * Copyright (c) 2016 Google Inc.
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

#ifndef  _TIMESYNC_H_
#define  _TIMESYNC_H_

#include <greybus/greybus.h>

/* TimeSync finite state machine */
enum timesync_state {
        TIMESYNC_STATE_INVALID       = 0,
        TIMESYNC_STATE_INACTIVE      = 1,
        TIMESYNC_STATE_SYNCING       = 2,
        TIMESYNC_STATE_ACTIVE        = 3,
        TIMESYNC_STATE_DEBUG_ACTIVE  = 4,
};

int timesync_enable(uint8_t strobe_count, uint64_t frame_time,
                    uint32_t strobe_delay, uint32_t refclk);
int timesync_disable(void);
int timesync_authoritative(uint64_t *frame_time);
int timesync_get_last_event(uint64_t *frame_time);

int timesync_strobe_handler(void);
int timesync_init(void);
void timesync_exit(void);

/* This returns the frame-time */
uint64_t timesync_get_frame_time(void);
int timesync_get_state(void);

#endif	/* _TIMESYNC_H_ */
