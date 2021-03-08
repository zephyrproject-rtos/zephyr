/*
 * Copyright (c) 2023 grandcentrix GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_WIFI_SILABS_WF200_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_WIFI_SILABS_WF200_H_

/**
 * Silabs PDS(Platform Data Set) Definitions
 *
 * These quite complicated looking, jsonish data contains board
 * specific configuration for the silabs wf(m) wifi chipsets.
 *
 * The more human readable versions can be found here:
 * https://github.com/SiliconLabs/wfx-pds
 *
 * All possible values can be found here:
 * https://github.com/SiliconLabs/wfx-firmware/blob/bcaeffdfd7ef3a594ec6b613cfa6be8b743767a1/PDS/definitions.in
 *
 * This can be transformed into the at this point needed format
 * using this python tool:
 * https://github.com/SiliconLabs/wfx-linux-tools/blob/64b958d6db14d6c887e75e8698cf83c4e3a9653f/pds_compress
 */

/**
 * PDS for SLEXP8022A - WF200 Evaluation Board
 *
 * This precompiled version of this PDS has been copied from
 * here:
 *
 * https://github.com/SiliconLabs/wfx-fullMAC-tools/blob/cab4ef709b69cbcc2886d22b628524a0605b3cbc/PDS/brd8022a_pds.h
 */
#define SILABS_WF200_PDS_WF200_EVAL \
	"{a:{a:4,b:0}}", \
	"{b:{a:{a:4,b:0,c:0,d:0,e:A},b:{a:4,b:0,c:0,d:0,e:B},c:{a:4,b:0,c:0,d:0,e:C},d:{a:4,b:0,c:0,d:0,e:D},e:{a:4,b:0,c:0,d:0,e:E},f:{a:4,b:0,c:0,d:0,e:F},g:{a:4,b:0,c:0,d:0,e:G},h:{a:4,b:0,c:0,d:0,e:H},i:{a:4,b:0,c:0,d:0,e:I},j:{a:4,b:0,c:0,d:0,e:J},k:{a:4,b:0,c:0,d:0,e:K},l:{a:4,b:0,c:0,d:1,e:L},m:{a:4,b:0,c:0,d:1,e:M}}}", \
	"{c:{a:{a:4},b:{a:6},c:{a:6,c:0},d:{a:6},e:{a:6},f:{a:6}}}", \
	"{e:{a:{a:3,b:6E,c:6E},b:0,c:0}}", \
	"{h:{e:0,a:50,b:0,d:0,c:[{a:1,b:[0,0,0,0,0,0]},{a:2,b:[0,0,0,0,0,0]},{a:[3,9],b:[0,0,0,0,0,0]},{a:A,b:[0,0,0,0,0,0]},{a:B,b:[0,0,0,0,0,0]},{a:[C,D],b:[0,0,0,0,0,0]},{a:E,b:[0,0,0,0,0,0]}]}}", \
	"{j:{a:0,b:0}}"

/**
 * PDS for SLEXP8023A - WFM200 Evaluation Board
 *
 * This precompiled version of this PDS has been copied from
 * here:
 *
 * https://github.com/SiliconLabs/wfx-fullMAC-tools/blob/cab4ef709b69cbcc2886d22b628524a0605b3cbc/PDS/brd8023a_pds.h
 */
#define SILABS_WF200_PDS_WFM200_EVAL \
	"{a:{a:4,b:0}}", \
	"{b:{a:{a:4,b:0,c:0,d:0,e:A},b:{a:4,b:0,c:0,d:0,e:B},c:{a:4,b:0,c:0,d:0,e:C},d:{a:4,b:0,c:0,d:0,e:D},e:{a:4,b:0,c:0,d:0,e:E},f:{a:4,b:0,c:0,d:0,e:F},g:{a:4,b:0,c:0,d:0,e:G},h:{a:4,b:0,c:0,d:0,e:H},i:{a:4,b:0,c:0,d:0,e:I},j:{a:4,b:0,c:0,d:0,e:J},k:{a:4,b:0,c:0,d:0,e:K},l:{a:4,b:0,c:0,d:1,e:L},m:{a:4,b:0,c:0,d:1,e:M}}}", \
	"{c:{a:{a:4},b:{a:6},c:{a:6,c:0},d:{a:6},e:{a:6},f:{a:6}}}", "{e:{b:0,c:1}}", \
	"{h:{e:0,a:50,b:0,d:0,c:[{a:1,b:[0,0,0,0,0,0]},{a:2,b:[0,0,0,0,0,0]},{a:[3,9],b:[0,0,0,0,0,0]},{a:A,b:[0,0,0,0,0,0]},{a:B,b:[0,0,0,0,0,0]},{a:[C,D],b:[0,0,0,0,0,0]},{a:E,b:[0,0,0,0,0,0]}]}}", \
	"{j:{a:0,b:0}}"

/**
 * PDS for RD4001A - WGM160P Evaluation Board
 *
 * This precompiled version of this PDS has been copied from
 * here:
 *
 * https://github.com/SiliconLabs/wfx-fullMAC-tools/blob/cab4ef709b69cbcc2886d22b628524a0605b3cbc/PDS/brd4001a_pds.h
 */
#define SILABS_WF200_PDS_WGM160P_EVAL \
	"{a:{a:4,b:0}}", \
	"{b:{a:{a:4,b:0,c:0,d:0,e:A},b:{a:4,b:0,c:0,d:0,e:B},c:{a:4,b:0,c:0,d:0,e:C},d:{a:4,b:0,c:0,d:0,e:D},e:{a:4,b:0,c:0,d:0,e:E},f:{a:4,b:0,c:0,d:0,e:F},g:{a:4,b:0,c:0,d:0,e:G},h:{a:4,b:0,c:0,d:0,e:H},i:{a:4,b:0,c:0,d:0,e:I},j:{a:4,b:0,c:0,d:0,e:J},k:{a:4,b:0,c:0,d:0,e:K},l:{a:4,b:0,c:0,d:1,e:L},m:{a:4,b:0,c:0,d:1,e:M}}}", \
	"{c:{a:{a:4},b:{a:6},c:{a:6,c:0},d:{a:6},e:{a:6},f:{a:6}}}", "{e:{b:0,c:1}}", \
	"{h:{e:0,a:50,b:0,d:0,c:[{a:1,b:[0,0,0,0,0,0]},{a:2,b:[0,0,0,0,0,0]},{a:[3,9],b:[0,0,0,0,0,0]},{a:A,b:[0,0,0,0,0,0]},{a:B,b:[0,0,0,0,0,0]},{a:[C,D],b:[0,0,0,0,0,0]},{a:E,b:[0,0,0,0,0,0]}]}}", \
	"{j:{a:0,b:0}}"

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_WIFI_SILABS_WF200_H_ */
