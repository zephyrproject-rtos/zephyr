/*
 * Copyright (c) 2023 Sendrato
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_NFC_NT3H2X11_H_
#define ZEPHYR_INCLUDE_DRIVERS_NFC_NT3H2X11_H_

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>

/* Memory Layout */
#ifdef CONFIG_NFC_NT3H2X11_2K
#define NT3H2X11_LAST_NDEF_BLOCK_MAX (0x7Fu)
#else
#define NT3H2X11_LAST_NDEF_BLOCK_MAX (0x37u)
#endif

/* Blocks and I2C addresses */
#define NT3H2X11_BLK_DEVICE        (0x00u)
#define NT3H2X11_BLK_SECTOR0_START (0x01u)
#define NT3H2X11_BLK_LOCK_AUTH     (0x38u)
#define NT3H2X11_BLK_ACCESS_PWD    (0x39u)
#define NT3H2X11_BLK_CONFIG        (0x3Au)
/* SECTOR1 only available if CONFIG_NFC_NT3H2X11_2K */
#define NT3H2X11_BLK_SECTOR1_START (0x40u)
#define NT3H2X11_BLK_SRAM_START    (0xF8u)
#define NT3H2X11_BLK_SESSION       (0xFEu)

/* Memory-sizes in bytes */
#define NT3H2X11_BYTES_BLK     ((uint16_t)(16u))
#define NT3H2X11_BYTES_SECTOR0 ((uint16_t)((NT3H2X11_BYTES_BLK * 55u) + 8u))
#define NT3H2X11_BYTES_SECTOR1 ((uint16_t)((NT3H2X11_BYTES_BLK * 64u) + 0u))
#define NT3H2X11_BYTES_SRAM    ((uint16_t)((NT3H2X11_BYTES_BLK * 4u) + 0u))

/*
 * Registers for Device blocks
 * ==> NT3H2X11_BLK_DEVICE
 */

#define NT3H2X11_REG_ADDR   (0x00u)
#define NT3H2X11_BYTES_ADDR (1u)

#define NT3H2X11_REG_SERIAL   (0x01u)
#define NT3H2X11_BYTES_SERIAL (6u)

#define NT3H2X11_REG_INTERNAL   (0x07u)
#define NT3H2X11_BYTES_INTERNAL (3u)

#define NT3H2X11_REG_STATIC_LOCK   (0x0Au)
#define NT3H2X11_BYTES_STATIC_LOCK (2u)

#define NT3H2X11_REG_CC   (0x0Cu)
#define NT3H2X11_BYTES_CC (4u)

/*
 * Registers for Password and Access blocks
 * ==> NT3H2X11_BLK_LOCK_AUTH
 * ==> NT3H2X11_BLK_ACCESS_PWD
 */

#define NT3H2X11_REG_DYNAMIC_LOCK   (0x08u)
#define NT3H2X11_BYTES_DYNAMIC_LOCK (3u)

#define NT3H2X11_REG_AUTH0         (0x0Fu)
#define NT3H2X11_BYTES_AUTH0       (1u)
/* Disable authentication value */
#define NT3H2X11_MSK_AUTH0_DISABLE (0xFFu)

#define NT3H2X11_REG_ACCESS   (0x00u)
#define NT3H2X11_BYTES_ACCESS (1u)
/* TODO: ACCESS-bits */

#define NT3H2X11_REG_PWD   (0x04u)
#define NT3H2X11_BYTES_PWD (4u)

#define NT3H2X11_REG_PACK   (0x08u)
#define NT3H2X11_BYTES_PACK (2u)

#define NT3H2X11_REG_PT_I2C   (0x0Cu)
#define NT3H2X11_BYTES_PT_I2C (1u)
/* TODO: PT_I2C-bits */

/*
 * Registers for CONFIG and SESSION blocks
 * ==> NT3H2X11_BLK_CONFIG
 * ==> NT3H2X11_BLK_SESSION
 */

