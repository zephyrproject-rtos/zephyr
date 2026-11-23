/*
 * Copyright (c) 2026, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <string.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest_error_hook.h>

uint32_t iram_data[17] __aligned(4) __attribute__((section(CONFIG_TEST_IRAM_SECTION), used));

/* Below arrays should be placed in memory that won't cause exceptions under test */
static uint32_t iram_data_init_template[17] = {
	0x12345678U, 0xABCDEF01U, 0xDEADBEEFU, 0x0D15EA5EU, 0xAAAAAAAAU, 0xAAAAAAAAU,
	0xAAAAAAAAU, 0xAAAAAAAAU, 0xAAAAAAAAU, 0xAAAAAAAAU, 0xAAAAAAAAU, 0xAAAAAAAAU,
	0xAAAAAAAAU, 0xAAAAAAAAU, 0xAAAAAAAAU, 0xAAAAAAAAU, 0xAAAAAAAAU};
static uint32_t expected_iram_data[17] = {0};

static void unsupported_unsigned_load_store_emulation_before(void *f)
{
	ARG_UNUSED(f);
	for (int i = 0; i < ARRAY_SIZE(iram_data); i++) {
		iram_data[i] = expected_iram_data[i] = iram_data_init_template[i];
	}
}

/* Checking IRAM is unmodified or modified only by preceding store */
#define CHECK_IRAM_DATA(type, mode, ptr)                                                           \
	do {                                                                                       \
		const char *type_ = (type);                                                        \
		const char *mode_ = (mode);                                                        \
		const void *ptr_ = (ptr);                                                          \
		for (size_t i_ = 0; i_ < ARRAY_SIZE(iram_data); i_++) {                            \
			zassert_equal(iram_data[i_], expected_iram_data[i_],                       \
				      "%s %s at %p, mismatch at %p: expected 0x%08X, got 0x%08X",  \
				      type_, mode_, ptr_, (void *)(iram_data + i_),                \
				      expected_iram_data[i_], iram_data[i_]);                      \
		}                                                                                  \
	} while (0)

#define STR_1(x) #x
#define STR(x)   STR_1(x)

/* Load into A# register; A# is at, destination register */
#define LOAD(instr, addr_val, offset, out_reg, out_val)                                            \
	__asm__ volatile(instr                                                                     \
			 " a" STR(out_reg) ", %1, " STR(offset) "\n\t"                             \
								"mov %0, a" STR(out_reg) "\n\t"    \
			 : "=a"(out_val)                                                           \
			 : "r"(addr_val)                                                           \
			 : "a" STR(out_reg), "memory")

/* Store from A# register; A# is at, source data register */
#define STORE(instr, addr_val, offset, in_reg, in_val)                                             \
	__asm__ volatile("mov a" STR(in_reg) ", %0\n\t" instr                                      \
					     " a" STR(in_reg) ", %1, " STR(offset) "\n\t"          \
			 :                                                                         \
			 : "a"(in_val), "r"(addr_val)                                              \
			 : "a" STR(in_reg), "memory")

#define ACCESS_A(reg, op, instr, addr_val, offset, val)                                            \
	do {                                                                                       \
		switch (reg) {                                                                     \
		/* A0 - A1 handled by assembly file */                                             \
		case 2:                                                                            \
			op(instr, addr_val, offset, 2, val);                                       \
			break;                                                                     \
		case 3:                                                                            \
			op(instr, addr_val, offset, 3, val);                                       \
			break;                                                                     \
		case 4:                                                                            \
			op(instr, addr_val, offset, 4, val);                                       \
			break;                                                                     \
		case 5:                                                                            \
			op(instr, addr_val, offset, 5, val);                                       \
			break;                                                                     \
		case 6:                                                                            \
			op(instr, addr_val, offset, 6, val);                                       \
			break;                                                                     \
		case 7:                                                                            \
			op(instr, addr_val, offset, 7, val);                                       \
			break;                                                                     \
		case 8:                                                                            \
			op(instr, addr_val, offset, 8, val);                                       \
			break;                                                                     \
		case 9:                                                                            \
			op(instr, addr_val, offset, 9, val);                                       \
			break;                                                                     \
		case 10:                                                                           \
			op(instr, addr_val, offset, 10, val);                                      \
			break;                                                                     \
		case 11:                                                                           \
			op(instr, addr_val, offset, 11, val);                                      \
			break;                                                                     \
		case 12:                                                                           \
			op(instr, addr_val, offset, 12, val);                                      \
			break;                                                                     \
		case 13:                                                                           \
			op(instr, addr_val, offset, 13, val);                                      \
			break;                                                                     \
		case 14:                                                                           \
			op(instr, addr_val, offset, 14, val);                                      \
			break;                                                                     \
		case 15:                                                                           \
			op(instr, addr_val, offset, 15, val);                                      \
			break;                                                                     \
		default:                                                                           \
			zassert_unreachable("Unsupported register");                               \
		}                                                                                  \
	} while (0)

