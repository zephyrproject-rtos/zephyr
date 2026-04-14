/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define BFLB_FLASH_MAGIC_1 "BFNP"
#define BFLB_FLASH_MAGIC_2 "FCFG"

struct bflb_flash_magic_1 {
	char magic[4];
	uint32_t revision;
} __packed;

struct bflb_flash_magic_2 {
	char magic[4];
} __packed;

/* Raw flash configuration data structure */
struct bflb_header_flash_cfg {
/* Serial flash interface mode, bit0-3:spi mode, bit4:unwrap, bit5:32-bits addr mode support */
	uint8_t  io_mode;
/* Support continuous read mode, bit0:continuous read mode support, bit1:read mode cfg */
	uint8_t  c_read_support;
/* SPI clock delay, bit0-3:delay,bit4-6:pad delay */
	uint8_t  clk_delay;
/* SPI clock phase invert, bit0:clck invert, bit1:rx invert, bit2-4:pad delay, bit5-7:pad delay */
	uint8_t  clk_invert;
/* Flash enable reset command */
	uint8_t  reset_en_cmd;
/* Flash reset command */
	uint8_t  reset_cmd;
/* Flash reset continuous read command */
	uint8_t  reset_c_read_cmd;
/* Flash reset continuous read command size */
	uint8_t  reset_c_read_cmd_size;
/* JEDEC ID command */
	uint8_t  jedec_id_cmd;
/* JEDEC ID command dummy clock */
	uint8_t  jedec_id_cmd_dmy_clk;
#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL60X) || \
	defined(CONFIG_SOC_SERIES_BL70XL)
/* QPI JEDEC ID command */
	uint8_t  qpi_jedec_id_cmd;
/* QPI JEDEC ID command dummy clock */
	uint8_t  qpi_jedec_id_cmd_dmy_clk;
#else
/* Enter 32-bits addr command */
	uint8_t  enter_32bits_addr_cmd;
/* Exit 32-bits addr command */
	uint8_t  exit_32bits_addr_cmd;
#endif
/* (x * 1024) bytes */
	uint8_t  sector_size;
/* Manufacturer ID */
	uint8_t  mid;
/* Page size */
	uint16_t page_size;
/* Chip erase cmd */
	uint8_t  chip_erase_cmd;
/* Sector erase command */
	uint8_t  sector_erase_cmd;
/* Block 32K erase command*/
	uint8_t  blk32_erase_cmd;
/* Block 64K erase command */
	uint8_t  blk64_erase_cmd;
/* Write enable command, needed before every erase or program, or register write */
	uint8_t  write_enable_cmd;
/* Page program command */
	uint8_t  page_program_cmd;
/* QIO page program cmd */
	uint8_t  qpage_program_cmd;
/* QIO page program address mode */
	uint8_t  qpp_addr_mode;
/* Fast read command */
	uint8_t  fast_read_cmd;
/* Fast read command dummy clock */
	uint8_t  fr_dmy_clk;
/* QPI fast read command */
	uint8_t  qpi_fast_read_cmd;
/* QPI fast read command dummy clock */
	uint8_t  qpi_fr_dmy_clk;
/* Fast read dual output command */
	uint8_t  fast_read_do_cmd;
/* Fast read dual output command dummy clock */
	uint8_t  fr_do_dmy_clk;
/* Fast read dual io command */
	uint8_t  fast_read_dio_cmd;
/* Fast read dual io command dummy clock */
	uint8_t  fr_dio_dmy_clk;
/* Fast read quad output command */
	uint8_t  fast_read_qo_cmd;
/* Fast read quad output command dummy clock */
	uint8_t  fr_qo_dmy_clk;
/* Fast read quad io command */
	uint8_t  fast_read_qio_cmd;
/* Fast read quad io command dummy clock */
	uint8_t  fr_qio_dmy_clk;
/* QPI fast read quad io command */
	uint8_t  qpi_fast_read_qio_cmd;
/* QPI fast read QIO dummy clock */
	uint8_t  qpi_fr_qio_dmy_clk;
/* QPI program command */
	uint8_t  qpi_page_program_cmd;
/* Enable write volatile reg */
	uint8_t  write_vreg_enable_cmd;
/* Write enable register index */
	uint8_t  wr_enable_index;
/* Quad mode enable register index */
	uint8_t  qe_index;
/* Busy status register index */
	uint8_t  busy_index;
/* Write enable register bit pos */
	uint8_t  wr_enable_bit;
/* Quad enable register bit pos */
	uint8_t  qe_bit;
/* Busy status register bit pos */
	uint8_t  busy_bit;
/* Register length of write enable */
	uint8_t  wr_enable_write_reg_len;
/* Register length of write enable status */
	uint8_t  wr_enable_read_reg_len;
/* Register length of quad enable */
	uint8_t  qe_write_reg_len;
/* Register length of quad enable status */
	uint8_t  qe_read_reg_len;
/* Release power down command */
	uint8_t  release_powerdown;
/* Register length of contain busy status */
	uint8_t  busy_read_reg_len;
/* Read register command buffer */
	uint8_t  read_reg_cmd[4];
/* Write register command buffer */
	uint8_t  write_reg_cmd[4];
/* Enter qpi command */
	uint8_t  enter_qpi;
/* Exit qpi command */
	uint8_t  exit_qpi;
/* Config data for continuous read mode */
	uint8_t  c_read_mode;
/* Config data for exit continuous read mode */
	uint8_t  c_rexit;
/* Enable burst wrap command */
	uint8_t  burst_wrap_cmd;
/* Enable burst wrap command dummy clock */
	uint8_t  burst_wrap_cmd_dmy_clk;
/* Data and address mode for this command */
	uint8_t  burst_wrap_data_mode;
/* Data to enable burst wrap */
	uint8_t  burst_wrap_data;
/* Disable burst wrap command */
	uint8_t  de_burst_wrap_cmd;
/* Disable burst wrap command dummy clock */
	uint8_t  de_burst_wrap_cmd_dmy_clk;
/* Data and address mode for this command */
	uint8_t  de_burst_wrap_data_mode;
/* Data to disable burst wrap */
	uint8_t  de_burst_wrap_data;
/* Typical 4K(usually) erase time */
	uint16_t time_e_sector;
/* Typical 32K erase time */
	uint16_t time_e_32k;
/* Typical 64K erase time */
	uint16_t time_e_64k;
/* Typical Page program time */
	uint16_t time_page_pgm;
/* Typical Chip erase time in ms */
	uint16_t time_ce;
/* Release power down command delay time for wake up */
	uint8_t  pd_delay;
/* QE set data */
	uint8_t  qe_data;
} __packed;

