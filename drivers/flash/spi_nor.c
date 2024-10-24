/*
 * Copyright (c) 2018 Savoir-Faire Linux.
 * Copyright (c) 2020 Peter Bigot Consulting, LLC
 * Copyright (c) 2023 Intercreate, Inc.
 *
 * This driver is heavily inspired from the spi_flash_w25qxxdv.c SPI NOR driver.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT jedec_spi_nor

#include <errno.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/init.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include "spi_nor.h"
#include "jesd216.h"
#include "flash_priv.h"

LOG_MODULE_REGISTER(spi_nor, CONFIG_FLASH_LOG_LEVEL);

/* Device Power Management Notes
 *
 * These flash devices have several modes during operation:
 * * When CSn is asserted (during a SPI operation) the device is
 *   active.
 * * When CSn is deasserted the device enters a standby mode.
 * * Some devices support a Deep Power-Down mode which reduces current
 *   to as little as 0.1% of standby.
 *
 * When mapped to the Zephyr Device Power Management states:
 * * PM_DEVICE_STATE_ACTIVE covers both active and standby modes;
 * * PM_DEVICE_STATE_SUSPENDED corresponds to deep-power-down mode;
 * * PM_DEVICE_STATE_OFF covers the powered off state;
 */

#define SPI_NOR_MAX_ADDR_WIDTH 4
#define SPI_NOR_3B_ADDR_MAX    0xFFFFFF

#define ANY_INST_HAS_TRUE_(idx, bool_prop)	\
	COND_CODE_1(DT_INST_PROP(idx, bool_prop), (1,), ())

#define ANY_INST_HAS_TRUE(bool_prop)	\
	COND_CODE_1(IS_EMPTY(DT_INST_FOREACH_STATUS_OKAY_VARGS(ANY_INST_HAS_TRUE_, bool_prop)), \
			     (0), (1))

#define ANY_INST_HAS_PROP_(idx, prop_name)	\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(idx, prop_name), (1,), ())
#define ANY_INST_HAS_PROP(prop_name)		\
	COND_CODE_1(IS_EMPTY(DT_INST_FOREACH_STATUS_OKAY_VARGS(ANY_INST_HAS_PROP_, prop_name)), \
			     (0), (1))

#define ANY_INST_HAS_MXICY_MX25R_POWER_MODE ANY_INST_HAS_PROP(mxicy_mx25r_power_mode)
#define ANY_INST_HAS_DPD ANY_INST_HAS_TRUE(has_dpd)
#define ANY_INST_HAS_T_EXIT_DPD ANY_INST_HAS_PROP(t_exit_dpd)
#define ANY_INST_HAS_DPD_WAKEUP_SEQUENCE ANY_INST_HAS_PROP(dpd_wakeup_sequence)
#define ANY_INST_HAS_RESET_GPIOS ANY_INST_HAS_PROP(reset_gpios)
#define ANY_INST_HAS_WP_GPIOS ANY_INST_HAS_PROP(wp_gpios)
#define ANY_INST_HAS_HOLD_GPIOS ANY_INST_HAS_PROP(hold_gpios)
#define ANY_INST_USE_4B_ADDR_OPCODES ANY_INST_HAS_TRUE(use_4b_addr_opcodes)

#ifdef CONFIG_SPI_NOR_ACTIVE_DWELL_MS
#define ACTIVE_DWELL_MS CONFIG_SPI_NOR_ACTIVE_DWELL_MS
#else
#define ACTIVE_DWELL_MS 0
#endif

#define DEV_CFG(_dev_) ((const struct spi_nor_config * const) (_dev_)->config)

/* MXICY Related defines*/
/* MXICY Low-power/high perf mode is second bit in configuration register 2 */
#define LH_SWITCH_BIT 9

#define JEDEC_MACRONIX_ID   0xc2
#define JEDEC_MX25R_TYPE_ID 0x28

/* Build-time data associated with the device. */
struct spi_nor_config {
	/* Devicetree SPI configuration */
	struct spi_dt_spec spi;

#if ANY_INST_HAS_RESET_GPIOS
	const struct gpio_dt_spec reset;
#endif

	/* Runtime SFDP stores no static configuration. */

#ifndef CONFIG_SPI_NOR_SFDP_RUNTIME
	/* Size of device in bytes, from size property */
	uint32_t flash_size;

#ifdef CONFIG_FLASH_PAGE_LAYOUT
	/* Flash page layout can be determined from devicetree. */
	struct flash_pages_layout layout;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

	/* Expected JEDEC ID, from jedec-id property */
	uint8_t jedec_id[SPI_NOR_MAX_ID_LEN];

#if defined(CONFIG_SPI_NOR_SFDP_MINIMAL)
	/* Optional support for entering 32-bit address mode. */
	uint8_t enter_4byte_addr;
#endif /* CONFIG_SPI_NOR_SFDP_MINIMAL */

#if defined(CONFIG_SPI_NOR_SFDP_DEVICETREE)
	/* Length of BFP structure, in 32-bit words. */
	uint8_t bfp_len;

	/* Pointer to the BFP table as read from the device
	 * (little-endian stored words), from sfdp-bfp property
	 */
	const struct jesd216_bfp *bfp;
#endif /* CONFIG_SPI_NOR_SFDP_DEVICETREE */
#endif /* CONFIG_SPI_NOR_SFDP_RUNTIME */

	/* Optional bits in SR to be cleared on startup.
	 *
	 * This information cannot be derived from SFDP.
	 */
	uint8_t has_lock;

#if ANY_INST_HAS_WP_GPIOS
	/* The write-protect GPIO (wp-gpios) */
	const struct gpio_dt_spec wp;
#endif

#if ANY_INST_HAS_HOLD_GPIOS
	/* The hold GPIO (hold-gpios) */
	const struct gpio_dt_spec hold;
#endif

#if ANY_INST_HAS_DPD
	uint16_t t_enter_dpd; /* in milliseconds */
	uint16_t t_dpdd_ms;   /* in milliseconds */
#if ANY_INST_HAS_T_EXIT_DPD
	uint16_t t_exit_dpd;  /* in milliseconds */
#endif
#endif

#if ANY_INST_HAS_DPD_WAKEUP_SEQUENCE
	uint16_t t_crdp_ms; /* in milliseconds */
	uint16_t t_rdp_ms;  /* in milliseconds */
#endif

#if ANY_INST_HAS_MXICY_MX25R_POWER_MODE
	bool mxicy_mx25r_power_mode;
#endif
	bool use_4b_addr_opcodes:1;

	/* exist flags for dts opt-ins */
	bool dpd_exist:1;
	bool dpd_wakeup_sequence_exist:1;
	bool mxicy_mx25r_power_mode_exist:1;
	bool reset_gpios_exist:1;
	bool requires_ulbpr_exist:1;
	bool wp_gpios_exist:1;
	bool hold_gpios_exist:1;
};

/**
 * struct spi_nor_data - Structure for defining the SPI NOR access
 * @sem: The semaphore to access to the flash
 */
struct spi_nor_data {
	struct k_sem sem;
#if ANY_INST_HAS_DPD
	/* Low 32-bits of uptime counter at which device last entered
	 * deep power-down.
	 */
	uint32_t ts_enter_dpd;
#endif

	/* Miscellaneous flags */

	/* If set addressed operations should use 32-bit rather than
	 * 24-bit addresses.
	 *
	 * This is ignored if the access parameter to a command
	 * explicitly specifies 24-bit or 32-bit addressing.
	 */
	bool flag_access_32bit: 1;

	/* Minimal SFDP stores no dynamic configuration.  Runtime and
	 * devicetree store page size and erase_types; runtime also
	 * stores flash size and layout.
	 */
#ifndef CONFIG_SPI_NOR_SFDP_MINIMAL

	struct jesd216_erase_type erase_types[JESD216_NUM_ERASE_TYPES];

	/* Number of bytes per page */
	uint16_t page_size;

#ifdef CONFIG_SPI_NOR_SFDP_RUNTIME
	/* Size of flash, in bytes */
	uint32_t flash_size;

#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout layout;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
#endif /* CONFIG_SPI_NOR_SFDP_RUNTIME */
#endif /* CONFIG_SPI_NOR_SFDP_MINIMAL */
};

