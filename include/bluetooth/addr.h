/** @file
 *  @brief Bluetooth device address definitions and utilities.
 */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_ADDR_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_ADDR_H_

#include <string.h>
#include <sys/printk.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bluetooth device address definitions and utilities.
 * @defgroup bt_addr Device Address
 * @ingroup bluetooth
 * @{
 */

#define BT_ADDR_LE_PUBLIC       0x00
#define BT_ADDR_LE_RANDOM       0x01
#define BT_ADDR_LE_PUBLIC_ID    0x02
#define BT_ADDR_LE_RANDOM_ID    0x03

/** Bluetooth Device Address */
typedef struct {
	uint8_t  val[6];
} bt_addr_t;

/** Bluetooth LE Device Address */
typedef struct {
	uint8_t      type;
	bt_addr_t a;
} bt_addr_le_t;

#define BT_ADDR_ANY     ((bt_addr_t[]) { { { 0, 0, 0, 0, 0, 0 } } })
#define BT_ADDR_NONE    ((bt_addr_t[]) { { \
			 { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } } })
#define BT_ADDR_LE_ANY  ((bt_addr_le_t[]) { { 0, { { 0, 0, 0, 0, 0, 0 } } } })
#define BT_ADDR_LE_NONE ((bt_addr_le_t[]) { { 0, \
			 { { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } } } })

static inline int bt_addr_cmp(const bt_addr_t *a, const bt_addr_t *b)
{
	return memcmp(a, b, sizeof(*a));
}

static inline int bt_addr_le_cmp(const bt_addr_le_t *a, const bt_addr_le_t *b)
{
	return memcmp(a, b, sizeof(*a));
}

static inline void bt_addr_copy(bt_addr_t *dst, const bt_addr_t *src)
{
	memcpy(dst, src, sizeof(*dst));
}

static inline void bt_addr_le_copy(bt_addr_le_t *dst, const bt_addr_le_t *src)
{
	memcpy(dst, src, sizeof(*dst));
}

#define BT_ADDR_IS_RPA(a)     (((a)->val[5] & 0xc0) == 0x40)
#define BT_ADDR_IS_NRPA(a)    (((a)->val[5] & 0xc0) == 0x00)
#define BT_ADDR_IS_STATIC(a)  (((a)->val[5] & 0xc0) == 0xc0)

#define BT_ADDR_SET_RPA(a)    ((a)->val[5] = (((a)->val[5] & 0x3f) | 0x40))
#define BT_ADDR_SET_NRPA(a)   ((a)->val[5] &= 0x3f)
#define BT_ADDR_SET_STATIC(a) ((a)->val[5] |= 0xc0)

int bt_addr_le_create_nrpa(bt_addr_le_t *addr);
int bt_addr_le_create_static(bt_addr_le_t *addr);

static inline bool bt_addr_le_is_rpa(const bt_addr_le_t *addr)
{
	if (addr->type != BT_ADDR_LE_RANDOM) {
		return false;
	}

	return BT_ADDR_IS_RPA(&addr->a);
}

static inline bool bt_addr_le_is_identity(const bt_addr_le_t *addr)
{
	if (addr->type == BT_ADDR_LE_PUBLIC) {
		return true;
	}

	return BT_ADDR_IS_STATIC(&addr->a);
}

/**
 * @def BT_ADDR_STR_LEN
 *
 * @brief Recommended length of user string buffer for Bluetooth address
 *
 * @details The recommended length guarantee the output of address
 * conversion will not lose valuable information about address being
 * processed.
 */
#define BT_ADDR_STR_LEN 18

/**
 * @def BT_ADDR_LE_STR_LEN
 *
 * @brief Recommended length of user string buffer for Bluetooth LE address
 *
 * @details The recommended length guarantee the output of address
 * conversion will not lose valuable information about address being
 * processed.
 */
#define BT_ADDR_LE_STR_LEN 30

/**
 * @brief Converts binary Bluetooth address to string.
 *
 * @param addr Address of buffer containing binary Bluetooth address.
 * @param str Address of user buffer with enough room to store formatted
 * string containing binary address.
 * @param len Length of data to be copied to user string buffer. Refer to
 * BT_ADDR_STR_LEN about recommended value.
 *
 * @return Number of successfully formatted bytes from binary address.
 */
static inline int bt_addr_to_str(const bt_addr_t *addr, char *str, size_t len)
{
	return snprintk(str, len, "%02X:%02X:%02X:%02X:%02X:%02X",
			addr->val[5], addr->val[4], addr->val[3],
			addr->val[2], addr->val[1], addr->val[0]);
}

/**
 * @brief Converts binary LE Bluetooth address to string.
 *
 * @param addr Address of buffer containing binary LE Bluetooth address.
 * @param str Address of user buffer with enough room to store
 * formatted string containing binary LE address.
 * @param len Length of data to be copied to user string buffer. Refer to
 * BT_ADDR_LE_STR_LEN about recommended value.
 *
 * @return Number of successfully formatted bytes from binary address.
 */
static inline int bt_addr_le_to_str(const bt_addr_le_t *addr, char *str,
				    size_t len)
{
	char type[10];

	switch (addr->type) {
	case BT_ADDR_LE_PUBLIC:
		strcpy(type, "public");
		break;
	case BT_ADDR_LE_RANDOM:
		strcpy(type, "random");
		break;
	case BT_ADDR_LE_PUBLIC_ID:
		strcpy(type, "public-id");
		break;
	case BT_ADDR_LE_RANDOM_ID:
		strcpy(type, "random-id");
		break;
	default:
		snprintk(type, sizeof(type), "0x%02x", addr->type);
		break;
	}

	return snprintk(str, len, "%02X:%02X:%02X:%02X:%02X:%02X (%s)",
			addr->a.val[5], addr->a.val[4], addr->a.val[3],
			addr->a.val[2], addr->a.val[1], addr->a.val[0], type);
}

/**
 * @brief Convert Bluetooth address from string to binary.
 *
 * @param[in]  str   The string representation of a Bluetooth address.
 * @param[out] addr  Address of buffer to store the Bluetooth address
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_addr_from_str(const char *str, bt_addr_t *addr);

/**
 * @brief Convert LE Bluetooth address from string to binary.
 *
 * @param[in]  str   The string representation of an LE Bluetooth address.
 * @param[in]  type  The string representation of the LE Bluetooth address
 *                   type.
 * @param[out] addr  Address of buffer to store the LE Bluetooth address
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_addr_le_from_str(const char *str, const char *type, bt_addr_le_t *addr);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_ADDR_H_ */
