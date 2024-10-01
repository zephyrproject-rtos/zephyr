/*
 * Copyright 2024 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * The following open-circuit voltage curves have been extracted from the datasheets of the
 * listed battery parts. They will not be 100% correct for all batteries of the chemistry,
 * but should provide a good baseline. These curves will also be affected by ambient temperature
 * and discharge current.
 *
 * Each curve is 11 elements representing the OCV voltage in microvolts for each charge percentage
 * from 0% to 100% inclusive in 10% increments.
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_BATTERY_BATTERY_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_BATTERY_BATTERY_H_

/* Panasonic KR-1800SCE */
#define BATTERY_OCV_CURVE_NICKEL_CADMIUM_DEFAULT                                                   \
	800000 1175000 1207000 1217000 1221000 1226000 1233000 1245000 1266000 1299000 1366000

/* Panasonic BK-1100FHU */
#define BATTERY_OCV_CURVE_NICKEL_METAL_HYDRIDE_DEFAULT                                             \
	1004000 1194000 1231000 1244000 1254000 1257000 1263000 1266000 1274000 1315000 1420000

/* Panasonic NCR18650BF */
#define BATTERY_OCV_CURVE_LITHIUM_ION_POLYMER_DEFAULT                                              \
	2502000 3146000 3372000 3449000 3532000 3602000 3680000 3764000 3842000 3936000 4032000

/* Drypower IFR18650 E1600 */
#define BATTERY_OCV_CURVE_LITHIUM_IRON_PHOSPHATE_DEFAULT                                           \
	2013000 3068000 3159000 3194000 3210000 3221000 3229000 3246000 3256000 3262000 3348000

/* FDK CR14250SE */
#define BATTERY_OCV_CURVE_LITHIUM_MANGANESE_DEFAULT                                                \
	1906000 2444000 2689000 2812000 2882000 2927000 2949000 2955000 2962000 2960000 2985000

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_BATTERY_BATTERY_H_ */