#ifdef CONFIG_SPI_NOR_SFDP_MINIMAL
/* The historically supported erase sizes. */
static const struct jesd216_erase_type minimal_erase_types[JESD216_NUM_ERASE_TYPES] = {
	{
		.cmd = SPI_NOR_CMD_BE,
		.exp = 16,
	},
	{
		.cmd = SPI_NOR_CMD_SE,
		.exp = 12,
	},
};
static const struct jesd216_erase_type minimal_erase_types_4b[JESD216_NUM_ERASE_TYPES] = {
	{
		.cmd = SPI_NOR_CMD_BE_4B,
		.exp = 16,
	},
	{
		.cmd = SPI_NOR_CMD_SE_4B,
		.exp = 12,
	},
};
#endif /* CONFIG_SPI_NOR_SFDP_MINIMAL */

/* Register writes should be ready extremely quickly */
#define WAIT_READY_REGISTER K_NO_WAIT
/* Page writes range from sub-ms to 10ms */
#define WAIT_READY_WRITE K_TICKS(1)
/* Erases can range from 45ms to 240sec */
#define WAIT_READY_ERASE K_MSEC(50)

static int spi_nor_write_protection_set(const struct device *dev,
					bool write_protect);

/* Get pointer to array of supported erase types.  Static const for
 * minimal, data for runtime and devicetree.
 */
static inline const struct jesd216_erase_type *
dev_erase_types(const struct device *dev)
{
#ifdef CONFIG_SPI_NOR_SFDP_MINIMAL
	if (IS_ENABLED(ANY_INST_USE_4B_ADDR_OPCODES) && DEV_CFG(dev)->use_4b_addr_opcodes) {
		return minimal_erase_types_4b;
	}
	return minimal_erase_types;
#else /* CONFIG_SPI_NOR_SFDP_MINIMAL */
	const struct spi_nor_data *data = dev->data;

	return data->erase_types;
#endif /* CONFIG_SPI_NOR_SFDP_MINIMAL */
}

/* Get the size of the flash device.  Data for runtime, constant for
 * minimal and devicetree.
 */
static inline uint32_t dev_flash_size(const struct device *dev)
{
#ifdef CONFIG_SPI_NOR_SFDP_RUNTIME
	const struct spi_nor_data *data = dev->data;

	return data->flash_size;
#else /* CONFIG_SPI_NOR_SFDP_RUNTIME */
	const struct spi_nor_config *cfg = dev->config;

	return cfg->flash_size;
#endif /* CONFIG_SPI_NOR_SFDP_RUNTIME */
}

/* Get the flash device page size.  Constant for minimal, data for
 * runtime and devicetree.
 */
static inline uint16_t dev_page_size(const struct device *dev)
{
#ifdef CONFIG_SPI_NOR_SFDP_MINIMAL
	return DT_INST_PROP_OR(0, page_size, 256);
#else /* CONFIG_SPI_NOR_SFDP_MINIMAL */
	const struct spi_nor_data *data = dev->data;

	return data->page_size;
#endif /* CONFIG_SPI_NOR_SFDP_MINIMAL */
}

static const struct flash_parameters flash_nor_parameters = {
	.write_block_size = 1,
	.erase_value = 0xff,
};

/* Capture the time at which the device entered deep power-down. */
static inline void record_entered_dpd(const struct device *const dev)
{
#if ANY_INST_HAS_DPD
	const struct spi_nor_config *const driver_config = dev->config;

	if (driver_config->dpd_exist) {
		struct spi_nor_data *const driver_data = dev->data;

		driver_data->ts_enter_dpd = k_uptime_get_32();
	}
#else
	ARG_UNUSED(dev);
#endif
}

/* Check the current time against the time DPD was entered and delay
 * until it's ok to initiate the DPD exit process.
 */
static inline void delay_until_exit_dpd_ok(const struct device *const dev)
{
#if ANY_INST_HAS_DPD
	const struct spi_nor_config *const driver_config = dev->config;

	if (driver_config->dpd_exist) {
		struct spi_nor_data *const driver_data = dev->data;
		int32_t since = (int32_t)(k_uptime_get_32() - driver_data->ts_enter_dpd);

		/* If the time is negative the 32-bit counter has wrapped,
		 * which is certainly long enough no further delay is
		 * required.  Otherwise we have to check whether it's been
		 * long enough taking into account necessary delays for
		 * entering and exiting DPD.
		 */
		if (since >= 0) {
			/* Subtract time required for DPD to be reached */
			since -= driver_config->t_enter_dpd;

			/* Subtract time required in DPD before exit */
			since -= driver_config->t_dpdd_ms;

			/* If the adjusted time is negative we have to wait
			 * until it reaches zero before we can proceed.
			 */
			if (since < 0) {
				k_sleep(K_MSEC((uint32_t)-since));
			}
		}
	}
#else
	ARG_UNUSED(dev);
#endif /* ANY_INST_HAS_DPD */
}

/* Indicates that an access command includes bytes for the address.
 * If not provided the opcode is not followed by address bytes.
 */
#define NOR_ACCESS_ADDRESSED BIT(0)

/* Indicates that addressed access uses a 24-bit address regardless of
 * spi_nor_data::flag_32bit_addr.
 */
#define NOR_ACCESS_24BIT_ADDR BIT(1)

/* Indicates that addressed access uses a 32-bit address regardless of
 * spi_nor_data::flag_32bit_addr.
 */
#define NOR_ACCESS_32BIT_ADDR BIT(2)

/* Indicates that an access command is performing a write.  If not
 * provided access is a read.
 */
#define NOR_ACCESS_WRITE BIT(7)

/*
 * @brief Send an SPI command
 *
 * @param dev Device struct
 * @param opcode The command to send
 * @param access flags that determine how the command is constructed.
 *        See NOR_ACCESS_*.
 * @param addr The address to send
 * @param data The buffer to store or read the value
 * @param length The size of the buffer
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nor_access(const struct device *const dev,
			  uint8_t opcode, unsigned int access,
			  off_t addr, void *data, size_t length)
{
	const struct spi_nor_config *const driver_cfg = dev->config;
	struct spi_nor_data *const driver_data = dev->data;
	bool is_addressed = (access & NOR_ACCESS_ADDRESSED) != 0U;
	bool is_write = (access & NOR_ACCESS_WRITE) != 0U;
	uint8_t buf[5] = { 0 };
	struct spi_buf spi_buf[2] = {
		{
			.buf = buf,
			.len = 1,
		},
		{
			.buf = data,
			.len = length
		}
	};

	buf[0] = opcode;
	if (is_addressed) {
		bool access_24bit = (access & NOR_ACCESS_24BIT_ADDR) != 0;
		bool access_32bit = (access & NOR_ACCESS_32BIT_ADDR) != 0;
		bool use_32bit = (access_32bit
				  || (!access_24bit
				      && driver_data->flag_access_32bit));
		union {
			uint32_t u32;
			uint8_t u8[4];
		} addr32 = {
			.u32 = sys_cpu_to_be32(addr),
		};

		if (use_32bit) {
			memcpy(&buf[1], &addr32.u8[0], 4);
			spi_buf[0].len += 4;
		} else {
			memcpy(&buf[1], &addr32.u8[1], 3);
			spi_buf[0].len += 3;
		}
	};

	const struct spi_buf_set tx_set = {
		.buffers = spi_buf,
		.count = (length != 0) ? 2 : 1,
	};

	const struct spi_buf_set rx_set = {
		.buffers = spi_buf,
		.count = 2,
	};

	if (is_write) {
		return spi_write_dt(&driver_cfg->spi, &tx_set);
	}

	return spi_transceive_dt(&driver_cfg->spi, &tx_set, &rx_set);
}

#define spi_nor_cmd_read(dev, opcode, dest, length) \
	spi_nor_access(dev, opcode, 0, 0, dest, length)
#define spi_nor_cmd_addr_read(dev, opcode, addr, dest, length) \
	spi_nor_access(dev, opcode, NOR_ACCESS_ADDRESSED, addr, dest, length)
#define spi_nor_cmd_addr_read_3b(dev, opcode, addr, dest, length)                                  \
	spi_nor_access(dev, opcode, NOR_ACCESS_24BIT_ADDR | NOR_ACCESS_ADDRESSED, addr, dest,      \
		       length)
#define spi_nor_cmd_addr_read_4b(dev, opcode, addr, dest, length)                                  \
	spi_nor_access(dev, opcode, NOR_ACCESS_32BIT_ADDR | NOR_ACCESS_ADDRESSED, addr, dest,      \
		       length)
#define spi_nor_cmd_write(dev, opcode) \
	spi_nor_access(dev, opcode, NOR_ACCESS_WRITE, 0, NULL, 0)
#define spi_nor_cmd_addr_write(dev, opcode, addr, src, length) \
	spi_nor_access(dev, opcode, NOR_ACCESS_WRITE | NOR_ACCESS_ADDRESSED, \
		       addr, (void *)src, length)
#define spi_nor_cmd_addr_write_3b(dev, opcode, addr, src, length)                                  \
	spi_nor_access(dev, opcode,                                                                \
		       NOR_ACCESS_24BIT_ADDR | NOR_ACCESS_WRITE | NOR_ACCESS_ADDRESSED, addr,      \
		       (void *)src, length)
#define spi_nor_cmd_addr_write_4b(dev, opcode, addr, src, length)                                  \
	spi_nor_access(dev, opcode,                                                                \
		       NOR_ACCESS_32BIT_ADDR | NOR_ACCESS_WRITE | NOR_ACCESS_ADDRESSED, addr,      \
		       (void *)src, length)

/**
 * @brief Wait until the flash is ready
 *
 * @note The device must be externally acquired before invoking this
 * function.
 *
 * This function should be invoked after every ERASE, PROGRAM, or
 * WRITE_STATUS operation before continuing.  This allows us to assume
 * that the device is ready to accept new commands at any other point
 * in the code.
 *
 * @param dev The device structure
 * @param poll_delay Duration between polls of status register
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nor_wait_until_ready(const struct device *dev, k_timeout_t poll_delay)
{
	int ret;
	uint8_t reg;

	ARG_UNUSED(poll_delay);

	while (true) {
		ret = spi_nor_cmd_read(dev, SPI_NOR_CMD_RDSR, &reg, sizeof(reg));
		/* Exit on error or no longer WIP */
		if (ret || !(reg & SPI_NOR_WIP_BIT)) {
			break;
		}