/* CONFIG and SESSION */
#define NT3H2X11_REG_NC_REG                    (0x00u)
#define NT3H2X11_MSK_NC_REG_RST_ON_OFF         ((uint8_t)0x80u)
#define NT3H2X11_MSK_NC_REG_PTHRU_ON_OFF       ((uint8_t)0x40u)
#define NT3H2X11_MSK_NC_REG_FD_OFF             ((uint8_t)0x30u)
#define NT3H2X11_MSK_NC_REG_FD_ON              ((uint8_t)0x0Cu)
#define NT3H2X11_MSK_NC_REG_SRAM_MIRROR_ON_OFF ((uint8_t)0x02u)
#define NT3H2X11_MSK_NC_REG_TRANSFER_DIR       ((uint8_t)0x01u)

/* CONFIG and SESSION */
#define NT3H2X11_REG_LAST_NDEF_BLOCK   (0x01u)
#define NT3H2X11_REG_SRAM_MIRROR_BLOCK (0x02u)
#define NT3H2X11_REG_WDT_LS            (0x03u)
#define NT3H2X11_REG_WDT_MS            (0x04u)

/* CONFIG and SESSION */
#define NT3H2X11_REG_I2C_CLOCK_STR           (0x05u)
#define NT3H2X11_MSK_I2C_CLOCK_STR_NEG_AUTH  ((uint8_t)0x02u) /* session only */
#define NT3H2X11_MSK_I2C_CLOCK_STR_CLOCK_STR ((uint8_t)0x01u)

/* SESSION ONLY */
#define NT3H2X11_REG_NS_REG                  (0x06u)
#define NT3H2X11_MSK_NS_REG_NDEF_DATA_READ   ((uint8_t)0x80u)
#define NT3H2X11_MSK_NS_REG_I2C_LOCKED       ((uint8_t)0x40u)
#define NT3H2X11_MSK_NS_REG_RF_LOCKED        ((uint8_t)0x20u)
#define NT3H2X11_MSK_NS_REG_SRAM_I2C_READY   ((uint8_t)0x10u)
#define NT3H2X11_MSK_NS_REG_SRAM_RF_READY    ((uint8_t)0x08u)
#define NT3H2X11_MSK_NS_REG_EEPROM_WR_ERR    ((uint8_t)0x04u)
#define NT3H2X11_MSK_NS_REG_EEPROM_WR_BUSY   ((uint8_t)0x02u)
#define NT3H2X11_MSK_NS_REG_RF_FIELD_PRESENT ((uint8_t)0x01u)

/* CONFIG ONLY */
#define NT3H2X11_REG_CFG_REG_LOCK          (0x06u)
#define NT3H2X11_MSK_CFG_REG_LOCK_LOCK_I2C ((uint8_t)0x02u)
#define NT3H2X11_MSK_CFG_REG_LOCK_LOCK_NFC ((uint8_t)0x01u)

enum nt3h2x11_fd_off {
	NT3H2X11_FD_OFF_RF_OFF = (0x0u << 4),
	NT3H2X11_FD_OFF_RF_OFF_OR_HALT = (0x1u << 4),
	NT3H2X11_FD_OFF_RF_OFF_OR_LAST_NDEF_READ = (0x2u << 4),
	NT3H2X11_FD_OFF_RF_OFF_OR_LAST_DATA_RW = (0x3u << 4)
};

enum nt3h2x11_fd_on {
	NT3H2X11_FD_ON_RF_ON = (0x0u << 2),
	NT3H2X11_FD_ON_RF_FIRST_VALID = (0x1u << 2),
	NT3H2X11_FD_ON_TAG_SELECTION = (0x2u << 2),
	NT3H2X11_FD_ON_RF_DATA_READY = (0x3u << 2)
};

enum nt3h2x11_transfer_dir {
	NT3H2X11_TRANSFER_DIR_I2C_TO_RF = (0x0u << 0),
	NT3H2X11_TRANSFER_DIR_RF_TO_I2C = (0x1u << 0)
};

enum nt3h2x11_csreg {
	NT3H2X11_CONFIG = NT3H2X11_BLK_CONFIG,
	NT3H2X11_SESSION = NT3H2X11_BLK_SESSION
};

struct __packed nt3h2x11_cc {
	uint8_t magic;
	uint8_t version;
	uint8_t size;
	uint8_t access;
};

struct __packed nt3h2x11_device {
	uint8_t addr[NT3H2X11_BYTES_ADDR];
	uint8_t serial[NT3H2X11_BYTES_SERIAL];
	uint8_t internal[NT3H2X11_BYTES_INTERNAL];
	uint8_t static_lock[NT3H2X11_BYTES_STATIC_LOCK];
	struct nt3h2x11_cc cc;
};

