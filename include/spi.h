/* spi.h - public SPI driver API */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __SPI_H__
#define __SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* SPI Polarity & Phase Modes */
#define SPI_MODE_CPOL		0x0
#define SPI_MODE_CPHA		0x1
#define SPI_MODE_LOOP		0x2

#define SPI_MODE_MASK		(0x3)
#define SPI_MODE(_in_)		(_in_ & SPI_MODE_MASK)

/* SPI Transfer modes (host controller dependent) */
#define SPI_TRANSFER_MSB	(0 << 2)
#define SPI_TRANSFER_LSB	(1 << 2)

#define SPI_TRANSFER_MASK	(0x4)

#define SPI_WORD_SIZE_MASK	(0xFF << 3)
#define SPI_WORD_SIZE_GET(_in_) ((_in_ & SPI_WORD_SIZE_MASK) >> 3)

/* application callback function signature */
typedef void (*spi_callback)(struct device *dev);

/*
 * config is a bit field with the following parts:
 * mode			[ 0 : 1 ]   - Polarity and phase mode
 * transfer_mode	[ 2 ]       - LSB or MSB first transfer mode
 * word_size		[ 3 : 10 ]  - Size of a data train in bits
 * RESERVED		[ 11 : 31 ] - undefined usage
 *
 * max_sys_freq is the maximum frequency supported by the slave it
 * will deal with. This value depends on the host controller (the driver
 * may present a specific format for setting it).
 */
struct spi_config {
	uint32_t	config;
	uint32_t	max_sys_freq;
	spi_callback	cb_receive;
	spi_callback	cb_transmit;
};

typedef int (*spi_api_configure)(struct device *dev, struct spi_config *config);
typedef int (*spi_api_io)(struct device *dev, uint8_t *buf, uint32_t len);
typedef int (*spi_api_control)(struct device *dev);

struct spi_driver_api {
	spi_api_configure configure;
	spi_api_io read;
	spi_api_io write;
	spi_api_control suspend;
	spi_api_control resume;
};

/**
 * @brief Configure a host controller for operating against slaves
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to the application provided configuration
 */
inline int spi_configure(struct device *dev, struct spi_config *config)
{
	struct spi_driver_api *api = (struct spi_driver_api *)dev->driver_api;
	return api->configure(dev, config);
}

/**
 * @brief Read a defined amount of data from an SPI driver
 * @param dev Pointer to the device structure for the driver instance
 * @param buf Memory pool that data should be transferred to
 * @param len Size of the memory pool available for writing to
 */
inline int spi_read(struct device *dev, uint8_t *buf, uint32_t len)
{
	struct spi_driver_api *api = (struct spi_driver_api *)dev->driver_api;
	return api->read(dev, buf, len);
}

/**
 * @brief Write a defined amount of data through an SPI driver
 * @param dev Pointer to the device structure for the driver instance
 * @param buf Memory pool that data should be transferred from
 * @param len Size of the memory pool available for reading from
 */
inline int spi_write(struct device *dev, uint8_t *buf, uint32_t len)
{
	struct spi_driver_api *api = (struct spi_driver_api *)dev->driver_api;
	return api->write(dev, buf, len);
}

/**
 * @brief Suspend the SPI host controller operations
 * @param dev Pointer to the device structure for the driver instance
 */
inline int spi_suspend(struct device *dev)
{
	struct spi_driver_api *api = (struct spi_driver_api *)dev->driver_api;
	return api->suspend(dev, buf, len);
}

/**
 * @brief Resume the SPI host controller operations
 * @param dev Pointer to the device structure for the driver instance
 */
inline int spi_resume(struct device *dev)
{
	struct spi_driver_api *api = (struct spi_driver_api *)dev->driver_api;
	return api->resume(dev, buf, len);
}

#ifdef __cplusplus
}
#endif

#endif /* __SPI_H__ */