#ifdef CONFIG_SPI_NOR_SLEEP_WHILE_WAITING_UNTIL_READY
		/* Don't monopolise the CPU while waiting for ready */
		k_sleep(poll_delay);
#endif /* CONFIG_SPI_NOR_SLEEP_WHILE_WAITING_UNTIL_READY */
	}
	return ret;
}

#if defined(CONFIG_SPI_NOR_SFDP_RUNTIME) || defined(CONFIG_FLASH_JESD216_API)
/*
 * @brief Read content from the SFDP hierarchy
 *
 * @note The device must be externally acquired before invoking this
 * function.
 *
 * @param dev Device struct
 * @param addr The address to send
 * @param data The buffer to store or read the value
 * @param length The size of the buffer
 * @return 0 on success, negative errno code otherwise
 */
static int read_sfdp(const struct device *const dev,
		     off_t addr, void *data, size_t length)
{
	/* READ_SFDP requires a 24-bit address followed by a single
	 * byte for a wait state.  This is effected by using 32-bit
	 * address by shifting the 24-bit address up 8 bits.
	 */
	return spi_nor_access(dev, JESD216_CMD_READ_SFDP,
			      NOR_ACCESS_32BIT_ADDR | NOR_ACCESS_ADDRESSED,
			      addr << 8, data, length);
}
#endif /* CONFIG_SPI_NOR_SFDP_RUNTIME */

static int enter_dpd(const struct device *const dev)
{
	int ret = 0;
	const struct spi_nor_config *cfg = dev->config;

	if (cfg->dpd_exist) {
		ret = spi_nor_cmd_write(dev, SPI_NOR_CMD_DPD);
		if (ret == 0) {
			record_entered_dpd(dev);
		}
	}
	return ret;
}

static int exit_dpd(const struct device *const dev)
{
	int ret = 0;
#if ANY_INST_HAS_DPD
	const struct spi_nor_config *cfg = dev->config;

	if (cfg->dpd_exist) {
		delay_until_exit_dpd_ok(dev);

		if (cfg->dpd_wakeup_sequence_exist) {
#if ANY_INST_HAS_DPD_WAKEUP_SEQUENCE
			/* Assert CSn and wait for tCRDP.
			 *
			 * Unfortunately the SPI API doesn't allow us to
			 * control CSn so fake it by writing a known-supported
			 * single-byte command, hoping that'll hold the assert
			 * long enough.  This is highly likely, since the
			 * duration is usually less than two SPI clock cycles.
			 */
			ret = spi_nor_cmd_write(dev, SPI_NOR_CMD_RDID);

			/* Deassert CSn and wait for tRDP */
			k_sleep(K_MSEC(cfg->t_rdp_ms));
#endif /* ANY_INST_HAS_DPD_WAKEUP_SEQUENCE */
		} else {
			ret = spi_nor_cmd_write(dev, SPI_NOR_CMD_RDPD);

#if ANY_INST_HAS_T_EXIT_DPD
			if (ret == 0) {
				if (cfg->dpd_exist) {
					k_sleep(K_MSEC(cfg->t_exit_dpd));
				}
			}
#endif /* T_EXIT_DPD */
		}
	}
#endif /* ANY_INST_HAS_DPD */
	return ret;
}

/* Everything necessary to acquire owning access to the device. */
static void acquire_device(const struct device *dev)
{
	const struct spi_nor_config *cfg = dev->config;

	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct spi_nor_data *const driver_data = dev->data;

		k_sem_take(&driver_data->sem, K_FOREVER);
	}

	(void)pm_device_runtime_get(cfg->spi.bus);
}

/* Everything necessary to release access to the device. */
static void release_device(const struct device *dev)
{
	const struct spi_nor_config *cfg = dev->config;

	(void)pm_device_runtime_put(cfg->spi.bus);

	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct spi_nor_data *const driver_data = dev->data;

		k_sem_give(&driver_data->sem);
	}
}

/**
 * @brief Read the status register.
 *
 * @note The device must be externally acquired before invoking this
 * function.
 *
 * @param dev Device struct
 *
 * @return the non-negative value of the status register, or an error code.
 */
static int spi_nor_rdsr(const struct device *dev)
{
	uint8_t reg;
	int ret = spi_nor_cmd_read(dev, SPI_NOR_CMD_RDSR, &reg, sizeof(reg));

	if (ret == 0) {
		ret = reg;
	}

	return ret;
}

/**
 * @brief Write the status register.
 *
 * @note The device must be externally acquired before invoking this
 * function.
 *
 * @param dev Device struct
 * @param sr The new value of the status register
 *
 * @return 0 on success or a negative error code.
 */
static int spi_nor_wrsr(const struct device *dev,
			uint8_t sr)
{
	int ret = spi_nor_cmd_write(dev, SPI_NOR_CMD_WREN);

	if (ret != 0) {
		return ret;
	}
	ret = spi_nor_access(dev, SPI_NOR_CMD_WRSR, NOR_ACCESS_WRITE, 0, &sr,
					sizeof(sr));
	if (ret != 0) {
		return ret;
	}
	return spi_nor_wait_until_ready(dev, WAIT_READY_REGISTER);
}

#if ANY_INST_HAS_MXICY_MX25R_POWER_MODE

/**
 * @brief Read the configuration register.
 *
 * @note The device must be externally acquired before invoking this
 * function.
 *
 * @param dev Device struct
 *
 * @return the non-negative value of the configuration register, or an error code.
 */
static int mxicy_rdcr(const struct device *dev)
{
	const struct spi_nor_config *cfg = dev->config;
	uint16_t cr = -ENOSYS;

	if (cfg->mxicy_mx25r_power_mode_exist) {
		int ret = spi_nor_cmd_read(dev, CMD_RDCR, &cr, sizeof(cr));

		if (ret < 0) {
			return ret;
		}
	}

	return cr;
}

/**
 * @brief Write the configuration register.
 *
 * @note The device must be externally acquired before invoking this
 * function.
 *
 * @param dev Device struct
 * @param cr  The new value of the configuration register
 *
 * @return 0 on success or a negative error code.
 */
