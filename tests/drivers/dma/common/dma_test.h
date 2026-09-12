/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_DRIVERS_DMA_COMMON_DMA_TEST_H_
#define TESTS_DRIVERS_DMA_COMMON_DMA_TEST_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

/*
 * The board describes each DMA controller under test in a
 * test-dma-memory-to-memory node, see dts/bindings/test.
 */
#define DT_DRV_COMPAT test_dma_memory_to_memory

#define DMA_TEST_DEV_COUNT DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)

BUILD_ASSERT(DMA_TEST_DEV_COUNT > 0, "no test-dma-memory-to-memory node enabled");

/* Largest transfer the tests do when no controller under test limits it. */
#define DMA_TEST_DEFAULT_TRANSFER_SIZE 8192

/* Most controllers under test the table below and the region choice handle. */
#define DMA_TEST_MAX_DEVS 4

BUILD_ASSERT(DMA_TEST_DEV_COUNT <= DMA_TEST_MAX_DEVS, "too many test-dma-memory-to-memory nodes");

/* Absent instances get the default, so the minimum below covers every one. */
#define DMA_TEST_INST_TRANSFER_SIZE(inst)                                                          \
	DT_INST_PROP_OR(inst, max_transfer_size, DMA_TEST_DEFAULT_TRANSFER_SIZE)

/* Transfer size every controller under test accepts, in bytes. */
#define DMA_TEST_TRANSFER_SIZE                                                                     \
	MIN(MIN(DMA_TEST_INST_TRANSFER_SIZE(0), DMA_TEST_INST_TRANSFER_SIZE(1)),                   \
	    MIN(DMA_TEST_INST_TRANSFER_SIZE(2), DMA_TEST_INST_TRANSFER_SIZE(3)))

/* Alignment the first controller under test needs for its buffers. */
#define DMA_TEST_BUF_ALIGN DT_PROP_OR(DT_INST_PHANDLE(0, dma), dma_buf_addr_alignment, 32)

/*
 * Transfer buffers go to the memory region the board names for a controller
 * under test, else to the nocache section when there is one. They are never
 * initialized at load time: neither section is zeroed, so tests fill them
 * before use.
 */
#define DMA_TEST_REGION_SECTION(inst)                                                              \
	Z_GENERIC_SECTION(LINKER_DT_NODE_REGION_NAME(DT_INST_PHANDLE(inst, memory_region)))

#if DT_INST_NODE_HAS_PROP(0, memory_region)
#define DMA_TEST_BUF_SECTION DMA_TEST_REGION_SECTION(0)
#elif DT_INST_NODE_HAS_PROP(1, memory_region)
#define DMA_TEST_BUF_SECTION DMA_TEST_REGION_SECTION(1)
#elif DT_INST_NODE_HAS_PROP(2, memory_region)
#define DMA_TEST_BUF_SECTION DMA_TEST_REGION_SECTION(2)
#elif DT_INST_NODE_HAS_PROP(3, memory_region)
#define DMA_TEST_BUF_SECTION DMA_TEST_REGION_SECTION(3)
#else
#define DMA_TEST_BUF_SECTION __nocache
#endif

/* Attributes of a transfer buffer. */
#define DMA_TEST_BUF __aligned(DMA_TEST_BUF_ALIGN) DMA_TEST_BUF_SECTION

/* Whether the board states a capability of a controller under test. */
#define DMA_TEST_INST_HAS(inst, prop) DT_INST_PROP(inst, prop)

/* A DMA controller under test. */
struct dma_test_dev {
	const struct device *dev;
	/* Channels the board lists for the tests. */
	const uint32_t *channels;
	size_t channel_count;
	/* Longest burst the controller transfers, in bytes, 0 when unlimited. */
	uint32_t max_burst_length;
};

/* Most channels a test uses on one controller at a time. */
#define DMA_TEST_MAX_CHANNELS 2

/* Channels a test obtained on a controller. */
struct dma_test_channels {
	uint32_t id[DMA_TEST_MAX_CHANNELS];
	size_t count;
	/* Allocated with dma_request_channel(), to be released. */
	bool allocated;
};

/* The controllers under test, in devicetree order. */
extern const struct dma_test_dev dma_test_devs[DMA_TEST_DEV_COUNT];

/**
 * @brief Get channels to use on a controller under test.
 *
 * Asks the driver for @p count channels and falls back to the channels the
 * board lists, or to channels 0 and 1, when the driver does not allocate
 * channels. Fewer channels than asked for are returned when the driver or
 * the board has fewer.
 *
 * @param td Controller under test.
 * @param count Channels wanted, at most DMA_TEST_MAX_CHANNELS.
 * @param chans Channels obtained.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p count is out of range.
 */
int dma_test_channels_get(const struct dma_test_dev *td, size_t count,
			  struct dma_test_channels *chans);

/**
 * @brief Give back channels obtained with dma_test_channels_get().
 *
 * @param td Controller under test.
 * @param chans Channels to give back.
 */
void dma_test_channels_put(const struct dma_test_dev *td, struct dma_test_channels *chans);

#endif /* TESTS_DRIVERS_DMA_COMMON_DMA_TEST_H_ */
