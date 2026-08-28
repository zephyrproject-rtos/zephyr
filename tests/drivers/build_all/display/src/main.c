/*
 * Copyright (c) 2022, Kumar Gala <galak@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/mipi_dbi.h>

/*
 * test_mipi_dbi_hx8353e_pwm (app.overlay) sits at reg = 0x27 on test_spi,
 * whose cs-gpios property only has 10 entries (index 0-9). Regression
 * check for MIPI_DBI_SPI_HAS_CS_GPIOS(): it must answer per this
 * device's own index, not just whether test_spi has a cs-gpios property
 * at all, since that property existing says nothing about whether index
 * 0x27 is actually one of its 10 entries.
 */
BUILD_ASSERT(MIPI_DBI_SPI_HAS_CS_GPIOS(DT_NODELABEL(test_mipi_dbi_hx8353e_pwm)) == 0,
	     "index 0x27 is out of range for a 10-entry cs-gpios property");

int main(void)
{
	return 0;
}