struct bflb_flash_header {
	struct bflb_flash_magic_1 magic_1;
	struct bflb_flash_magic_2 magic_2;
	struct bflb_header_flash_cfg flash_cfg;
	uint32_t flash_cfg_crc;
} __packed;

struct bflb_flash_command {
/* Read write 0: read 1 : write */
	uint8_t rw;
/* Command mode 0: 1 line, 1: 4 lines */
	uint8_t cmd_mode;
/* SPI mode 0: IO 1: DO 2: QO 3: DIO 4: QIO */
	uint8_t spi_mode;
/* Address size */
	uint8_t addr_size;
/* Dummy clocks */
	uint8_t dummy_clks;
/* Transfer number of bytes */
	uint32_t nb_data;
/* Command buffer */
	uint32_t cmd_buf[2];
};

enum flash_bflb_nxip_message_id {
	NXIP_MSG_NONE = -1,
	NXIP_MSG_READ_INVALID = 0,
	NXIP_MSG_BAD_BUS_IAHB,
	NXIP_MSG_BAD_BUS_SAHB,
	NXIP_MSG_BAD_QE,
	NXIP_MSG_BUSY,
	NXIP_MSG_BUSY_FLASH,
	NXIP_MSG_BAD_SFDP,
	NXIP_MSG_SAD_SFDP,
	NXIP_MSG_NOTSUP_SFDP,
	NXIP_MSG_SADSUP_SFDP,
	NXIP_MSG_INITSEQ_FAIL,
	NXIP_MSG_SFDP_BADSIZE,
	NXIP_MSG_MAX
};

enum flash_bflb_bank {
	BANK1 = 1,
	BANK2 = 2,
	BANKMAX
};

