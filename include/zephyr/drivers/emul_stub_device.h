/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Stub device support for emulators without a corresponding driver.
 * @ingroup io_emulators
 */

#ifndef ZEPHYR_INCLUDE_EMUL_STUB_DEVICE_H_
#define ZEPHYR_INCLUDE_EMUL_STUB_DEVICE_H_

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

/*
 * Needed for emulators without corresponding DEVICE_DT_DEFINE drivers
 */

/** @cond INTERNAL_HIDDEN */
struct emul_stub_dev_data {
	/* Stub */
};
struct emul_stub_dev_config {
	/* Stub */
};
struct emul_stub_dev_api {
	/* Stub */
};
/** @endcond */

/**
 * @brief Stub out a device for an emulator instance.
 *
 * For every instance of a @c DT_DRV_COMPAT, define a stub device so that
 * the emulator's devicetree node has a corresponding device.
 *
 * @param n @c DT_DRV_COMPAT instance number
 */
#define EMUL_STUB_DEVICE(n)                                                                        \
	__maybe_unused static int emul_init_stub_##n(const struct device *dev)                     \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	static struct emul_stub_dev_data stub_data_##n;                                            \
	static struct emul_stub_dev_config stub_config_##n;                                        \
	static struct emul_stub_dev_api stub_api_##n;                                              \
	DEVICE_DT_INST_DEFINE(n, &emul_init_stub_##n, NULL, &stub_data_##n, &stub_config_##n,      \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &stub_api_##n);

#endif /* ZEPHYR_INCLUDE_EMUL_STUB_DEVICE_H_ */
