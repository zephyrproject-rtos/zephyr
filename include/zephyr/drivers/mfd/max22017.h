/*
 * Copyright (c) 2024 Analog Devices Inc.
 * Copyright (c) 2024 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_MAX22017_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_MAX22017_H_

#include <zephyr/device.h>

#define MAX22017_LDAC_TOGGLE_TIME 200
#define MAX22017_MAX_CHANNEL      2
#define MAX22017_CRC_POLY         0x8c /* reversed 0x31 poly for crc8-maxim */

#define MAX22017_GEN_ID_OFF     0x00
#define MAX22017_GEN_ID_PROD_ID GENMASK(15, 8)
#define MAX22017_GEN_ID_REV_ID  GENMASK(7, 0)

#define MAX22017_GEN_SERIAL_MSB_OFF        0x01
#define MAX22017_GEN_SERIAL_MSB_SERIAL_MSB GENMASK(15, 0)

#define MAX22017_GEN_SERIAL_LSB_OFF        0x02
#define MAX22017_GEN_SERIAL_LSB_SERIAL_LSB GENMASK(15, 0)

#define MAX22017_GEN_CNFG_OFF                0x03
#define MAX22017_GEN_CNFG_OPENWIRE_DTCT_CNFG GENMASK(15, 14)
#define MAX22017_GEN_CNFG_TMOUT_SEL          GENMASK(13, 10)
#define MAX22017_GEN_CNFG_TMOUT_CNFG         BIT(9)
#define MAX22017_GEN_CNFG_TMOUT_EN           BIT(8)
#define MAX22017_GEN_CNFG_THSHDN_CNFG        GENMASK(7, 6)
#define MAX22017_GEN_CNFG_OVC_SHDN_CNFG      GENMASK(5, 4)
#define MAX22017_GEN_CNFG_OVC_CNFG           GENMASK(3, 2)
#define MAX22017_GEN_CNFG_CRC_EN             BIT(1)
#define MAX22017_GEN_CNFG_DACREF_SEL         BIT(0)

#define MAX22017_GEN_GPIO_CTRL_OFF      0x04
#define MAX22017_GEN_GPIO_CTRL_GPIO_EN  GENMASK(13, 8)
#define MAX22017_GEN_GPIO_CTRL_GPIO_DIR GENMASK(5, 0)

#define MAX22017_GEN_GPIO_DATA_OFF      0x05
#define MAX22017_GEN_GPIO_DATA_GPO_DATA GENMASK(13, 8)
#define MAX22017_GEN_GPIO_DATA_GPI_DATA GENMASK(5, 0)

#define MAX22017_GEN_GPI_INT_OFF              0x06
#define MAX22017_GEN_GPI_INT_GPI_POS_EDGE_INT GENMASK(13, 8)
#define MAX22017_GEN_GPI_INT_GPI_NEG_EDGE_INT GENMASK(5, 0)

#define MAX22017_GEN_GPI_INT_STA_OFF                  0x07
#define MAX22017_GEN_GPI_INT_STA_GPI_POS_EDGE_INT_STA GENMASK(13, 8)
#define MAX22017_GEN_GPI_INT_STA_GPI_NEG_EDGE_INT_STA GENMASK(5, 0)

#define MAX22017_GEN_INT_OFF               0x08
#define MAX22017_GEN_INT_FAIL_INT          BIT(15)
#define MAX22017_GEN_INT_CONV_OVF_INT      GENMASK(13, 12)
#define MAX22017_GEN_INT_OPENWIRE_DTCT_INT GENMASK(11, 10)
#define MAX22017_GEN_INT_HVDD_INT          BIT(9)
#define MAX22017_GEN_INT_TMOUT_INT         BIT(8)
#define MAX22017_GEN_INT_THSHDN_INT        GENMASK(7, 6)
#define MAX22017_GEN_INT_THWRNG_INT        GENMASK(5, 4)
#define MAX22017_GEN_INT_OVC_INT           GENMASK(3, 2)
#define MAX22017_GEN_INT_CRC_INT           BIT(1)
#define MAX22017_GEN_INT_GPI_INT           BIT(0)

