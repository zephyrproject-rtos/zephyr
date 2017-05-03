/*
 * nm_types.h
 *
 *  Created on: May 8, 2017
 *      Author: max
 */

#ifndef ZEPHYR_EXT_DRIVERS_ATMEL_WINC1500_COMMON_INCLUDE_NM_TYPES_H_
#define ZEPHYR_EXT_DRIVERS_ATMEL_WINC1500_COMMON_INCLUDE_NM_TYPES_H_

/*!<
 *        Attribute used to define memory section to map Functions in host memory.
 */
#define CONST const
/*!<
 *     Used for code portability.
 */


#define BSP_MIN(x, y) ((x) > (y)?(y):(x))
/*!<
 *     Computes the minimum of \b x and \b y.
 */


/**@defgroup  DataT  DataTypes
 * @ingroup nm_bsp
 * @{
 */

/*!
 * @ingroup DataTypes
 * @typedef      unsigned char	uint8;
 * @brief        Range of values between 0 to 255
 */
typedef unsigned char	uint8;

/*!
 * @ingroup DataTypes
 * @typedef      unsigned short	uint16;
 * @brief        Range of values between 0 to 65535
 */
typedef unsigned short	uint16;

/*!
 * @ingroup Data Types
 * @typedef      unsigned long	uint32;
 * @brief        Range of values between 0 to 4294967295
 */
typedef unsigned long	uint32;


/*!
 * @ingroup Data Types
 * @typedef      signed char		sint8;
 * @brief        Range of values between -128 to 127
 */
typedef signed char		sint8;

/*!
 * @ingroup DataTypes
 * @typedef      signed short	sint16;
 * @brief        Range of values between -32768 to 32767
 */
typedef signed short	sint16;

/*!
 * @ingroup DataTypes
 * @typedef      signed long		sint32;
 * @brief        Range of values between -2147483648 to 2147483647
 */

typedef signed long		sint32;



#ifdef _NM_BSP_BIG_END
#define NM_BSP_B_L_32(x) \
((((x) & 0x000000FF) << 24) + \
(((x) & 0x0000FF00) << 8)  + \
(((x) & 0x00FF0000) >> 8)   + \
(((x) & 0xFF000000) >> 24))
#define NM_BSP_B_L_16(x) \
((((x) & 0x00FF) << 8) + \
(((x)  & 0xFF00) >> 8))
#else
#define NM_BSP_B_L_32(x)  (x)
#define NM_BSP_B_L_16(x)  (x)
#endif

#endif /* ZEPHYR_EXT_DRIVERS_ATMEL_WINC1500_COMMON_INCLUDE_NM_TYPES_H_ */
