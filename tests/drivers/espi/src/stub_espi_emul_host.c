/*
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#define DT_DRV_COMPAT zephyr_espi_emul_espi_host

/* Stub out an espi host device struct to use espi_host emulator. */
static int emul_espi_host_init_stub(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

struct emul_espi_host_stub_dev_data {
	/* Stub */
};
struct emul_espi_host_stub_dev_config {
	/* Stub */
};
struct emul_espi_host_stub_dev_api {
	/* Stub */
};

/* Since this is only stub, allocate the structs once. */
static struct emul_espi_host_stub_dev_data stub_host_data;
static struct emul_espi_host_stub_dev_config stub_host_config;
static struct emul_espi_host_stub_dev_api stub_host_api;

#define EMUL_ESPI_HOST_DEVICE_STUB(n)                                                              \
	DEVICE_DT_INST_DEFINE(n, &emul_espi_host_init_stub, NULL, &stub_host_data,                 \
			      &stub_host_config, POST_KERNEL, CONFIG_ESPI_INIT_PRIORITY,           \
			      &stub_host_api)

DT_INST_FOREACH_STATUS_OKAY(EMUL_ESPI_HOST_DEVICE_STUB);
