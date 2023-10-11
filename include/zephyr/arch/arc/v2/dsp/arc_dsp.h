/*
 * Copyright (c) 2022 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_DSP_ARC_DSP_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_DSP_ARC_DSP_H_

/**
 * @brief Disable dsp context preservation
 *
 * The function is used to disable the preservation of dsp
 * and agu context registers for a particular thread.
 *
 * The @a options parameter indicates which register sets will
 * not be used by the specified thread. It is used by ARC only.
 *
 * @param thread  ID of thread.
 * @param options register sets options
 *
 */
void arc_dsp_disable(struct k_thread *thread, unsigned int options);

/**
 * @brief Enable dsp context preservation
 *
 * The function is used to enable the preservation of dsp
 * and agu context registers for a particular thread.
 *
 * The @a options parameter indicates which register sets will
 * be used by the specified thread. It is used by ARC only.
 *
 * @param thread  ID of thread.
 * @param options register sets options
 *
 */
void arc_dsp_enable(struct k_thread *thread, unsigned int options);

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_DSP_ARC_DSP_H_ */
