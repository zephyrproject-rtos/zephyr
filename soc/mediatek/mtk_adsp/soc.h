/* Copyright 2023 The ChromiumOS Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_MTK_ADSP_SOC_H
#define ZEPHYR_SOC_MTK_ADSP_SOC_H

#include <zephyr/device.h>

void mtk_adsp_cpu_freq_init(void);
void mtk_adsp_set_cpu_freq(int mhz);

/* Mailbox Driver: */

/* Hardware defines multiple "channel" bits that can be independently
 * signaled and cleared.  An interrupt is latched if any bits are
 * set.
 */
#define MTK_ADSP_MBOX_CHANNELS 5

typedef void (*mtk_adsp_mbox_handler_t)(const struct device *mbox, void *arg);

void mtk_adsp_mbox_set_handler(const struct device *mbox, uint32_t chan,
			       mtk_adsp_mbox_handler_t handler, void *arg);

/* Mailbox hardware has an array of unstructured "message" data in
 * each direction.  Any value can be placed in the registers.
 */
#define MTK_ADSP_MBOX_MSG_WORDS 5
void mtk_adsp_mbox_set_msg(const struct device *mbox, uint32_t idx, uint32_t val);
uint32_t mtk_adsp_mbox_get_msg(const struct device *mbox, uint32_t idx);

/* Signal an interrupt on the specified channel for the other side */
void mtk_adsp_mbox_signal(const struct device *mbox, uint32_t chan);

#endif /* ZEPHYR_SOC_MTK_ADSP_SOC_H */
