/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#ifndef ZEPHYRTC_DEMO_MEDIA_STATS_ZEPHYR_H_
#define ZEPHYRTC_DEMO_MEDIA_STATS_ZEPHYR_H_

#include <stdint.h>

uint64_t media_zephyr_generated_audio(void);
uint64_t media_zephyr_late_audio(void);
int media_zephyr_quiesce(void);

#endif /* ZEPHYRTC_DEMO_MEDIA_STATS_ZEPHYR_H_ */