struct __packed nt3h2x11_cfg_auth {
	uint8_t dyn_lock[NT3H2X11_BYTES_DYNAMIC_LOCK];
	uint8_t auth0[NT3H2X11_BYTES_AUTH0];
	uint8_t access[NT3H2X11_BYTES_ACCESS];
	uint8_t pwd[NT3H2X11_BYTES_PWD];
	uint8_t pack[NT3H2X11_BYTES_PACK];
	uint8_t pt_i2c[NT3H2X11_BYTES_PT_I2C];
};

/* DEFAULT VALUES */
#define NT3H2X11_DEFAULT_CC_MAGIC   (0xE1u)
#define NT3H2X11_DEFAULT_CC_VERSION (0x10u)
#define NT3H2X11_DEFAULT_CC_SIZE    (0xE9u)
#define NT3H2X11_DEFAULT_CC_ACCESS  (0x00u)

#define NT3H2X11_DEFAULT_DEVICE_ADDR                                                               \
	{                                                                                          \
		0x04u                                                                              \
	}
#define NT3H2X11_DEFAULT_DEVICE_SERIAL                                                             \
	{                                                                                          \
		0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u                                           \
	}
#define NT3H2X11_DEFAULT_DEVICE_INTERNAL                                                           \
	{                                                                                          \
		0x00u, 0x00u, 0x00u                                                                \
	}
#define NT3H2X11_DEFAULT_DEVICE_STATIC_LOCK                                                        \
	{                                                                                          \
		0x00u, 0x00u                                                                       \
	}

#define NT3H2X11_DEFAULT_CFG_AUTH_DYN_LOCK                                                         \
	{                                                                                          \
		0x00u, 0x00u, 0x00u                                                                \
	}
#define NT3H2X11_DEFAULT_CFG_AUTH_AUTH0                                                            \
	{                                                                                          \
		0xFFu                                                                              \
	}
#define NT3H2X11_DEFAULT_CFG_AUTH_ACCESS                                                           \
	{                                                                                          \
		0x00u                                                                              \
	}
#define NT3H2X11_DEFAULT_CFG_AUTH_PWD                                                              \
	{                                                                                          \
		0xFFu, 0xFFu, 0xFFu, 0xFFu                                                         \
	}
#define NT3H2X11_DEFAULT_CFG_AUTH_PACK                                                             \
	{                                                                                          \
		0x00u, 0x00u                                                                       \
	}
#define NT3H2X11_DEFAULT_CFG_AUTH_PT_I2C                                                           \
	{                                                                                          \
		0x00u                                                                              \
	}

#define NT3H2X11_DEFAULT_CC                                                                        \
	{                                                                                          \
		NT3H2X11_DEFAULT_CC_MAGIC, NT3H2X11_DEFAULT_CC_VERSION, NT3H2X11_DEFAULT_CC_SIZE,  \
			NT3H2X11_DEFAULT_CC_ACCESS                                                 \
	}

#define NT3H2X11_DEFAULT_DEVICE                                                                    \
	{                                                                                          \
		NT3H2X11_DEFAULT_DEVICE_ADDR, NT3H2X11_DEFAULT_DEVICE_SERIAL,                      \
			NT3H2X11_DEFAULT_DEVICE_INTERNAL, NT3H2X11_DEFAULT_DEVICE_STATIC_LOCK,     \
			NT3H2X11_DEFAULT_CC                                                        \
	}

#define NT3H2X11_DEFAULT_CFG_AUTH                                                                  \
	{                                                                                          \
		NT3H2X11_DEFAULT_CFG_AUTH_DYN_LOCK, NT3H2X11_DEFAULT_CFG_AUTH_AUTH0,               \
			NT3H2X11_DEFAULT_CFG_AUTH_ACCESS, NT3H2X11_DEFAULT_CFG_AUTH_PWD,           \
			NT3H2X11_DEFAULT_CFG_AUTH_PACK, NT3H2X11_DEFAULT_CFG_AUTH_PT_I2C           \
	}

/******************* EVENTS *******************/

