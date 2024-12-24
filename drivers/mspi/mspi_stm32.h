/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MSPI_STM32_H_
#define ZEPHYR_DRIVERS_MSPI_STM32_H_

/* Macro to check if any xspi device has a domain clock or more */
#define MSPI_STM32_DOMAIN_CLOCK_INST_SUPPORT(inst) DT_CLOCKS_HAS_IDX(DT_INST_PARENT(inst), 1) ||
#define MSPI_STM32_INST_DEV_DOMAIN_CLOCK_SUPPORT                                                   \
	(DT_INST_FOREACH_STATUS_OKAY(MSPI_STM32_DOMAIN_CLOCK_INST_SUPPORT) 0)

/* This symbol takes the value 1 if device instance has a domain clock in its dts */
#if MSPI_STM32_INST_DEV_DOMAIN_CLOCK_SUPPORT
#define MSPI_STM32_DOMAIN_CLOCK_SUPPORT 1
#else
#define MSPI_STM32_DOMAIN_CLOCK_SUPPORT 0
#endif

#define MSPI_STM32_FIFO_THRESHOLD 4U

#define MSPI_MAX_FREQ      250000000
#define MSPI_MAX_DEVICE    2
#define MSPI_TIMEOUT_US    1000000
#define STM32_MSPI_INST_ID 0

/* Valid range is [0, 255] */
#define MSPI_STM32_CLOCK_PRESCALER_MIN                0U
#define MSPI_STM32_CLOCK_PRESCALER_MAX                255U
#define MSPI_STM32_CLOCK_COMPUTE(bus_freq, prescaler) ((bus_freq) / ((prescaler) + 1U))

/* Max Time value during reset or erase operation */
#define MSPI_STM32_RESET_MAX_TIME              100U
#define MSPI_STM32_BULK_ERASE_MAX_TIME         460000U
#define MSPI_STM32_SECTOR_ERASE_MAX_TIME       1000U
#define MSPI_STM32_SUBSECTOR_4K_ERASE_MAX_TIME 400U
#define MSPI_STM32_WRITE_REG_MAX_TIME          40U
#define MSPI_STM32_MAX_FREQ                    48000000
#define MSPI_MAX_DEVICE                        2
/* used as default value for DTS writeoc */
#define MSPI_STM32_WRITEOC_NONE                0xFF

#define MSPI_STM32_CMD_WRSR         0x01 /* Write status register */
#define MSPI_STM32_CMD_RDSR         0x05 /* Read status register */
#define MSPI_STM32_CMD_READ         0x03 /* Read data */
#define MSPI_STM32_CMD_READ_FAST    0x0B /* Read data */
#define MSPI_STM32_CMD_PP           0x02 /* Page program */
#define MSPI_STM32_CMD_READ_4B      0x13 /* Read data 4 Byte Address */
#define MSPI_STM32_CMD_READ_FAST_4B 0x0C /* Fast Read 4 Byte Address */
#define MSPI_STM32_CMD_PP_4B        0x12 /* Page Program 4 Byte Address */
#define MSPI_STM32_CMD_WREN         0x06 /* Write enable */
#define MSPI_STM32_CMD_RDPD         0xAB /* Release from Deep Power Down */
#define MSPI_STM32_CMD_RD_CFGREG2   0x71 /* Read config register 2 */
#define MSPI_STM32_CMD_WR_CFGREG2   0x72 /* Write config register 2 */

#define MSPI_STM32_OCMD_RDSR       0x05FA /* Octal Read status register */
#define MSPI_STM32_OCMD_RD         0xEC13 /* Octal IO read command */
#define MSPI_STM32_OCMD_PAGE_PRG   0x12ED /* Octal Page Prog */
#define MSPI_STM32_OCMD_WREN       0x06F9 /* Octal Write enable */
#define MSPI_STM32_OCMD_DTR_RD     0xEE11 /* Octal IO DTR read command */
#define MSPI_STM32_OCMD_WR_CFGREG2 0x728D /* Octal Write configuration Register2 */
#define MSPI_STM32_OCMD_RD_CFGREG2 0x718E /* Octal Read configuration Register2 */

/* Value to poll the status bus register corresponding to the NOR_MX_STATUS_xxx */
#define MSPI_STM32_STATUS_MEM_RDY    1
#define MSPI_STM32_STATUS_MEM_WREN   2
#define MSPI_STM32_STATUS_MEM_ERASED 3

