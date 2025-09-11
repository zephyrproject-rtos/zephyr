/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * nRF SoC specific public APIs for MVDMA peripheral.
 */

#ifndef SOC_NORDIC_COMMON_MVDMA_H_
#define SOC_NORDIC_COMMON_MVDMA_H_

#include <zephyr/sys/slist.h>
#include <stddef.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/** MVDMA Attribute field offset. */
#define NRF_MVDMA_ATTR_OFF 24

/** MVDMA Extended attribute field offset. */
#define NRF_MVDMA_EXT_ATTR_OFF 30

/**
 * @anchor NRF_MVDMA_ATTR
 * @name MVDMA job attributes.
 * @{
 */

/** @brief Default transfer. */
#define NRF_MVDMA_ATTR_DEFAULT 7

/** @brief Byte swapping to change the endianness.
 *
 * When set, endianness on the data pointed to by memory address will be swapped.
 */
#define NRF_MVDMA_ATTR_BYTE_SWAP 0

/** @brief Chaining job lists.
 *
 * When set, pointer in the descriptor is treated as a pointer to the next job description.
 */
#define NRF_MVDMA_ATTR_JOB_LINK 1

/** @brief Fill sink address with zeroes.
 *
 * The source list must contain a single null job. This attribute is the exception to the rule
 * that the total number of bytes in the source and sink list must match.
 */
#define NRF_MVDMA_ATTR_ZERO_FILL 2

/** @brief Fixed attributes for all jobs in the descriptor.
 *
 * It can be used to compress the descriptor. With this attribute, only one attribute/size
 * word is read with the first job and the same information is used for the rest of the jobs
 * that only have addresses.
 */
#define NRF_MVDMA_ATTR_FIXED_PARAMS 3

/** @brief Fixed address.
 *
 * This mode can be used to read multiple entries from a peripheral where the output changes
 * every read, for example a TRNG or FIFO.
 */
#define NRF_MVDMA_ATTR_STATIC_ADDR 4

/** @brief Optimization for many short bursts.
 *
 * This attribute can be used to transfer data using bufferable write accesses. MVDMA may receive a
 * write response before the data reaches the destination and starts fetching the next job. This can
 * provide better performance especially when there are many shorts bursts being sent or many small
 * jobs with different user-defined attributes.
 */
#define NRF_MVDMA_ATTR_SHORT_BURSTS 5

/** @brief Calculate CRC
 *
 * This attribute causes a CRC checksum to be calculated for the complete source job list.
 * When used, all jobs in the joblist must have this attribute.
 */
#define NRF_MVDMA_ATTR_CRC 6

/**@} */

/**
 * @anchor NRF_MVDMA_EXT_ATTR
 * @name MVDMA extended job attributes.
 * @{
 */
/** @brief Job is accessing a peripheral register. */
#define NRF_MVDMA_EXT_ATTR_PERIPH 1

/** @brief Enable event on job completion.
 *
 * If this bit is set, an event will be generated when this job descriptor has been processed.
 * An interrupt will be triggered if enabled for that event.
 */
#define NRF_MVDMA_EXT_ATTR_INT 2

/**@} */


/** @brief Signature of a transfer completion handler.
 *
 * @brief user_data User data.
 * @brief status Operation status.
 */
typedef void (*mvdma_handler_t)(void *user_data, int status);

/** @brief Helper macro for building a 32 bit word for job descriptor with attributes and length.
 *
 * @param _size Transfer length in the job.
 * @param _attr Job attribute. See @ref NRF_MVDMA_ATTR.
 * @param _ext_attr Extended job attribute. See @ref NRF_MVDMA_EXT_ATTR.
 */
#define NRF_MVDMA_JOB_ATTR(_size, _attr, _ext_attr)  \
	(((_size) & 0xFFFFFF) | ((_attr) << NRF_MVDMA_ATTR_OFF) | \
	((_ext_attr) << NRF_MVDMA_EXT_ATTR_OFF))

/** @brief Helper macro for building a 2 word job descriptor.
 *
 * @param _ptr Address.
 * @param _size Transfer length in the job.
 * @param _attr Job attribute. See @ref NRF_MVDMA_ATTR.
 * @param _ext_attr Extended job attribute. See @ref NRF_MVDMA_EXT_ATTR.
 */
