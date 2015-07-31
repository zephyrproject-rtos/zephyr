/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * @file
 * @brief Profiler support.
 */


#include <misc/profiler.h>
#include <misc/util.h>
#include <init.h>

uint32_t _sys_profiler_buffer[CONFIG_PROFILER_BUFFER_SIZE];

/**
 * @brief Initialize the profiler system.
 *
 * @details Initialize the ring buffer and the sync semaphore.
 *
 * @return No return value.
 */
static int _sys_profiler_init(struct device *arg)
{
	ARG_UNUSED(arg);

	sys_event_logger_init(&sys_profiler_logger, _sys_profiler_buffer,
		CONFIG_PROFILER_BUFFER_SIZE);

	return 0;
}
DECLARE_DEVICE_INIT_CONFIG(profiler_0, "", _sys_profiler_init, NULL);
nano_early_init(profiler_0, NULL);


void sys_profiler_put_timed(uint16_t event_id)
{
	uint32_t data[1];

	data[0] = nano_tick_get_32();

	sys_event_logger_put(&sys_profiler_logger, event_id, data,
		ARRAY_SIZE(data));
}
