/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _ASM_INLINE_GCC_H
#define _ASM_INLINE_GCC_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The file must not be included directly
 * Include arch/cpu.h instead
 */

#ifndef _ASMLANGUAGE

#include <sys_io.h>

/**
 *
 * @brief find most significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the most significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return most significant bit set, 0 if @a op is 0
 */

static ALWAYS_INLINE unsigned int find_msb_set(uint32_t op)
{
	ARG_UNUSED(op);
	/* STUB */
	return 0;
}

/**
 *
 * @brief find least significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the least significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return least significant bit set, 0 if @a op is 0
 */

static ALWAYS_INLINE unsigned int find_lsb_set(uint32_t op)
{
	ARG_UNUSED(op);
	/* STUB */
	return 0;
}

/* Implementation of sys_io.h's documented functions */

static inline __attribute__((always_inline))
	void sys_out8(uint8_t data, io_port_t port)
{
	ARG_UNUSED(data);
	ARG_UNUSED(port);
	/* STUB */
}

static inline __attribute__((always_inline))
	uint8_t sys_in8(io_port_t port)
{
	ARG_UNUSED(port);
	/* STUB */
	return 0;
}

static inline __attribute__((always_inline))
	void sys_out16(uint16_t data, io_port_t port)
{
	ARG_UNUSED(data);
	ARG_UNUSED(port);
	/* STUB */
}

static inline __attribute__((always_inline))
	uint16_t sys_in16(io_port_t port)
{
	ARG_UNUSED(port);
	/* STUB */
	return 0;

}

static inline __attribute__((always_inline))
	void sys_out32(uint32_t data, io_port_t port)
{
	ARG_UNUSED(data);
	ARG_UNUSED(port);
	/* STUB */
}

static inline __attribute__((always_inline))
	uint32_t sys_in32(io_port_t port)
{
	ARG_UNUSED(port);
	/* STUB */
	return 0;
}

static inline __attribute__((always_inline))
	void sys_io_set_bit(io_port_t port, unsigned int bit)
{
	ARG_UNUSED(port);
	ARG_UNUSED(bit);
	/* STUB */
}

static inline __attribute__((always_inline))
	void sys_io_clear_bit(io_port_t port, unsigned int bit)
{
	ARG_UNUSED(port);
	ARG_UNUSED(bit);
	/* STUB */
}

static inline __attribute__((always_inline))
	int sys_io_test_bit(io_port_t port, unsigned int bit)
{
	ARG_UNUSED(port);
	ARG_UNUSED(bit);
	/* STUB */
	return 0;
}

static inline __attribute__((always_inline))
	int sys_io_test_and_set_bit(io_port_t port, unsigned int bit)
{
	ARG_UNUSED(port);
	ARG_UNUSED(bit);
	/* STUB */
	return 0;
}

static inline __attribute__((always_inline))
	int sys_io_test_and_clear_bit(io_port_t port, unsigned int bit)
{
	ARG_UNUSED(port);
	ARG_UNUSED(bit);
	/* STUB */
	return 0;
}

static inline __attribute__((always_inline))
	void sys_write8(uint8_t data, mm_reg_t addr)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(data);
	/* STUB */
}

static inline __attribute__((always_inline))
	uint8_t sys_read8(mm_reg_t addr)
{
	ARG_UNUSED(addr);
	/* STUB */
	return 0;
}

static inline __attribute__((always_inline))
	void sys_write16(uint16_t data, mm_reg_t addr)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(data);
	/* STUB */
}

static inline __attribute__((always_inline))
	uint16_t sys_read16(mm_reg_t addr)
{
	ARG_UNUSED(addr);
	/* STUB */
	return 0;
}

static inline __attribute__((always_inline))
	void sys_write32(uint32_t data, mm_reg_t addr)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(data);
	/* STUB */
}

static inline __attribute__((always_inline))
	uint32_t sys_read32(mm_reg_t addr)
{
	ARG_UNUSED(addr);
	/* STUB */
	return 0;
}

static inline __attribute__((always_inline))
	void sys_set_bit(mem_addr_t addr, unsigned int bit)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(bit);

	/* STUB */
}

static inline __attribute__((always_inline))
	void sys_clear_bit(mem_addr_t addr, unsigned int bit)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(bit);

	/* STUB */
}

static inline __attribute__((always_inline))
	int sys_test_bit(mem_addr_t addr, unsigned int bit)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(bit);

	/* STUB */

	return 0;
}

static inline __attribute__((always_inline))
	int sys_test_and_set_bit(mem_addr_t addr, unsigned int bit)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(bit);

	/* STUB */

	return 0;
}

static inline __attribute__((always_inline))
	int sys_test_and_clear_bit(mem_addr_t addr, unsigned int bit)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(bit);

	/* STUB */

	return 0;
}

static inline __attribute__((always_inline))
	void sys_bitfield_set_bit(mem_addr_t addr, unsigned int bit)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(bit);

	/* STUB */
}

static inline __attribute__((always_inline))
	void sys_bitfield_clear_bit(mem_addr_t addr, unsigned int bit)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(bit);

	/* STUB */

}

static inline __attribute__((always_inline))
	int sys_bitfield_test_bit(mem_addr_t addr, unsigned int bit)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(bit);

	/* STUB */

	return 0;
}

static inline __attribute__((always_inline))
	int sys_bitfield_test_and_set_bit(mem_addr_t addr, unsigned int bit)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(bit);

	/* STUB */

	return 0;
}

static inline __attribute__((always_inline))
	int sys_bitfield_test_and_clear_bit(mem_addr_t addr, unsigned int bit)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(bit);

	/* STUB */

	return 0;
}


#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ASM_INLINE_GCC_PUBLIC_GCC_H */
