/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The IronSide TE bundle configuration the client's layout.h derives the application's
 * NVM slot and RAM window from. ironside_bundle.py export-config --app-header writes it
 * from a bundle; here it is read from the devicetree, where a Zephyr application describes
 * the layout it is built for, so that an overlay is all it takes to change the partitioning.
 * The partition sizes come from the partition nodes and the key slot storage size from
 * where non-secure RAM starts.
 */

#ifndef IRONSIDE_BUNDLE_CONFIG_H
#define IRONSIDE_BUNDLE_CONFIG_H

#include <zephyr/devicetree.h>

#include <ironside_client/internal/layout_abi.h>

/* UICR.IRONSIDE.IROT.PARTITIONS, in 4 KB blocks. */
#define IRONSIDE_BASECONF_PARTITIONS_DFUBANK_SIZE4KB \
	(DT_REG_SIZE(DT_NODELABEL(dfubank_partition)) / 4096)
#define IRONSIDE_BASECONF_PARTITIONS_APPSTORAGE_SIZE4KB \
	(DT_REG_SIZE(DT_NODELABEL(storage_partition)) / 4096)
#define IRONSIDE_BASECONF_PARTITIONS_UROTSTORAGE_SIZE4KB \
	(DT_REG_SIZE(DT_NODELABEL(urotstorage_partition)) / 4096)
#define IRONSIDE_BASECONF_PARTITIONS_DFUIMAGE_SIZE4KB \
	(DT_REG_SIZE(DT_NODELABEL(dfuimage_partition)) / 4096)

/* UICR.IRONSIDE.UROT.KEYSLOTSTORAGESIZE, in bytes: the key slots end where non-secure RAM
 * starts.
 */
#define IRONSIDE_BASECONF_KEYSLOTSTORAGESIZE \
	(DT_REG_ADDR(DT_NODELABEL(sram_ns)) - IRONSIDE_PSA_KEY_ARRAY_ADDR)

#endif /* IRONSIDE_BUNDLE_CONFIG_H */
