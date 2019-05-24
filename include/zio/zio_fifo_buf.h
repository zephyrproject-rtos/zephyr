/*
 * Copyright (c) 2019 Thomas Burdick <thomas.burdick@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file zio_fifo_buf.h
 *
 * @brief A ZIO Buffer implementation backed by a FIFO
 *
 * Implements the zio_buf interface using a zio_fifo. This can be used by
 * hardware drivers to provide a software implementation of a FIFO.
 *
 * The expectation is that each driver instance is statically allocated
 * along with an instance of a zio_fifo_buf, sized using Kconfig options
 * specific to the driver instance, where the fifo element is driver specific
 * and self described by the drivers statically defined channel array.
 */

#ifndef ZEPHYR_INCLUDE_ZIO_FIFO_BUF_H_
#define ZEPHYR_INCLUDE_ZIO_FIFO_BUF_H_

#include <zio/zio_fifo.h>
#include <zio/zio_buf.h>

/**
 * @private
 * @brief Software FIFO implementation of a zio_buf
 */
struct z_zio_fifo_buf {
	u32_t watermark;
	struct zio_fifo *fifo;
	struct zio_buf *zbuf;
};

/**
 * @brief Statically initialize an anonymous representing a zio_fifo_buf
 *
 * Statically initialize a zio_fifo_buf with a fixed number of elements of a
 * given type
 */
#define ZIO_FIFO_BUF_INITIALIZER(name, type, pow) \
	{ \
		.buf = { \
			.fifo = &(name).fifo.zfifo, \
			.watermark = 1, \
			.zbuf = NULL, \
		}, \
		.fifo = ZIO_FIFO_INITIALIZER((name.fifo), type, pow) \
	}

/**
 * @brief Declare an anonymous struct type for a zio_fifo_buf
 *
 * Declare a zio_fifo_buf with a fixed number of elements of a given type
 *
 * @param name Name of the zio_fifo_buf
 * @param type Element type stored in the FIFO
 * @param pow Power of 2 to size the fifo
 *
 */
#define ZIO_FIFO_BUF_DECLARE(name, type, pow) \
	struct { \
		struct z_zio_fifo_buf buf; \
		ZIO_FIFO_DECLARE(fifo, type, pow); \
	} name

/**
 * @brief Define a zio_fifo_buf
 *
 * @param name Name of the zio_fifo_buf
 * @param type Element type stored in the FIFO
 * @param pow Power of 2 to size the fifo
 */
#define ZIO_FIFO_BUF_DEFINE(name, type, pow) \
	ZIO_FIFO_BUF_DECLARE(name, type, pow) =	\
		ZIO_FIFO_BUF_INITIALIZER(name, type, pow)

/**
 * @brief Push a datum into the fifo notifying event pollers if needed
 *
 * If a zio_buf is attached and POLL is enabled poller are notified
 *
 * @param fifobuf Pointer to a "zio_fifo_buf" definition
 * @param datum Value to push into zio_fifo_buf
 *
 * @return Returns 1 if watermark has been reached, 0 otherwise
 */
#define zio_fifo_buf_push(fifobuf, datum)				\
	({								\
		int ret = 0;						\
		zio_fifo_push(&(fifobuf)->fifo, datum);			\
		u32_t length = zio_fifo_used(&(fifobuf)->fifo);   \
		if (length >= (fifobuf)->buf.watermark) {		\
			if ((fifobuf)->buf.zbuf != NULL) {		\
				z_zio_buf_sem_give((fifobuf)->buf.zbuf, 1); \
			}						\
			ret = 1;					\
		}							\
		ret;							\
	})

/**
 * @brief Attach a zio_buf to this zio_fifo_buf
 *
 * @param fifobuf Pointer to a "zio_fifo_buf" definition
 * @param ziobuf Pointer to a zio_buf
 *
 * @return Returns 0 on succees, -EINVAL if a zio_buf was already attached.
 */
#define zio_fifo_buf_attach(fifobuf, ziobuf) \
	({ \
		int ret = 0; \
		if ((fifobuf)->buf.zbuf != NULL) { \
			ret = -EINVAL; \
		} else { \
			(fifobuf)->buf.zbuf = (ziobuf); \
			z_zio_buf_attach((ziobuf), &zio_fifo_buf_api, \
					&((fifobuf)->buf)); \
		} \
		ret; \
	})

/**
 * @brief Detach a zio_buf from this zio_fifo_buf
 *
 * @param fifobuf Pointer to a "zio_fifo_buf" definition
 */
#define zio_fifo_buf_detach(fifobuf) \
	({ \
		int ret = 0; \
		if ((fifobuf)->buf.zbuf != NULL) { \
			z_zio_buf_detach((fifobuf)->buf.zbuf); \
		} \
		(fifobuf)->buf.zbuf = NULL; \
		ret; \
	})


extern struct zio_buf_api zio_fifo_buf_api;

#endif /* ZEPHYR_INCLUDE_ZIO_FIFO_BUF_H_ */