#define LOAD_A(out_reg, instr, addr_val, offset, out_val)                                          \
	ACCESS_A(out_reg, LOAD, instr, addr_val, offset, out_val)

#define STORE_A(in_reg, instr, addr_val, offset, in_val)                                           \
	ACCESS_A(in_reg, STORE, instr, addr_val, offset, in_val)

extern uint8_t l8ui_a0(void *addr);
extern uint16_t l16ui_a1(uint8_t *addr);

extern void s16i_a0(uint8_t *addr, uint16_t val);
extern void s32in_a1(uint8_t *addr, uint32_t val);

ZTEST(unsupported_unsigned_load_store_emulation, test_load)
{
	/* Byte loads into A2 - A15 at all byte offsets in word */
	for (int i = 0; i < 14; i++) {
		uint8_t *byte_ptr = (uint8_t *)(iram_data) + i;
		uint8_t expected_byte = *((uint8_t *)(expected_iram_data) + i);
		uint8_t actual_byte = 0;

		LOAD_A(i + 2, "l8ui", byte_ptr, 0, actual_byte);

		zassert_equal(expected_byte, actual_byte,
			      "byte load at %p, mismatch at %p: expected 0x%02X, got 0x%02X",
			      (void *)(byte_ptr), (void *)(byte_ptr), expected_byte, actual_byte);
		CHECK_IRAM_DATA("byte", "load", byte_ptr);
	}

	/* Half word loads into A2 - A15 at all byte offsets in word */
	for (int i = 0; i < 14; i++) {
		uint8_t *byte_ptr = (uint8_t *)(iram_data) + i;
		uint16_t expected_half = 0;
		uint16_t actual_half = 0;

		memcpy(&expected_half, (uint8_t *)(expected_iram_data) + i, sizeof(uint16_t));
		LOAD_A(i + 2, "l16ui", byte_ptr, 0, actual_half);

		zassert_equal(expected_half, actual_half,
			      "half load at %p, mismatch at %p: expected 0x%04X, got 0x%04X",
			      (void *)(byte_ptr), (void *)(byte_ptr), expected_half, actual_half);
		CHECK_IRAM_DATA("half", "load", byte_ptr);
	}

	/* Word loads into A2 - A15 at all byte offsets in word */
	for (int i = 0; i < 14; i++) {
		uint8_t *byte_ptr = (uint8_t *)(iram_data) + i;
		uint32_t expected_word = 0;
		uint32_t actual_word = 0;

		memcpy(&expected_word, (uint8_t *)(expected_iram_data) + i, sizeof(uint32_t));

		/* Prevent compiler transformation to l32i.n by giving offset > 60 */
		uintptr_t offset_addr = (uintptr_t)(byte_ptr)-64;

		LOAD_A(i + 2, "l32i", offset_addr, 64, actual_word);

		zassert_equal(expected_word, actual_word,
			      "word load at %p, mismatch at %p: expected 0x%08X, got 0x%08X",
			      (void *)(byte_ptr), (void *)(byte_ptr), expected_word, actual_word);
		CHECK_IRAM_DATA("word", "load", byte_ptr);

		LOAD_A(i + 2, "l32i.n", byte_ptr, 0, actual_word);

		zassert_equal(expected_word, actual_word,
			      "narrow-encoding word load at %p, mismatch at %p: expected 0x%08X, "
			      "got 0x%08X",
			      (void *)(byte_ptr), (void *)(byte_ptr), expected_word, actual_word);
		CHECK_IRAM_DATA("narrow-encoding word", "load", byte_ptr);
	}
}