enum nt3h2x11_event {
	/* not used */
	NT3H2X11_EVENT_NONE = 0,
	/* External NFC field detected */
	NT3H2X11_EVENT_FD_ON,
	/* External NFC field has been removed */
	NT3H2X11_EVENT_FD_OFF,
	/* Reader has started communication */
	NT3H2X11_EVENT_START_OF_COMM,
	/* Reader has selected this tag for communication */
	NT3H2X11_EVENT_SELECTED,
	/* NFC session has ended. Reader has sent HALT command */
	NT3H2X11_EVENT_HALTED,
	/* Read has read last NDEF packet */
	NT3H2X11_EVENT_LAST_NDEF_READ,
	/* Passthrough NFC>I2C: Data is ready to be read by I2C */
	NT3H2X11_EVENT_DATA_READY_I2C,
	/* Passthrough NFC>I2C: I2C has read last data packet */
	NT3H2X11_EVENT_LAST_DATA_READ_I2C,
	/* Passthrough I2C>NFC: I2C has written last data packet */
	NT3H2X11_EVENT_LAST_DATA_WRITTEN_I2C,
	/* Passthrough I2C>NFC: Reader has read last data packet */
	NT3H2X11_EVENT_LAST_DATA_READ_NFC,
};

/**
 * @brief Application callback to receive IRQ events
 *
 * This callback runs at its own worker, not in the ISR context
 *
 * @param[in] *dev   : Pointer to nt3h2x11 device
 * @param[in] event  : @ref(enum nt3h2x11_event) IRQ event
 * @param[in] nc_reg : NC_REG contents (session)
 * @param[in] ns_reg : NS_REG contents (session)
 */
typedef void (*nt3h2x11_irq_callback_t)(const struct device *dev, enum nt3h2x11_event event,
					uint8_t nc_reg, uint8_t ns_reg);

/******************* DEVICE SETTINGS *******************/

/**
 * @brief Set device settings
 *
 * Corrects i2c-address to DTS-value
 *
 * @param[in] *dev    : Pointer to nt3h2x11 device
 * @param[in] *device : Pointer to @ref(nt3h2x11_device)
 * @return            : 0 on success, negative upon error.
 */
int nt3h2x11_set_device(const struct device *dev, struct nt3h2x11_device *device);

/**
 * @brief Read device settings
 *
 * @param[in] *dev    : Pointer to nt3h2x11 device
 * @param[in] *device : Pointer to @ref(nt3h2x11_device)
 * @return            : 0 on success, negative upon error.
 */
int nt3h2x11_get_device(const struct device *dev, struct nt3h2x11_device *device);

/******************* AUTHENTICATION SETTINGS *******************/

/**
 * @brief Set authentication settings
 *
 * Sets data in block NT3H2X11_BLK_LOCK_AUTH and NT3H2X11_BLK_ACCESS_PWD.
 *
 * @param[in] *dev    : Pointer to nt3h2x11 device
 * @param[in] *device : Pointer to @ref(nt3h2x11_cfg_auth)
 * @return            : 0 on success, negative upon error.
 */
int nt3h2x11_set_cfg_auth(const struct device *dev, struct nt3h2x11_cfg_auth *cfg_auth);

/**
 * @brief Read authentication settings
 *
 * Reads data from block NT3H2X11_BLK_LOCK_AUTH and NT3H2X11_BLK_ACCESS_PWD.
 *
 * @param[in] *dev    : Pointer to nt3h2x11 device
 * @param[in] *device : Pointer to @ref(nt3h2x11_cfg_auth)
 * @return            : 0 on success, negative upon error.
 */
int nt3h2x11_get_cfg_auth(const struct device *dev, struct nt3h2x11_cfg_auth *cfg_auth);

/******************* SESSION AND CONFIG OPERATIONS *******************/

/**
 * @brief Set NFCS_I2C_RST_ON_OFF
 *
 * Enables the NFC silence feature and enables soft reset through I2C repeated
 * start - see Section 9.3 of datasheet for more information.
 *
 * Default : 0b
 *
 * @param[in] *dev   : Pointer to nt3h2x11 device
 * @param[in] csreg  : SESSION or CONFIG register-block
 * @param[in] enable : enable, 0=disable, 1=enable
 * @return           : 0 on success, negative upon error.
 */
int nt3h2x11_set_softreset_en(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t enable);