static int mxicy_wrcr(const struct device *dev,
			uint16_t cr)
{
	const struct spi_nor_config *cfg = dev->config;
	int ret = -ENOSYS;
	/* The configuration register bytes on the Macronix MX25R devices are
	 * written using the Write Status Register command where the configuration
	 * register bytes are written as two extra bytes after the status register.
	 * First read out the current status register to preserve the value.
	 */

	if (cfg->mxicy_mx25r_power_mode_exist) {
		int sr = spi_nor_rdsr(dev);

		if (sr < 0) {
			LOG_ERR("Read status register failed: %d", sr);
			return sr;
		}

		ret = spi_nor_cmd_write(dev, SPI_NOR_CMD_WREN);
		if (ret != 0) {
			return ret;
		}

		uint8_t data[] = {
			sr,
			cr & 0xFF,	/* Configuration register 1 */
			cr >> 8		/* Configuration register 2 */
		};

		ret = spi_nor_access(dev, SPI_NOR_CMD_WRSR, NOR_ACCESS_WRITE, 0,
			data, sizeof(data));
		if (ret != 0) {
			return ret;
		}

		ret = spi_nor_wait_until_ready(dev, WAIT_READY_REGISTER);
	}

	return ret;
}

static int mxicy_configure(const struct device *dev, const uint8_t *jedec_id)
{
	const struct spi_nor_config *cfg = dev->config;
	int ret = -ENOSYS;

	if (cfg->mxicy_mx25r_power_mode_exist) {
		/* Low-power/high perf mode is second bit in configuration register 2 */
		int current_cr, new_cr;
		/* lh_switch enum index:
		 * 0: Ultra low power
		 * 1: High performance mode
		 */
		const bool use_high_perf = cfg->mxicy_mx25r_power_mode;

		/* Only supported on Macronix MX25R Ultra Low Power series. */
		if (jedec_id[0] != JEDEC_MACRONIX_ID || jedec_id[1] != JEDEC_MX25R_TYPE_ID) {
			LOG_WRN("L/H switch not supported for device id: %02x %02x %02x",
				jedec_id[0], jedec_id[1], jedec_id[2]);
			/* Do not return an error here because the flash still functions */
			return 0;
		}

		acquire_device(dev);

		/* Read current configuration register */

		ret = mxicy_rdcr(dev);
		if (ret < 0) {
			release_device(dev);
			return ret;
		}
		current_cr = ret;

		LOG_DBG("Use high performance mode? %d", use_high_perf);
		new_cr = current_cr;
		WRITE_BIT(new_cr, LH_SWITCH_BIT, use_high_perf);
		if (new_cr != current_cr) {
			ret = mxicy_wrcr(dev, new_cr);
		} else {
			ret = 0;
		}

		if (ret < 0) {
			LOG_ERR("Enable high performace mode failed: %d", ret);
		}

		release_device(dev);
	}

	return ret;
}

#endif /* ANY_INST_HAS_MXICY_MX25R_POWER_MODE */

static int spi_nor_read(const struct device *dev, off_t addr, void *dest,
			size_t size)
{
	const size_t flash_size = dev_flash_size(dev);
	int ret;

	/* should be between 0 and flash size */
	if ((addr < 0) || ((addr + size) > flash_size)) {
		return -EINVAL;
	}

	/* Ensure flash is powered before read */
	if (pm_device_runtime_get(dev) < 0) {
		return -EIO;
	}

	acquire_device(dev);

	if (IS_ENABLED(ANY_INST_USE_4B_ADDR_OPCODES) && DEV_CFG(dev)->use_4b_addr_opcodes) {
		if (addr > SPI_NOR_3B_ADDR_MAX) {
			ret = spi_nor_cmd_addr_read_4b(dev, SPI_NOR_CMD_READ_4B, addr, dest, size);
		} else {
			ret = spi_nor_cmd_addr_read_3b(dev, SPI_NOR_CMD_READ, addr, dest, size);
		}
	} else {
		ret = spi_nor_cmd_addr_read(dev, SPI_NOR_CMD_READ, addr, dest, size);
	}

	release_device(dev);

	/* Release flash power requirement */
	(void)pm_device_runtime_put_async(dev, K_MSEC(ACTIVE_DWELL_MS));
	return ret;
}

