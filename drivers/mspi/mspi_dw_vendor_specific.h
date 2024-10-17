/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MSPI_MSPI_DW_VENDOR_SPECIFIC_H_
#define ZEPHYR_DRIVERS_MSPI_MSPI_DW_VENDOR_SPECIFIC_H_

#ifdef __cplusplus
extern "C" {
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_exmif)

#include <nrf.h>

static void vendor_specific_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	NRF_EXMIF->INTENSET = BIT(EXMIF_INTENSET_CORE_Pos);
	NRF_EXMIF->TASKS_START = 1;
}

static void vendor_specific_irq_clear(const struct device *dev)
{
	ARG_UNUSED(dev);

	NRF_EXMIF->EVENTS_CORE = 0;
}

static int vendor_specific_xip_enable(const struct device *dev,
				      const struct mspi_dev_id *dev_id,
				      const struct mspi_xip_cfg *cfg)
{
	ARG_UNUSED(dev);

	if (dev_id->dev_idx == 0) {
		NRF_EXMIF->EXTCONF1.OFFSET = cfg->address_offset;
		NRF_EXMIF->EXTCONF1.SIZE = cfg->address_offset
					 + cfg->size;
		NRF_EXMIF->EXTCONF1.ENABLE = 1;
	} else if (dev_id->dev_idx == 1) {
		NRF_EXMIF->EXTCONF2.OFFSET = cfg->address_offset;
		NRF_EXMIF->EXTCONF2.SIZE = cfg->address_offset
					 + cfg->size;
		NRF_EXMIF->EXTCONF2.ENABLE = 1;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int vendor_specific_xip_disable(const struct device *dev,
				       const struct mspi_dev_id *dev_id,
				       const struct mspi_xip_cfg *cfg)
{
	ARG_UNUSED(dev);

	if (dev_id->dev_idx == 0) {
		NRF_EXMIF->EXTCONF1.ENABLE = 0;
	} else if (dev_id->dev_idx == 1) {
		NRF_EXMIF->EXTCONF2.ENABLE = 0;
	} else {
		return -EINVAL;
	}

	return 0;
}

#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_exmif) */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MSPI_MSPI_DW_VENDOR_SPECIFIC_H_ */
