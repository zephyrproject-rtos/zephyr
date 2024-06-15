/*
 * Copyright (c) 2024 Ambiq Micro Inc. <www.ambiq.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/mspi_emul.h>
#define DT_DRV_COMPAT zephyr_mspi_emul_device

/* Stub out a mspi device struct to use mspi_device emulator. */
static int emul_mspi_device_init_stub(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int emul_mspi_init_stub(const struct emul *stub_emul, const struct device *bus)
{
	ARG_UNUSED(stub_emul);
	ARG_UNUSED(bus);

	return 0;
}

struct emul_mspi_device_stub_dev_data {
	/* Stub */
};
struct emul_mspi_device_stub_dev_config {
	/* Stub */
};
struct emul_mspi_device_stub_dev_api {
	/* Stub */
};

#define EMUL_MSPI_DEVICE_DEVICE_STUB(n)                                                           \
	static struct emul_mspi_device_stub_dev_data stub_device_data_##n;                        \
	static struct emul_mspi_device_stub_dev_config stub_device_config_##n;                    \
	static struct emul_mspi_device_stub_dev_api stub_device_api_##n;                          \
	DEVICE_DT_INST_DEFINE(n,                                                                  \
			      emul_mspi_device_init_stub,                                         \
			      NULL,                                                               \
			      &stub_device_data_##n,                                              \
			      &stub_device_config_##n,                                            \
			      POST_KERNEL,                                                        \
			      CONFIG_MSPI_INIT_PRIORITY,                                          \
			      &stub_device_api_##n);

#define EMUL_TEST(n)                                                                              \
	EMUL_DT_INST_DEFINE(n,                                                                    \
			    emul_mspi_init_stub,                                                  \
			    NULL,                                                                 \
			    NULL,                                                                 \
			    NULL,                                                                 \
			    NULL);

DT_INST_FOREACH_STATUS_OKAY(EMUL_TEST)

DT_INST_FOREACH_STATUS_OKAY(EMUL_MSPI_DEVICE_DEVICE_STUB)