#if defined(CONFIG_FLASH_EX_OP_ENABLED)
static int flash_spi_nor_ex_op(const struct device *dev, uint16_t code,
			const uintptr_t in, void *out)
{
	int ret;

	ARG_UNUSED(in);
	ARG_UNUSED(out);

	if (pm_device_runtime_get(dev) < 0) {
		return -EIO;
	}

	acquire_device(dev);

	switch (code) {
	case FLASH_EX_OP_RESET:
		ret = spi_nor_cmd_write(dev, SPI_NOR_CMD_RESET_EN);
		if (ret == 0) {
			ret = spi_nor_cmd_write(dev, SPI_NOR_CMD_RESET_MEM);
		}
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	release_device(dev);
	(void)pm_device_runtime_put_async(dev, K_MSEC(ACTIVE_DWELL_MS));
	return ret;
}
#endif

static int spi_nor_write(const struct device *dev, off_t addr,
			 const void *src,
			 size_t size)
{
	const size_t flash_size = dev_flash_size(dev);
	const uint16_t page_size = dev_page_size(dev);
	int ret;

	/* should be between 0 and flash size */
	if ((addr < 0) || ((size + addr) > flash_size)) {
		return -EINVAL;
	}

	/* Ensure flash is powered before write */
	if (pm_device_runtime_get(dev) < 0) {
		return -EIO;
	}

	acquire_device(dev);
	ret = spi_nor_write_protection_set(dev, false);
	if (ret == 0) {
		while (size > 0) {
			size_t to_write = size;

			/* Don't write more than a page. */
			if (to_write >= page_size) {
				to_write = page_size;
			}

			/* Don't write across a page boundary */
			if (((addr + to_write - 1U) / page_size)
			!= (addr / page_size)) {
				to_write = page_size - (addr % page_size);
			}

			ret = spi_nor_cmd_write(dev, SPI_NOR_CMD_WREN);

			if (ret != 0) {
				break;
			}

			if (IS_ENABLED(ANY_INST_USE_4B_ADDR_OPCODES) &&
			    DEV_CFG(dev)->use_4b_addr_opcodes) {
				if (addr > SPI_NOR_3B_ADDR_MAX) {
					ret = spi_nor_cmd_addr_write_4b(dev, SPI_NOR_CMD_PP_4B,
									addr, src, to_write);
				} else {
					ret = spi_nor_cmd_addr_write_3b(dev, SPI_NOR_CMD_PP, addr,
									src, to_write);
				}
			} else {
				ret = spi_nor_cmd_addr_write(dev, SPI_NOR_CMD_PP, addr, src,
							     to_write);
			}

			if (ret != 0) {
				break;
			}

			size -= to_write;
			src = (const uint8_t *)src + to_write;
			addr += to_write;

			ret = spi_nor_wait_until_ready(dev, WAIT_READY_WRITE);
			if (ret != 0) {
				break;
			}
		}
	}

	int ret2 = spi_nor_write_protection_set(dev, true);

	if (!ret) {
		ret = ret2;
	}

	release_device(dev);

	/* Release flash power requirement */
	(void)pm_device_runtime_put_async(dev, K_MSEC(ACTIVE_DWELL_MS));
	return ret;
}

static int spi_nor_erase(const struct device *dev, off_t addr, size_t size)
{
	const size_t flash_size = dev_flash_size(dev);
	int ret;

	/* erase area must be subregion of device */
	if ((addr < 0) || ((size + addr) > flash_size)) {
		return -EINVAL;
	}

	/* address must be sector-aligned */
	if (!SPI_NOR_IS_SECTOR_ALIGNED(addr)) {
		return -EINVAL;
	}

	/* size must be a multiple of sectors */
	if ((size % SPI_NOR_SECTOR_SIZE) != 0) {
		return -EINVAL;
	}

	/* Ensure flash is powered before erase */
	if (pm_device_runtime_get(dev) < 0) {
		return -EIO;
	}

	acquire_device(dev);
	ret = spi_nor_write_protection_set(dev, false);

	while ((size > 0) && (ret == 0)) {
		ret = spi_nor_cmd_write(dev, SPI_NOR_CMD_WREN);
		if (ret) {
			break;
		}

		if (size == flash_size) {
			/* chip erase */
			ret = spi_nor_cmd_write(dev, SPI_NOR_CMD_CE);
			size -= flash_size;
		} else {
			const struct jesd216_erase_type *erase_types =
				dev_erase_types(dev);
			const struct jesd216_erase_type *bet = NULL;

			for (uint8_t ei = 0; ei < JESD216_NUM_ERASE_TYPES; ++ei) {
				const struct jesd216_erase_type *etp =
					&erase_types[ei];

				if ((etp->exp != 0)
				    && SPI_NOR_IS_ALIGNED(addr, etp->exp)
				    && (size >= BIT(etp->exp))
				    && ((bet == NULL)
					|| (etp->exp > bet->exp))) {
					bet = etp;
				}
			}
			if (bet != NULL) {
				if (IS_ENABLED(ANY_INST_USE_4B_ADDR_OPCODES) &&
				    DEV_CFG(dev)->use_4b_addr_opcodes) {
					ret = spi_nor_cmd_addr_write_4b(dev, bet->cmd, addr, NULL,
									0);
				} else {
					ret = spi_nor_cmd_addr_write(dev, bet->cmd, addr, NULL, 0);
				}
				addr += BIT(bet->exp);
				size -= BIT(bet->exp);
			} else {
				LOG_DBG("Can't erase %zu at 0x%lx",
					size, (long)addr);
				ret = -EINVAL;
			}
		}
		if (ret != 0) {
			break;
		}

		ret = spi_nor_wait_until_ready(dev, WAIT_READY_ERASE);
	}

	int ret2 = spi_nor_write_protection_set(dev, true);

	if (!ret) {
		ret = ret2;
	}

	release_device(dev);

	/* Release flash power requirement */
	(void)pm_device_runtime_put_async(dev, K_MSEC(ACTIVE_DWELL_MS));
	return ret;
}

/* @note The device must be externally acquired before invoking this
 * function.
 */
static int spi_nor_write_protection_set(const struct device *dev,
					bool write_protect)
{
	int ret;
	const struct spi_nor_config *cfg = dev->config;

#if ANY_INST_HAS_WP_GPIOS
	if (DEV_CFG(dev)->wp_gpios_exist && write_protect == false) {
		gpio_pin_set_dt(&(DEV_CFG(dev)->wp), 0);
	}
#endif

	ret = spi_nor_cmd_write(dev, (write_protect) ?
	      SPI_NOR_CMD_WRDI : SPI_NOR_CMD_WREN);

	if (cfg->requires_ulbpr_exist
	    && (ret == 0)
	    && !write_protect) {
		ret = spi_nor_cmd_write(dev, SPI_NOR_CMD_ULBPR);
	}

#if ANY_INST_HAS_WP_GPIOS
	if (DEV_CFG(dev)->wp_gpios_exist && write_protect == true) {
		gpio_pin_set_dt(&(DEV_CFG(dev)->wp), 1);
	}
#endif

	return ret;
}

#if defined(CONFIG_FLASH_JESD216_API) || defined(CONFIG_SPI_NOR_SFDP_RUNTIME)

static int spi_nor_sfdp_read(const struct device *dev, off_t addr,
			     void *dest, size_t size)
{
	if (pm_device_runtime_get(dev) < 0) {
		return -EIO;
	}

	acquire_device(dev);

	int ret = read_sfdp(dev, addr, dest, size);

	release_device(dev);

	(void)pm_device_runtime_put_async(dev, K_MSEC(ACTIVE_DWELL_MS));

	return ret;
}

#endif /* CONFIG_FLASH_JESD216_API || CONFIG_SPI_NOR_SFDP_RUNTIME */

static int spi_nor_read_jedec_id(const struct device *dev,
				 uint8_t *id)
{
	if (id == NULL) {
		return -EINVAL;
	}

	if (pm_device_runtime_get(dev) < 0) {
		return -EIO;
	}

	acquire_device(dev);

	int ret = spi_nor_cmd_read(dev, SPI_NOR_CMD_RDID, id, SPI_NOR_MAX_ID_LEN);

	release_device(dev);

	(void)pm_device_runtime_put_async(dev, K_MSEC(ACTIVE_DWELL_MS));

	return ret;
}

/* Put the device into the appropriate address mode, if supported.
 *
 * On successful return spi_nor_data::flag_access_32bit has been set
 * (cleared) if the device is configured for 4-byte (3-byte) addresses
 * for read, write, and erase commands.
 *
 * @param dev the device
 *
 * @param enter_4byte_addr the Enter 4-Byte Addressing bit set from
 * DW16 of SFDP BFP.  A value of all zeros or all ones is interpreted
 * as "not supported".
 *
 * @retval -ENOTSUP if 4-byte addressing is supported but not in a way
 * that the driver can handle.
 * @retval negative codes if the attempt was made and failed
 * @retval 0 if the device is successfully left in 24-bit mode or
 *         reconfigured to 32-bit mode.
 */
static int spi_nor_set_address_mode(const struct device *dev,
				    uint8_t enter_4byte_addr)
{
	int ret = 0;

	LOG_DBG("Checking enter-4byte-addr %02x", enter_4byte_addr);

	/* Do nothing if not provided (either no bits or all bits
	 * set).
	 */
	if ((enter_4byte_addr == 0)
	    || (enter_4byte_addr == 0xff)) {
		return 0;
	}

	/* This currently only supports command 0xB7 (Enter 4-Byte
	 * Address Mode), with or without preceding WREN.
	 */
	if ((enter_4byte_addr & 0x03) == 0) {
		return -ENOTSUP;
	}

	acquire_device(dev);

	if ((enter_4byte_addr & 0x02) != 0) {
		/* Enter after WREN. */
		ret = spi_nor_cmd_write(dev, SPI_NOR_CMD_WREN);
	}

	if (ret == 0) {
		ret = spi_nor_cmd_write(dev, SPI_NOR_CMD_4BA);

		if (ret == 0) {
			struct spi_nor_data *data = dev->data;

			data->flag_access_32bit = true;
		}
	}

	release_device(dev);

	return ret;
}

#ifndef CONFIG_SPI_NOR_SFDP_MINIMAL

static int spi_nor_process_bfp(const struct device *dev,
			       const struct jesd216_param_header *php,
			       const struct jesd216_bfp *bfp)
{
	struct spi_nor_data *data = dev->data;
	struct jesd216_erase_type *etp = data->erase_types;
	const size_t flash_size = jesd216_bfp_density(bfp) / 8U;

	LOG_INF("%s: %u %ciBy flash", dev->name,
		(flash_size < (1024U * 1024U)) ? (uint32_t)(flash_size >> 10)
					       : (uint32_t)(flash_size >> 20),
		(flash_size < (1024U * 1024U)) ? 'k' : 'M');

	/* Copy over the erase types, preserving their order.  (The
	 * Sector Map Parameter table references them by index.)
	 */
	memset(data->erase_types, 0, sizeof(data->erase_types));
	for (uint8_t ti = 1; ti <= ARRAY_SIZE(data->erase_types); ++ti) {
		if (jesd216_bfp_erase(bfp, ti, etp) == 0) {
			LOG_DBG("Erase %u with %02x", (uint32_t)BIT(etp->exp), etp->cmd);
		}
		++etp;
	}

	data->page_size = jesd216_bfp_page_size(php, bfp);
#ifdef CONFIG_SPI_NOR_SFDP_RUNTIME
	data->flash_size = flash_size;
#else /* CONFIG_SPI_NOR_SFDP_RUNTIME */
	if (flash_size != dev_flash_size(dev)) {
		LOG_ERR("BFP flash size mismatch with devicetree");
		return -EINVAL;
	}
#endif /* CONFIG_SPI_NOR_SFDP_RUNTIME */

	LOG_DBG("Page size %u bytes", data->page_size);

	/* If 4-byte addressing is supported, switch to it. */
	if (jesd216_bfp_addrbytes(bfp) != JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B) {
		struct jesd216_bfp_dw16 dw16;
		int rc = 0;

		if (IS_ENABLED(ANY_INST_USE_4B_ADDR_OPCODES) && DEV_CFG(dev)->use_4b_addr_opcodes) {
			LOG_DBG("4-byte addressing supported, using it via specific opcodes");
			return 0;
		}

		if (jesd216_bfp_decode_dw16(php, bfp, &dw16) == 0) {
			rc = spi_nor_set_address_mode(dev, dw16.enter_4ba);
		}

		if (rc != 0) {
			LOG_ERR("Unable to enter 4-byte mode: %d\n", rc);
			return rc;
		}
	}
	return 0;
}

static int spi_nor_process_sfdp(const struct device *dev)
{
	int rc;

#if defined(CONFIG_SPI_NOR_SFDP_RUNTIME)
	struct spi_nor_data *dev_data = dev->data;
	/* For runtime we need to read the SFDP table, identify the
	 * BFP block, and process it.
	 */
	const uint8_t decl_nph = 2;
	union {
		/* We only process BFP so use one parameter block */
		uint8_t raw[JESD216_SFDP_SIZE(decl_nph)];
		struct jesd216_sfdp_header sfdp;
	} u_header;
	const struct jesd216_sfdp_header *hp = &u_header.sfdp;

	rc = spi_nor_sfdp_read(dev, 0, u_header.raw, sizeof(u_header.raw));
	if (rc != 0) {
		LOG_ERR("SFDP read failed: %d", rc);
		return rc;
	}

	uint32_t magic = jesd216_sfdp_magic(hp);

	if (magic != JESD216_SFDP_MAGIC) {
		LOG_ERR("SFDP magic %08x invalid", magic);
		return -EINVAL;
	}

	LOG_INF("%s: SFDP v %u.%u AP %x with %u PH", dev->name,
		hp->rev_major, hp->rev_minor, hp->access, 1 + hp->nph);

	const struct jesd216_param_header *php = hp->phdr;
	const struct jesd216_param_header *phpe = php + MIN(decl_nph, 1 + hp->nph);

	while (php != phpe) {
		uint16_t id = jesd216_param_id(php);

		LOG_INF("PH%u: %04x rev %u.%u: %u DW @ %x",
			(php - hp->phdr), id, php->rev_major, php->rev_minor,
			php->len_dw, jesd216_param_addr(php));

		if (id == JESD216_SFDP_PARAM_ID_BFP) {
			union {
				uint32_t dw[MIN(php->len_dw, 20)];
				struct jesd216_bfp bfp;
			} u_param;
			const struct jesd216_bfp *bfp = &u_param.bfp;

			rc = spi_nor_sfdp_read(dev, jesd216_param_addr(php),
				u_param.dw, sizeof(u_param.dw));
			if (rc == 0) {
				rc = spi_nor_process_bfp(dev, php, bfp);
			}

			if (rc != 0) {
				LOG_INF("SFDP BFP failed: %d", rc);
				break;
			}
		}
		if (id == JESD216_SFDP_PARAM_ID_4B_ADDR_INSTR) {
			if (IS_ENABLED(ANY_INST_USE_4B_ADDR_OPCODES) &&
			    DEV_CFG(dev)->use_4b_addr_opcodes) {
				/*
				 * Check table 4 byte address instruction table to get supported
				 * erase opcodes when running in 4 byte address mode
				 */
				union {
					uint32_t dw[2];
					struct {
						uint32_t dummy;
						uint8_t type[4];
					} types;
				} u2;
				rc = spi_nor_sfdp_read(
					dev, jesd216_param_addr(php), (uint8_t *)u2.dw,
					MIN(sizeof(uint32_t) * php->len_dw, sizeof(u2.dw)));
				if (rc != 0) {
					break;
				}
				for (uint8_t ei = 0; ei < JESD216_NUM_ERASE_TYPES; ++ei) {
					struct jesd216_erase_type *etp = &dev_data->erase_types[ei];
					const uint8_t cmd = u2.types.type[ei];
					/* 0xff means not supported */
					if (cmd == 0xff) {
						etp->exp = 0;
						etp->cmd = 0;
					} else {
						etp->cmd = cmd;
					};
				}

				if (!((sys_le32_to_cpu(u2.dw[0]) & BIT(0)) &&
				      (sys_le32_to_cpu(u2.dw[1]) & BIT(6)))) {
					LOG_ERR("4-byte addressing not supported");
					return -ENOTSUP;
				}
			}
		}
		++php;
	}
#elif defined(CONFIG_SPI_NOR_SFDP_DEVICETREE)
	/* For devicetree we need to synthesize a parameter header and
	 * process the stored BFP data as if we had read it.
	 */
	const struct spi_nor_config *cfg = dev->config;
	struct jesd216_param_header bfp_hdr = {
		.len_dw = cfg->bfp_len,
	};

	rc = spi_nor_process_bfp(dev, &bfp_hdr, cfg->bfp);
#else
#error Unhandled SFDP choice
#endif

	return rc;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static int setup_pages_layout(const struct device *dev)
{
#if defined(CONFIG_SPI_NOR_SFDP_RUNTIME)
	struct spi_nor_data *data = dev->data;
	const size_t flash_size = dev_flash_size(dev);
	const uint32_t layout_page_size = CONFIG_SPI_NOR_FLASH_LAYOUT_PAGE_SIZE;
	uint8_t exp = 0;

	/* Find the smallest erase size. */
	for (size_t i = 0; i < ARRAY_SIZE(data->erase_types); ++i) {
		const struct jesd216_erase_type *etp = &data->erase_types[i];

		if ((etp->cmd != 0)
		    && ((exp == 0) || (etp->exp < exp))) {
			exp = etp->exp;
		}
	}

	if (exp == 0) {
		return -ENOTSUP;
	}

	uint32_t erase_size = BIT(exp);

	/* Error if layout page size is not a multiple of smallest
	 * erase size.
	 */
	if ((layout_page_size % erase_size) != 0) {
		LOG_ERR("layout page %u not compatible with erase size %u",
			layout_page_size, erase_size);
		return -EINVAL;
	}

	/* Warn but accept layout page sizes that leave inaccessible
	 * space.
	 */
	if ((flash_size % layout_page_size) != 0) {
		LOG_INF("layout page %u wastes space with device size %zu",
			layout_page_size, flash_size);
	}

	data->layout.pages_size = layout_page_size;
	data->layout.pages_count = flash_size / layout_page_size;
	LOG_DBG("layout %u x %u By pages", data->layout.pages_count, data->layout.pages_size);
#elif defined(CONFIG_SPI_NOR_SFDP_DEVICETREE)
	const struct spi_nor_config *cfg = dev->config;
	const struct flash_pages_layout *layout = &cfg->layout;
	const size_t flash_size = dev_flash_size(dev);
	size_t layout_size = layout->pages_size * layout->pages_count;

	if (flash_size != layout_size) {
		LOG_ERR("device size %u mismatch %zu * %zu By pages",
			flash_size, layout->pages_count, layout->pages_size);
		return -EINVAL;
	}
#else /* CONFIG_SPI_NOR_SFDP_RUNTIME */
#error Unhandled SFDP choice
#endif /* CONFIG_SPI_NOR_SFDP_RUNTIME */

	return 0;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
#endif /* CONFIG_SPI_NOR_SFDP_MINIMAL */

/**
 * @brief Configure the flash
 *
 * @param dev The flash device structure
 * @param info The flash info structure
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nor_configure(const struct device *dev)
{
	const struct spi_nor_config *cfg = dev->config;
	uint8_t jedec_id[SPI_NOR_MAX_ID_LEN];
	int rc;

	/* Validate bus and CS is ready */
	if (!spi_is_ready_dt(&cfg->spi)) {
		return -ENODEV;
	}

#if ANY_INST_HAS_RESET_GPIOS

	if (cfg->reset_gpios_exist) {
		if (!gpio_is_ready_dt(&cfg->reset)) {
			LOG_ERR("Reset pin not ready");
			return -ENODEV;
		}
		if (gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_ACTIVE)) {
			LOG_ERR("Couldn't configure reset pin");
			return -ENODEV;
		}
		rc = gpio_pin_set_dt(&cfg->reset, 0);
		if (rc) {
			return rc;
		}
	}
#endif

	/* After a soft-reset the flash might be in DPD or busy writing/erasing.
	 * Exit DPD and wait until flash is ready.
	 */
	acquire_device(dev);

	rc = exit_dpd(dev);
	if (rc < 0) {
		LOG_ERR("Failed to exit DPD (%d)", rc);
		release_device(dev);
		return -ENODEV;
	}

	rc = spi_nor_rdsr(dev);
	if (rc > 0 && (rc & SPI_NOR_WIP_BIT)) {
		LOG_WRN("Waiting until flash is ready");
		rc = spi_nor_wait_until_ready(dev, WAIT_READY_REGISTER);
	}
	release_device(dev);
	if (rc < 0) {
		LOG_ERR("Failed to wait until flash is ready (%d)", rc);
		return -ENODEV;
	}

	/* now the spi bus is configured, we can verify SPI
	 * connectivity by reading the JEDEC ID.
	 */

	rc = spi_nor_read_jedec_id(dev, jedec_id);
	if (rc != 0) {
		LOG_ERR("JEDEC ID read failed: %d", rc);
		return -ENODEV;
	}

#ifndef CONFIG_SPI_NOR_SFDP_RUNTIME
	/* For minimal and devicetree we need to check the JEDEC ID
	 * against the one from devicetree, to ensure we didn't find a
	 * device that has different parameters.
	 */

	if (memcmp(jedec_id, cfg->jedec_id, sizeof(jedec_id)) != 0) {
		LOG_ERR("Device id %02x %02x %02x does not match config %02x %02x %02x",
			jedec_id[0], jedec_id[1], jedec_id[2],
			cfg->jedec_id[0], cfg->jedec_id[1], cfg->jedec_id[2]);
		return -EINVAL;
	}
#endif

	/* Check for block protect bits that need to be cleared.  This
	 * information cannot be determined from SFDP content, so the
	 * devicetree node property must be set correctly for any device
	 * that powers up with block protect enabled.
	 */
	if (cfg->has_lock != 0) {
		acquire_device(dev);

		rc = spi_nor_rdsr(dev);

		/* Only clear if RDSR worked and something's set. */
		if (rc > 0) {
			rc = spi_nor_wrsr(dev, rc & ~cfg->has_lock);
		}

		release_device(dev);

		if (rc != 0) {
			LOG_ERR("BP clear failed: %d\n", rc);
			return -ENODEV;
		}
	}

#ifdef CONFIG_SPI_NOR_SFDP_MINIMAL
	/* For minimal we support some overrides from specific
	 * devicertee properties.
	 */
	if (cfg->enter_4byte_addr != 0) {
		rc = spi_nor_set_address_mode(dev, cfg->enter_4byte_addr);

		if (rc != 0) {
			LOG_ERR("Unable to enter 4-byte mode: %d\n", rc);
			return -ENODEV;
		}
	}

#else /* CONFIG_SPI_NOR_SFDP_MINIMAL */
	/* For devicetree and runtime we need to process BFP data and
	 * set up or validate page layout.
	 */
	rc = spi_nor_process_sfdp(dev);
	if (rc != 0) {
		LOG_ERR("SFDP read failed: %d", rc);
		return -ENODEV;
	}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	rc = setup_pages_layout(dev);
	if (rc != 0) {
		LOG_ERR("layout setup failed: %d", rc);
		return -ENODEV;
	}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
#endif /* CONFIG_SPI_NOR_SFDP_MINIMAL */

#if ANY_INST_HAS_MXICY_MX25R_POWER_MODE
	if (cfg->mxicy_mx25r_power_mode_exist) {
	/* Do not fail init if setting configuration register fails */
		(void)mxicy_configure(dev, jedec_id);
	}
#endif /* ANY_INST_HAS_MXICY_MX25R_POWER_MODE */

	return 0;
}

static int spi_nor_pm_control(const struct device *dev, enum pm_device_action action)
{
	int rc = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		acquire_device(dev);
		rc = enter_dpd(dev);
		release_device(dev);
		break;
	case PM_DEVICE_ACTION_RESUME:
		acquire_device(dev);
		rc = exit_dpd(dev);
		release_device(dev);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		/* Coming out of power off */
		rc = spi_nor_configure(dev);
		if (rc == 0) {
			/* Move to DPD, the correct device state
			 * for PM_DEVICE_STATE_SUSPENDED
			 */
			acquire_device(dev);
			rc = enter_dpd(dev);
			release_device(dev);
		}
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	default:
		rc = -ENOSYS;
	}

	return rc;
}

