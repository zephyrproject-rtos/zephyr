/* dw_i2c.h - header for Design Ware I2C operations */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_I2C_I2C_DW_H_
#define ZEPHYR_DRIVERS_I2C_I2C_DW_H_

#include <zephyr/drivers/i2c.h>
#include <stdbool.h>

#define DT_DRV_COMPAT snps_designware_i2c

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
BUILD_ASSERT(IS_ENABLED(CONFIG_PCIE), "DW I2C in DT needs CONFIG_PCIE");
#include <zephyr/drivers/pcie/pcie.h>
#endif

#if defined(CONFIG_RESET)
#include <zephyr/drivers/reset.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_DW_MAGIC_KEY 0x44570140

typedef void (*i2c_isr_cb_t)(const struct device *port);

#define IC_ACTIVITY   (1 << 0)
#define IC_ENABLE_BIT (1 << 0)

/* dev->state values from IC_DATA_CMD Data transfer mode settings (bit 8) */
#define I2C_DW_STATE_READY (0)
#define I2C_DW_CMD_SEND    (1 << 0)
#define I2C_DW_CMD_RECV    (1 << 1)
#define I2C_DW_CMD_ERROR   (1 << 2)
#define I2C_DW_BUSY        (1 << 3)
#define I2C_DW_TX_ABRT     (1 << 4)
#define I2C_DW_NACK        (1 << 5)
#define I2C_DW_SCL_STUCK   (1 << 6)
#define I2C_DW_SDA_STUCK   (1 << 7)

#define I2C_DW_ERR_MASK (I2C_DW_CMD_ERROR | I2C_DW_SCL_STUCK | I2C_DW_SDA_STUCK | I2C_DW_NACK)

#define I2C_DW_STUCK_ERR_MASK (I2C_DW_SCL_STUCK | I2C_DW_SDA_STUCK)

#ifdef CONFIG_I2C_DW_EXTENDED_SUPPORT
#define DW_ENABLE_TX_INT_I2C_MASTER                                                                \
	(DW_INTR_STAT_TX_OVER | DW_INTR_STAT_TX_EMPTY | DW_INTR_STAT_TX_ABRT |                     \
	 DW_INTR_STAT_STOP_DET | DW_INTR_STAT_SCL_STUCK_LOW)
#else
#define DW_ENABLE_TX_INT_I2C_MASTER                                                                \
	(DW_INTR_STAT_TX_OVER | DW_INTR_STAT_TX_EMPTY | DW_INTR_STAT_TX_ABRT |                     \
	 DW_INTR_STAT_STOP_DET)
#endif
#define DW_ENABLE_RX_INT_I2C_MASTER                                                                \
	(DW_INTR_STAT_RX_UNDER | DW_INTR_STAT_RX_OVER | DW_INTR_STAT_RX_FULL |                     \
	 DW_INTR_STAT_STOP_DET)

#define DW_ENABLE_TX_INT_I2C_SLAVE                                                                 \
	(DW_INTR_STAT_RD_REQ | DW_INTR_STAT_TX_ABRT | DW_INTR_STAT_STOP_DET)
#define DW_ENABLE_RX_INT_I2C_SLAVE (DW_INTR_STAT_RX_FULL | DW_INTR_STAT_STOP_DET)

#define DW_DISABLE_ALL_I2C_INT 0x00000000

/* IC_CON Low count and high count default values */
/* TODO verify values for high speed */
#define I2C_STD_HCNT (CONFIG_I2C_DW_CLOCK_SPEED * 4)
#define I2C_STD_LCNT (CONFIG_I2C_DW_CLOCK_SPEED * 5)
#define I2C_FS_HCNT  ((CONFIG_I2C_DW_CLOCK_SPEED * 6) / 8)
#define I2C_FS_LCNT  ((CONFIG_I2C_DW_CLOCK_SPEED * 7) / 8)
#define I2C_FSP_HCNT ((CONFIG_I2C_DW_CLOCK_SPEED * 2) / 8)
#define I2C_FSP_LCNT ((CONFIG_I2C_DW_CLOCK_SPEED * 2) / 8)
#define I2C_HS_HCNT  ((CONFIG_I2C_DW_CLOCK_SPEED * 6) / 8)
#define I2C_HS_LCNT  ((CONFIG_I2C_DW_CLOCK_SPEED * 7) / 8)

