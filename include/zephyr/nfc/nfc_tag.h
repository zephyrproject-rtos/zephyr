/*
 * Copyright (c) 2023 Sendrato
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file NFC-tag subsystem
 */

#ifndef ZEPHYR_INCLUDE_NFC_TAG_H_
#define ZEPHYR_INCLUDE_NFC_TAG_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <string.h>

/**
 * @brief NFC Tag Events
 */
enum nfc_tag_event {
	/* not used */
	NFC_TAG_EVENT_NONE = 0,
	/* External NFC field detected: tag is selected by reader */
	NFC_TAG_EVENT_FIELD_ON,
	/* External NFC field has been removed / session ended */
	NFC_TAG_EVENT_FIELD_OFF,
	/* When a reader has read all data */
	NFC_TAG_EVENT_READ_DONE,
	/* When all data has been written to device */
	NFC_TAG_EVENT_WRITE_DONE,
	/* NFC driver has stopped */
	NFC_TAG_EVENT_STOPPED,
	/* When data is successfully sent to reader */
	NFC_TAG_EVENT_DATA_TRANSMITTED,
	/* Data-block sent by reader */
	NFC_TAG_EVENT_DATA_IND,
	/* Last-block of data which has been sent by reader */
	NFC_TAG_EVENT_DATA_IND_DONE,
};

/**
 * @brief NFC tag types in which subsysten can operate.
 */
enum nfc_tag_type {
	NFC_TAG_TYPE_UNDEF = 0,
	NFC_TAG_TYPE_T2T,
	NFC_TAG_TYPE_T4T
};

/**
 * @brief NFC CMDs
 */
enum nfc_tag_cmd {
	/* not used */
	NFC_TAG_CMD_NONE = 0,
	/* External NFC field detected: tag is selected by reader */
	NFC_TAG_CMD_FIELD_ON,
};

/**
 * @brief NFC driver callback
 *
 * @param[in] *dev      : Pointer to NFC device which triggered interrupt.
 * @param[in] event     : @ref(nfc_tag_event) event.
 * @param[in] *data     : Pointer to databuf of event. Might be NULL.
 * @param[in] data_len  : Length of the databuf. Might be 0.
 */
typedef void (*nfc_tag_cb_t)(const struct device *dev, enum nfc_tag_event event,
			     const uint8_t *data, size_t data_len);

/** DRIVER API TYPEDEFS **/

typedef int (*nfc_tag_init_t)(const struct device *dev, nfc_tag_cb_t cb);

typedef int (*nfc_tag_set_type_t)(const struct device *dev, enum nfc_tag_type type);

typedef int (*nfc_tag_get_type_t)(const struct device *dev, enum nfc_tag_type *type);

typedef int (*nfc_tag_start_t)(const struct device *dev);

typedef int (*nfc_tag_stop_t)(const struct device *dev);

typedef int (*nfc_tag_set_ndef_t)(const struct device *dev, uint8_t *buf, uint16_t len);

typedef int (*nfc_tag_cmd_t)(const struct device *dev, enum nfc_tag_cmd cmd, uint8_t *buf,
			     uint16_t *buf_len);

struct nfc_tag_driver_api {
	nfc_tag_init_t init;
	nfc_tag_set_type_t set_type;
	nfc_tag_get_type_t get_type;
	nfc_tag_start_t start;
	nfc_tag_stop_t stop;
	nfc_tag_set_ndef_t set_ndef;
	nfc_tag_cmd_t cmd;
};

/**
 * @brief Initialize NFC Tag subsystem
 *
 * @param[in] *dev : Pointer to NFC device
 * @param[in] cb   : @ref(nfc_tag_cb_t) callback for event handling
 * @return         : 0 on success, negative upon error.
 */
static inline int nfc_tag_init(const struct device *dev, nfc_tag_cb_t cb)
{
	const struct nfc_tag_driver_api *api = (const struct nfc_tag_driver_api *)dev->api;

	return api->init(dev, cb);
}

/**
 * @brief Set specific NFC Tag mode
 *
 * @param[in] *dev : Pointer to NFC device
 * @param[in] cb   : @ref(nfc_tag_type)
 * @return         : 0 on success, negative upon error.
 */
static inline int nfc_tag_set_type(const struct device *dev, enum nfc_tag_type type)
{
	const struct nfc_tag_driver_api *api = (const struct nfc_tag_driver_api *)dev->api;

	return api->set_type(dev, type);
}

/**
 * @brief Read configured NFC Tag mode
 *
 * @param[in] *dev : Pointer to NFC device
 * @param[in] cb   : @ref(nfc_tag_type)
 * @return         : 0 on success, negative upon error.
 */
static inline int nfc_tag_get_type(const struct device *dev, enum nfc_tag_type *type)
{
	const struct nfc_tag_driver_api *api = (const struct nfc_tag_driver_api *)dev->api;

	return api->get_type(dev, type);
}

/**
 * @brief Activate NFC Tag Field
 *
 * @param[in] *dev : Pointer to NFC device
 * @return         : 0 on success, negative upon error.
 */
static inline int nfc_tag_start(const struct device *dev)
{
	const struct nfc_tag_driver_api *api = (const struct nfc_tag_driver_api *)dev->api;

	return api->start(dev);
}

/**
 * @brief Stop NFC Tag Field
 *
 * @param[in] *dev : Pointer to NFC device
 * @return         : 0 on success, negative upon error.
 */
static inline int nfc_tag_stop(const struct device *dev)
{
	const struct nfc_tag_driver_api *api = (const struct nfc_tag_driver_api *)dev->api;

	return api->stop(dev);
}

/**
 * @brief Write payload data NFC device.
 *
 * This data will be read by a NFC-reader upon a READ request.
 *
 * @param[in] *dev     : Pointer to NFC device
 * @param[in] *buf     : Pointer to buffer to be written
 * @param[in]  buf_len : Buffer length.
 * @return             : 0 on success, negative upon error.
 */
static inline int nfc_tag_set_ndef(const struct device *dev, uint8_t *buf, uint16_t buf_len)
{
	const struct nfc_tag_driver_api *api = (const struct nfc_tag_driver_api *)dev->api;

	return api->set_ndef(dev, buf, buf_len);
}

/**
 * @brief Set driver specific setting.
 *
 * This a generic interface allowing customisation of the low-level driver,
 * like setting of specific data-register. Available commands and their effect
 * is driver-implementation depending.
 *
 * @param[in]     *dev     : Pointer to NFC device
 * @param[in]      cmd     : @ref(enum nfc_tag_cmd) command
 * @param[in,out] *buf     : Pointer to buffer which can hold data to be written
 *                           or allocated memory to store read-data.
 * @param[in,out] *buf_len : Pointer to buffer size. When writing, it shall
 *                           specify the length of @ref(buf). When reading it
 *                           shall hold the length of the returned buf.
 * @return                 : 0 on success, negative upon error.
 */
static inline int nfc_tag_cmd(const struct device *dev, enum nfc_tag_cmd cmd, uint8_t *buf,
			      uint16_t *buf_len)
{
	const struct nfc_tag_driver_api *api = (const struct nfc_tag_driver_api *)dev->api;

	return api->cmd(dev, cmd, buf, buf_len);
}

#endif /* ZEPHYR_INCLUDE_NFC_TAG_H_ */