/**
 * @brief Initialize and configure the flash
 *
 * @param name The flash name
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nor_init(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct spi_nor_data *const driver_data = dev->data;

		k_sem_init(&driver_data->sem, 1, K_SEM_MAX_LIMIT);
	}

#if ANY_INST_HAS_WP_GPIOS
	if (DEV_CFG(dev)->wp_gpios_exist) {
		if (!device_is_ready(DEV_CFG(dev)->wp.port)) {
			LOG_ERR("Write-protect pin not ready");
			return -ENODEV;
		}
		if (gpio_pin_configure_dt(&(DEV_CFG(dev)->wp), GPIO_OUTPUT_ACTIVE)) {
			LOG_ERR("Write-protect pin failed to set active");
			return -ENODEV;
		}
	}
#endif /* ANY_INST_HAS_WP_GPIOS */
#if ANY_INST_HAS_HOLD_GPIOS
	if (DEV_CFG(dev)->hold_gpios_exist) {
		if (!device_is_ready(DEV_CFG(dev)->hold.port)) {
			LOG_ERR("Hold pin not ready");
			return -ENODEV;
		}
		if (gpio_pin_configure_dt(&(DEV_CFG(dev)->hold), GPIO_OUTPUT_INACTIVE)) {
			LOG_ERR("Hold pin failed to set inactive");
			return -ENODEV;
		}
	}
