/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _AIO_DW_COMPARATOR_H_
#define _AIO_DW_COMPARATOR_H_

#include <board.h>
#include <device.h>
#include <aio_comparator.h>

#define AIO_DW_CMP_DRV_NAME		"dw_aio_cmp"

/**
 * @brief Number of AIO/Comparator on board
 */
#define AIO_DW_CMP_COUNT		(19)

/**
 * AIO/Comparator Register block type.
 */
struct dw_aio_cmp_t {
	volatile uint32_t en;		/**< Enable Register (0x00) */
	volatile uint32_t ref_sel;	/**< Reference Selection Register (0x04) */
	volatile uint32_t ref_pol;	/**< Reference Polarity Register (0x08) */
	volatile uint32_t pwr;		/**< Power Register (0x0C) */
	uint32_t reversed[6];
	volatile uint32_t stat_clr;	/**< Status Clear Register (0x28) */
};

struct dw_aio_cmp_cb {
	aio_cmp_cb cb;
	void *param;
};

struct dw_aio_cmp_dev_cfg_t {
	/** Base register address */
	uint32_t base_address;

	/** Interrupt number */
	uint32_t interrupt_num;

	/** Config function */
	int (*config_func)(struct device *dev);
};

struct dw_aio_cmp_dev_data_t {
	/** Number of total comparators */
	uint8_t num_cmp;

	/** Callback for each comparator */
	struct dw_aio_cmp_cb cb[AIO_DW_CMP_COUNT];
};

#endif /* _AIO_DW_COMPARATOR_H_ */