#define MAX22017_GEN_INTEN_OFF                 0x09
#define MAX22017_GEN_INTEN_CONV_OVF_INTEN      GENMASK(13, 12)
#define MAX22017_GEN_INTEN_OPENWIRE_DTCT_INTEN GENMASK(11, 10)
#define MAX22017_GEN_INTEN_HVDD_INTEN          BIT(9)
#define MAX22017_GEN_INTEN_TMOUT_INTEN         BIT(8)
#define MAX22017_GEN_INTEN_THSHDN_INTEN        GENMASK(7, 6)
#define MAX22017_GEN_INTEN_THWRNG_INTEN        GENMASK(5, 4)
#define MAX22017_GEN_INTEN_OVC_INTEN           GENMASK(3, 2)
#define MAX22017_GEN_INTEN_CRC_INTEN           BIT(1)
#define MAX22017_GEN_INTEN_GPI_INTEN           BIT(0)

#define MAX22017_GEN_RST_CTRL_OFF             0x0A
#define MAX22017_GEN_RST_CTRL_AO_COEFF_RELOAD GENMASK(15, 14)
#define MAX22017_GEN_RST_CTRL_GEN_RST         BIT(9)

#define MAX22017_AO_CMD_OFF        0x40
#define MAX22017_AO_CMD_AO_LD_CTRL GENMASK(15, 14)

#define MAX22017_AO_STA_OFF      0x41
#define MAX22017_AO_STA_BUSY_STA GENMASK(15, 14)
#define MAX22017_AO_STA_SLEW_STA GENMASK(13, 12)
#define MAX22017_AO_STA_FAIL_STA BIT(0)

#define MAX22017_AO_CNFG_OFF                  0x42
#define MAX22017_AO_CNFG_AO_LD_CNFG           GENMASK(15, 14)
#define MAX22017_AO_CNFG_AO_CM_SENSE          GENMASK(13, 12)
#define MAX22017_AO_CNFG_AO_UNI               GENMASK(11, 10)
#define MAX22017_AO_CNFG_AO_MODE              GENMASK(9, 8)
#define MAX22017_AO_CNFG_AO_OPENWIRE_DTCT_LMT GENMASK(5, 4)
#define MAX22017_AO_CNFG_AI_EN                GENMASK(3, 2)
#define MAX22017_AO_CNFG_AO_EN                GENMASK(1, 0)

#define MAX22017_AO_SLEW_RATE_CHn_OFF(n)                (0x43 + n)
#define MAX22017_AO_SLEW_RATE_CHn_AO_SR_EN_CHn          BIT(5)
#define MAX22017_AO_SLEW_RATE_CHn_AO_SR_SEL_CHn         BIT(4)
#define MAX22017_AO_SLEW_RATE_CHn_AO_SR_STEP_SIZE_CHn   GENMASK(3, 2)
#define MAX22017_AO_SLEW_RATE_CHn_AO_SR_UPDATE_RATE_CHn GENMASK(1, 0)

#define MAX22017_AO_DATA_CHn_OFF(n)     (0x45 + n)
#define MAX22017_AO_DATA_CHn_AO_DATA_CH GENMASK(15, 0)

#define MAX22017_AO_OFFSET_CORR_CHn_OFF(n)       (0x47 + (2 * n))
#define MAX22017_AO_OFFSET_CORR_CHn_AO_OFFSET_CH GENMASK(15, 0)

#define MAX22017_AO_GAIN_CORR_CHn_OFF(n)     (0x48 + (2 * n))
#define MAX22017_AO_GAIN_CORR_CHn_AO_GAIN_CH GENMASK(15, 0)

#define MAX22017_SPI_TRANS_ADDR    GENMASK(7, 1)
#define MAX22017_SPI_TRANS_DIR     BIT(0)
#define MAX22017_SPI_TRANS_PAYLOAD GENMASK(15, 0)

/**
 * @defgroup mdf_interface_max22017 MFD MAX22017 interface
 * @ingroup mfd_interfaces
 * @{
 */

/**
 * @brief Read register from max22017
 *
 * @param dev max22017 mfd device
 * @param addr register address to read from
 * @param value pointer to buffer for received data
 * @retval 0 If successful
 * @retval -errno In case of any error (see spi_transceive_dt())
 */
int max22017_reg_read(const struct device *dev, uint8_t addr, uint16_t *value);

/**
 * @brief Write register to max22017
 *
 * @param dev max22017 mfd device
 * @param addr register address to write to
 * @param value content to write
 * @retval 0 If successful
 * @retval -errno In case of any error (see spi_write_dt())
 */
int max22017_reg_write(const struct device *dev, uint8_t addr, uint16_t value);

/**
 * @}
 */

struct max22017_data {
	const struct device *dev;
	struct k_mutex lock;
	struct k_work int_work;
	struct gpio_callback callback_int;
	bool crc_enabled;
#ifdef CONFIG_GPIO_MAX22017
	sys_slist_t callbacks_gpi;
#endif
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_MAX22017_H_ */