/*
 * DesignWare speed values don't directly translate from the Zephyr speed
 * selections in include/i2c.h so here we do a little translation
 */
#define I2C_DW_SPEED_STANDARD  0x1
#define I2C_DW_SPEED_FAST      0x2
#define I2C_DW_SPEED_FAST_PLUS 0x2
#define I2C_DW_SPEED_HIGH      0x3

/*
 * These values have been randomly selected.  It would be good to test different
 * watermark levels for performance capabilities
 */
#define I2C_DW_TX_WATERMARK 2
#define I2C_DW_RX_WATERMARK 7

struct i2c_dw_rom_config {
	DEVICE_MMIO_ROM;
	i2c_isr_cb_t config_func;
	uint32_t bitrate;
	uint32_t irqnumber;
	int16_t lcnt_offset;
	int16_t hcnt_offset;

#if defined(CONFIG_PINCTRL)
	const struct pinctrl_dev_config *pcfg;
#endif
#if defined(CONFIG_RESET)
	const struct reset_dt_spec reset;
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
	struct pcie_dev *pcie;
#endif /* I2C_DW_PCIE_ENABLED */

#ifdef CONFIG_I2C_DW_LPSS_DMA
	const struct device *dma_dev;
#endif

#ifdef CONFIG_I2C_DW_EXTENDED_SUPPORT
	uint32_t sda_timeout_value;
	uint32_t scl_timeout_value;
#endif
};

struct i2c_dw_dev_config {
	DEVICE_MMIO_RAM;
	struct k_sem device_sync_sem;
	struct k_sem bus_sem;
	uint32_t app_config;

	volatile uint8_t *xfr_buf;
	volatile uint32_t xfr_len;
	volatile uint32_t rx_pending;

	uint16_t hcnt;
	uint16_t lcnt;

	volatile uint8_t state; /* last direction of transfer */
	uint32_t request_bytes;
	uint8_t xfr_flags;
	bool support_hs_mode;
	bool read_in_progress;
#ifdef CONFIG_I2C_DW_LPSS_DMA
	uintptr_t phy_addr;
	uintptr_t base_addr;
	/* For dma transfer */
	bool xfr_status;
#endif

	struct i2c_target_config *slave_cfg;

	i2c_api_recover_bus_t recover_bus_cb;
	struct device *recover_bus_dev;
#if CONFIG_I2C_ALLOW_NO_STOP_TRANSACTIONS
	bool need_setup;
#endif
};

#define Z_REG_READ(__sz)  sys_read##__sz
#define Z_REG_WRITE(__sz) sys_write##__sz
#define Z_REG_SET_BIT     sys_set_bit
#define Z_REG_CLEAR_BIT   sys_clear_bit
#define Z_REG_TEST_BIT    sys_test_bit

#define DEFINE_MM_REG_READ(__reg, __off, __sz)                                                     \
	static inline uint32_t read_##__reg(uint32_t addr)                                         \
	{                                                                                          \
		return Z_REG_READ(__sz)(addr + __off);                                             \
	}
#define DEFINE_MM_REG_WRITE(__reg, __off, __sz)                                                    \
	static inline void write_##__reg(uint32_t data, uint32_t addr)                             \
	{                                                                                          \
		Z_REG_WRITE(__sz)(data, addr + __off);                                             \
	}

#define DEFINE_SET_BIT_OP(__reg_bit, __reg_off, __bit)                                             \
	static inline void set_bit_##__reg_bit(uint32_t addr)                                      \
	{                                                                                          \
		Z_REG_SET_BIT(addr + __reg_off, __bit);                                            \
	}

#define DEFINE_CLEAR_BIT_OP(__reg_bit, __reg_off, __bit)                                           \
	static inline void clear_bit_##__reg_bit(uint32_t addr)                                    \
	{                                                                                          \
		Z_REG_CLEAR_BIT(addr + __reg_off, __bit);                                          \
	}

#define DEFINE_TEST_BIT_OP(__reg_bit, __reg_off, __bit)                                            \
	static inline int test_bit_##__reg_bit(uint32_t addr)                                      \
	{                                                                                          \
		return Z_REG_TEST_BIT(addr + __reg_off, __bit);                                    \
	}

void i2c_dw_register_recover_bus_cb(const struct device *dw_i2c_dev,
				    i2c_api_recover_bus_t recover_bus_cb,
				    const struct device *wrapper_dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2C_I2C_DW_H_ */