#endif /* ANY_INST_HAS_HOLD_GPIOS */

	return pm_device_driver_init(dev, spi_nor_pm_control);
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)

static void spi_nor_pages_layout(const struct device *dev,
				 const struct flash_pages_layout **layout,
				 size_t *layout_size)
{
	/* Data for runtime, const for devicetree and minimal. */
#ifdef CONFIG_SPI_NOR_SFDP_RUNTIME
	const struct spi_nor_data *data = dev->data;

	*layout = &data->layout;
#else /* CONFIG_SPI_NOR_SFDP_RUNTIME */
	const struct spi_nor_config *cfg = dev->config;

	*layout = &cfg->layout;
#endif /* CONFIG_SPI_NOR_SFDP_RUNTIME */

	*layout_size = 1;
}

#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *
flash_nor_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_nor_parameters;
}

static const struct flash_driver_api spi_nor_api = {
	.read = spi_nor_read,
	.write = spi_nor_write,
	.erase = spi_nor_erase,
	.get_parameters = flash_nor_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = spi_nor_pages_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = spi_nor_sfdp_read,
	.read_jedec_id = spi_nor_read_jedec_id,
#endif
#if defined(CONFIG_FLASH_EX_OP_ENABLED)
	.ex_op = flash_spi_nor_ex_op,
#endif
};

#define PAGE_LAYOUT_GEN(idx)								\
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(idx, size),					\
		"jedec,spi-nor size required for non-runtime SFDP page layout");	\
	enum {										\
		INST_##idx##_BYTES = (DT_INST_PROP(idx, size) / 8)			\
	};										\
	BUILD_ASSERT(SPI_NOR_IS_SECTOR_ALIGNED(CONFIG_SPI_NOR_FLASH_LAYOUT_PAGE_SIZE),	\
		"SPI_NOR_FLASH_LAYOUT_PAGE_SIZE must be multiple of 4096");		\
	enum {										\
		LAYOUT_PAGES_##idx##_COUNT =						\
			(INST_##idx##_BYTES / CONFIG_SPI_NOR_FLASH_LAYOUT_PAGE_SIZE)	\
	};										\
	BUILD_ASSERT((CONFIG_SPI_NOR_FLASH_LAYOUT_PAGE_SIZE * LAYOUT_PAGES_##idx##_COUNT) ==	\
			INST_##idx##_BYTES,						\
	     "SPI_NOR_FLASH_LAYOUT_PAGE_SIZE incompatible with flash size");

#define SFDP_BFP_ATTR_GEN(idx)							\
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(idx, sfdp_bfp),			\
		"jedec,spi-nor sfdp-bfp required for devicetree SFDP");		\
	static const __aligned(4) uint8_t bfp_##idx##_data[] = DT_INST_PROP(idx, sfdp_bfp);

#define INST_ATTR_GEN(idx)								\
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(idx, jedec_id),				\
		"jedec,spi-nor jedec-id required for non-runtime SFDP");		\
	IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT, (PAGE_LAYOUT_GEN(idx)))			\
	IF_ENABLED(CONFIG_SPI_NOR_SFDP_DEVICETREE, (SFDP_BFP_ATTR_GEN(idx)))