/**
 * @brief Read NFCS_I2C_RST_ON_OFF
 *
 * @param[in]  *dev    : Pointer to nt3h2x11 device
 * @param[in]   csreg  : SESSION or CONFIG register-block
 * @param[out] *enable : Pointer to enable, 0=disable, 1=enable
 * @return             : 0 on success, negative upon error.
 */
int nt3h2x11_get_softreset_en(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t *enable);

/**
 * @brief Set PTHRU_ON_OFF
 *
 * Activates the pass-through mode, that uses the NTAG's 64 Byte SRAM
 * for communication between a NFC device and the I2C bus
 *
 * 1b: pass-through mode using SRAM enabled and SRAM mapped to end of Sector 0.
 * 0b: pass-through mode disabled
 *
 * Default : 0b
 *
 * @param[in] *dev   : Pointer to nt3h2x11 device
 * @param[in] csreg  : SESSION or CONFIG register-block
 * @param[in] enable : enable, 0=disable, 1=enable
 * @return           : 0 on success, negative upon error.
 */
int nt3h2x11_set_pthru_en(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t enable);

/**
 * @brief Read PTHRU_ON_OFF
 *
 * @param[in]  *dev    : Pointer to nt3h2x11 device
 * @param[in]   csreg  : SESSION or CONFIG register-block
 * @param[out] *enable : Pointer to enable, 0=disable, 1=enable
 * @return             : 0 on success, negative upon error.
 */
int nt3h2x11_get_pthru_en(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t *enable);

/**
 * @brief Set FD_OFF
 *
 * Defines the event upon which the signal output on the FD pin is released
 * 00b: if the field is switched off
 * 01b: if the field is switched off or the tag is set to the HALT state
 * 10b: if the field is switched off or the last page of the NDEF message has
 *      been read (defined in LAST_ NDEF_BLOCK)
 * 11b: (if FD_ON = 11b) if the field is switched off or if last data is read
 *      by I2C (in pass-through mode NFC ---> I2C) or last data is written by
 *      I2C (in pass-through mode I2C---> NFC)
 * 11b: (if FD_ON = 00b or 01b or 10b) if the field is switched of
 *
 * See Section 8.4 of datasheet for more details
 *
 * Default : 00b
 *
 * @param[in] *dev   : Pointer to nt3h2x11 device
 * @param[in] csreg  : SESSION or CONFIG register-block
 * @param[in] fd_off : @ref(nt3h2x11_fd_off) function to set
 * @return           : 0 on success, negative upon error.
 */
int nt3h2x11_set_fd_off(const struct device *dev, enum nt3h2x11_csreg csreg,
			enum nt3h2x11_fd_off fd_off);

/**
 * @brief Read FD_OFF
 *
 * @param[in]  *dev    : Pointer to nt3h2x11 device
 * @param[in]   csreg  : SESSION or CONFIG register-block
 * @param[out] *fd_off : Pointer to @ref(nt3h2x11_fd_off)
 * @return             : 0 on success, negative upon error.
 */
int nt3h2x11_get_fd_off(const struct device *dev, enum nt3h2x11_csreg csreg,
			enum nt3h2x11_fd_off *fd_off);

/**
 * @brief Set FD_ON
 *
 * Defines the event upon which the signal output on the FD pin is pulled low
 * 00b: if the field is switched on
 * 01b: by first valid start of communication (SoC)
 * 10b: by selection of the tag
 * 11b: (in pass-through mode NFC-->I2C) if the data is ready to be read from
 *      the I2C interface
 * 11b: (in pass-through mode I2
 *
 * See Section 8.4 of datasheet for more details
 *
 * Default : 00b
 *
 * @param[in] *dev  : Pointer to nt3h2x11 device
 * @param[in] csreg : SESSION or CONFIG register-block
 * @param[in] fd_on : @ref(nt3h2x11_fd_on) function to set
 * @return          : 0 on success, negative upon error.
 */
int nt3h2x11_set_fd_on(const struct device *dev, enum nt3h2x11_csreg csreg,
		       enum nt3h2x11_fd_on fd_on);

/**
 * @brief Read FD_ON
 *
 * @param[in]  *dev   : Pointer to nt3h2x11 device
 * @param[in]   csreg : SESSION or CONFIG register-block
 * @param[out] *fd_on : Pointer to @ref(nt3h2x11_fd_on)
 * @return            : 0 on success, negative upon error.
 */
