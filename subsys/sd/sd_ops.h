/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_SUBSYS_SD_SD_OPS_H_
#define ZEPHYR_SUBSYS_SD_SD_OPS_H_

/*
 * Switches voltage of SD card to 1.8V, as described by
 * "Signal volatage switch procedure" in section 3.6.1 of SD specification.
 */
int sdmmc_switch_voltage(struct sd_card *card);

/*
 * Reads card identification register, and decodes it
 */
int sdmmc_read_cid(struct sd_card *card);

/*
 * Read card specific data register
 */
int sdmmc_read_csd(struct sd_card *card);

/*
 * Requests card to publish a new relative card address, and move from
 * identification to data mode
 */
int sdmmc_request_rca(struct sd_card *card);

/*
 * Selects card, moving it into data transfer mode
 */
int sdmmc_select_card(struct sd_card *card);

/* Returns 1 if host supports UHS, zero otherwise */
static inline int sdmmc_host_uhs(struct sdhc_host_props *props)
{
	return (props->host_caps.sdr50_support |
		props->host_caps.uhs_2_support |
		props->host_caps.sdr104_support |
		props->host_caps.ddr50_support)
		& (props->host_caps.vol_180_support);
}

#endif /* ZEPHYR_SUBSYS_SD_SD_OPS_H_ */
