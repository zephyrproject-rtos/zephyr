/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LiteX control and status register access helpers.
 * @ingroup litex_csr
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_LITEX_LITEX_CSR_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_LITEX_LITEX_CSR_H_

#include <zephyr/devicetree.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/sys_io.h>

#ifndef _ASMLANGUAGE
/**
 * @defgroup litex_csr LiteX control and status registers
 * @ingroup misc_interfaces
 * @since 4.6
 * @version 0.1.0
 *
 * Access CSRs using the configured subregister width and ordering.
 * Each subregister occupies a 4-byte address slot.
 *
 * @{
 */

/** Address stride between consecutive CSR subregisters, in bytes. */
#define LITEX_CSR_OFFSET_BYTES 4

/**
 * @brief Read an 8-bit CSR.
 *
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 *
 * @return Value read from the CSR.
 */
static inline uint8_t litex_read8(mem_addr_t addr)
{
	return sys_read8(addr);
}

/**
 * @brief Read a 16-bit CSR.
 *
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 *
 * @return Value read from the CSR.
 */
static inline uint16_t litex_read16(mem_addr_t addr)
{
	return sys_read16(addr);
}

/**
 * @brief Read a 32-bit CSR.
 *
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 *
 * @return Value read from the CSR.
 */
static inline uint32_t litex_read32(mem_addr_t addr)
{
	return sys_read32(addr);
}

/**
 * @brief Read a 64-bit CSR.
 *
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 *
 * @return Value read from the CSR.
 */
static inline uint64_t litex_read64(mem_addr_t addr)
{
#ifdef CONFIG_LITEX_CSR_ORDERING_BIG
	return ((uint64_t)sys_read32(addr) << 32) | (uint64_t)sys_read32(addr + 0x4);
#else
	return sys_read64(addr);
#endif
}

/**
 * @brief Write an 8-bit CSR.
 *
 * @param value Value to write to the CSR.
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 */
static inline void litex_write8(uint8_t value, mem_addr_t addr)
{
	sys_write8(value, addr);
}

/**
 * @brief Write a 16-bit CSR.
 *
 * @param value Value to write to the CSR.
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 */
static inline void litex_write16(uint16_t value, mem_addr_t addr)
{
	sys_write16(value, addr);
}

/**
 * @brief Write a 32-bit CSR.
 *
 * @param value Value to write to the CSR.
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 */
static inline void litex_write32(uint32_t value, mem_addr_t addr)
{
	sys_write32(value, addr);
}

/**
 * @brief Write a 64-bit CSR.
 *
 * @param value Value to write to the CSR.
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 */
static inline void litex_write64(uint64_t value, mem_addr_t addr)
{
#ifdef CONFIG_LITEX_CSR_ORDERING_BIG
	sys_write32(value >> 32, addr);
	sys_write32(value, addr + 0x4);
#else
	sys_write64(value, addr);
#endif
}

/**
 * @brief Write a CSR of the specified size.
 *
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 * @param size CSR size in bytes: 1, 2, or 4. Other values cause no write.
 * @param value Value to write; only the least significant @p size bytes are used.
 */
static inline void litex_write(mem_addr_t addr, uint8_t size, uint32_t value)
{
	switch (size) {
	case 1:
		litex_write8(value, addr);
		break;
	case 2:
		litex_write16(value, addr);
		break;
	case 4:
		litex_write32(value, addr);
		break;
	default:
		break;
	}
}

/**
 * @brief Read a CSR of the specified size.
 *
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 * @param size CSR size in bytes: 1, 2, or 4.
 *
 * @return Value read from the CSR, zero-extended to 32 bits.
 * @retval 0 Unsupported size.
 */
static inline uint32_t litex_read(mem_addr_t addr, uint32_t size)
{
	switch (size) {
	case 1:
		return litex_read8(addr);
	case 2:
		return litex_read16(addr);
	case 4:
		return litex_read32(addr);
	default:
		return 0;
	}
}

/**
 * @brief Write consecutive 32-bit CSR words.
 *
 * The buffer stores the least significant word at index zero. With big-endian
 * CSR ordering, the highest buffer index is accessed at the lowest CSR address.
 *
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 * @param[in] buf Caller-owned buffer containing at least @p cnt words to write.
 *            Must remain valid until this function returns; may be NULL if @p cnt is zero.
 * @param cnt Number of 32-bit words to write.
 */
static inline void litex_write32_array(mem_addr_t addr, uint32_t *buf, size_t cnt)
{
	size_t i;

	for (i = 0; i < cnt; i++) {
#ifdef CONFIG_LITEX_CSR_ORDERING_BIG
		litex_write32(buf[cnt - 1 - i], addr);
#else
		litex_write32(buf[i], addr);
#endif
		addr += LITEX_CSR_OFFSET_BYTES;
	}
}

/**
 * @brief Read consecutive 32-bit CSR words.
 *
 * The buffer stores the least significant word at index zero. With big-endian
 * CSR ordering, the highest buffer index is accessed at the lowest CSR address.
 *
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 * @param[out] buf Caller-owned buffer with space for at least @p cnt words read.
 *            Must remain valid until this function returns; may be NULL if @p cnt is zero.
 * @param cnt Number of 32-bit words to read.
 */
static inline void litex_read32_array(mem_addr_t addr, uint32_t *buf, size_t cnt)
{
	size_t i;

	for (i = 0; i < cnt; i++) {
#ifdef CONFIG_LITEX_CSR_ORDERING_BIG
		buf[cnt - 1 - i] = litex_read32(addr);
#else
		buf[i] = litex_read32(addr);
#endif
		addr += LITEX_CSR_OFFSET_BYTES;
	}
}

/** @} */

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_LITEX_LITEX_CSR_H_ */
