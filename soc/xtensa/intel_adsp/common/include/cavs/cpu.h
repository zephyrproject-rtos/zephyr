/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2019 Intel Corporation. All rights reserved.
 *
 * Author: Bartosz Kokoszko <bartoszx.kokoszko@linux.intel.com>
 */

/**
 * \file cavs/lib/cpu.h
 * \brief DSP parameters, common for cAVS platforms.
 */

#ifndef __CAVS_CPU_H__
#define __CAVS_CPU_H__

/** \brief Number of available DSP cores (conf. by kconfig) */
#define PLATFORM_CORE_COUNT	CONFIG_CORE_COUNT

/** \brief Id of master DSP core */
#define PLATFORM_MASTER_CORE_ID	0

#endif /* __CAVS_CPU_H__ */
