/*
 * Copyright (c) 2018 Savoir-Faire Linux.
 * Copyright (c) 2020 Peter Bigot Consulting, LLC
 *
 * This driver is heavily inspired from the spi_flash_w25qxxdv.c SPI NOR driver.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT jedec_spi_nor

#include <errno.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/init.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>

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
 * The power reduction from DPD is sufficient to warrant allowing its
 * use even in cases where Zephyr's device power management is not
 * available.  This is selected through the SPI_NOR_IDLE_IN_DPD
 * Kconfig option.
 *
 * When mapped to the Zephyr Device Power Management states:
 * * PM_DEVICE_STATE_ACTIVE covers both active and standby modes;
 * * PM_DEVICE_STATE_SUSPENDED, and PM_DEVICE_STATE_OFF all correspond to
 *   deep-power-down mode.
 */

#define SPI_NOR_MAX_ADDR_WIDTH 4

#if DT_INST_NODE_HAS_PROP(0, t_enter_dpd)
#define T_DP_MS ceiling_fraction(DT_INST_PROP(0, t_enter_dpd), NSEC_PER_MSEC)
#else /* T_ENTER_DPD */
#define T_DP_MS 0
#endif /* T_ENTER_DPD */
#if DT_INST_NODE_HAS_PROP(0, t_exit_dpd)
#define T_RES1_MS ceiling_fraction(DT_INST_PROP(0, t_exit_dpd), NSEC_PER_MSEC)
#endif /* T_EXIT_DPD */
#if DT_INST_NODE_HAS_PROP(0, dpd_wakeup_sequence)
#define T_DPDD_MS ceiling_fraction(DT_INST_PROP_BY_IDX(0, dpd_wakeup_sequence, 0), NSEC_PER_MSEC)
#define T_CRDP_MS ceiling_fraction(DT_INST_PROP_BY_IDX(0, dpd_wakeup_sequence, 1), NSEC_PER_MSEC)
#define T_RDP_MS ceiling_fraction(DT_INST_PROP_BY_IDX(0, dpd_wakeup_sequence, 2), NSEC_PER_MSEC)
#else /* DPD_WAKEUP_SEQUENCE */
#define T_DPDD_MS 0
#endif /* DPD_WAKEUP_SEQUENCE */

/* Build-time data associated with the device. */
struct spi_nor_config {
	/* Devicetree SPI configuration */
	struct spi_dt_spec spi;

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
};

/**
 * struct spi_nor_data - Structure for defining the SPI NOR access
 * @sem: The semaphore to access to the flash
 */