/* Flash Auto-polling values */
#define MSPI_STM32_WREN_MATCH 0x02
#define MSPI_STM32_WREN_MASK  0x02

#define MSPI_STM32_WEL_MATCH 0x00
#define MSPI_STM32_WEL_MASK  0x02

#define MSPI_STM32_MEM_RDY_MATCH 0x00
#define MSPI_STM32_MEM_RDY_MASK  0x01

#define MSPI_STM32_AUTO_POLLING_INTERVAL 0x10
/* Flash Dummy Cycles values */
#define MSPI_STM32_DUMMY_RD              8U
#define MSPI_STM32_DUMMY_RD_OCTAL        6U
#define MSPI_STM32_DUMMY_RD_OCTAL_DTR    6U
#define MSPI_STM32_DUMMY_REG_OCTAL       4U
#define MSPI_STM32_DUMMY_REG_OCTAL_DTR   5U

/* Memory registers address */
#define MSPI_STM32_REG2_ADDR1             0x0000000
#define MSPI_STM32_CR2_STR_OPI_EN         0x01
#define MSPI_STM32_CR2_DTR_OPI_EN         0x02
#define MSPI_STM32_REG2_ADDR3             0x00000300
#define MSPI_STM32_CR2_DUMMY_CYCLES_66MHZ 0x07

#if MSPI_STM32_USE_DMA
/* Lookup table to set dma priority from the DTS */
static const uint32_t table_priority[] = {
	DMA_LOW_PRIORITY_LOW_WEIGHT,
	DMA_LOW_PRIORITY_MID_WEIGHT,
	DMA_LOW_PRIORITY_HIGH_WEIGHT,
	DMA_HIGH_PRIORITY,
};

/* Lookup table to set dma channel direction from the DTS */
static const uint32_t table_direction[] = {
	DMA_MEMORY_TO_MEMORY,
	DMA_MEMORY_TO_PERIPH,
	DMA_PERIPH_TO_MEMORY,
};

struct stream {
	DMA_TypeDef *reg;
	const struct device *dev;
	uint32_t channel;
	struct dma_config cfg;
	uint8_t priority;
	bool src_addr_increment;
	bool dst_addr_increment;
};
#endif /* MSPI_STM32_USE_DMA */

typedef void (*irq_config_func_t)(void);

enum mspi_access_mode {
	MSPI_ACCESS_ASYNC = 1,
	MSPI_ACCESS_SYNC = 2,
	MSPI_ACCESS_DMA = 3
};

struct mspi_context {
	const struct mspi_dev_id *owner;

	struct mspi_xfer xfer;

	int packets_left;
	int packets_done;

	mspi_callback_handler_t callback;
	struct mspi_callback_context *callback_ctx;
	bool asynchronous;

	struct k_sem lock;
};

struct mspi_stm32_conf {
	const struct stm32_pclken *pclken;
	size_t pclk_len;
	irq_config_func_t irq_config;
	uint32_t reg_base;
	uint32_t reg_size;

	struct mspi_cfg mspicfg;
	const struct pinctrl_dev_config *pcfg;
#if MSPI_STM32_RESET_GPIO
	const struct gpio_dt_spec reset;
#endif /* MSPI_STM32_RESET_GPIO */
};

/* mspi data includes the controller specific config variable */
struct mspi_stm32_data {
	/* XSPI handle is modifiable ; so part of data struct */
	XSPI_HandleTypeDef hmspi;
	struct mspi_dev_id *dev_id;

	/* controller access mutex */
	struct k_mutex lock;
	struct k_sem sync;

	struct mspi_dev_cfg dev_cfg;
	struct mspi_xip_cfg xip_cfg;
	struct mspi_scramble_cfg scramble_cfg;
	/* Timing configurations */
	struct mspi_timing_cfg timing_cfg;

	mspi_callback_handler_t cbs[MSPI_BUS_EVENT_MAX];
	struct mspi_callback_context *cb_ctxs[MSPI_BUS_EVENT_MAX];

	struct mspi_context ctx;
	int cmd_status;
};

#endif /* ZEPHYR_DRIVERS_MSPI_STM32_H_ */