#define NRF_MVDMA_JOB_DESC(_ptr, _size, _attr, _ext_attr) \
	(uint32_t)(_ptr), NRF_MVDMA_JOB_ATTR(_size, _attr, _ext_attr)

/** @brief Helper macro for creating a termination entry in the array with job descriptors. */
#define NRF_MVDMA_JOB_TERMINATE 0, 0

/** @brief A structure used for a basic transfer that has a single job in source and sink.
 *
 * @note It must be aligned to a data cache line!
 */
struct mvdma_basic_desc {
	uint32_t source;
	uint32_t source_attr_len;
	uint32_t source_terminate;
	uint32_t source_padding;
	uint32_t sink;
	uint32_t sink_attr_len;
	uint32_t sink_terminate;
	uint32_t sink_padding;
};

BUILD_ASSERT(COND_CODE_1(CONFIG_DCACHE,
			 (sizeof(struct mvdma_basic_desc) == CONFIG_DCACHE_LINE_SIZE), (1)));

/** @brief A structure that holds the information about job descriptors.
 *
 * Job descriptors must be data cache line aligned and padded.
 */
struct mvdma_jobs_desc {
	/** Pointer to an array with source job descriptors terminated with 2 null words. */
	uint32_t *source;

	/** Size in bytes of an array with source job descriptors (including 2 null words). */
	size_t source_desc_size;

	/** Pointer to an array with sink job descriptors terminated with 2 null words. */
	uint32_t *sink;

	/** Size in bytes of an array with sink job descriptors (including 2 null words). */
	size_t sink_desc_size;
};

enum mvdma_err {
	NRF_MVDMA_ERR_NO_ERROR,
	NRF_MVDMA_ERR_SOURCE,
	NRF_MVDMA_ERR_SINK,
};

/** @brief A helper macro for initializing a basic descriptor.
 *
 * @param _src Source address.
 * @param _src_len Source length.
 * @param _src_attr Source attributes.
 * @param _src_ext_attr Source extended attributes.
 * @param _sink Source address.
 * @param _sink_len Source length.
 * @param _sink_attr Source attributes.
 * @param _sink_ext_attr Source extended attributes.
 */
#define NRF_MVDMA_BASIC_INIT(_src, _src_len, _src_attr, _src_ext_attr,			\
			     _sink, _sink_len, _sink_attr, _sink_ext_attr)		\
{											\
	.source = (uint32_t)(_src),							\
	.source_attr_len = NRF_MVDMA_JOB_ATTR((_src_len), _src_attr, _src_ext_attr),	\
	.source_terminate = 0,								\
	.source_padding = 0,								\
	.sink = (uint32_t)(_sink),							\
	.sink_attr_len = NRF_MVDMA_JOB_ATTR((_sink_len), _sink_attr, _sink_ext_attr),	\
	.sink_terminate = 0,								\
	.sink_padding = 0,								\
}

/** @brief A helper macro for initializing a basic descriptor to perform a memcpy.
 *
 * @param _dst Destination address.
 * @param _src Source address.
 * @param _len Length.
 */
#define NRF_MVDMA_BASIC_MEMCPY_INIT(_dst, _src, _len)					\
	NRF_MVDMA_BASIC_INIT(_src, _len, NRF_MVDMA_ATTR_DEFAULT, 0,			\
			     _dst, _len, NRF_MVDMA_ATTR_DEFAULT, 0)

/** @brief A helper macro for initializing a basic descriptor to fill a buffer with 0's.
 *
 * @param _ptr Buffer address.
 * @param _len Length.
 */
#define NRF_MVDMA_BASIC_ZERO_INIT(_ptr, _len)						\
	NRF_MVDMA_BASIC_INIT(NULL, 0, 0, 0, _ptr, _len, NRF_MVDMA_ATTR_ZERO_FILL, 0)

/** @brief Control structure used for the MVDMA transfer. */
struct mvdma_ctrl {
	/** Internally used. */
	sys_snode_t node;

	/** User handler. Free running mode if null. */
	mvdma_handler_t handler;

	/** User data passed to the handler. */
	void *user_data;

	uint32_t source;

	uint32_t sink;
};

