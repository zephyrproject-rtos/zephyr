/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INTEL_ADSP_HDA_H
#define ZEPHYR_INCLUDE_INTEL_ADSP_HDA_H

#include <zephyr/arch/xtensa/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <adsp_shim.h>
#include <adsp_memory.h>

/**
 * @brief HDA stream functionality for Intel ADSP
 *
 * Provides low level calls to support Intel ADSP HDA streams with
 * minimal abstraction that allows testing the hardware
 * and its demands separately from the intended DMA API
 * usage. The only requirement is that you define the base
 * addresses, the stream count, and the size of the ip blocks.
 */

#ifndef HDA_HOST_OUT_BASE
#error Must define HDA_HOST_OUT_BASE
#endif
#ifndef HDA_HOST_IN_BASE
#error Must define HDA_HOST_IN_BASE
#endif
#ifndef HDA_STREAM_COUNT
#error Must define HDA_STREAM_COUNT
#endif
#ifndef HDA_REGBLOCK_SIZE
#error Must define HDA_REGBLOCK_SIZE
#endif

/* The read/write positions are masked to 24 bits */
#define HDA_RWP_MASK 0x00FFFFFF

/* Buffers must be 128 byte aligned, this mask enforces that */
#define HDA_ALIGN_MASK 0xFFFFFF80


#define HDA_ADDR(base, stream) ((base) + (stream)*HDA_REGBLOCK_SIZE)


/* Gateway Control and Status Register */
#define DGCS(base, stream) ((volatile uint32_t *)HDA_ADDR(base, stream))
#define DGCS_SCS BIT(31) /* Sample container size */
#define DGCS_GEN BIT(26) /* Gateway Enable */
#define DGCS_L1ETP BIT(25) /* L1 Enter Prevent */
#define DGCS_L1EXP BIT(25) /* L1 Exit Prevent */
#define DGCS_FWCB BIT(23) /* Firmware Control Buffer */
#define DGCS_GBUSY BIT(15) /* Gateway Busy */
#define DGCS_TE BIT(14) /* Transfer Error */
#define DGCS_BSC BIT(11) /* Buffer Segment Completion */
#define DGCS_BOR BIT(10) /* Buffer Overrun */
#define DGCS_BUR BIT(10) /* Buffer Underrun */
#define DGCS_BF BIT(9) /* Buffer Full */
#define DGCS_BNE BIT(8) /* Buffer Not Empty */
#define DGCS_FIFORDY BIT(5) /* Enable FIFO */
#define DGCS_BSCIE BIT(3) /* Buffer Segment Completion Interrupt Enable */

/* Gateway Buffer Base Address */
#define DGBBA(base, stream) ((volatile uint32_t *)(HDA_ADDR(base, stream) + 0x04))

/* Gateway Buffer Size */
#define DGBS(base, stream) ((volatile uint32_t *)(HDA_ADDR(base, stream) + 0x08))

/* Gateway Buffer Position Increment */
#define DGBFPI(base, stream) ((volatile uint32_t *)(HDA_ADDR(base, stream) + 0x0c))

/* Gateway Buffer Read Position */
#define DGBRP(base, stream) ((volatile uint32_t *)(HDA_ADDR(base, stream) + 0x10))

/* Gateway Buffer Write Position */
#define DGBWP(base, stream) ((volatile uint32_t *)(HDA_ADDR(base, stream) + 0x14))

/* Gateway Buffer Segment Position */
#define DGBSP(base, stream) ((volatile uint32_t *)(HDA_ADDR(base, stream) + 0x18))

/* Gateway Minimum Buffer Size */
#define DGMBS(base, stream) ((volatile uint32_t *)(HDA_ADDR(base, stream) + 0x1c))

/* Gateway Linear Link Position Increment */
#define DGLLPI(base, stream) ((volatile uint32_t *)(HDA_ADDR(base, stream) + 0x24))