int nt3h2x11_get_fd_on(const struct device *dev, enum nt3h2x11_csreg csreg,
		       enum nt3h2x11_fd_on *fd_on);

/**
 * @brief Set SRAM_MIRROR_ON_OFF
 *
 * 1b: SRAM mirror enabled and mirrored SRAM starts at page SRAM_MIRROR_BLOCK
 * 0b: SRAM mirror disabled
 *
 * Default : 0b
 *
 * @param[in] *dev    : Pointer to nt3h2x11 device
 * @param[in]  csreg  : SESSION or CONFIG register-block
 * @param[in]  enable : enable, 0=disable, 1=enable
 * @return            : 0 on success, negative upon error.
 */
int nt3h2x11_set_srammirror_en(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t enable);

/**
 * @brief Read SRAM_MIRROR_ON_OFF
 *
 * @param[in]  *dev    : Pointer to nt3h2x11 device
 * @param[in]   csreg  : SESSION or CONFIG register-block
 * @param[out] *enable : Pointer to enable, 0=disable, 1=enable
 * @return             : 0 on success, negative upon error.
 */
int nt3h2x11_get_srammirror_en(const struct device *dev, enum nt3h2x11_csreg csreg,
			       uint8_t *enable);

/**
 * @brief Set TRANSFER_DIR
 *
 * Defines the data flow direction when pass-through mode is enabled
 * 0b: from I2C to NFC interface
 * 1b: from NFC to I2C interface
 * In case the pass-through mode is NOT enabled, this bit should be set to 1b,
 * otherwise there is no WRITE access from the NFC perspective
 *
 * If the transfer direction is already the desired direction nothing is done.
 * If the Pthru is switched on, it will be switched off and back on after the
 * direction change.
 *
 * Default : 1b
 *
 * @param[in] *dev  : Pointer to nt3h2x11 device
 * @param[in] csreg : SESSION or CONFIG register-block
 * @param[in] dir   : @ref(nt3h2x11_transfer_dir) direction
 * @return          : 0 on success, negative upon error.
 */
int nt3h2x11_set_transfer_dir(const struct device *dev, enum nt3h2x11_csreg csreg,
			      enum nt3h2x11_transfer_dir dir);

/**
 * @brief Read TRANSFER_DIR
 *
 * @param[in]  *dev : Pointer to nt3h2x11 device
 * @param[in] csreg : SESSION or CONFIG register-block
 * @param[out] *dir : Pointer to @ref(nt3h2x11_transfer_dir) direction
 * @return          : 0 on success, negative upon error.
 */
int nt3h2x11_get_transfer_dir(const struct device *dev, enum nt3h2x11_csreg csreg,
			      enum nt3h2x11_transfer_dir *dir);

/**
 * @brief Set LAST_NDEF_BLOCK
 *
 * I2C block address of I2C block, which contains last byte(s) of stored NDEF
 * message. An NFC read of the last page of this I2C block sets the register
 * NDEF_DATA_READ to 1b and triggers field detection pin if FD_OFF is set to 10b.
 * Valid range starts from 01h (NFC page 04h) up to 37h (NFC page DCh) for NTAG
 * I2C plus 1k or up to 7Fh (NFC page FCh on Sector 1) for NTAG I2C plus 2k.
 *
 * Granularity is 4 pages(4bytes each), so block * 4 is the real page address.
 *
 * Max Block address is @ref(NT3H2X11_LAST_NDEF_BLOCK_MAX)
 *
 * Default : 0x00
 *
 * @param[in] *dev  : Pointer to nt3h2x11 device
 * @param[in] csreg : SESSION or CONFIG register-block
 * @param[in] block : Block address
 * @return          : 0 on success, negative upon error.
 */
int nt3h2x11_set_last_ndef_blk(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t block);

/**
 * @brief Read LAST_NDEF_BLOCK
 *
 * @param[in]  *dev   : Pointer to nt3h2x11 device
 * @param[in] csreg   : SESSION or CONFIG register-block
 * @param[out] *block : Pointer to Block address
 * @return            : 0 on success, negative upon error.
 */
int nt3h2x11_get_last_ndef_blk(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t *block);

