/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sfp.h>
#include <zephyr/kernel.h>
#include <zephyr/sff/sff8472.h>
#include <zephyr/sys/printk.h>

#define SAMPLE_READINGS 3

static const struct device *const sfp = DEVICE_DT_GET_ONE(sff_sfp);

/* Print thousandths of a unit, keeping the sign of values between -1 and 0. */
static void print_milli(const char *label, int64_t milli, const char *unit)
{
	uint64_t mag = (milli < 0) ? (uint64_t)(-milli) : (uint64_t)milli;

	printk("%s: %s%u.%03u %s\n", label, (milli < 0) ? "-" : "", (unsigned int)(mag / 1000U),
	       (unsigned int)(mag % 1000U), unit);
}

static void print_id(const struct sff8472_id *id)
{
	printk("Module     : %s, %s connector\n", sff8472_identifier_name(id->identifier),
	       sff8472_connector_name(id->connector));
	printk("Vendor     : %s\n", id->vendor_name);
	printk("Part number: %s rev %s\n", id->vendor_pn, id->vendor_rev);
	printk("Serial     : %s (date code %s)\n", id->vendor_sn, id->date_code);
	printk("Wavelength : %u nm, %u MBd\n", id->wavelength_nm, id->br_nominal_mbd);
}

static void print_diagnostics(const struct sff8472_diagnostics *diag)
{
	print_milli("Temperature", diag->temp_mdegc, "C");
	print_milli("Vcc        ", diag->vcc_uv / 1000U, "V");
	print_milli("TX bias    ", diag->tx_bias_ua, "mA");
	print_milli("TX power   ", diag->tx_power_nw / 1000U, "mW");
	print_milli("RX power   ", diag->rx_power_nw / 1000U, "mW");
	printk("Flags      : alarms 0x%03x, warnings 0x%03x\n", diag->alarm_flags,
	       diag->warning_flags);
}

int main(void)
{
	struct sff8472_diagnostics diag;
	struct sff8472_id id;
	int ret;

	if (!device_is_ready(sfp)) {
		printk("%s is not ready\n", sfp->name);
		return 0;
	}

	ret = sfp_read_id(sfp, &id);
	if (ret == -ENODEV) {
		printk("No module in %s\n", sfp->name);
		return 0;
	} else if (ret != 0) {
		printk("Reading the module ID failed (%d)\n", ret);
		return 0;
	}

	print_id(&id);

	for (int i = 0; i < SAMPLE_READINGS; i++) {
		ret = sfp_read_diagnostics(sfp, &diag);
		if (ret == -ENOTSUP) {
			printk("This module's diagnostics are not supported\n");
			break;
		} else if (ret != 0) {
			printk("Reading diagnostics failed (%d)\n", ret);
			break;
		}

		print_diagnostics(&diag);
		k_sleep(K_SECONDS(1));
	}

	printk("Done\n");

	return 0;
}
