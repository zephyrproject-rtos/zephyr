/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_FLASH_FLASH_SAM_QSPI_H_
#define ZEPHYR_DRIVERS_FLASH_FLASH_SAM_QSPI_H_

/**
 * enum spi_flash_protocol - SPI flash command protocol
 */
#define SFLASH_PROTO_INST_SHIFT	16
#define SFLASH_PROTO_INST_MASK	(0xFFUL << 16)
#define SFLASH_PROTO_INST(nbits)					\
	((((unsigned long)(nbits)) << SFLASH_PROTO_INST_SHIFT) &	\
	 SFLASH_PROTO_INST_MASK)

#define SFLASH_PROTO_ADDR_SHIFT	8
#define SFLASH_PROTO_ADDR_MASK	(0xFFUL << 8)
#define SFLASH_PROTO_ADDR(nbits)					\
	((((unsigned long)(nbits)) << SFLASH_PROTO_ADDR_SHIFT) &	\
	 SFLASH_PROTO_ADDR_MASK)

#define SFLASH_PROTO_DATA_SHIFT	0
#define SFLASH_PROTO_DATA_MASK	(0xFFUL << 0)
#define SFLASH_PROTO_DATA(nbits)					\
	((((unsigned long)(nbits)) << SFLASH_PROTO_DATA_SHIFT) &	\
	 SFLASH_PROTO_DATA_MASK)

#define SFLASH_PROTO(inst_nbits, addr_nbits, data_nbits)	\
	(SFLASH_PROTO_INST(inst_nbits) |			\
	 SFLASH_PROTO_ADDR(addr_nbits) |			\
	 SFLASH_PROTO_DATA(data_nbits))

enum spi_flash_protocol {
	SFLASH_PROTO_1_1_1 = SFLASH_PROTO(1, 1, 1),
	SFLASH_PROTO_1_1_2 = SFLASH_PROTO(1, 1, 2),
	SFLASH_PROTO_1_1_4 = SFLASH_PROTO(1, 1, 4),
	SFLASH_PROTO_1_2_2 = SFLASH_PROTO(1, 2, 2),
	SFLASH_PROTO_1_4_4 = SFLASH_PROTO(1, 4, 4),
	SFLASH_PROTO_2_2_2 = SFLASH_PROTO(2, 2, 2),
	SFLASH_PROTO_4_4_4 = SFLASH_PROTO(4, 4, 4),
	SFLASH_PROTO_1_1_8 = SFLASH_PROTO(1, 1, 8),
	SFLASH_PROTO_1_8_8 = SFLASH_PROTO(1, 8, 8),
	SFLASH_PROTO_8_8_8 = SFLASH_PROTO(8, 8, 8),
};


static inline
uint8_t spi_flash_protocol_get_inst_nbits(enum spi_flash_protocol proto)
{
	return ((uint32_t)(proto & SFLASH_PROTO_INST_MASK)) >>
		SFLASH_PROTO_INST_SHIFT;
}

static inline
uint8_t spi_flash_protocol_get_addr_nbits(enum spi_flash_protocol proto)
{
	return ((uint32_t)(proto & SFLASH_PROTO_ADDR_MASK)) >>
		SFLASH_PROTO_ADDR_SHIFT;
}

static inline
uint8_t spi_flash_protocol_get_data_nbits(enum spi_flash_protocol proto)
{
	return ((uint32_t)(proto & SFLASH_PROTO_DATA_MASK)) >>
		SFLASH_PROTO_DATA_SHIFT;
}


#define SFLASH_INST_ULBPR			0x98
#define SFLASH_MAX_ID_LEN			6

/*
 * enum qspi_mem_data_dir - describes the direction of a QSPI memory data
 * transfer from the controller perspective
 * @QSPI_MEM_NO_DATA: no data transferred
 * @QSPI_MEM_DATA_IN: data coming from the SPI memory
 * @QSPI_MEM_DATA_OUT: data sent to the SPI memory
 */
enum qspi_mem_data_dir {
	QSPI_MEM_NO_DATA,
	QSPI_MEM_DATA_IN,
	QSPI_MEM_DATA_OUT,
};

struct qspi_priv {
	qspi_registers_t *base;
	uint32_t mem;
	const struct device *dma;
	uint32_t dma_channel;
};

/*
 * struct qspi_mem_op - describes a QSPI memory operation
 */
struct qspi_mem_op {
	uint32_t proto;
	struct {
		uint8_t modebits;
		uint8_t waitstates;
		uint8_t dtr;
		uint16_t opcode;
	} cmd;

	struct {
		uint8_t nbytes;
		uint32_t val;
	} addr;

	struct {
		enum qspi_mem_data_dir dir;
		uint32_t nbytes;
		union {
			void *in;
			const void *out;
		} buf;
	} data;
};

int qspi_sama7g5_init(struct qspi_priv *priv);
int qspi_exec_op(struct qspi_priv *priv, const struct qspi_mem_op *op);

#endif /* ZEPHYR_DRIVERS_FLASH_FLASH_SAM_QSPI_H_ */