/**
 * @brief Set SRAM_MIRROR_BLOCK
 *
 * I2C block address of SRAM when mirrored into the User memory.
 * Valid range starts from 01h (NFC page 04h) up to 34h (NFC page D0h) for NTAG
 * I2C plus 1k or up to 7Ch (NFC page F0h on memory Sector 1) for NTAG I2C plus 2k
 *
 * Granularity is 4 pages(4bytes each), so block * 4 is the real page address.
 *
 * Default : 0xF8
 *
 * @param[in] *dev  : Pointer to nt3h2x11 device
 * @param[in] csreg : SESSION or CONFIG register-block
 * @param[in] block : block to which the SRAM should be mirrored
 * @return          : 0 on success, negative upon error.
 */
int nt3h2x11_set_srammirror_blk(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t block);

/**
 * @brief Read SRAM_MIRROR_BLOCK
 *
 * @param[in]  *dev   : Pointer to nt3h2x11 device
 * @param[in]   csreg : SESSION or CONFIG register-block
 * @param[out] *block : Pointer to block address
 * @return            : 0 on success, negative upon error.
 */
int nt3h2x11_get_srammirror_blk(const struct device *dev, enum nt3h2x11_csreg csreg,
				uint8_t *block);

/**
 * @brief Set the value of the watchdog timer
 *
 * Sets both WDT_LS and WDT_MS
 *
 * Default: 0x0848
 *
 * @param[in] *dev  : Pointer to nt3h2x11 device
 * @param[in] csreg : SESSION or CONFIG register-block
 * @param[in] time  : new time value of the watchdog timer
 * @return          : 0 on success, negative upon error.
 */
int nt3h2x11_set_wdt(const struct device *dev, enum nt3h2x11_csreg csreg, uint16_t time);

/**
 * @brief Read the value of the watchdog timer
 *
 * @param[in]  *dev  : Pointer to nt3h2x11 device
 * @param[in]  csreg : SESSION or CONFIG register-block
 * @param[out] *time : Pointer to watchdog time
 * @return           : 0 on success, negative upon error.
 */
int nt3h2x11_get_wdt(const struct device *dev, enum nt3h2x11_csreg csreg, uint16_t *time);

/**
 * @brief Set I2C_CLOCK_STR
 *
 * Enables (1b) or disable (0b) the I2C clock stretching
 *
 * Default : 1b
 *
 * @param[in] *dev    : Pointer to nt3h2x11 device
 * @param[in]  csreg  : SESSION or CONFIG register-block
 * @param[in]  enable : enable, 0=disable, 1=enable
 * @return            : 0 on success, negative upon error.
 */
int nt3h2x11_set_i2c_clkstr_en(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t enable);

/**
 * @brief Set I2C_CLOCK_STR
 *
 * @param[in]  *dev    : Pointer to nt3h2x11 device
 * @param[in]   csreg  : SESSION or CONFIG register-block
 * @param[out] *enable : Pointer to enable, 0=disable, 1=enable
 * @return             : 0 on success, negative upon error.
 */
int nt3h2x11_get_i2c_clkstr_en(const struct device *dev, enum nt3h2x11_csreg csreg,
			       uint8_t *enable);

/******************* CONFIG ONLY OPERATIONS *******************/

/**
 * @brief Read REG_LOCK_I2C
 *
 * I2C Configuration Lock Bit,
 * 0b: Configuration bytes may be changed via I2C
 * 1b: Configuration bytes cannot be changed via I2C
 *
 * Once set to 1b, cannot be reset to 0b anymore
 *
 * Default : 0b
 *
 * @param[in]  *dev    : Pointer to nt3h2x11 device
 * @param[out] *locked : Pointer to lock-bit
 * @return             : 0 on success, negative upon error.
 */
int nt3h2x11_get_i2c_lock_config(const struct device *dev, uint8_t *locked);

/**
 * @brief Read REG_LOCK_NFC
 *
 * NFC Configuration Lock Bit,
 * 0b: Configuration bytes may be changed via NFC
 * 1b: Configuration bytes cannot be changed via NFC
 *
 * Once set to 1b, cannot be reset to 0b anymore
 *
 * Default : 0b
 *
 * @param[in]  *dev    : Pointer to nt3h2x11 device
 * @param[out] *locked : Pointer to lock-bit
 * @return             : 0 on success, negative upon error.
 */
int nt3h2x11_get_nfc_lock_config(const struct device *dev, uint8_t *locked);

