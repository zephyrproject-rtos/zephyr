/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MARLIN3_HAL_SFC_H
#define __MARLIN3_HAL_SFC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "uwp_hal.h"

	typedef struct SPIFLASH_ExtCfg {
		int voltage;
		u32_t desity;
		u32_t reserved1;
		u32_t reserved2;
		u32_t reserved3;
		u32_t reserved4;
		u32_t reserved5;
	} *Spiflash_ExtCfg_PRT;

	typedef struct nor_flash_config_s {
		u32_t bank_num;
		u32_t sect_num;
		u32_t file_sect_num;
		u32_t sect_size;
		u32_t start_addr;
		u32_t efs_start_addr;
		u32_t flash_size;
		u32_t fixnv_addr;
		u32_t prodinfo_addr;
		u32_t mmi_res;
		u32_t umem_addr;
		u32_t umem_size;
		u32_t spload_addr;
		u32_t ps_addr;
	} NOR_FLASH_CONFIG_T, *NOR_FLASH_CONFIG_PTR;

	typedef struct DFILE_CONFIG_Tag {
		u32_t magic_first;
		u32_t magic_second;
		u32_t image_addr;
		u32_t res_addr;
		u32_t nv_addr;
		u32_t dsp_addr;
		u32_t reserved2;
		u32_t ext[24];
		u32_t magic_end;
	} DFILE_CONFIG_T;

	struct spi_flash_region {
		unsigned int count;
		unsigned int size;
	};

	typedef enum READ_CMD_TYPE_E_TAG {
		READ_SPI = 0,
		READ_SPI_FAST,
		READ_SPI_2IO,
		READ_SPI_4IO,
		READ_QPI_FAST,
		READ_QPI_4IO,
	} READ_CMD_TYPE_E;

	struct spi_flash {
		u32_t cs;

		const char *name;
		u32_t size;
		u32_t page_size;
		u32_t sector_size;
		u32_t dummy_bytes;
		u8_t work_mode;
		u8_t support_4addr;
		int spi_rw_mode;

		int (*read_noxip) (struct spi_flash * flash, u32_t address,
				u8_t * buf, u32_t buf_size, READ_CMD_TYPE_E type);
		int (*read) (struct spi_flash * flash, u32_t offset, u32_t * buf,
				u32_t dump_len, READ_CMD_TYPE_E type);
		int (*write) (struct spi_flash * flash, u32_t offset, u32_t len,
				const void *buf);
		int (*read_sec_noxip) (struct spi_flash * flash, u8_t * buf,
				u32_t buf_size, READ_CMD_TYPE_E type);
		int (*read_sec) (struct spi_flash * flash, u32_t offset, u32_t * buf,
				u32_t dump_len, READ_CMD_TYPE_E type);
		int (*write_sec) (struct spi_flash * flash, u32_t offset, u32_t len,
				const void *buf);
		int (*erase) (struct spi_flash * flash, u32_t offset, u32_t len);
		int (*erase_chip) (struct spi_flash * flash);

		int (*reset) (void);
		int (*suspend) (struct spi_flash * flash);
		int (*resume) (struct spi_flash * flash);
		int (*lock) (struct spi_flash * flash, u32_t offset, u32_t len);
		int (*unlock) (struct spi_flash * flash, u32_t offset, u32_t len);
		int (*set_4io) (struct spi_flash * flash, u32_t op);
		int (*set_qpi) (struct spi_flash * flash, u32_t op);

		int (*set_encrypt) (u32_t op);

		void *priv;
	};

	struct spi_flash_spec_s {
		u16_t id_manufacturer;
		u16_t table_num;
		struct spi_flash_params *table;
	};

	struct spi_flash_params {
		u16_t idcode1;
		u16_t idcode2;
		u16_t page_size;
		u16_t sector_size;
		u16_t nr_sectors;
		u16_t nr_blocks;
		u16_t support_qpi;
		u16_t read_freq_max;
		u16_t dummy_clocks;
		const char *name;
	};

	struct spi_flash_struct {
		struct spi_flash flash;
		const struct spi_flash_params *params;
	};

	void uwp_spi_xip_init(void);

	int uwp_spi_flash_init(struct spi_flash *flash,
			struct spi_flash_params **params);
	void spi_flash_free(struct spi_flash *flash);

#ifdef __cplusplus
}
#endif

#endif