enum flash_bflb_pad {
	PAD1 = 1,
	PAD2 = 2,
	PAD3 = 3,
	PADMAX
};

enum flash_bflb_bus_mode {
	BUS_NIO = 0,
	BUS_DO = 1,
	BUS_QO = 2,
	BUS_DIO = 3,
	BUS_QIO = 4,
};

struct flash_bflb_device_commands {
	/* Auto = commands used with instruction bus (= XIP / memory mapped access) */
	uint8_t					auto_read;
	uint8_t					auto_read_dmycy;
	uint8_t					auto_write;
	uint8_t					auto_write_dmycy;
	/* Manual = commands used with system bus (= CPU command access) */
	uint8_t					manual_read;
	/* Left so fast read can be used */
	uint8_t					manual_read_dmycy;
	/* Up to 4 read/write registers commands */
	uint8_t					read_reg[4];
	uint8_t					write_reg[4];
	uint8_t					contread_on;
	uint8_t					contread_off;
	uint8_t					burstwrap;
	uint8_t					burstwrap_dmycy;
	uint8_t					burstwrap_on_data;
	uint8_t					burstwrap_off_data;
	uint8_t					write_enable;
	uint8_t					page_program;
	uint8_t					sector_erase;
	uint8_t					block_erase;
	uint8_t					enter_32bits_addr;
	uint8_t					exit_32bits_addr;
	uint8_t					enter_qpi;
	uint8_t					exit_qpi;
	/* Extended ops */
	uint8_t					reset_enable;
	uint8_t					reset;
	uint8_t					powerdown;
	uint8_t					release_powerdown;
};

/* Register access
 * Index indicates which register command to use
 * Bit indicates position of the bit in the register of size len
 * Len indicates the length to interact with: Some flash require reading each register
 * separately, but write all at once (MX25Lxxx45G for example).
 */
struct flash_bflb_device_registers {
	uint8_t					write_enable_index;
	uint8_t					write_enable_bit;
	uint8_t					write_enable_read_len;
	uint8_t					quad_enable_index;
	uint8_t					quad_enable_bit;
	uint8_t					quad_enable_read_len;
	uint8_t					quad_enable_write_len;
	uint8_t					busy_index;
	uint8_t					busy_bit;
	uint8_t					busy_read_len;
};

struct flash_bflb_pad_cfg {
	enum flash_bflb_pad			id;
	bool					is_external;
	uint8_t					read_delay;
	bool					clock_invert;
	bool					rx_clock_invert;
	uint8_t					dod;
	uint8_t					did;
	uint8_t					csd;
	uint8_t					clkd;
	uint8_t					oed;
};

struct flash_bflb_device_cfg {
	/* SPI modes to use */
	uint8_t					auto_spi_mode;
	uint8_t					manual_spi_mode;
	bool					use_qpi;
	struct flash_bflb_device_commands	cmd;
	struct flash_bflb_device_registers	reg;
	struct flash_bflb_pad_cfg		pad;
	uint32_t				size;
	uint32_t				page_size;
	uint32_t				sector_size;
	uint32_t				block_size;
	uint8_t					jedec_id[3];
	uint32_t				*init_seq;
	size_t					init_seq_len;
	uint8_t					*quirk_bytes_write;
	size_t					quirk_bytes_write_len;
	uint8_t					*quirk_bytes_read;
	size_t					quirk_bytes_read_len;
	bool					use_sfdp;
	struct flash_pages_layout		layout;
	struct flash_parameters			parameters;
};

struct flash_bflb_bank_data;

struct flash_bflb_controller_data {
	bool					override_bank1;
	bool					addr_32bits;
	struct flash_bflb_bank_data		*banks[2];
	uint8_t					bank_cnt;
	uint8_t					clk_divider;
};

struct flash_bflb_bank_data {
	uintptr_t				reg;
	enum flash_bflb_bank			bank;
	struct flash_bflb_controller_data	*controller;
	struct flash_bflb_device_cfg		cfg;
	uintptr_t				xip_base;
	enum flash_bflb_nxip_message_id		nxip_message;
	uint32_t				nxip_message_args[3];
	uint32_t				last_flash_offset;
};

struct flash_bflb_bank_config {
	const struct pinctrl_dev_config		*pincfg;
};
