/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAL_IPI_H
#define __HAL_IPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "uwp_hal.h"

#define REG_IPI_INT_GEN			(BASE_IPI + 0x0)
#define REG_IPI_INT_CLR_COREN	(BASE_IPI + 0x4)
#define REG_IPI_INT_CLR_LOCAL	(BASE_IPI + 0x8)

	enum ipi_int_type {
		IPI_TYPE_IRQ0 = 0,
		IPI_TYPE_FIQ0,
		IPI_TYPE_IRQ1,
		IPI_TYPE_FIQ1,
		IPI_TYPE_MAX_NUM,
	};

	enum ipi_core {
		IPI_CORE_BTWF = 0,
		IPI_CORE_GNSS,
		IPI_CORE_MAX_NUM,
	};

	static inline void uwp_ipi_trigger(enum ipi_core core,
			enum ipi_int_type type)
	{
		if (core >= IPI_CORE_MAX_NUM) {
			printk("invalid ipi core number: %d.\n", core);
			return;
		}

		sci_write32(REG_IPI_INT_GEN, BIT((core << 2) + type));
	}

	static inline void uwp_ipi_clear_remote(enum ipi_core core,
			enum ipi_int_type type)
	{
		if (core >= IPI_CORE_MAX_NUM) {
			printk("invalid ipi core number: %d.\n", core);
			return;
		}

		//sci_write32(REG_IPI_INT_CLR_COREN, 0xFFFFFFFF);
		sci_write32(REG_IPI_INT_CLR_COREN, BIT((core << 2) + type));
	}

	static inline void uwp_ipi_clear_local(enum ipi_core core,
			enum ipi_int_type type)
	{
		if (core >= IPI_CORE_MAX_NUM) {
			printk("invalid ipi core number: %d.\n", core);
			return;
		}

		sci_write32(REG_IPI_INT_CLR_LOCAL, BIT((core << 2) + type));
	}

#ifdef __cplusplus
}
#endif

#endif

