/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for port, memory-mapped register, and memory bit manipulation APIs.
 * @ingroup sys_io_apis
 */

#ifndef ZEPHYR_INCLUDE_SYS_SYS_IO_H_
#define ZEPHYR_INCLUDE_SYS_SYS_IO_H_

#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System I/O APIs
 *
 * @defgroup sys_io_apis System I/O APIs
 * @ingroup utilities
 *
 * Low-level primitives for port I/O, memory-mapped register I/O, and in-memory bit manipulation.
 * @{
 */

/** I/O port address. */
typedef uint32_t io_port_t;

/** Memory-mapped register address. */
typedef uintptr_t mm_reg_t;

/** Memory address. */
typedef uintptr_t mem_addr_t;

#ifdef __DOXYGEN__

/**
 * @defgroup sys_io_port_apis Port I/O APIs
 * @ingroup sys_io_apis
 *
 * @internal
 * Port I/O is optional and only implemented by a few architectures (e.g. x86), in their
 * architecture-specific headers. Those headers are excluded from the Doxygen build, so the
 * declarations below exist solely to give the documentation a set of symbols to attach to.
 * @endinternal
 *
 * @{
 */

/**
 * @brief Output a byte to an I/O port
 *
 * This function writes a byte to the given port.
 *
 * @param data the byte to write
 * @param port the port address where to write the byte
 */
static inline void sys_out8(uint8_t data, io_port_t port);

/**
 * @brief Input a byte from an I/O port
 *
 * This function reads a byte from the port.
 *
 * @param port the port address from where to read the byte
 *
 * @return the byte read
 */
static inline uint8_t sys_in8(io_port_t port);

/**
 * @brief Output a 16 bits to an I/O port
 *
 * This function writes a 16 bits to the given port.
 *
 * @param data the 16 bits to write
 * @param port the port address where to write the 16 bits
 */
static inline void sys_out16(uint16_t data, io_port_t port);

/**
 * @brief Input 16 bits from an I/O port
 *
 * This function reads 16 bits from the port.
 *
 * @param port the port address from where to read the 16 bits
 *
 * @return the 16 bits read
 */
static inline uint16_t sys_in16(io_port_t port);

/**
 * @brief Output 32 bits to an I/O port
 *
 * This function writes 32 bits to the given port.
 *
 * @param data the 32 bits to write
 * @param port the port address where to write the 32 bits
 */
static inline void sys_out32(uint32_t data, io_port_t port);

/**
 * @brief Input 32 bits from an I/O port
 *
 * This function reads 32 bits from the port.
 *
 * @param port the port address from where to read the 32 bits
 *
 * @return the 32 bits read
 */
static inline uint32_t sys_in32(io_port_t port);

/**
 * @brief Set the designated bit from port to 1
 *
 * This functions takes the designated bit starting from port and sets it to 1.
 *
 * @param port the port address from where to look for the bit
 * @param bit the designated bit to set (from 0 to n)
 */
static inline void sys_io_set_bit(io_port_t port, unsigned int bit);

/**
 * @brief Clear the designated bit from port to 0
 *
 * This functions takes the designated bit starting from port and sets it to 0.
 *
 * @param port the port address from where to look for the bit
 * @param bit the designated bit to clear (from 0 to n)
 */
static inline void sys_io_clear_bit(io_port_t port, unsigned int bit);

/**
 * @brief Test the bit from port if it is set or not
 *
 * This functions takes the designated bit starting from port and tests its
 * current setting. It will return the current setting.
 *
 * @param port the port address from where to look for the bit
 * @param bit the designated bit to test (from 0 to n)
 *
 * @return 1 if it is set, 0 otherwise
 */
static inline int sys_io_test_bit(io_port_t port, unsigned int bit);

/**
 * @brief Test the bit from port and set it
 *
 * This functions takes the designated bit starting from port, tests its
 * current setting and sets it. It will return the previous setting.
 *
 * @param port the port address from where to look for the bit
 * @param bit the designated bit to test and set (from 0 to n)
 *
 * @return 1 if it was set, 0 otherwise
 */
static inline int sys_io_test_and_set_bit(io_port_t port, unsigned int bit);

/**
 * @brief Test the bit from port and clear it
 *
 * This functions takes the designated bit starting from port, tests its
 * current setting and clears it. It will return the previous setting.
 *
 * @param port the port address from where to look for the bit
 * @param bit the designated bit to test and clear (from 0 to n)
 *
 * @return 0 if it was clear, 1 otherwise
 */
static inline int sys_io_test_and_clear_bit(io_port_t port, unsigned int bit);

/** @} */

/**
 * @defgroup sys_io_mmio_apis Memory-mapped register I/O APIs
 * @ingroup sys_io_apis
 *
 * @internal
 * Memory mapped registers I/O is optional and only implemented by a few architectures (e.g. x86),
 * in their architecture-specific headers. Those headers are excluded from the Doxygen build, so the
 * declarations below exist solely to give the documentation a set of symbols to attach to.
 * @endinternal
 *
 * @{
 */

/**
 * @brief Write a byte to a memory mapped register
 *
 * This function writes a byte to the given memory mapped register.
 *
 * @param data the byte to write
 * @param addr the memory mapped register address where to write the byte
 */
static inline void sys_write8(uint8_t data, mm_reg_t addr);

/**
 * @brief Read a byte from a memory mapped register
 *
 * This function reads a byte from the given memory mapped register.
 *
 * @param addr the memory mapped register address from where to read the byte
 *
 * @return the byte read
 */
static inline uint8_t sys_read8(mm_reg_t addr);

/**
 * @brief Write 16 bits to a memory mapped register
 *
 * This function writes 16 bits to the given memory mapped register.
 *
 * @param data the 16 bits to write
 * @param addr the memory mapped register address where to write the 16 bits
 */
static inline void sys_write16(uint16_t data, mm_reg_t addr);

/**
 * @brief Read 16 bits from a memory mapped register
 *
 * This function reads 16 bits from the given memory mapped register.
 *
 * @param addr the memory mapped register address from where to read
 *        the 16 bits
 *
 * @return the 16 bits read
 */
static inline uint16_t sys_read16(mm_reg_t addr);

/**
 * @brief Write 32 bits to a memory mapped register
 *
 * This function writes 32 bits to the given memory mapped register.
 *
 * @param data the 32 bits to write
 * @param addr the memory mapped register address where to write the 32 bits
 */
static inline void sys_write32(uint32_t data, mm_reg_t addr);

/**
 * @brief Read 32 bits from a memory mapped register
 *
 * This function reads 32 bits from the given memory mapped register.
 *
 * @param addr the memory mapped register address from where to read
 *        the 32 bits
 *
 * @return the 32 bits read
 */
static inline uint32_t sys_read32(mm_reg_t addr);

/**
 * @brief Write 64 bits to a memory mapped register
 *
 * This function writes 64 bits to the given memory mapped register.
 *
 * @param data the 64 bits to write
 * @param addr the memory mapped register address where to write the 64 bits
 */
static inline void sys_write64(uint64_t data, mm_reg_t addr);

/**
 * @brief Read 64 bits from a memory mapped register
 *
 * This function reads 64 bits from the given memory mapped register.
 *
 * @param addr the memory mapped register address from where to read
 *        the 64 bits
 *
 * @return the 64 bits read
 */
static inline uint64_t sys_read64(mm_reg_t addr);

/** @} */

/**
 * @defgroup sys_io_bits_apis Memory bit manipulation APIs
 * @ingroup sys_io_apis
 *
 * @internal
 * These functions have a generic implementation in the arch/common/sys_bitops.h file and
 * architecture-specific implementations may override them. The declarations below exist solely to
 * give the documentation a set of symbols to attach to.
 * @endinternal
 *
 * @{
 */

/**
 * @brief Set the designated bit from addr to 1
 *
 * This functions takes the designated bit starting from addr and sets it to 1.
 *
 * @param addr the memory address from where to look for the bit
 * @param bit the designated bit to set (from 0 to 31)
 */
static ALWAYS_INLINE void sys_set_bit(mem_addr_t addr, unsigned int bit);

/**
 * @brief Masking the designated bits from addr to 1
 *
 * This functions masking designated bits from addr to 1.
 *
 * @param addr the memory address from where to look for the bits
 * @param mask the bit mask of a 32 bits data to set
 */
static ALWAYS_INLINE void sys_set_bits(mem_addr_t addr, unsigned int mask);

/**
 * @brief Masking the designated bits from addr to 0
 *
 * This functions masking designated bits from addr to 0.
 *
 * @param addr the memory address from where to look for the bits
 * @param mask the bit mask of a 32 bits data to set
 */
static ALWAYS_INLINE void sys_clear_bits(mem_addr_t addr, unsigned int mask);

/**
 * @brief Clear the designated bit from addr to 0
 *
 * This functions takes the designated bit starting from addr and sets it to 0.
 *
 * @param addr the memory address from where to look for the bit
 * @param bit the designated bit to clear (from 0 to 31)
 */
static ALWAYS_INLINE void sys_clear_bit(mem_addr_t addr, unsigned int bit);

/**
 * @brief Test the bit if it is set or not
 *
 * This functions takes the designated bit starting from addr and tests its
 * current setting. It will return the current setting.
 *
 * @param addr the memory address from where to look for the bit
 * @param bit the designated bit to test (from 0 to 31)
 *
 * @return the bitwise AND result of @p addr content and (1 << @p bit).
 * Result is 0 only if the bit is cleared otherwise result is a non-0 value.
 */
static ALWAYS_INLINE int sys_test_bit(mem_addr_t addr, unsigned int bit);

/**
 * @brief Test the bit and set it
 *
 * This functions takes the designated bit starting from addr, tests its
 * current setting and sets it. It will return the previous setting.
 *
 * @param addr the memory address from where to look for the bit
 * @param bit the designated bit to test and set (from 0 to 31)
 *
 * @return the bitwise AND result of @p addr content and (1 << @p bit) before target bit
 * is set. Result is 0 only if the bit was cleared otherwise result is a non-0 value.
 */
static ALWAYS_INLINE int sys_test_and_set_bit(mem_addr_t addr, unsigned int bit);

/**
 * @brief Test the bit and clear it
 *
 * This functions takes the designated bit starting from addr, test its
 * current setting and clears it. It will return the previous setting.
 *
 * @param addr the memory address from where to look for the bit
 * @param bit the designated bit to test and clear (from 0 to 31)
 *
 * @return the bitwise AND result of @p addr content and (1 << @p bit) before target bit
 * is cleared. Result is 0 only if the bit was cleared otherwise result is a non-0 value.
 */
static ALWAYS_INLINE int sys_test_and_clear_bit(mem_addr_t addr, unsigned int bit);

/**
 * @brief Set the designated bit from addr to 1
 *
 * This functions takes the designated bit starting from addr and sets it to 1.
 *
 * @param addr the memory address from where to look for the bit
 * @param bit the designated bit to set (arbitrary)
 */
static ALWAYS_INLINE void sys_bitfield_set_bit(mem_addr_t addr, unsigned int bit);

/**
 * @brief Clear the designated bit from addr to 0
 *
 * This functions takes the designated bit starting from addr and sets it to 0.
 *
 * @param addr the memory address from where to look for the bit
 * @param bit the designated bit to clear (arbitrary)
 */
static ALWAYS_INLINE void sys_bitfield_clear_bit(mem_addr_t addr, unsigned int bit);

/**
 * @brief Test the bit if it is set or not
 *
 * This functions takes the designated bit starting from addr and tests its
 * current setting. It will return the current setting.
 *
 * @param addr the memory address from where to look for the bit
 * @param bit the designated bit to test (arbitrary)
 *
 * @return the bitwise AND result of @p addr content and (1 << @p bit). Result is 0
 * only if the bit is cleared otherwise result is a non-0 value.
 */
static ALWAYS_INLINE int sys_bitfield_test_bit(mem_addr_t addr, unsigned int bit);

/**
 * @brief Test the bit and set it
 *
 * This functions takes the designated bit starting from addr, tests its
 * current setting and sets it. It will return the previous setting.
 *
 * @param addr the memory address from where to look for the bit
 * @param bit the designated bit to test and set (arbitrary)
 *
 * @return the bitwise AND result of @p addr content and (1 << @p bit) before target bit
 * is set. Result is 0 only if the bit was cleared otherwise result is a non-0 value.
 */
static ALWAYS_INLINE int sys_bitfield_test_and_set_bit(mem_addr_t addr, unsigned int bit);

/**
 * @brief Test the bit and clear it
 *
 * This functions takes the designated bit starting from addr, test its
 * current setting and clears it. It will return the previous setting.
 *
 * @param addr the memory address from where to look for the bit
 * @param bit the designated bit to test and clear (arbitrary)
 *
 * @return the bitwise AND result of @p addr content and (1 << @p bit) before target bit
 * is cleared. Result is 0 only if the bit was cleared otherwise result is a non-0 value.
 */
static ALWAYS_INLINE int sys_bitfield_test_and_clear_bit(mem_addr_t addr, unsigned int bit);

/** @} */ /* sys_io_bits_apis */

/** @} */ /* sys_io_apis */

#endif /* __DOXYGEN__ */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_SYS_IO_H_ */