struct spi_nor_data {
	struct k_sem sem;
#if DT_INST_NODE_HAS_PROP(0, has_dpd)
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
#endif /* CONFIG_SPI_NOR_SFDP_MINIMAL */

static int spi_nor_write_protection_set(const struct device *dev,
					bool write_protect);

/* Get pointer to array of supported erase types.  Static const for
 * minimal, data for runtime and devicetree.
 */
static inline const struct jesd216_erase_type *
dev_erase_types(const struct device *dev)
{
#ifdef CONFIG_SPI_NOR_SFDP_MINIMAL
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
	return 256;
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
#if DT_INST_NODE_HAS_PROP(0, has_dpd)
	struct spi_nor_data *const driver_data = dev->data;

	driver_data->ts_enter_dpd = k_uptime_get_32();
#endif
}

/* Check the current time against the time DPD was entered and delay
 * until it's ok to initiate the DPD exit process.
 */
static inline void delay_until_exit_dpd_ok(const struct device *const dev)
{
#if DT_INST_NODE_HAS_PROP(0, has_dpd)
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
		since -= T_DP_MS;

		/* Subtract time required in DPD before exit */
		since -= T_DPDD_MS;

		/* If the adjusted time is negative we have to wait
		 * until it reaches zero before we can proceed.
		 */
		if (since < 0) {
			k_sleep(K_MSEC((uint32_t)-since));
		}
	}
#endif /* DT_INST_NODE_HAS_PROP(0, has_dpd) */
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
#define spi_nor_cmd_write(dev, opcode) \
	spi_nor_access(dev, opcode, NOR_ACCESS_WRITE, 0, NULL, 0)
#define spi_nor_cmd_addr_write(dev, opcode, addr, src, length) \
	spi_nor_access(dev, opcode, NOR_ACCESS_WRITE | NOR_ACCESS_ADDRESSED, \
		       addr, (void *)src, length)

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
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nor_wait_until_ready(const struct device *dev)
{
	int ret;
	uint8_t reg;

	do {
		ret = spi_nor_cmd_read(dev, SPI_NOR_CMD_RDSR, &reg, sizeof(reg));
	} while (!ret && (reg & SPI_NOR_WIP_BIT));

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

	if (IS_ENABLED(DT_INST_PROP(0, has_dpd))) {
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

	if (IS_ENABLED(DT_INST_PROP(0, has_dpd))) {
		delay_until_exit_dpd_ok(dev);

#if DT_INST_NODE_HAS_PROP(0, dpd_wakeup_sequence)
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
		k_sleep(K_MSEC(T_RDP_MS));
#else /* DPD_WAKEUP_SEQUENCE */
		ret = spi_nor_cmd_write(dev, SPI_NOR_CMD_RDPD);

		if (ret == 0) {
#if DT_INST_NODE_HAS_PROP(0, t_exit_dpd)
			k_sleep(K_MSEC(T_RES1_MS));
#endif /* T_EXIT_DPD */
		}
#endif /* DPD_WAKEUP_SEQUENCE */
	}
	return ret;
}

/* Everything necessary to acquire owning access to the device.
 *
 * This means taking the lock and, if necessary, waking the device
 * from deep power-down mode.
 */
static void acquire_device(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct spi_nor_data *const driver_data = dev->data;

		k_sem_take(&driver_data->sem, K_FOREVER);
	}

	if (IS_ENABLED(CONFIG_SPI_NOR_IDLE_IN_DPD)) {
		exit_dpd(dev);
	}
}

/* Everything necessary to release access to the device.
 *
 * This means (optionally) putting the device into deep power-down
 * mode, and releasing the lock.
 */
static void release_device(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_SPI_NOR_IDLE_IN_DPD)) {
		enter_dpd(dev);
	}

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

	if (ret == 0) {
		ret = spi_nor_access(dev, SPI_NOR_CMD_WRSR, NOR_ACCESS_WRITE, 0, &sr,
				     sizeof(sr));
		spi_nor_wait_until_ready(dev);
	}

	return ret;
}

static int spi_nor_read(const struct device *dev, off_t addr, void *dest,
			size_t size)
{
	const size_t flash_size = dev_flash_size(dev);
	int ret;

	/* should be between 0 and flash size */
	if ((addr < 0) || ((addr + size) > flash_size)) {
		return -EINVAL;
	}

	acquire_device(dev);

	ret = spi_nor_cmd_addr_read(dev, SPI_NOR_CMD_READ, addr, dest, size);

	release_device(dev);
	return ret;
}

static int spi_nor_write(const struct device *dev, off_t addr,
			 const void *src,
			 size_t size)
{
	const size_t flash_size = dev_flash_size(dev);
	const uint16_t page_size = dev_page_size(dev);
	int ret = 0;

	/* should be between 0 and flash size */
	if ((addr < 0) || ((size + addr) > flash_size)) {
		return -EINVAL;
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

			spi_nor_cmd_write(dev, SPI_NOR_CMD_WREN);
			ret = spi_nor_cmd_addr_write(dev, SPI_NOR_CMD_PP, addr,
						src, to_write);
			if (ret != 0) {
				break;
			}

			size -= to_write;
			src = (const uint8_t *)src + to_write;
			addr += to_write;

			spi_nor_wait_until_ready(dev);
		}
	}

	int ret2 = spi_nor_write_protection_set(dev, true);

	if (!ret) {
		ret = ret2;
	}

	release_device(dev);
	return ret;
}

static int spi_nor_erase(const struct device *dev, off_t addr, size_t size)
{
	const size_t flash_size = dev_flash_size(dev);
	int ret = 0;

	/* erase area must be subregion of device */
	if ((addr < 0) || ((size + addr) > flash_size)) {
		return -ENODEV;
	}

	/* address must be sector-aligned */
	if (!SPI_NOR_IS_SECTOR_ALIGNED(addr)) {
		return -EINVAL;
	}

	/* size must be a multiple of sectors */
	if ((size % SPI_NOR_SECTOR_SIZE) != 0) {
		return -EINVAL;
	}

	acquire_device(dev);
	ret = spi_nor_write_protection_set(dev, false);

	while ((size > 0) && (ret == 0)) {
		spi_nor_cmd_write(dev, SPI_NOR_CMD_WREN);

		if (size == flash_size) {
			/* chip erase */
			spi_nor_cmd_write(dev, SPI_NOR_CMD_CE);
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
				    && SPI_NOR_IS_ALIGNED(size, etp->exp)
				    && ((bet == NULL)
					|| (etp->exp > bet->exp))) {
					bet = etp;
				}
			}
			if (bet != NULL) {
				spi_nor_cmd_addr_write(dev, bet->cmd, addr, NULL, 0);
				addr += BIT(bet->exp);
				size -= BIT(bet->exp);
			} else {
				LOG_DBG("Can't erase %zu at 0x%lx",
					size, (long)addr);
				ret = -EINVAL;
			}
		}

