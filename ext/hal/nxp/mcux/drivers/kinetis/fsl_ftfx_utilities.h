/*
* Copyright 2017-2018 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*
*/

#ifndef _FSL_FTFX_UTILITIES_H_
#define _FSL_FTFX_UTILITIES_H_

/*!
 * @addtogroup ftfx_utilities
 * @{
 */
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief Constructs the version number for drivers. */
#if !defined(MAKE_VERSION)
#define MAKE_VERSION(major, minor, bugfix) (((major) << 16) | ((minor) << 8) | (bugfix))
#endif

/*! @brief Constructs a status code value from a group and a code number. */
#if !defined(MAKE_STATUS)
#define MAKE_STATUS(group, code) ((((group)*100) + (code)))
#endif

/*! @brief Constructs the four character code for the Flash driver API key. */
#if !defined(FOUR_CHAR_CODE)
#define FOUR_CHAR_CODE(a, b, c, d) (((d) << 24) | ((c) << 16) | ((b) << 8) | ((a)))
#endif

/*! @brief Alignment(down) utility. */
#if !defined(ALIGN_DOWN)
#define ALIGN_DOWN(x, a) ((x) & (uint32_t)(-((int32_t)(a))))
#endif

/*! @brief Alignment(up) utility. */
#if !defined(ALIGN_UP)
#define ALIGN_UP(x, a) (-((int32_t)((uint32_t)(-((int32_t)(x))) & (uint32_t)(-((int32_t)(a))))))
#endif

/*! @brief bytes2word utility. */
#define B1P4(b) (((uint32_t)(b)&0xFFU) << 24)
#define B1P3(b) (((uint32_t)(b)&0xFFU) << 16)
#define B1P2(b) (((uint32_t)(b)&0xFFU) << 8)
#define B1P1(b) ((uint32_t)(b)&0xFFU)
#define B2P3(b) (((uint32_t)(b)&0xFFFFU) << 16)
#define B2P2(b) (((uint32_t)(b)&0xFFFFU) << 8)
#define B2P1(b) ((uint32_t)(b)&0xFFFFU)
#define B3P2(b) (((uint32_t)(b)&0xFFFFFFU) << 8)
#define B3P1(b) ((uint32_t)(b)&0xFFFFFFU)

#define BYTE2WORD_1_3(x, y) (B1P4(x) | B3P1(y))
#define BYTE2WORD_2_2(x, y) (B2P3(x) | B2P1(y))
#define BYTE2WORD_3_1(x, y) (B3P2(x) | B1P1(y))
#define BYTE2WORD_1_1_2(x, y, z) (B1P4(x) | B1P3(y) | B2P1(z))
#define BYTE2WORD_1_2_1(x, y, z) (B1P4(x) | B2P2(y) | B1P1(z))
#define BYTE2WORD_2_1_1(x, y, z) (B2P3(x) | B1P2(y) | B1P1(z))
#define BYTE2WORD_1_1_1_1(x, y, z, w) (B1P4(x) | B1P3(y) | B1P2(z) | B1P1(w))


#endif /* _FSL_FTFX_UTILITIES_H_ */