ZTEST(unsupported_unsigned_load_store_emulation, test_store)
{
	/* Byte stores from A2 - A15 / word 0 of template into word 4 of IRAM */
	for (int i = 0; i < 14; i++) {
		uint8_t *byte_ptr_src = ((uint8_t *)(iram_data_init_template)) + i;
		uint8_t *byte_ptr_dst_expected = ((uint8_t *)(expected_iram_data + 4)) + i;
		uint8_t *byte_ptr_dst_actual = ((uint8_t *)(iram_data + 4)) + i;

		*byte_ptr_dst_expected = *byte_ptr_src;
		STORE_A(i + 2, "s8i", byte_ptr_dst_actual, 0, *byte_ptr_src);

		CHECK_IRAM_DATA("byte", "store", byte_ptr_dst_actual);
	}

	/* Half word stores from A2 - A15 / word 0 of template into word 4 of IRAM */
	for (int i = 0; i < 14; i++) {
		uint8_t *byte_ptr_src = ((uint8_t *)(iram_data_init_template)) + i;
		uint8_t *byte_ptr_dst_expected = ((uint8_t *)(expected_iram_data + 4)) + i;
		uint8_t *byte_ptr_dst_actual = ((uint8_t *)(iram_data + 4)) + i;
		uint16_t source_data = 0;

		memcpy(&source_data, byte_ptr_src, sizeof(uint16_t));

		memcpy(byte_ptr_dst_expected, &source_data, sizeof(uint16_t));
		STORE_A(i + 2, "s16i", byte_ptr_dst_actual, 0, source_data);

		CHECK_IRAM_DATA("half", "store", byte_ptr_dst_actual);
	}

	for (int i = 0; i < 14; i++) {
		uint8_t *byte_ptr_src = ((uint8_t *)(iram_data_init_template)) + i;
		uint8_t *byte_ptr_dst_expected = ((uint8_t *)(expected_iram_data + 4)) + i;
		uint8_t *byte_ptr_dst_actual = ((uint8_t *)(iram_data + 4)) + i;
		uint32_t source_data = 0;

		memcpy(&source_data, byte_ptr_src, sizeof(uint32_t));

		memcpy(byte_ptr_dst_expected, &source_data, sizeof(uint32_t));

		/* Prevent compiler transformation to s32i.n by giving offset > 60 */
		uintptr_t offset_addr = (uintptr_t)(byte_ptr_dst_actual)-64;

		STORE_A(i + 2, "s32i", offset_addr, 64, source_data);

		CHECK_IRAM_DATA("word", "store", byte_ptr_dst_actual);

		memcpy(byte_ptr_dst_expected + 32, byte_ptr_src, sizeof(uint32_t));
		STORE_A(i + 2, "s32i.n", byte_ptr_dst_actual + 32, 0, source_data);

		CHECK_IRAM_DATA("word", "store", byte_ptr_dst_actual + 32);
	}
}

/* Test a small subset of unsupported load/store instructions with low registers */
ZTEST(unsupported_unsigned_load_store_emulation, test_load_store_low_registers)
{
	uint8_t *byte_ptr = (uint8_t *)(iram_data);
	uint8_t expected_byte = *(uint8_t *)(expected_iram_data);
	uint8_t actual_byte = l8ui_a0(byte_ptr);

	zassert_equal(expected_byte, actual_byte,
		      "byte load at %p, mismatch at %p: expected 0x%02X, got 0x%02X",
		      (void *)byte_ptr, (void *)byte_ptr, expected_byte, actual_byte);
	CHECK_IRAM_DATA("byte", "load", byte_ptr);

	/* Unaligned half-word load from byte 1 */
	byte_ptr = (uint8_t *)(iram_data) + 1;
	uint16_t expected_half = 0;
	uint16_t actual_half = 0;

	memcpy(&expected_half, (uint8_t *)(expected_iram_data) + 1, sizeof(uint16_t));
	actual_half = l16ui_a1((void *)byte_ptr);

	zassert_equal(expected_half, actual_half,
		      "half-word load at %p, mismatch at %p: expected 0x%04X, got 0x%04X",
		      (void *)byte_ptr, (void *)byte_ptr, expected_half, actual_half);
	CHECK_IRAM_DATA("half", "load", byte_ptr);

	/* Unaligned half-word store to byte 1 */
	uint16_t source_half_data = 0xBEEF;

	memcpy((uint8_t *)(expected_iram_data) + 1, &source_half_data, sizeof(uint16_t));
	s16i_a0(byte_ptr, source_half_data);

	CHECK_IRAM_DATA("half", "store", byte_ptr);

	/* Unaligned word store to byte 3 */
	byte_ptr += 2;
	uint32_t source_word_data = 0xDEADBEEFU;

	memcpy((uint8_t *)(expected_iram_data) + 3, &source_word_data, sizeof(uint32_t));
	s32in_a1(byte_ptr, source_word_data);

	CHECK_IRAM_DATA("word", "store", byte_ptr);
}

static void trigger_cannot_handle_fault(void)
{
	uint16_t half = 0;

	LOAD_A(2, "l16si", iram_data, 0, half);
}

ZTEST(unsupported_unsigned_load_store_emulation, test_invalid_instruction)
{
	ztest_set_fault_valid(true);
	trigger_cannot_handle_fault();
	zassert_unreachable("Fault was not triggered for unsupported instruction");
}

ZTEST(unsupported_unsigned_load_store_emulation, test_memcpy_memset)
{
	/* memcpy from first half of IRAM to second half with unaligned destination and source */
	memcpy((uint8_t *)(expected_iram_data) + 18, (uint8_t *)(expected_iram_data) + 1, 13);
	memcpy((uint8_t *)(iram_data) + 18, (uint8_t *)(iram_data) + 1, 13);

	CHECK_IRAM_DATA("unaligned src/dst", "memcpy", (uint8_t *)(iram_data) + 18);

	/* memset first half of IRAM with unaligned destination */
	memset((uint8_t *)(expected_iram_data) + 1, 0xCD, 13);
	memset((uint8_t *)(iram_data) + 1, 0xCD, 13);

	CHECK_IRAM_DATA("unaligned dst", "memset", (uint8_t *)(iram_data) + 1);
}

ZTEST_SUITE(unsupported_unsigned_load_store_emulation, NULL, NULL,
	    unsupported_unsigned_load_store_emulation_before, NULL, NULL);
