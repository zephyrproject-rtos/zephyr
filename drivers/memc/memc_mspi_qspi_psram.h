/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MEMC_MEMC_MSPI_QSPI_PSRAM_H_
#define ZEPHYR_DRIVERS_MEMC_MEMC_MSPI_QSPI_PSRAM_H_

#include <stdint.h>

/**
 * @brief Supported QPI PSRAM chip variants.
 *
 * Indices 0..N must match the chip-variant enum order in the DT binding.
 * GENERIC is the sentinel used when no chip-variant is specified in DT; the
 * driver then uses standard QPI init commands but reads all transfer
 * parameters (read_cmd, write_cmd, rx-dummy, cmd/addr length, CE timing)
 * from DT properties.
 */
enum qspi_psram_variant {
	QSPI_PSRAM_VARIANT_ESP64H,
	QSPI_PSRAM_VARIANT_IS66WVS4M8BLL,
	QSPI_PSRAM_VARIANT_IS66WVS8M8BLL,
	QSPI_PSRAM_VARIANT_GENERIC, /* must be last */
};

/**
 * @brief Per-chip parameters sourced from the device datasheet.
 *
 * These values are fixed for a given chip and are not user-configurable.
 * The driver applies them during initialization, overriding any DT
 * ce-break-config values.
 */
/*
 * DT enum indices for command-length and address-length (mspi-device.yaml).
 * Used to populate chip_params without depending on the DT binding headers.
 */
#define QSPI_PSRAM_CMD_LEN_1BYTE   1   /* "INSTR_1_BYTE" */
#define QSPI_PSRAM_ADDR_LEN_3BYTE  3   /* "ADDR_3_BYTE"  */

struct qspi_psram_chip_params {
	uint8_t  enter_qpi_cmd;       /* SPI command to enter QPI mode         */
	uint8_t  exit_qpi_cmd;        /* QPI command to exit back to SPI mode  */
	uint8_t  read_cmd;            /* QPI fast-read command                 */
	uint8_t  write_cmd;           /* QPI write command                     */
	uint8_t  reset_en_cmd;        /* Reset Enable command                  */
	uint8_t  reset_cmd;           /* Reset command                         */
	uint8_t  read_id_cmd;         /* Read ID command (returns MF/KGD/EID)  */
	uint8_t  kgd_value;           /* Expected KGD byte in Read ID response */
	uint8_t  cmd_length;          /* Command length enum index (DT binding) */
	uint8_t  addr_length;         /* Address length enum index (DT binding) */
	uint8_t  default_rx_dummy;    /* Recommended read dummy cycles         */
	uint8_t  default_tx_dummy;    /* Recommended write dummy cycles        */
	uint16_t ce_max_burst_bytes;  /* Max bytes per CE cycle (mem_boundary) */
	uint32_t ce_refresh_us;       /* Max CE assertion time in μs           */
};

/* Frequency used for initial SPI-mode register access before entering QPI */
#define QSPI_PSRAM_SPI_INIT_FREQ    24000000U

/* Minimum delay after software reset before first access (tRST) */
#define QSPI_PSRAM_RESET_DELAY_US   200U

#endif /* ZEPHYR_DRIVERS_MEMC_MEMC_MSPI_QSPI_PSRAM_H_ */
