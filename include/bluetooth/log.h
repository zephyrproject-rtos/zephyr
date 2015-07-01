/** @file
 *  @brief Bluetooth subsystem logging helpers.
 */

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
#ifndef __BT_LOG_H
#define __BT_LOG_H

#include <stdio.h>

#if defined(CONFIG_BLUETOOTH_DEBUG)
#define BT_DBG(fmt, ...) printf("bt: %s (%p): " fmt, __func__, \
				context_self_get(), ##__VA_ARGS__)
#define BT_ERR(fmt, ...) printf("bt: %s: " fmt, __func__, ##__VA_ARGS__)
#define BT_WARN(fmt, ...) printf("bt: %s: " fmt, __func__, ##__VA_ARGS__)
#define BT_INFO(fmt, ...) printf("bt: " fmt,  ##__VA_ARGS__)
#else
#define BT_DBG(fmt, ...)
#define BT_ERR(fmt, ...)
#define BT_WARN(fmt, ...)
#define BT_INFO(fmt, ...)
#endif /* CONFIG_BLUETOOTH_DEBUG */

#endif /* __BT_LOG_H */