#define ATTRIBUTES_DEFINE(idx) COND_CODE_1(CONFIG_SPI_NOR_SFDP_RUNTIME, EMPTY(),	\
	(INST_ATTR_GEN(idx)))

#define DEFINE_PAGE_LAYOUT(idx)								\
	IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,						\
		   (.layout = {								\
			.pages_count = LAYOUT_PAGES_##idx##_COUNT,			\
			.pages_size = CONFIG_SPI_NOR_FLASH_LAYOUT_PAGE_SIZE,		\
		    },))

#define INST_HAS_LOCK(idx) DT_INST_NODE_HAS_PROP(idx, has_lock)

#define LOCK_DEFINE(idx)								\
	IF_ENABLED(INST_HAS_LOCK(idx), (BUILD_ASSERT(DT_INST_PROP(idx, has_lock) ==	\
					(DT_INST_PROP(idx, has_lock) & 0xFF),		\
					"Need support for lock clear beyond SR1");))

#define CONFIGURE_4BYTE_ADDR(idx) .enter_4byte_addr = DT_INST_PROP_OR(idx, enter_4byte_addr, 0),

#define INIT_T_ENTER_DPD(idx)								\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(idx, t_enter_dpd),				\
		(.t_enter_dpd =								\
			DIV_ROUND_UP(DT_INST_PROP(idx, t_enter_dpd), NSEC_PER_MSEC)),\
		(.t_enter_dpd = 0))

#if ANY_INST_HAS_T_EXIT_DPD
#define INIT_T_EXIT_DPD(idx)								\
	COND_CODE_1(									\
		DT_INST_NODE_HAS_PROP(idx, t_exit_dpd),					\
		(.t_exit_dpd = DIV_ROUND_UP(DT_INST_PROP(idx, t_exit_dpd), NSEC_PER_MSEC)),\
		(.t_exit_dpd = 0))
#endif

#define INIT_WP_GPIOS(idx) .wp = GPIO_DT_SPEC_INST_GET_OR(idx, wp_gpios, {0})

#define INIT_HOLD_GPIOS(idx) .hold = GPIO_DT_SPEC_INST_GET_OR(idx, hold_gpios, {0})

#define INIT_WAKEUP_SEQ_PARAMS(idx)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(idx, dpd_wakeup_sequence),			\
		(.t_dpdd_ms = DIV_ROUND_UP(						\
			DT_INST_PROP_BY_IDX(idx, dpd_wakeup_sequence, 0), NSEC_PER_MSEC),\
		.t_crdp_ms = DIV_ROUND_UP(						\
			DT_INST_PROP_BY_IDX(idx, dpd_wakeup_sequence, 1), NSEC_PER_MSEC),\
		.t_rdp_ms = DIV_ROUND_UP(						\
			DT_INST_PROP_BY_IDX(idx, dpd_wakeup_sequence, 2), NSEC_PER_MSEC)),\
		(.t_dpdd_ms = 0, .t_crdp_ms = 0, .t_rdp_ms = 0))

#define INIT_MXICY_MX25R_POWER_MODE(idx)							\
	.mxicy_mx25r_power_mode = DT_INST_ENUM_IDX_OR(idx, mxicy_mx25r_power_mode, 0)

#define INIT_RESET_GPIOS(idx) .reset = GPIO_DT_SPEC_INST_GET_OR(idx, reset_gpios, {0})

#define INST_CONFIG_STRUCT_GEN(idx)								\
	DEFINE_PAGE_LAYOUT(idx)									\
	.flash_size = DT_INST_PROP(idx, size) / 8,						\
	.jedec_id = DT_INST_PROP(idx, jedec_id),						\
	IF_ENABLED(CONFIG_SPI_NOR_SFDP_MINIMAL, (CONFIGURE_4BYTE_ADDR(idx)))			\
	IF_ENABLED(CONFIG_SPI_NOR_SFDP_DEVICETREE,						\
		(.bfp_len = sizeof(bfp_##idx##_data) / 4,					\
		 .bfp = (const struct jesd216_bfp *)bfp_##idx##_data,))

#define GENERATE_CONFIG_STRUCT(idx)								\
	static const struct spi_nor_config spi_nor_##idx##_config = {				\
		.spi = SPI_DT_SPEC_INST_GET(idx, SPI_WORD_SET(8), CONFIG_SPI_NOR_CS_WAIT_DELAY),\
		.dpd_exist = DT_INST_PROP(idx, has_dpd),					\
		.dpd_wakeup_sequence_exist = DT_INST_NODE_HAS_PROP(idx, dpd_wakeup_sequence),	\
		.mxicy_mx25r_power_mode_exist =							\
			DT_INST_NODE_HAS_PROP(idx, mxicy_mx25r_power_mode),			\
		.reset_gpios_exist = DT_INST_NODE_HAS_PROP(idx, reset_gpios),			\
		.requires_ulbpr_exist = DT_INST_PROP(idx, requires_ulbpr),			\
		.wp_gpios_exist = DT_INST_NODE_HAS_PROP(idx, wp_gpios),				\
		.hold_gpios_exist = DT_INST_NODE_HAS_PROP(idx, hold_gpios),			\
		.use_4b_addr_opcodes = DT_INST_PROP(idx, use_4b_addr_opcodes),			\
		IF_ENABLED(INST_HAS_LOCK(idx), (.has_lock = DT_INST_PROP(idx, has_lock),))	\
		IF_ENABLED(ANY_INST_HAS_DPD, (INIT_T_ENTER_DPD(idx),))				\
		IF_ENABLED(UTIL_AND(ANY_INST_HAS_DPD, ANY_INST_HAS_T_EXIT_DPD),			\
			(INIT_T_EXIT_DPD(idx),))						\
		IF_ENABLED(ANY_INST_HAS_DPD_WAKEUP_SEQUENCE, (INIT_WAKEUP_SEQ_PARAMS(idx),))	\
		IF_ENABLED(ANY_INST_HAS_MXICY_MX25R_POWER_MODE,					\
			(INIT_MXICY_MX25R_POWER_MODE(idx),))					\
		IF_ENABLED(ANY_INST_HAS_RESET_GPIOS, (INIT_RESET_GPIOS(idx),))			\
		IF_ENABLED(ANY_INST_HAS_WP_GPIOS, (INIT_WP_GPIOS(idx),))			\
		IF_ENABLED(ANY_INST_HAS_HOLD_GPIOS, (INIT_HOLD_GPIOS(idx),))			\
		IF_DISABLED(CONFIG_SPI_NOR_SFDP_RUNTIME, (INST_CONFIG_STRUCT_GEN(idx)))};

#define ASSIGN_PM(idx)							\
		PM_DEVICE_DT_INST_DEFINE(idx, spi_nor_pm_control);

#define SPI_NOR_INST(idx)							\
	ASSIGN_PM(idx)								\
	ATTRIBUTES_DEFINE(idx)							\
	LOCK_DEFINE(idx)							\
	GENERATE_CONFIG_STRUCT(idx)						\
	static struct spi_nor_data spi_nor_##idx##_data;			\
	DEVICE_DT_INST_DEFINE(idx, &spi_nor_init, PM_DEVICE_DT_INST_GET(idx),	\
			&spi_nor_##idx##_data, &spi_nor_##idx##_config,		\
			POST_KERNEL, CONFIG_SPI_NOR_INIT_PRIORITY, &spi_nor_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_NOR_INST)