#ifdef __XCC__
		/*
		 * FIXME: remove this hack once XCC is fixed.
		 *
		 * Without this volatile return value, XCC would segfault
		 * compiling this file complaining about failure in CGPREP
		 * phase.
		 */
		volatile int xcc_ret =
#endif
		spi_nor_wait_until_ready(dev);
	}

	int ret2 = spi_nor_write_protection_set(dev, true);

	if (!ret) {
		ret = ret2;
	}

	release_device(dev);

	return ret;
}

/* @note The device must be externally acquired before invoking this
 * function.
 */
static int spi_nor_write_protection_set(const struct device *dev,
					bool write_protect)
{
	int ret;

	ret = spi_nor_cmd_write(dev, (write_protect) ?
	      SPI_NOR_CMD_WRDI : SPI_NOR_CMD_WREN);

	if (IS_ENABLED(DT_INST_PROP(0, requires_ulbpr))
	    && (ret == 0)
	    && !write_protect) {
		ret = spi_nor_cmd_write(dev, SPI_NOR_CMD_ULBPR);
	}

	return ret;
}

#if defined(CONFIG_FLASH_JESD216_API)

static int spi_nor_sfdp_read(const struct device *dev, off_t addr,
			     void *dest, size_t size)
{
	acquire_device(dev);

	int ret = read_sfdp(dev, addr, dest, size);

	release_device(dev);

	return ret;
}

#endif /* CONFIG_FLASH_JESD216_API */

static int spi_nor_read_jedec_id(const struct device *dev,
				 uint8_t *id)
{
	if (id == NULL) {
		return -EINVAL;
	}

	acquire_device(dev);

	int ret = spi_nor_cmd_read(dev, SPI_NOR_CMD_RDID, id, SPI_NOR_MAX_ID_LEN);

	release_device(dev);

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

	/* Do nothing if not provided (either no bits or all bits
	 * set).
	 */
	if ((enter_4byte_addr == 0)
	    || (enter_4byte_addr == 0xff)) {
		return 0;
	}

	LOG_DBG("Checking enter-4byte-addr %02x", enter_4byte_addr);

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
	}

	if (ret == 0) {
		struct spi_nor_data *data = dev->data;

		data->flag_access_32bit = true;
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

	LOG_INF("%s: %u MiBy flash", dev->name, (uint32_t)(flash_size >> 20));

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
	/* For runtime we need to read the SFDP table, identify the
	 * BFP block, and process it.
	 */
	const uint8_t decl_nph = 2;
	union {
		/* We only process BFP so use one parameter block */
		uint8_t raw[JESD216_SFDP_SIZE(decl_nph)];
		struct jesd216_sfdp_header sfdp;
	} u;
	const struct jesd216_sfdp_header *hp = &u.sfdp;

	rc = spi_nor_sfdp_read(dev, 0, u.raw, sizeof(u.raw));
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
			} u;
			const struct jesd216_bfp *bfp = &u.bfp;

			rc = spi_nor_sfdp_read(dev, jesd216_param_addr(php), u.dw, sizeof(u.dw));
			if (rc == 0) {
				rc = spi_nor_process_bfp(dev, php, bfp);
			}

			if (rc != 0) {
				LOG_INF("SFDP BFP failed: %d", rc);
				break;
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
	int rv = 0;

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

	return rv;
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
	if (!spi_is_ready(&cfg->spi)) {
		return -ENODEV;
	}

	/* Might be in DPD if system restarted without power cycle. */
	exit_dpd(dev);

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

		if (rc != 0) {
			LOG_ERR("BP clear failed: %d\n", rc);
			return -ENODEV;
		}

		release_device(dev);
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

	if (IS_ENABLED(CONFIG_SPI_NOR_IDLE_IN_DPD)
	    && (enter_dpd(dev) != 0)) {
		return -ENODEV;
	}

	return 0;
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

	return spi_nor_configure(dev);
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
};

