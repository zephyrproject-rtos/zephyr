/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for SMBus Driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SMBUS_H_
#error "Should only be included by zephyr/drivers/smbus.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_SMBUS_INTERNAL_SMBUS_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_SMBUS_INTERNAL_SMBUS_IMPL_H_

#include <errno.h>
#include <zephyr/sys/slist.h>
#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_smbus_configure(const struct device *dev,
					 uint32_t dev_config)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	return api->configure(dev, dev_config);
}

static inline int z_impl_smbus_get_config(const struct device *dev,
					  uint32_t *dev_config)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->get_config == NULL) {
		return -ENOSYS;
	}

	return api->get_config(dev, dev_config);
}

static inline int z_impl_smbus_smbalert_remove_cb(const struct device *dev,
						  struct smbus_callback *cb)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_smbalert_remove_cb == NULL) {
		return -ENOSYS;
	}

	return api->smbus_smbalert_remove_cb(dev, cb);
}

static inline int z_impl_smbus_host_notify_remove_cb(const struct device *dev,
						     struct smbus_callback *cb)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_host_notify_remove_cb == NULL) {
		return -ENOSYS;
	}

	return api->smbus_host_notify_remove_cb(dev, cb);
}

static inline int z_impl_smbus_quick(const struct device *dev, uint16_t addr,
				     enum smbus_direction direction)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_quick == NULL) {
		return -ENOSYS;
	}

	if (direction != SMBUS_MSG_READ && direction != SMBUS_MSG_WRITE) {
		return -EINVAL;
	}

	return  api->smbus_quick(dev, addr, direction);
}

static inline int z_impl_smbus_byte_write(const struct device *dev,
					  uint16_t addr, uint8_t byte)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_byte_write == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_byte_write(dev, addr, byte);
}

static inline int z_impl_smbus_byte_read(const struct device *dev,
					 uint16_t addr, uint8_t *byte)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_byte_read == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_byte_read(dev, addr, byte);
}

static inline int z_impl_smbus_byte_data_write(const struct device *dev,
					       uint16_t addr, uint8_t cmd,
					       uint8_t byte)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_byte_data_write == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_byte_data_write(dev, addr, cmd, byte);
}

static inline int z_impl_smbus_byte_data_read(const struct device *dev,
					      uint16_t addr, uint8_t cmd,
					      uint8_t *byte)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_byte_data_read == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_byte_data_read(dev, addr, cmd, byte);
}

static inline int z_impl_smbus_word_data_write(const struct device *dev,
					       uint16_t addr, uint8_t cmd,
					       uint16_t word)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_word_data_write == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_word_data_write(dev, addr, cmd, word);
}

static inline int z_impl_smbus_word_data_read(const struct device *dev,
					      uint16_t addr, uint8_t cmd,
					      uint16_t *word)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_word_data_read == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_word_data_read(dev, addr, cmd, word);
}

static inline int z_impl_smbus_pcall(const struct device *dev,
				     uint16_t addr, uint8_t cmd,
				     uint16_t send_word, uint16_t *recv_word)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_pcall == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_pcall(dev, addr, cmd, send_word, recv_word);
}

static inline int z_impl_smbus_block_write(const struct device *dev,
					   uint16_t addr, uint8_t cmd,
					   uint8_t count, uint8_t *buf)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_block_write == NULL) {
		return -ENOSYS;
	}

	if (count < 1 || count > SMBUS_BLOCK_BYTES_MAX) {
		return -EINVAL;
	}

	return  api->smbus_block_write(dev, addr, cmd, count, buf);
}

static inline int z_impl_smbus_block_read(const struct device *dev,
					  uint16_t addr, uint8_t cmd,
					  uint8_t *count, uint8_t *buf)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_block_read == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_block_read(dev, addr, cmd, count, buf);
}

static inline int z_impl_smbus_block_pcall(const struct device *dev,
					   uint16_t addr, uint8_t cmd,
					   uint8_t snd_count, uint8_t *snd_buf,
					   uint8_t *rcv_count, uint8_t *rcv_buf)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_block_pcall == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_block_pcall(dev, addr, cmd, snd_count, snd_buf,
				       rcv_count, rcv_buf);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SMBUS_INTERNAL_SMBUS_IMPL_H_ */
