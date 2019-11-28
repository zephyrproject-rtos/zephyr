/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <stdio.h>
#include <string.h>

#include <linker/linker-defs.h>

#define CCM_DATA_VAR_8_VAL	0x12
#define CCM_DATA_VAR_16_VAL	0x3456
#define CCM_DATA_VAR_32_VAL	0x789ABCDE

#define CCM_DATA_ARRAY_SIZE	5
#define CCM_BSS_ARRAY_SIZE	7
#define CCM_NOINIT_ARRAY_SIZE	11

#define CCM_DATA_ARRAY_VAL(i)	(((i)+1)*0x11)

u8_t __ccm_data_section ccm_data_var_8 = CCM_DATA_VAR_8_VAL;
u16_t __ccm_data_section ccm_data_var_16 = CCM_DATA_VAR_16_VAL;
u32_t __ccm_data_section ccm_data_var_32 = CCM_DATA_VAR_32_VAL;

u8_t __ccm_data_section ccm_data_array[CCM_DATA_ARRAY_SIZE] = {
	CCM_DATA_ARRAY_VAL(0),
	CCM_DATA_ARRAY_VAL(1),
	CCM_DATA_ARRAY_VAL(2),
	CCM_DATA_ARRAY_VAL(3),
	CCM_DATA_ARRAY_VAL(4),
};
u8_t __ccm_bss_section ccm_bss_array[CCM_BSS_ARRAY_SIZE];
u8_t __ccm_noinit_section ccm_noinit_array[CCM_NOINIT_ARRAY_SIZE];

void print_array(const void *array, u32_t size)
{
	int i;

	for (i = 0; i < size; i++) {
		printf("0x%02x ", ((const char *)array)[i]);
	}
}

void check_initial_var_values(void)
{
	int i;
	int check_failed = 0;

	printf("\nChecking initial variable values: ... ");

	if (ccm_data_var_8 != CCM_DATA_VAR_8_VAL) {
		check_failed = 1;
		printf("\nccm_data_var_8 incorrect: 0x%02x should be 0x%02x",
			ccm_data_var_8, CCM_DATA_VAR_8_VAL);
	}

	if (ccm_data_var_16 != CCM_DATA_VAR_16_VAL) {
		check_failed = 1;
		printf("\nccm_data_var_16 incorrect: 0x%04x should be 0x%04x",
			ccm_data_var_16, CCM_DATA_VAR_16_VAL);
	}

	if (ccm_data_var_32 != CCM_DATA_VAR_32_VAL) {
		check_failed = 1;
		printf("\nccm_data_var_32 incorrect: 0x%08x should be 0x%08x",
			ccm_data_var_32, CCM_DATA_VAR_32_VAL);
	}

	for (i = 0; i < CCM_DATA_ARRAY_SIZE; i++) {
		if (ccm_data_array[i] != CCM_DATA_ARRAY_VAL(i)) {
			check_failed = 1;
			printf("\nccm_data_array[%d] incorrect: "
				"0x%02x should be 0x%02x",
				i, ccm_data_array[i], CCM_DATA_ARRAY_VAL(i));
		}
	}

	for (i = 0; i < CCM_BSS_ARRAY_SIZE; i++) {
		if (ccm_bss_array[i] != 0x00) {
			check_failed = 1;
			printf("\nccm_bss_array[%d] incorrect: "
				"0x%02x should be 0x00",
				i, ccm_bss_array[i]);
		}
	}

	if (!check_failed) {
		printf("PASSED\n");
	}
}

void print_var_values(void)
{
	printf("ccm_data_var_8  addr: %p value: 0x%02x\n",
		&ccm_data_var_8, ccm_data_var_8);
	printf("ccm_data_var_16 addr: %p value: 0x%04x\n",
		&ccm_data_var_16, ccm_data_var_16);
	printf("ccm_data_var_32 addr: %p value: 0x%08x\n",
		&ccm_data_var_32, ccm_data_var_32);
	printf("ccm_data_array  addr: %p size: %d value:\n\t",
		ccm_data_array, (int)sizeof(ccm_data_array));
	print_array(ccm_data_array, sizeof(ccm_data_array));
	printf("\nccm_bss_array addr: %p size: %d value:\n\t",
		ccm_bss_array, (int)sizeof(ccm_bss_array));
	print_array(ccm_bss_array, sizeof(ccm_bss_array));
	printf("\nccm_noinit_array addr: %p size: %d value:\n\t",
		ccm_noinit_array, (int)sizeof(ccm_noinit_array));
	print_array(ccm_noinit_array, sizeof(ccm_noinit_array));
	printf("\n");
}

void main(void)
{
	printf("\nCCM (Core Coupled Memory) usage example\n\n");

	printf("The total used CCM area   : [%p, %p)\n",
		&__ccm_start, &__ccm_end);
	printf("Zero initialized BSS area : [%p, %p)\n",
		&__ccm_bss_start, &__ccm_bss_end);
	printf("Unitialized NOINIT area   : [%p, %p)\n",
		&__ccm_noinit_start, &__ccm_noinit_end);
	printf("Initialised DATA area     : [%p, %p)\n",
		&__ccm_data_start, &__ccm_data_end);
	printf("Start of DATA in FLASH    : %p\n",
		&__ccm_data_rom_start);

	check_initial_var_values();

	printf("\nInitial variable values:\n");
	print_var_values();

	ccm_data_var_8  = ~CCM_DATA_VAR_8_VAL;
	ccm_data_var_16 = ~CCM_DATA_VAR_16_VAL;
	ccm_data_var_32 = ~CCM_DATA_VAR_32_VAL;

	(void)memset(ccm_data_array, 0xAA, sizeof(ccm_data_array));
	(void)memset(ccm_bss_array, 0xBB, sizeof(ccm_bss_array));
	(void)memset(ccm_noinit_array, 0xCC, sizeof(ccm_noinit_array));

	printf("\nVariable values after writing:\n");
	print_var_values();

	printf("\nExample end\n");
}

