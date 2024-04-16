/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/spi.h>

const struct rtio_iodev_api spi_iodev_api = {
	.submit = spi_iodev_submit,
};