/** @brief A helper macro for initializing a control structure.
 *
 * @param _handler User handler. If null transfer will be free running.
 * @param _user_data User data passed to the handler.
 */
#define NRF_MVDMA_CTRL_INIT(_handler, _user_data) {					\
	.handler = _handler,								\
	.user_data = _user_data								\
}

/** @brief Perform a basic transfer.
 *
 * Basic transfer allows for a single source and sink job descriptor. Since those descriptors
 * fit into a single data cache line setup is a bit simple and faster than using a generic
 * approach. The driver is handling cache flushing on that descriptor.
 *
 * Completion of the transfer can be reported with the handler called from MVDMA interrupt
 * handler. Alternatively, transfer can be free running and @ref mvdma_xfer_check is
 * used to poll for transfer completion. When free running mode is used then it is mandatory
 * to poll until @ref mvdma_xfer_check reports transfer completion due to the power
 * management.
 *
 * @note @p ctrl structure is owned by the driver during the transfer and must persist during
 * that time and cannot be modified by the user until transfer is finished.
 *
 * @param[in,out] ctrl Control structure. Use null handler for free running mode.
 * @param[in] desc Transfer descriptor. Must be data cache line aligned.
 * @param[in] queue If true transfer will be queued if MVDMA is busy. If false then transfer
 *		    will not be started.
 *
 * retval 0 transfer is successfully started. MVDMA was idle.
 * retval 1 transfer is queued and will start when MVDMA is ready. MVDMA was busy.
 * retval -EBUSY transfer is not started because MVDMA was busy and @p queue was false.
 */
int mvdma_basic_xfer(struct mvdma_ctrl *ctrl, struct mvdma_basic_desc *desc,
			 bool queue);

/** @brief Perform a transfer.
 *
 * User must prepare job descriptors. The driver is handling cache flushing on that descriptors.
 *
 * Completion of the transfer can be reported with the handler called from the MVDMA interrupt
 * handler. Alternatively, transfer can be free running and @ref mvdma_xfer_check is
 * used to poll for transfer completion. When free running mode is used then it is mandatory
 * to poll until @ref mvdma_xfer_check reports transfer completion due to the power
 * management.
 *
 * @note Descriptors must not contain chaining (@ref NRF_MVDMA_ATTR_JOB_LINK) or
 * compressed descriptors (@ref NRF_MVDMA_ATTR_FIXED_PARAMS).
 *
 * @note @p ctrl structure is owned by the driver during the transfer and must persist during
 * that time and cannot be modified by the user until transfer is finished.
 *
 * @param[in,out] ctrl Control structure. Use null handler for free running mode.
 * @param[in] desc Transfer descriptor. Must be data cache line aligned and padded.
 * @param[in] queue If true transfer will be queued if MVDMA is busy. If false then transfer
 *		    will not be started.
 *
 * retval 0 transfer is successfully started. MVDMA was idle.
 * retval 1 transfer is queued and will start when MVDMA is ready. MVDMA was busy.
 * retval -EBUSY transfer is not started because MVDMA was busy and @p queue was false.
 */
int mvdma_xfer(struct mvdma_ctrl *ctrl, struct mvdma_jobs_desc *desc, bool queue);

/** @brief Poll for completion of free running transfer.
 *
 * It is mandatory to poll for the transfer until completion is reported. Not doing that may
 * result in increased power consumption.
 *
 * @param ctrl Control structure used for the transfer.
 *
 * @retval 0 transfer is completed. MVDMA is idle.
 * @retval 1 transfer is completed. MVDMA has started another transfer.
 * @retval 2 transfer is completed. MVDMA continues with other transfer.
 * @retval -EBUSY transfer is in progress.
 * @retval -EIO MVDMA reported an error.
 */
int mvdma_xfer_check(const struct mvdma_ctrl *ctrl);

/** @brief Read and clear error reported by the MDVMA.
 *
 * @return Enum with and error.
 */
enum mvdma_err mvdma_error_check(void);

/** @brief Restore MVDMA configuration.
 *
 * Function must be called when system restores from the suspend to RAM sleep.
 */
void mvdma_resume(void);

/** @brief Initialize MVDMA driver.
 *
 * retval 0 success.
 */
int mvdma_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SOC_NORDIC_COMMON_MVDMA_H_ */