#ifndef CONFIG_SPI_NOR_SFDP_RUNTIME
/* We need to know the size and ID of the configuration data we're
 * using so we can disable the device we see at runtime if it isn't
 * compatible with what we're taking from devicetree or minimal.
 */
BUILD_ASSERT(DT_INST_NODE_HAS_PROP(0, jedec_id),
	     "jedec,spi-nor jedec-id required for non-runtime SFDP");

#if defined(CONFIG_FLASH_PAGE_LAYOUT)

/* For devicetree or minimal page layout we need to know the size of
 * the device.  We can't extract it from the raw BFP data, so require
 * it to be present in devicetree.
 */
BUILD_ASSERT(DT_INST_NODE_HAS_PROP(0, size),
	     "jedec,spi-nor size required for non-runtime SFDP page layout");

/* instance 0 size in bytes */
#define INST_0_BYTES (DT_INST_PROP(0, size) / 8)

BUILD_ASSERT(SPI_NOR_IS_SECTOR_ALIGNED(CONFIG_SPI_NOR_FLASH_LAYOUT_PAGE_SIZE),
	     "SPI_NOR_FLASH_LAYOUT_PAGE_SIZE must be multiple of 4096");

/* instance 0 page count */
#define LAYOUT_PAGES_COUNT (INST_0_BYTES / CONFIG_SPI_NOR_FLASH_LAYOUT_PAGE_SIZE)

BUILD_ASSERT((CONFIG_SPI_NOR_FLASH_LAYOUT_PAGE_SIZE * LAYOUT_PAGES_COUNT)
	     == INST_0_BYTES,
	     "SPI_NOR_FLASH_LAYOUT_PAGE_SIZE incompatible with flash size");

#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#ifdef CONFIG_SPI_NOR_SFDP_DEVICETREE
BUILD_ASSERT(DT_INST_NODE_HAS_PROP(0, sfdp_bfp),
	     "jedec,spi-nor sfdp-bfp required for devicetree SFDP");

static const __aligned(4) uint8_t bfp_data_0[] = DT_INST_PROP(0, sfdp_bfp);
#endif /* CONFIG_SPI_NOR_SFDP_DEVICETREE */

#endif /* CONFIG_SPI_NOR_SFDP_RUNTIME */

#if DT_INST_NODE_HAS_PROP(0, has_lock)
/* Currently we only know of devices where the BP bits are present in
 * the first byte of the status register.  Complain if that changes.
 */
BUILD_ASSERT(DT_INST_PROP(0, has_lock) == (DT_INST_PROP(0, has_lock) & 0xFF),
	     "Need support for lock clear beyond SR1");
#endif

static const struct spi_nor_config spi_nor_config_0 = {
	.spi = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8),
				    CONFIG_SPI_NOR_CS_WAIT_DELAY),
#if !defined(CONFIG_SPI_NOR_SFDP_RUNTIME)

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.layout = {
		.pages_count = LAYOUT_PAGES_COUNT,
		.pages_size = CONFIG_SPI_NOR_FLASH_LAYOUT_PAGE_SIZE,
	},
#undef LAYOUT_PAGES_COUNT
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

	.flash_size = DT_INST_PROP(0, size) / 8,
	.jedec_id = DT_INST_PROP(0, jedec_id),

#if DT_INST_NODE_HAS_PROP(0, has_lock)
	.has_lock = DT_INST_PROP(0, has_lock),
#endif
#if defined(CONFIG_SPI_NOR_SFDP_MINIMAL)		\
	&& DT_INST_NODE_HAS_PROP(0, enter_4byte_addr)
	.enter_4byte_addr = DT_INST_PROP(0, enter_4byte_addr),
#endif
#ifdef CONFIG_SPI_NOR_SFDP_DEVICETREE
	.bfp_len = sizeof(bfp_data_0) / 4,
	.bfp = (const struct jesd216_bfp *)bfp_data_0,
#endif /* CONFIG_SPI_NOR_SFDP_DEVICETREE */

#endif /* CONFIG_SPI_NOR_SFDP_RUNTIME */
};

static struct spi_nor_data spi_nor_data_0;

DEVICE_DT_INST_DEFINE(0, &spi_nor_init, NULL,
		 &spi_nor_data_0, &spi_nor_config_0,
		 POST_KERNEL, CONFIG_SPI_NOR_INIT_PRIORITY,
		 &spi_nor_api);
