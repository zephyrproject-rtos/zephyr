/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INTEL_ADSP_HDA_H
#define ZEPHYR_INCLUDE_INTEL_ADSP_HDA_H

#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <adsp_shim.h>
#include <adsp_memory.h>
#include <adsp_shim.h>

/**
 * @brief HDA stream functionality for Intel ADSP
 *
 * Provides low level calls to support Intel ADSP HDA streams with
 * minimal abstraction that allows testing the hardware
 * and its demands separately from the intended DMA API
 * usage. The only requirement is that you define the base
 * addresses, the stream count, and the size of the ip blocks.
 */

/* The read/write positions are masked to 24 bits */
#define HDA_RWP_MASK 0x00FFFFFF

/* Buffers must be 128 byte aligned, this mask enforces that */
#define HDA_ALIGN_MASK 0xFFFFFF80

/* Buffer size must match the mask of BS field in DGBS register */
#define HDA_BUFFER_SIZE_MASK 0x00FFFFF0

/* Calculate base address of the stream registers */
#define HDA_ADDR(base, regblock_size, stream) ((base) + (stream)*(regblock_size))

/* Gateway Control and Status Register */
#define DGCS(base, regblock_size, stream) \
	((volatile uint32_t *)HDA_ADDR(base, regblock_size, stream))
#define DGCS_SCS BIT(31) /* Sample container size */
#define DGCS_GEN BIT(26) /* Gateway Enable */
#define DGCS_L1ETP BIT(25) /* L1 Enter Prevent */
#define DGCS_L1EXP BIT(24) /* L1 Exit Prevent */
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
#define DGBBA(base, regblock_size, stream) \
	((volatile uint32_t *)(HDA_ADDR(base, regblock_size, stream) + 0x04))

/* Gateway Buffer Size */
#define DGBS(base, regblock_size, stream) \
	((volatile uint32_t *)(HDA_ADDR(base, regblock_size, stream) + 0x08))

/* Gateway Buffer Position Increment */
#define DGBFPI(base, regblock_size, stream) \
	((volatile uint32_t *)(HDA_ADDR(base, regblock_size, stream) + 0x0c))

/* Gateway Buffer Read Position */
#define DGBRP(base, regblock_size, stream) \
	((volatile uint32_t *)(HDA_ADDR(base, regblock_size, stream) + 0x10))

/* Gateway Buffer Write Position */
#define DGBWP(base, regblock_size, stream) \
	((volatile uint32_t *)(HDA_ADDR(base, regblock_size, stream) + 0x14))

/* Gateway Buffer Segment Position */
#define DGBSP(base, regblock_size, stream) \
	((volatile uint32_t *)(HDA_ADDR(base, regblock_size, stream) + 0x18))

/* Gateway Minimum Buffer Size */
#define DGMBS(base, regblock_size, stream) \
	((volatile uint32_t *)(HDA_ADDR(base, regblock_size, stream) + 0x1c))

/* Gateway Linear Link Position Increment */
#define DGLLPI(base, regblock_size, stream) \
	((volatile uint32_t *)(HDA_ADDR(base, regblock_size, stream) + 0x24))

/* Gateway Linear Position In Buffer Increment */
#define DGLPIBI(base, regblock_size, stream) \
	((volatile uint32_t *)(HDA_ADDR(base, regblock_size, stream) + 0x28))

/**
 * @brief Dump all the useful registers of an HDA stream to printk
 *
 * This can be invaluable when finding out why HDA isn't doing (or maybe is)
 * doing what you want it to do. Macro so you get the file and line
 * of the call site included.
 *
 * @param name String that contains a name of the hda stream (or anything really)
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream ID
 */
#define intel_adsp_hda_dbg(name, base, regblock_size, sid)			\
		printk("%s:%u %s(%u:0x%p), dgcs: 0x%x, dgbba 0x%x, "		\
		       "dgbs %u, dgbrp %u, dgbwp %u, dgbsp %u, "		\
		       "dgmbs %u, dgbllpi 0x%x, dglpibi 0x%x\n",		\
		       __FILE__, __LINE__, name,				\
		       sid, DGCS(base, regblock_size, sid),			\
		       *DGCS(base, regblock_size, sid),				\
		       *DGBBA(base, regblock_size, sid),			\
		       *DGBS(base, regblock_size, sid),				\
		       *DGBRP(base, regblock_size, sid),			\
		       *DGBWP(base, regblock_size, sid),			\
		       *DGBSP(base, regblock_size, sid),			\
		       *DGMBS(base, regblock_size, sid),			\
		       *DGLLPI(base, regblock_size, sid),			\
		       *DGLPIBI(base, regblock_size, sid))



/**
 * @brief Initialize an HDA stream for use with the firmware
 *
 * @param base Base address of the IP register block
 * @param sid Stream ID
 */