/******************* SESSION ONLY OPERATIONS *******************/

/**
 * @brief Read NEG_AUTH_REACHED
 *
 * Status bit to show the number of negative PWD_ AUTH attempts reached
 * 0b: PWD_AUTH still possible
 * 1b: PWD_AUTH locked
 *
 * Default : 0b
 *
 * @param[in]  *dev    : Pointer to nt3h2x11 device
 * @param[out] *enable : Pointer to enable, 0=disable, 1=enable
 * @return             : 0 on success, negative upon error.
 */
int nt3h2x11_get_auth_en(const struct device *dev, uint8_t *enable);

/**
 * @brief Read NS_REG
 *
 * @param[in]  *dev   : Pointer to nt3h2x11 device
 * @param[out] *nsreg : Pointer to nsreg-register dump
 * @return            : 0 on success, negative upon error.
 */
int nt3h2x11_get_nsreg(const struct device *dev, uint8_t *nsreg);

/******************* GENERIC READ/WRITE *******************/

/**
 * @brief Read X Blocks
 *
 * @param[in]  *dev  : Pointer to nt3h2x11 device
 * @param[in]  block : Block-number to start read from
 * @param[out] *buf  : Pointer to buffer to which block-data is stored
 * @param[in]  count : Number of blocks to read
 * @return           : 0 on success, negative upon error.
 */
int nt3h2x11_read_blocks(const struct device *dev, uint8_t block, uint8_t *buf, uint8_t count);

/**
 * @brief Write X Blocks
 *
 * Note: This function does not protect I2C address at reg 00 of block 00.
 *
 * @param[in] *dev   : Pointer to nt3h2x11 device
 * @param[in]  block : Block-number to start write from
 * @param[in] *buf   : Pointer to buffer of which data is written
 * @param[in]  count : Number of blocks to write
 * @return           : 0 on success, negative upon error.
 */
int nt3h2x11_write_blocks(const struct device *dev, uint8_t block, uint8_t *buf, uint8_t count);

/**
 * @brief Read X Bytes
 *
 * Note: uses memory addresses instead of blocks.
 * A single block consists of @ref(NT3H2X11_BYTES_BLK) bytes / memory addresses.
 *
 * @param[in]  *dev     : Pointer to nt3h2x11 device
 * @param[in]   addr    : Memory address to read from.
 * @param[out] *buf     : Buffer to write to
 * @param[in]   buf_len : Number of bytes to read
 * @return              : 0 on success, negative upon error.
 */
int nt3h2x11_read_bytes(const struct device *dev, uint16_t addr, uint8_t *buf, uint16_t buf_len);

/**
 * @brief Write X Bytes
 *
 * Note: uses memory addresses instead of blocks.
 * A single block consists of @ref(NT3H2X11_BYTES_BLK) bytes / memory addresses.
 *
 * Note: This function does not protect I2C address at reg 00 of block 00.
 *
 * @param[in]  *dev     : Pointer to nt3h2x11 device
 * @param[in]   addr    : Memory address to write to.
 * @param[out] *buf     : Buffer to write to
 * @param[in]   buf_len : Number of bytes to read
 * @return              : 0 on success, negative upon error.
 */
int nt3h2x11_write_bytes(const struct device *dev, uint16_t addr, uint8_t *buf, uint16_t buf_len);

/******************* IRQ HANDLING *******************/

/**
 * @brief Set application callback to receive interrupt-events
 *
 * @param[in] *dev    : Pointer to nt3h2x11 device
 * @param[in] cb      : Application callback for interrupts
 */
int nt3h2x11_irq_set_callback(const struct device *dev, nt3h2x11_irq_callback_t cb);

/******************* DEVICE SETUP *******************/

/**
 * @brief Force i2c address
 *
 * After updating the address successfully, device need too be reset.
 *
 * @param[in] *dev      : Pointer to nt3h2x11 device
 * @param[in]  addr_old : Old (current) i2c address
 * @param[in]  addr_new : New i2c address
 * @return              : negative upon error. Infinite loop or reboot on
 *                        success (depending on CONFIG_REBOOT).
 */
int nt3h2x11_set_i2c_addr(const struct device *dev, uint8_t addr_old, uint8_t addr_new);

#endif /* ZEPHYR_INCLUDE_DRIVERS_NFC_NT3H2X11_H_ */
