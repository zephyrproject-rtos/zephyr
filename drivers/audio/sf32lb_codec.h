/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_AUDIO_SF32LB_CODEC_H_
#define ZEPHYR_INCLUDE_AUDIO_SF32LB_CODEC_H_

/**
 * @defgroup sf32lb_codec_interface SF32LB_CODEC
 * @since 4.2
 * @version 1.0.0
 * @ingroup io_interfaces
 * @brief Interfaces for SiFli onchip codec controllers.
 *
 * @{
 */

#include <errno.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SF32LB_CODEC_NAME   "sf32lb_codec@0"

/**
 * @brief SF32LB Audio Direction
 */
enum sf32lb_audio_dir {
	/** Transmit data */
	SF32LB_AUDIO_TX = BIT(0),
	/** Receive data */
	SF32LB_AUDIO_RX = BIT(1),
	/** Both receive and transmit data */
	SF32LB_AUDIO_TXRX = SF32LB_AUDIO_RX | SF32LB_AUDIO_TX,
};

struct sf32lb_codec_cfg {
	/** audio direction bit map*/
	enum sf32lb_audio_dir dir;
	/** one word bits*/
	uint8_t bit_width;
	/** channles */
	uint8_t channels;
	/** samperate*/
	uint32_t format;
	/** Size of one RX/TX memory block (buffer) in bytes. */
	uint32_t block_size;

	uint32_t samplerate;

	void (*tx_done)(void);
	void (*rx_done)(uint8_t *pbuf, uint32_t len);
	/** reserved now, should be zero*/
	uint32_t reserved;

};

struct sf32lb_codec_driver_api {
	int (*configure)(const struct device *dev, struct sf32lb_codec_cfg *cfg);
	void (*start)(const struct device *dev, enum sf32lb_audio_dir dir);
	void (*stop)(const struct device *dev, enum sf32lb_audio_dir dir);
	/** dac volume, volume rang is [0, 15] */
	void (*set_dac_volume)(const struct device *dev, uint8_t volume);
	/** mute or unmute dac */
	void (*set_dac_mute)(const struct device *dev, bool is_mute);
	int (*write)(const struct device *dev, const uint8_t *data, uint32_t len);
};

/**
 * @brief config sf32lb codec
 *
 * @param dev Pointer to the device structure for codec driver instance.
 * @param cfg The codec config to set
 *
 * @return 0 on success, negative error code on failure
 */
static inline int sf32lb_codec_api_config(const struct device *dev, struct sf32lb_codec_cfg *cfg)
{
	const struct sf32lb_codec_driver_api *api =
		(const struct sf32lb_codec_driver_api *)dev->api;

	return api->configure(dev, cfg);
}

/**
 * @brief start sf32lb codec
 *
 * @param dev Pointer to the device structure for codec driver instance.
 * @param dir The codec direction to start
 *
 * @return void
 */
static inline void sf32lb_codec_api_start(const struct device *dev, enum sf32lb_audio_dir dir)
{
	const struct sf32lb_codec_driver_api *api =
		(const struct sf32lb_codec_driver_api *)dev->api;

	return api->start(dev, dir);
}

/**
 * @brief stop sf32lb codec
 *
 * @param dev Pointer to the device structure for codec driver instance.
 * @param dir The codec direction to stop
 *
 * @return void
 */
static inline void sf32lb_codec_api_stop(const struct device *dev, enum sf32lb_audio_dir dir)
{
	const struct sf32lb_codec_driver_api *api =
		(const struct sf32lb_codec_driver_api *)dev->api;

	return api->stop(dev, dir);
}

/**
 * @brief set sf32lb codec dac volume, invalid when dac is muted
 *
 * @param dev Pointer to the device structure for codec driver instance.
 * @param volume volume from dac, volume rang is [0, 15]
 *
 * @return void
 */
static inline void sf32lb_codec_api_set_dac_volume(const struct device *dev, uint8_t volume)
{
	const struct sf32lb_codec_driver_api *api =
		(const struct sf32lb_codec_driver_api *)dev->api;

	return api->set_dac_volume(dev, volume);
}

/**
 * @brief set sf32lb codec dac volume
 *
 * @param dev Pointer to the device structure for codec driver instance.
 * @param is_mute mute or umute dac
 *
 * @return void
 */
static inline void sf32lb_codec_api_set_dac_mute(const struct device *dev, bool is_mute)
{
	const struct sf32lb_codec_driver_api *api =
		(const struct sf32lb_codec_driver_api *)dev->api;

	api->set_dac_mute(dev, is_mute);
}

/**
 * @brief write data to  sf32lb codec dac
 *
 * @param dev Pointer to the device structure for codec driver instance.
 * @param data pcm data to write to dac
 * @param len pcm data length in bytes
 *
 * @return 0 on success, negative error code on failure
 */
static inline int sf32lb_codec_api_write(const struct device *dev, const uint8_t *data, uint32_t len)
{
	const struct sf32lb_codec_driver_api *api =
		(const struct sf32lb_codec_driver_api *)dev->api;

	return api->write(dev, data, len);
}

/**
 * @brief get sf32lb codec device
 *
 * @return A pointer to the device objec
 */
static inline const struct device * sf32lb_codec_api_find(void)
{
    //todo: using device tree
    return NULL;
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_AUDIO_SF32LB_CODEC_H_ */



