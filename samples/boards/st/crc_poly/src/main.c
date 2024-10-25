/*
 * Copyright (c) 2024 STMicroelectronics
 * Basic CRC calculation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_crc.h>

#define BUFFER_SIZE 39 /* 9 u32 + 1 u16 + 1 u8 */

/*
 * 8-bit long user defined Polynomial value for this example
 * In this example, the polynomial is set manually to 0x9B
 * that is X^8 + X^7 + X^4 + X^3 + X + 1.
 */
#define CRC8_POLYNOMIAL_VALUE 0x9B

/* Used for storing CRC Value */
__IO uint8_t crc_value;

static const uint8_t data_buffer[BUFFER_SIZE] = {
	0x21, 0x10, 0x00, 0x00, 0x63, 0x30, 0x42, 0x20, 0xa5, 0x50, 0x84, 0x40, 0xe7,
	0x70, 0xc6, 0x60, 0x4a, 0xa1, 0x29, 0x91, 0x8c, 0xc1, 0x6b, 0xb1, 0xce, 0xe1,
	0xad, 0xd1, 0x31, 0x12, 0xef, 0xf1, 0x52, 0x22, 0x73, 0x32, 0xa1, 0xb2, 0xc3};

/* Expected CRC Value */
uint32_t expected_crc = 0x0c;

/* Init and config the CRC instance : not defined by a DTS node */
void CRC_init(void)
{
	/* Enable CRC peripheral clock */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_CRC);

	LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_NONE);
	LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_NONE);
	LL_CRC_SetPolynomialCoef(CRC, LL_CRC_DEFAULT_CRC32_POLY);
	LL_CRC_SetPolynomialSize(CRC, LL_CRC_POLYLENGTH_32B);
	LL_CRC_SetInitialData(CRC, LL_CRC_DEFAULT_CRC_INITVALUE);

	/* Configure CRC calculation unit with user defined polynomial value, 8-bit long */
	LL_CRC_SetPolynomialCoef(CRC, CRC8_POLYNOMIAL_VALUE);
	LL_CRC_SetPolynomialSize(CRC, LL_CRC_POLYLENGTH_8B);
}

/*
 * This function performs CRC calculation on BufferSize bytes from input data buffer.
 * buffer_size Nb of bytes to be processed for CRC calculation
 * return the 8-bit CRC value computed on input data buffer
 */
uint8_t CRC_calculate(uint32_t buffer_size)
{
	register uint32_t data = 0;
	register uint32_t index = 0;

	/* Compute the CRC of Data Buffer array*/
	for (index = 0; index < (buffer_size / 4); index++) {
		data = (uint32_t)((data_buffer[4 * index] << 24) |
				  (data_buffer[4 * index + 1] << 16) |
				  (data_buffer[4 * index + 2] << 8) | data_buffer[4 * index + 3]);
		LL_CRC_FeedData32(CRC, data);
	}

	/* Last bytes specific handling */
	if ((BUFFER_SIZE % 4) != 0) {
		if (BUFFER_SIZE % 4 == 1) {
			LL_CRC_FeedData8(CRC, data_buffer[4 * index]);
		}
		if (BUFFER_SIZE % 4 == 2) {
			LL_CRC_FeedData16(CRC, (uint16_t)((data_buffer[4 * index] << 8) |
							  data_buffer[4 * index + 1]));
		}
		if (BUFFER_SIZE % 4 == 3) {
			LL_CRC_FeedData16(CRC, (uint16_t)((data_buffer[4 * index] << 8) |
							  data_buffer[4 * index + 1]));
			LL_CRC_FeedData8(CRC, data_buffer[4 * index + 2]);
		}
	}

	/* Return computed CRC value */
	return (LL_CRC_ReadData8(CRC));
}

int main(void)
{
	printf("\nCRC computation\n");
	printf("===============\n\n");

	/* Init and config the CRC peripheral */
	CRC_init();

	/* Compute the CRC of the data_buffer */
	crc_value = CRC_calculate(BUFFER_SIZE);

	if (crc_value != expected_crc) {
		printf("Wrong CRC calculation 0x%x (expected 0x%x)\n", crc_value, expected_crc);
		return -1;
	}

	printf("CRC = 0x%x\n", crc_value);

	return 0;
}