/* Gateway Linear Position In Buffer Increment */
#define DGLPIBI(base, stream) ((volatile uint32_t *)(HDA_ADDR(base, stream) + 0x28))

/**
 * @brief Dump all the useful registers of an HDA stream to printk
 *
 * This can be invaluable when finding out why HDA isn't doing (or maybe is)
 * doing what you want it to do. Macro so you get the file and line
 * of the call site included.
 *
 * @param name String that contains a name of the hda stream (or anything really)
 * @param base Base address of the IP register block
 * @param sid Stream ID
 */
#define intel_adsp_hda_dbg(name, base, sid)				\
		printk("%s:%u %s(%u:0x%p), dgcs: 0x%x, dgbba 0x%x, "	\
		       "dgbs %u, dgbrp %u, dgbwp %u, dgbsp %u, "	\
		       "dgmbs %u, dgbllpi 0x%x, dglpibi 0x%x\n",	\
		       __FILE__, __LINE__, name,			\
		       sid, DGCS(base, sid),				\
		       *DGCS(base, sid),				\
		       *DGBBA(base, sid),				\
		       *DGBS(base, sid),				\
		       *DGBRP(base, sid),				\
		       *DGBWP(base, sid),				\
		       *DGBSP(base, sid),				\
		       *DGMBS(base, sid),				\
		       *DGLLPI(base, sid),				\
		       *DGLPIBI(base, sid))



/**
 * @brief Initialize an HDA stream for use with the firmware
 *
 * @param hda Stream set to work with
 * @param sid Stream ID
 */
static inline void intel_adsp_hda_init(uint32_t base, uint32_t sid)
{
	*DGCS(base, sid) |= DGCS_FWCB;
}

/**
 * @brief Set the buffer, size, and element size for an HDA stream
 *
 * Sanity checks that the buffer address and size are valid and that the
 * stream isn't enabled or busy.
 *
 * Prior to enabling an HDA stream to/from the host this is the minimum configuration
 * that is required. It must be set *after* the host has configured its own buffers.
 *
 *
 * @param hda Stream set to work with
 * @param sid Stream ID
 * @param buf Buffer address to use for the shared FIFO. Must be in L2 and 128 byte aligned.
 * @param buf_size Buffer size in bytes Must be 128 byte aligned
 *
 * @retval -EBUSY if the HDA stream is already enabled
 * @retval -EINVAL if the buf is not in L2, buf isn't aligned on 128 byte boundaries
 * @retval 0 on Success
 */
static inline int intel_adsp_hda_set_buffer(uint32_t base, uint32_t sid,
				     uint8_t *buf, uint32_t buf_size)
{
	/* While we don't actually care if the pointer is in the cached
	 * region or not, we do need a consistent address space to check
	 * against for our assertion. This is cheap.
	 */
	uint32_t addr = (uint32_t)arch_xtensa_cached_ptr(buf);
	uint32_t aligned_addr = addr & HDA_ALIGN_MASK;
	uint32_t aligned_size = buf_size & HDA_ALIGN_MASK;

	__ASSERT(aligned_addr == addr, "Buffer must be 128 byte aligned");
	__ASSERT(aligned_addr >= L2_SRAM_BASE
		 && aligned_addr < L2_SRAM_BASE + L2_SRAM_SIZE,
		 "Buffer must be in L2 address space");
	__ASSERT(aligned_size == buf_size,
		 "Buffer must be 128 byte aligned in size");

	__ASSERT(aligned_addr + aligned_size < L2_SRAM_BASE + L2_SRAM_SIZE,
		 "Buffer must end in L2 address space");

	if (*DGCS(base, sid) & DGCS_GEN) {
		return -EBUSY;
	}

	if (*DGCS(base, sid) & DGCS_GBUSY) {
		return -EBUSY;
	}

	*DGBBA(base, sid) = aligned_addr;
	*DGBS(base, sid) = aligned_size;

	return 0;
}

