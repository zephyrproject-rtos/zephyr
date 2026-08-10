/*
 * Copyright (c) 2022, Kumar Gala <galak@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/sys/util.h>

/*
 * ssd1322@9 is the last MIPI DBI child inside the ten cs-gpios entries this fixture's SPI
 * controller declares, and st75256@a is the first one past them. Other boards build this
 * suite with a display fixture of their own, so this is scoped to the nodes it is about.
 */
#if DT_NODE_EXISTS(DT_NODELABEL(test_mipi_dbi_ssd1322)) && \
	DT_NODE_EXISTS(DT_NODELABEL(test_mipi_dbi_st75256))
BUILD_ASSERT(MIPI_DBI_SPI_DEV_HAS_CS_GPIOS(DT_NODELABEL(test_mipi_dbi_ssd1322)),
	     "the last child inside cs-gpios must resolve to a GPIO chip select");
BUILD_ASSERT(!MIPI_DBI_SPI_DEV_HAS_CS_GPIOS(DT_NODELABEL(test_mipi_dbi_st75256)),
	     "a child past the end of cs-gpios must not resolve to a GPIO chip select");
#endif

int main(void)
{
	return 0;
}