static inline void intel_adsp_hda_init(uint32_t base, uint32_t regblock_size, uint32_t sid)
{
	*DGCS(base, regblock_size, sid) |= DGCS_FWCB;
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
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream ID
 * @param buf Buffer address to use for the shared FIFO. Must be in L2 and 128 byte aligned.
 * @param buf_size Buffer size in bytes Must be 128 byte aligned
 *
 * @retval -EBUSY if the HDA stream is already enabled
 * @retval -EINVAL if the buf is not in L2, buf isn't aligned on 128 byte boundaries
 * @retval 0 on Success
 */
static inline int intel_adsp_hda_set_buffer(uint32_t base,
					    uint32_t regblock_size,
					    uint32_t sid,
					    uint8_t *buf,
					    uint32_t buf_size)
{
	/* While we don't actually care if the pointer is in the cached
	 * region or not, we do need a consistent address space to check
	 * against for our assertion. This is cheap.
	 */
	uint32_t addr = (uint32_t)sys_cache_cached_ptr_get(buf);
	uint32_t aligned_addr = addr & HDA_ALIGN_MASK;
	uint32_t aligned_size = buf_size & HDA_BUFFER_SIZE_MASK;

	__ASSERT(aligned_addr == addr, "Buffer must be 128 byte aligned");
	__ASSERT(aligned_addr >= L2_SRAM_BASE
		 && aligned_addr < L2_SRAM_BASE + L2_SRAM_SIZE,
		 "Buffer must be in L2 address space");
	__ASSERT(aligned_size == buf_size,
		 "Buffer must be 16 byte aligned in size");

	__ASSERT(aligned_addr + aligned_size < L2_SRAM_BASE + L2_SRAM_SIZE,
		 "Buffer must end in L2 address space");

	if (*DGCS(base, regblock_size, sid) & DGCS_GEN) {
		return -EBUSY;
	}

	if (*DGCS(base, regblock_size, sid) & DGCS_GBUSY) {
		return -EBUSY;
	}

	*DGBBA(base, regblock_size, sid) = aligned_addr;
	*DGBS(base, regblock_size, sid) = aligned_size;

	return 0;
}

/**
 * @brief Get the buffer size
 *
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream ID
 *
 * @retval buf_size Buffer size in bytes
 */
static inline uint32_t intel_adsp_hda_get_buffer_size(uint32_t base,
					    uint32_t regblock_size,
					    uint32_t sid)
{
	return *DGBS(base, regblock_size, sid);
}

/**
 * @brief Enable the stream
 *
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream ID
 */
static inline void intel_adsp_hda_enable(uint32_t base, uint32_t regblock_size,
					 uint32_t sid, bool set_fifordy)
{
	*DGCS(base, regblock_size, sid) |= DGCS_GEN;

	if (set_fifordy) {
		*DGCS(base, regblock_size, sid) |= DGCS_FIFORDY;
	}
}

/**
 * @brief Disable stream
 *
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream ID
 */
static inline void intel_adsp_hda_disable(uint32_t base, uint32_t regblock_size, uint32_t sid)
{
	*DGCS(base, regblock_size, sid) &= ~(DGCS_GEN | DGCS_FIFORDY);
}

/**
 * @brief Check if stream is enabled
 *
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream ID
 */
static inline bool intel_adsp_hda_is_enabled(uint32_t base, uint32_t regblock_size, uint32_t sid)
{
	return *DGCS(base, regblock_size, sid) & (DGCS_GEN | DGCS_FIFORDY);
}

/**
 * @brief Determine the number of unused bytes in the buffer
 *
 * This is useful primarily for a  host in (dsp -> host) stream.
 *
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream ID within the register block
 *
 * @retval n Number of unused bytes
 */
static inline uint32_t intel_adsp_hda_unused(uint32_t base, uint32_t regblock_size, uint32_t sid)
{
	uint32_t dgcs = *DGCS(base, regblock_size, sid);
	uint32_t dgbs = *DGBS(base, regblock_size, sid);

	/* Check if buffer is empty */
	if ((dgcs & DGCS_BNE) == 0) {
		return dgbs;
	}

	/* Check if the buffer is full */
	if (dgcs & DGCS_BF) {
		return 0;
	}

	int32_t rp = *DGBRP(base, regblock_size, sid);
	int32_t wp = *DGBWP(base, regblock_size, sid);
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
 * @param regblock_size Register block size
 * @param sid Stream ID within the register block
 * @param len Len to increment postion by
 */
static inline void intel_adsp_hda_host_commit(uint32_t base,
					      uint32_t regblock_size,
					      uint32_t sid,
					      uint32_t len)
{
	*DGBFPI(base, regblock_size, sid) = len;
	*DGLLPI(base, regblock_size, sid) = len;
	*DGLPIBI(base, regblock_size, sid) = len;
}

/**
 * @brief Commit a number of bytes that have been transferred to/from link
 *
 * Writes the length to BFPI.
 *
 * @seealso intel_adsp_hda_host_commit
 *
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream ID within the register block
 * @param len Len to increment postion by
 */
static inline void intel_adsp_hda_link_commit(uint32_t base,
					      uint32_t regblock_size,
					      uint32_t sid,
					      uint32_t len)
{
	*DGBFPI(base, regblock_size, sid) = len;
}

/**
 * @brief Read the buffer full bit of the given stream.
 *
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream ID within the register block
 *
 * @retval true If the buffer full flag is set
 */
static inline bool intel_adsp_hda_buf_full(uint32_t base, uint32_t regblock_size, uint32_t sid)
{
	return *DGCS(base, regblock_size, sid) & DGCS_BF;
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
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream D
 *
 * @retval true If the read and write positions are equal
 */
static inline bool intel_adsp_hda_wp_rp_eq(uint32_t base, uint32_t regblock_size, uint32_t sid)
{
	return *DGBWP(base, regblock_size, sid) == *DGBRP(base, regblock_size, sid);
}

static inline bool intel_adsp_hda_is_buffer_overrun(uint32_t base, uint32_t regblock_size,
						    uint32_t sid)
{
	return (*DGCS(base, regblock_size, sid) & DGCS_BOR) == DGCS_BOR ? 1 : 0;
}

static inline bool intel_adsp_hda_is_buffer_underrun(uint32_t base, uint32_t regblock_size,
						     uint32_t sid)
{
	return (*DGCS(base, regblock_size, sid) & DGCS_BUR) == DGCS_BUR ? 1 : 0;
}

static inline void intel_adsp_hda_overrun_clear(uint32_t base, uint32_t regblock_size,
						uint32_t sid)
{
	*DGCS(base, regblock_size, sid) |= DGCS_BOR;
}

static inline void intel_adsp_hda_underrun_clear(uint32_t base, uint32_t regblock_size,
						 uint32_t sid)
{
	*DGCS(base, regblock_size, sid) |= DGCS_BUR;
}

/**
 * @brief Set the buffer segment ptr
 *
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream ID
 * @param size
 */
static inline void intel_adsp_hda_set_buffer_segment_ptr(uint32_t base, uint32_t regblock_size,
							 uint32_t sid, uint32_t size)
{
	*DGBSP(base, regblock_size, sid) = size;
}

/**
 * @brief Get the buffer segment ptr
 *
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream ID
 *
 * @retval buffer segment ptr
 */
static inline uint32_t intel_adsp_hda_get_buffer_segment_ptr(uint32_t base, uint32_t regblock_size,
							     uint32_t sid)
{
	return *DGBSP(base, regblock_size, sid);
}

/**
 * @brief Enable BSC interrupt
 *
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream ID
 */
static inline void intel_adsp_hda_enable_buffer_interrupt(uint32_t base, uint32_t regblock_size,
							  uint32_t sid)
{
	*DGCS(base, regblock_size, sid) |= DGCS_BSCIE;
}

/**
 * @brief Disable BSC interrupt
 *
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream ID
 */
static inline void intel_adsp_hda_disable_buffer_interrupt(uint32_t base, uint32_t regblock_size,
							   uint32_t sid)
{
	*DGCS(base, regblock_size, sid) &= ~DGCS_BSCIE;
}

/**
 * @brief Check if BSC interrupt enabled
 *
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream ID
 */
static inline bool intel_adsp_hda_is_buffer_interrupt_enabled(uint32_t base,
							      uint32_t regblock_size, uint32_t sid)
{
	return (*DGCS(base, regblock_size, sid) & DGCS_BSCIE) == DGCS_BSCIE;
}

static inline void intel_adsp_force_dmi_l0_state(void)
{
#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	ACE_DfPMCCH.svcfg |= ADSP_FORCE_DECOUPLED_HDMA_L1_EXIT_BIT;
#endif
}

static inline void intel_adsp_allow_dmi_l1_state(void)
{
#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	ACE_DfPMCCH.svcfg &= ~(ADSP_FORCE_DECOUPLED_HDMA_L1_EXIT_BIT);
#endif
}

/**
 * @brief Clear BSC interrupt
 *
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream ID
 */
static inline void intel_adsp_hda_clear_buffer_interrupt(uint32_t base, uint32_t regblock_size,
							 uint32_t sid)
{
	*DGCS(base, regblock_size, sid) |= DGCS_BSC;
}

/**
 * @brief Get status of BSC interrupt
 *
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream ID
 *
 * @retval interrupt status
 */
static inline uint32_t intel_adsp_hda_check_buffer_interrupt(uint32_t base, uint32_t regblock_size,
							     uint32_t sid)
{
	return (*DGCS(base, regblock_size, sid) & DGCS_BSC) == DGCS_BSC;
}

/**
 * @brief Set the Sample Container Size (SCS)
 *
 * Sample Container Size indicates the container size of the audio samples in local memory
 * SCS bit must cleared to 0 for 32bit sample size (HD Audio container size)
 * SCS bit must be set to 1 for non 32bit sample sizes
 *
 * @param base Base address of the IP register block
 * @param regblock_size Register block size
 * @param sid Stream ID
 * @param sample_size
 */
static inline void intel_adsp_hda_set_sample_container_size(uint32_t base, uint32_t regblock_size,
							    uint32_t sid, uint32_t sample_size)
{
	if (sample_size <= 3) {
		*DGCS(base, regblock_size, sid) |= DGCS_SCS;
	} else {
		*DGCS(base, regblock_size, sid) &= ~DGCS_SCS;
	}
}

#endif /* ZEPHYR_INCLUDE_INTEL_ADSP_HDA_H */
