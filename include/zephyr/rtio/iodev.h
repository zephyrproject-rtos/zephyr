/*
 * SPDX-FileCopyrightText: Copyright (c) 2022 Intel Corporation
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RTIO I/O Device and Related Functions
 */

#ifndef ZEPHYR_INCLUDE_RTIO_IODEV_H_
#define ZEPHYR_INCLUDE_RTIO_IODEV_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup rtio
 * @{
 */

/** @cond ignore */
struct rtio_iodev_sqe;
/** @endcond */

/**
 * @brief API that an RTIO IO device should implement
 */
struct rtio_iodev_api {
	/**
	 * @brief Submit to the iodev an entry to work on
	 *
	 * This call should be short in duration and most likely
	 * either enqueue or kick off an entry with the hardware.
	 *
	 * @param iodev_sqe Submission queue entry
	 */
	void (*submit)(struct rtio_iodev_sqe *iodev_sqe);
};

/**
 * @brief An IO device with a function table for submitting requests
 */
struct rtio_iodev {
	/* Function pointer table */
	const struct rtio_iodev_api *api;

	/* Data associated with this iodev */
	void *data;
};

/* Do not try and reformat the macros */
/* clang-format off */

/**
 * @brief Statically define and initialize an RTIO IODev
 *
 * @param name Name of the iodev
 * @param iodev_api Pointer to struct rtio_iodev_api
 * @param iodev_data Data pointer
 */
#define RTIO_IODEV_DEFINE(name, iodev_api, iodev_data)		\
	STRUCT_SECTION_ITERABLE(rtio_iodev, name) = {		\
		.api = (iodev_api),				\
		.data = (iodev_data),				\
	}

/* clang-format on */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RTIO_IODEV_H_ */
