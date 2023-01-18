/*
 * Copyright (c) 2022 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief common definitions for the DSP sharing test application
 */

#ifndef _DSPCONTEXT_H
#define _DSPCONTEXT_H

struct dsp_volatile_register_set {
#ifdef CONFIG_ARC_DSP_BFLY_SHARING
	uintptr_t dsp_bfly0;
#endif
#ifdef CONFIG_ARC_AGU_SHARING
	uintptr_t agu_ap0;
	uintptr_t agu_os0;
#endif
};

struct dsp_non_volatile_register_set {
	/* No non-volatile dsp registers */
};

#define SIZEOF_DSP_VOLATILE_REGISTER_SET sizeof(struct dsp_volatile_register_set)
#define SIZEOF_DSP_NON_VOLATILE_REGISTER_SET 0

/* the set of ALL dsp registers */

struct dsp_register_set {
	struct dsp_volatile_register_set dsp_volatile;
	struct dsp_non_volatile_register_set dsp_non_volatile;
};

#define SIZEOF_DSP_REGISTER_SET \
	(SIZEOF_DSP_VOLATILE_REGISTER_SET + SIZEOF_DSP_NON_VOLATILE_REGISTER_SET)

/*
 * The following constants define the initial byte value used by the background
 * task, and the thread when loading up the dsp registers.
 */

#define MAIN_DSP_REG_CHECK_BYTE ((unsigned char)0xe5)
#define FIBER_DSP_REG_CHECK_BYTE ((unsigned char)0xf9)

#endif /* _DSPCONTEXT_H */