/**
 * @brief Enable the stream
 *
 * @param hda HDA stream set
 * @param sid Stream ID
 */
static inline void intel_adsp_hda_enable(uint32_t base, uint32_t sid)
{
	*DGCS(base, sid) |= DGCS_GEN | DGCS_FIFORDY;
}

/**
 * @brief Disable stream
 *
 * @param hda HDA stream set
 * @param sid Stream ID
 */
static inline void intel_adsp_hda_disable(uint32_t base, uint32_t sid)
{
	*DGCS(base, sid) &= ~(DGCS_GEN | DGCS_FIFORDY);
}

/**
 * @brief Determine the number of unused bytes in the buffer
 *
 * This is useful primarily for a  host in (dsp -> host) stream.
 *
 * @param base Base address of the IP register block
 * @param sid Stream ID within the register block
 *
 * @retval n Number of unused bytes
 */
static inline uint32_t intel_adsp_hda_unused(uint32_t base, uint32_t sid)
{
	uint32_t dgcs = *DGCS(base, sid);
	uint32_t dgbs = *DGBS(base, sid);

	/* Check if buffer is empty */
	if ((dgcs & DGCS_BNE) == 0) {
		return dgbs;
	}

	/* Check if the buffer is full */
	if (dgcs & DGCS_BF) {
		return 0;
	}

	int32_t rp = *DGBRP(base, sid);
	int32_t wp = *DGBWP(base, sid);
	int32_t size = rp - wp;

	if (size <= 0) {
		size += dgbs;
	}

	return size;
}

/**
 * @brief Commit a number of bytes that have been transferred to/from host
 *
 * Writes the length to BFPI. For host transfers LLPI and LPIB are
 * also written to with the given length.
 *
 * This then updates the read or write position depending on the direction.
 *
 * LPIBI writes here can be seen on the host side of the transfer in the
 * matching LPIB register.
 *
 * LLPI seems to, from behavior, inform the hardware to actually read/write
 * from the buffer. Without updating BFPI AND LLPI, the transfer doesn't
 * happen in testing for host transfers.
 *
 * @param base Base address of the IP register block
 * @param sid Stream ID within the register block
 * @param len Len to increment postion by
 */
static inline void intel_adsp_hda_host_commit(uint32_t base, uint32_t sid, uint32_t len)
{
	*DGBFPI(base, sid) = len;
	*DGLLPI(base, sid) = len;
	*DGLPIBI(base, sid) = len;
}

/**
 * @brief Commit a number of bytes that have been transferred to/from link
 *
 * Writes the length to BFPI.
 *
 * @seealso intel_adsp_hda_host_commit
 *
 * @param base Base address of the IP register block
 * @param sid Stream ID within the register block
 * @param len Len to increment postion by
 */
static inline void intel_adsp_hda_link_commit(uint32_t base, uint32_t sid, uint32_t len)
{
	*DGBFPI(base, sid) = len;
}

/**
 * @brief Read the buffer full bit of the given stream.
 *
 * @param base Base address of the IP register block
 * @param sid Stream ID within the register block
 *
 * @retval true If the buffer full flag is set
 */
static inline bool intel_adsp_hda_buf_full(uint32_t base, uint32_t sid)
{
	return *DGCS(base, sid) & DGCS_BF;
}

/**
 * @brief Check if the write and read position are equal
 *
 * For HDA this does not mean that the buffer is full or empty
 * there are bit flags for those cases.
 *
 * Useful for waiting on the hardware to catch up to
 * reads or writes (e.g. after a intel_adsp_hda_commit)
 *
 * @param dev HDA Stream device
 * @param sid Stream D
 *
 * @retval true If the read and write positions are equal
 */
static inline bool intel_adsp_hda_wp_rp_eq(uint32_t base, uint32_t sid)
{
	return *DGBWP(base, sid) == *DGBRP(base, sid);
}

#endif /* ZEPHYR_INCLUDE_INTEL_ADSP_HDA_H */
