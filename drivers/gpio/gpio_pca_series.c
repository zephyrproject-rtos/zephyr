/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Driver for PCA(L)xxxx SERIES I2C-based GPIO expander.
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_pca_series);

/** Private debug macro, enable more error checking and print more log. */
/* #define GPIO_NXP_PCA_SERIES_DEBUG */

/* Feature flags */
#define PCA_HAS_LATCH	   BIT(0)	/** + output_drive_strength, + input_latch */
#define PCA_HAS_PULL	   BIT(1)	/** + pull_enable, + pull_select */
#define PCA_HAS_INT_MASK   BIT(2)	/** + interrupt_mask, + int_status */
#define PCA_HAS_INT_EXTEND BIT(3)	/** + interrupt_edge, + interrupt_clear */
#define PCA_HAS_OUT_CONFIG BIT(4)	/** + input_status, + output_config */

/* get port and pin from gpio_pin_t */
#define PCA_PORT(gpio_pin) (gpio_pin >> 3)
#define PCA_PIN(gpio_pin)  (gpio_pin & GENMASK(2, 0))

#define PCA_REG_INVALID (0xff)

/**
 * @brief part number definition.
 */
enum gpio_pca_series_part_no {
	PCA_PART_NO_PCA9538,
	PCA_PART_NO_PCA9539,
	PCA_PART_NO_PCA9554,
	PCA_PART_NO_PCA9555,
	PCA_PART_NO_PCAL6524,
	PCA_PART_NO_PCAL6534,
};

/**
 * @brief part name definition for debug.
 *
 * @note must be consistent in order with @ref enum gpio_pca_series_part_no
 */
const char *const gpio_pca_series_part_name[] = {
	"pca9538",
	"pca9539",
	"pca9554",
	"pca9555",
	"pcal6524",
	"pcal6534",
};

/**
 * Device reg layout types:
 * - Type 0: PCA953X, PCA955X
 * - Type 1: PCAL953X, PCAL955X, PCAL64XXA
 * - Type 2: PCA957X
 * - Type 3: PCAL65XX
 */

enum gpio_pca_series_reg_type {			/** Type0 Type1 Type2 Type3 */
	PCA_REG_TYPE_1B_INPUT_PORT = 0U,	/**   x     x     x     x   */
	PCA_REG_TYPE_1B_OUTPUT_PORT,		/**   x     x     x     x   */
/*  PCA_REG_TYPE_1B_POLARITY_INVERSION,		      x     x     x     x   * (unused, omitted) */
	PCA_REG_TYPE_1B_CONFIGURATION,		/**   x     x     x     x   */
	PCA_REG_TYPE_2B_OUTPUT_DRIVE_STRENGTH,	/**         x           x   */
	PCA_REG_TYPE_1B_INPUT_LATCH,		/**         x           x   */
	PCA_REG_TYPE_1B_PULL_ENABLE,		/**         x     x*1   x   */
	PCA_REG_TYPE_1B_PULL_SELECT,		/**         x     x     x   */
	PCA_REG_TYPE_1B_INPUT_STATUS,		/**                     x   */
	PCA_REG_TYPE_1B_OUTPUT_CONFIG,		/**                     x*2 */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT		/**                         */
	PCA_REG_TYPE_1B_INTERRUPT_MASK,		/**         x     x     x   */
	PCA_REG_TYPE_1B_INTERRUPT_STATUS,	/**         x     x     x   */
	PCA_REG_TYPE_2B_INTERRUPT_EDGE,		/**                     x   */
	PCA_REG_TYPE_1B_INTERRUPT_CLEAR,	/**                     x   */
# ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	PCA_REG_TYPE_1B_INPUT_HISTORY,		/**   x     x     x         * (cache registry) */
	PCA_REG_TYPE_1B_INTERRUPT_RISE,		/**   x     x     x         * (cache registry) */
	PCA_REG_TYPE_1B_INTERRUPT_FALL,		/**   x     x     x         * (cache registry) */
# endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT				    */
	PCA_REG_TYPE_COUNT,			/** not a register          */
};
/**
 * #1: "pull_enable" register is named "bus_hold" in PCA957x datasheet.
 * #2: this is for "individual pin output configuration register". We do not use
 *     port-level "pin output configuration" register.
 */

const char *const gpio_pca_series_reg_name[] = {
	"1b_input_port",
	"1b_output_port",
/*  "1b_polarity_inversion," */
	"1b_configuration",
	"2b_output_drive_strength",
	"1b_input_latch",
	"1b_pull_enable",
	"1b_pull_select",
	"1b_input_status",
	"1b_output_config",
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	"1b_interrupt_mask",
	"1b_interrupt_status",
	"2b_interrupt_edge",
	"1b_interrupt_clear",
# ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	"1b_input_history",
	"1b_interrupt_rise",
	"ib_interrupt_fall",
# endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
	"reg_end",
};

/**
 * @brief interrupt config for interrupt_edge register
 *
 * @note Only applies to part no with PCA_HAS_INT_EXTEND capability.
 */
enum PCA_INTERRUPT_config_extended {
	PCA_INTERRUPT_LEVEL_CHANGE = 0U, /* default */
	PCA_INTERRUPT_RISING_EDGE,
	PCA_INTERRUPT_FALLING_EDGE,
	PCA_INTERRUPT_EITHER_EDGE,
};

struct gpio_pca_series_part_config {
	uint8_t port_no;                      /* number of 8-pin ports on device */
	uint8_t flags;                        /* capability flags */
	const uint8_t *regs;                  /* pointer to register map */
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
# ifdef GPIO_NXP_PCA_SERIES_DEBUG
	uint8_t cache_size;
# endif /* GPIO_NXP_PCA_SERIES_DEBUG */
	const uint8_t *cache_map;
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
};

/** Configuration data */
struct gpio_pca_series_config {
	struct gpio_driver_config common; /* gpio_driver_config needs to be first */
	struct i2c_dt_spec i2c;           /* i2c bus dt spec */
	const struct gpio_pca_series_part_config *part_cfg; /* config of part unmber */
	struct gpio_dt_spec gpio_rst;                       /* device reset gpio */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	struct gpio_dt_spec gpio_int; /** device interrupt gpio */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};

/** Runtime driver data */
struct gpio_pca_series_data {
	struct gpio_driver_data common; /** gpio_driver_data needs to be first */
	struct k_sem lock;
	void *cache;  /** device spicific reg cache
			*  - if CONFIG_GPIO_PCA_SERIES_CACHE_ALL is set,
			*    it points to device specific cache memory.
			*  - if CONFIG_GPIO_PCA_SERIES_CACHE_ALL is not set,
			*    it point to a struct gpio_pca_series_reg_cache_mini
			*    instance.
			*/
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	const struct device *self;    /** Self-reference to the driver instance */
	struct gpio_callback gpio_cb; /** gpio_int ISR callback */
	sys_slist_t callbacks;        /** port pin callbacks list */
	struct k_work int_work;       /** worker that fire callbacks */
#endif
};

/**
 * gpio_pca_reg_access_api
 * {
 */

/**
 * @brief get internal address of register from register type
 *
 * @param dev device struct
 * @param reg_type value from enum gpio_pca_series_reg_type
 * @return uint8_t PCA_REG_INVALID      if reg is not used
 *                 other                internal address of register
 */
static inline uint8_t gpio_pca_series_reg_get_addr(const struct device *dev,
						   enum gpio_pca_series_reg_type reg_type)
{
	const struct gpio_pca_series_config *cfg = dev->config;

#ifdef GPIO_NXP_PCA_SERIES_DEBUG
	if (reg_type >= PCA_REG_TYPE_COUNT) {
		LOG_ERR("reg_type %d out of range", reg_type);
		return 0;
	}
#endif /* GPIO_NXP_PCA_SERIES_DEBUG */

	return cfg->part_cfg->regs[reg_type];
}

/**
 * @brief get per-port size for register
 *
 * @param dev device struct
 * @param reg_type value from enum gpio_pca_series_reg_type
 * @return uint32_t size in bytes
 *                  0 if fail
 */
static inline uint32_t gpio_pca_series_reg_size_per_port(const struct device *dev,
						enum gpio_pca_series_reg_type reg_type)
{
#ifdef GPIO_NXP_PCA_SERIES_DEBUG
	if (reg_type >= PCA_REG_TYPE_COUNT) {
		LOG_ERR("reg_type %d out of range", reg_type);
		return 0;
	}
#endif /* GPIO_NXP_PCA_SERIES_DEBUG */

	switch (reg_type) {
	case PCA_REG_TYPE_1B_INPUT_PORT:
	case PCA_REG_TYPE_1B_OUTPUT_PORT:
/*  case PCA_REG_TYPE_1B_POLARITY_INVERSION: */
	case PCA_REG_TYPE_1B_CONFIGURATION:
	case PCA_REG_TYPE_1B_INPUT_LATCH:
	case PCA_REG_TYPE_1B_PULL_ENABLE:
	case PCA_REG_TYPE_1B_PULL_SELECT:
	case PCA_REG_TYPE_1B_INPUT_STATUS:
	case PCA_REG_TYPE_1B_OUTPUT_CONFIG:
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	case PCA_REG_TYPE_1B_INTERRUPT_MASK:
	case PCA_REG_TYPE_1B_INTERRUPT_STATUS:
	case PCA_REG_TYPE_1B_INTERRUPT_CLEAR:
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	case PCA_REG_TYPE_1B_INPUT_HISTORY:
	case PCA_REG_TYPE_1B_INTERRUPT_RISE:
	case PCA_REG_TYPE_1B_INTERRUPT_FALL:
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
		return 1;
	case PCA_REG_TYPE_2B_OUTPUT_DRIVE_STRENGTH:
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	case PCA_REG_TYPE_2B_INTERRUPT_EDGE:
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
		return 2;
	default:
		LOG_ERR("unsupported reg type %d", reg_type);
		return 0; /** should never happen */
	}
}

/**
 * @brief get read size for register
 *
 * @param dev device struct
 * @param reg_type value from enum gpio_pca_series_reg_type
 * @return uint32_t size in bytes
 *                  0 if fail
 */
static inline uint32_t gpio_pca_series_reg_size(const struct device *dev,
						enum gpio_pca_series_reg_type reg_type)
{
	const struct gpio_pca_series_config *cfg = dev->config;

	return gpio_pca_series_reg_size_per_port(dev, reg_type) * cfg->part_cfg->port_no;
}

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
/** forward declarations */
static inline uint8_t gpio_pca_series_reg_cache_offset(const struct device *dev,
							   enum gpio_pca_series_reg_type reg_type);
static inline int gpio_pca_series_reg_cache_update(const struct device *dev,
						   enum gpio_pca_series_reg_type reg_type,
						   const uint8_t *buf);
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

/**
 * @brief read register with i2c interface.
 *
 * @note if CONFIG_GPIO_PCA_SERIES_CACHE_ALL is enabled, this will
 *       not update reg cache. Cache must be updated with
 *       @ref gpio_pca_series_reg_cache_update.
 *
 * @param dev           device struct
 * @param reg_type      value from enum gpio_pca_series_reg_type
 * @param buf           pointer to data in little-endian byteorder
 * @return int 0        if success
 *             -EFAULT  if register is not supported
 *             -EIO     if i2c failure
 */
static inline int gpio_pca_series_reg_read(const struct device *dev,
					   enum gpio_pca_series_reg_type reg_type, uint8_t *buf)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	int ret = 0;
	uint32_t size = gpio_pca_series_reg_size(dev, reg_type);
	uint8_t addr = gpio_pca_series_reg_get_addr(dev, reg_type);

	LOG_DBG("device read type %d addr 0x%x len %d", reg_type, addr, size);

#ifdef GPIO_NXP_PCA_SERIES_DEBUG
	if (!buf) {
		return -EFAULT;
	}

	if (addr == PCA_REG_INVALID) {
		LOG_ERR("trying to read unsupported reg, reg type %d", reg_type);
		return -EFAULT;
	}
#endif /* GPIO_NXP_PCA_SERIES_DEBUG */

	ret = i2c_write_read_dt(&cfg->i2c, (uint8_t *)&addr, 1, (uint8_t *)buf, size);
	if (ret) {
		LOG_ERR("i2c read error [%d]", ret);
	}
	return ret;
}

/**
 * @brief write register with i2c interface.
 *
 * @note if CONFIG_GPIO_PCA_SERIES_CACHE_ALL is enabled, this will
 *       also update reg cache.
 *
 * @param dev           device struct
 * @param reg_type      value from enum gpio_pca_series_reg_type
 * @param buf           pointer to data in little-endian byteorder
 * @return int 0        if success
 *             -EFAULT  if register is not supported
 *             -EIO     if i2c failure
 */
static inline int gpio_pca_series_reg_write(const struct device *dev,
					enum gpio_pca_series_reg_type reg_type, const uint8_t *buf)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	int ret = 0;
	struct i2c_msg msg[2];
	uint32_t size = gpio_pca_series_reg_size(dev, reg_type);
	uint8_t addr = gpio_pca_series_reg_get_addr(dev, reg_type);

#ifdef GPIO_NXP_PCA_SERIES_DEBUG
	if (!buf) {
		return -EFAULT;
	}

	if (addr == PCA_REG_INVALID) {
		LOG_ERR("trying to write unsupported reg type %d", reg_type);
		return -EFAULT;
	}
#endif /* GPIO_NXP_PCA_SERIES_DEBUG */

	LOG_DBG("device write type %d addr 0x%x len %d", reg_type, addr, size);

	msg[0].buf = (uint8_t *)&addr;
	msg[0].len = 1;
	msg[0].flags = I2C_MSG_WRITE;
	msg[1].buf = (uint8_t *)buf;
	msg[1].len = size;
	msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;
	ret = i2c_transfer_dt(&cfg->i2c, msg, 2);
	if (ret) {
		LOG_ERR("i2c write error [%d]", ret);
		return ret;
	}

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	if (gpio_pca_series_reg_cache_offset(dev, reg_type) != PCA_REG_INVALID) {
		(void)gpio_pca_series_reg_cache_update(dev, reg_type, buf);
	}
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

	return ret;
}

/**
 * }
 * gpio_pca_reg_access_api
 */

/**
 * gpio_pca_reg_cache_api
 * {
 * @note full cache is stored in le byteorder, consistent with reg layout.
 *       mini cache is stored in cpu byteorder
 */

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL

/**
 * @brief get memory offset of register cache from register type
 *
 * @param dev      device struct
 * @param reg_type value from enum gpio_pca_series_reg_type
 * @return uint8_t PCA_REG_INVALID      if reg is not used or uncacheable
 *                 other                offset in bytes related to cache pointer
 */
static inline uint8_t gpio_pca_series_reg_cache_offset(const struct device *dev,
							   enum gpio_pca_series_reg_type reg_type)
{
	const struct gpio_pca_series_config *cfg = dev->config;

	if (cfg->part_cfg->cache_map[reg_type] == PCA_REG_INVALID) {
		return PCA_REG_INVALID;
	} else {
		return cfg->part_cfg->cache_map[reg_type] * cfg->part_cfg->port_no;
	}
}

/**
 * @brief read all cacheable physical registers from device and update them
 *        in cache
 *
 * @param dev device struct
 * @param reg_type value from enum gpio_pca_series_reg_type
 * @return uint8_t PCA_REG_INVALID      if reg is not used or uncacheable
 *                 other                offset in bytes related to cache pointer
 */
static inline int gpio_pca_series_reg_cache_reset(const struct device *dev)
{
	struct gpio_pca_series_data *data = dev->data;
	int ret = 0;

	for (int reg_type = 0; reg_type < PCA_REG_TYPE_COUNT; ++reg_type) {
		uint8_t cache_offset = gpio_pca_series_reg_cache_offset(dev, reg_type);

		if (cache_offset == PCA_REG_INVALID) {
			continue;
		}

		LOG_DBG("cache init type %d", reg_type);

#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
		/**
		 * On devices without PCA_HAS_INT_EXTEND calabilitiy,
		 * PCA_REG_TYPE_2B_INTERRUPT_EDGE caches mask of rising and falling pins,
		 * while the actual register is not present. Account for that here:
		 */
		uint8_t reg_addr = gpio_pca_series_reg_get_addr(dev, reg_type);

		if (reg_addr == PCA_REG_INVALID) {
			const uint8_t reset_value_0[] = {
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

			switch (reg_type) {
			case PCA_REG_TYPE_1B_INPUT_HISTORY:
				ret = gpio_pca_series_reg_read(dev, PCA_REG_TYPE_1B_INPUT_PORT,
					((uint8_t *)data->cache) + cache_offset);
				if (ret) {
					LOG_ERR("cache initial input read failed %d", ret);
				}
				break;
			case PCA_REG_TYPE_1B_INTERRUPT_RISE:
			case PCA_REG_TYPE_1B_INTERRUPT_FALL:
				ret = gpio_pca_series_reg_cache_update(dev, reg_type,
					reset_value_0);
				if (ret) {
					LOG_ERR("init initial interrupt config failed %d", ret);
				}
				break;
			default:
				LOG_ERR("trying to cache reg that is not present");
				break;
			}
			if (ret) {
				break;
			}
			continue;
		}
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */

		ret = gpio_pca_series_reg_read(dev, reg_type,
			((uint8_t *)data->cache) + cache_offset);
		if (ret) {
			LOG_ERR("reg type %d cache init fail %d", reg_type, ret);
			break;
		}
	}
	return ret;
}

/**
 * @brief read register value from reg cache
 *
 * @param dev device struct
 * @param reg_type value from enum gpio_pca_series_reg_type
 * @param buf pointer to read data in little-endian byteorder
 * @return int 0        if success
 *             -EINVAL  if invalid arguments
 *             -EACCES  if register is uncacheable
 */
static inline int gpio_pca_series_reg_cache_read(const struct device *dev,
						 enum gpio_pca_series_reg_type reg_type,
						 uint8_t *buf)
{
	struct gpio_pca_series_data *data = dev->data;
	int ret = 0;
	uint8_t offset = gpio_pca_series_reg_cache_offset(dev, reg_type);
	uint32_t size = gpio_pca_series_reg_size(dev, reg_type);
	uint8_t *src;

#ifdef GPIO_NXP_PCA_SERIES_DEBUG
	if (!buf) {
		return -EINVAL;
	}

	if (offset == PCA_REG_INVALID) {
		LOG_ERR("can not get noncacheable reg %d");
		return -EFAULT;
	}
#endif /* GPIO_NXP_PCA_SERIES_DEBUG */

	src = ((uint8_t *)data->cache) + offset;
	LOG_DBG("cache read type %d len %d mem addr 0x%p", reg_type, size, src);
	memcpy(buf, src, size);
	return ret;
}

/**
 * @brief update register cache from device or existing value.
 *
 * @param dev device struct
 * @param reg_type value from enum gpio_pca_series_reg_type
 * @param buf pointer to new data to update from, in little-endian byteorder
 * @return int 0        if success
 *             -EINVAL  if invalid arguments
 *             -EACCES  if register is uncacheable
 */
static inline int gpio_pca_series_reg_cache_update(const struct device *dev,
						   enum gpio_pca_series_reg_type reg_type,
						   const uint8_t *buf)
{
	struct gpio_pca_series_data *data = dev->data;
	uint8_t offset = gpio_pca_series_reg_cache_offset(dev, reg_type);
	uint32_t size = gpio_pca_series_reg_size(dev, reg_type);
	uint8_t *dst;

#ifdef GPIO_NXP_PCA_SERIES_DEBUG
	if (!buf) {
		return -EINVAL;
	}

	if (offset == PCA_REG_INVALID) {
		LOG_ERR("can not update non-cacheable reg type %d", reg_type);
		return -EACCES;
	}
#endif /* GPIO_NXP_PCA_SERIES_DEBUG */

	LOG_DBG("cache update type %d len %d from %s", reg_type, size,
		(buf ? "buffer" : "device"));

	dst = ((uint8_t *)data->cache) + offset;
	LOG_DBG("cache write mem addr 0x%p len %d", dst, size);

	/** update cache from buf */
	memcpy(dst, buf, size);

	return 0;
}

#else /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

#define gpio_pca_series_reg_cache_read gpio_pca_series_reg_read

struct gpio_pca_series_reg_cache_mini {
	uint32_t output;   /** cache output value for faster output */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	uint32_t input_old;    /** only used when interrupt mask & edge config is not present */
	uint32_t int_rise; /** only used if interrupt edge is software-compared */
	uint32_t int_fall; /** only used if interrupt edge is software-compared */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};

static inline struct gpio_pca_series_reg_cache_mini *gpio_pca_series_reg_cache_mini_get(
	const struct device *dev)
{
	struct gpio_pca_series_data *data = dev->data;
	struct gpio_pca_series_reg_cache_mini *cache =
		(struct gpio_pca_series_reg_cache_mini *)(&data->cache);
	LOG_DBG("mini cache addr 0x%p", cache);
	return cache;
}

static inline int gpio_pca_series_reg_cache_mini_reset(const struct device *dev)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	struct gpio_pca_series_reg_cache_mini *cache =
		gpio_pca_series_reg_cache_mini_get(dev);
	int ret = 0;

	ret = gpio_pca_series_reg_read(dev, PCA_REG_TYPE_1B_OUTPUT_PORT,
		(uint8_t *)&cache->output);
	if (ret) {
		LOG_ERR("minimum cache failed to read initial output: %d", ret);
		goto out;
	}

	cache->output = sys_le32_to_cpu(cache->output);

#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	cache->int_rise = 0;
	cache->int_fall = 0;

	/* Read initial input value */
	enum gpio_pca_series_reg_type input_reg =
		cfg->part_cfg->flags & PCA_HAS_OUT_CONFIG ?
		PCA_REG_TYPE_1B_INPUT_STATUS : PCA_REG_TYPE_1B_INPUT_PORT;

	ret = gpio_pca_series_reg_read(dev, input_reg, (uint8_t *)&cache->input_old);
	if (ret) {
		LOG_ERR("minimum cache failed to read initial input: %d", ret);
	}

	cache->input_old = sys_le32_to_cpu(cache->input_old);
#else
	ARG_UNUSED(cfg);
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */

out:
	return ret;
}

#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

/**
 * }
 * gpio_pca_cache_api
 */

/**
 * gpio_pca_custom_api
 * {
 */

/**
 * @brief Reset function of pca_series
 *
 * This function pulls reset pin to reset a pca_series
 * device if reset_pin is present. Otherwise it write
 * reset value to device registers.
 */
static inline void gpio_pca_series_reset(const struct device *dev)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	const uint8_t reset_value_0[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	const uint8_t reset_value_1[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	int ret = 0;

	/** Reset pin connected, do hardware reset */
	if (cfg->gpio_rst.port != NULL) {
		if (!device_is_ready(cfg->gpio_rst.port)) {
			goto sw_rst;
		}
		/* Reset gpio should be set to active LOW in dts */
		ret = gpio_pin_configure_dt(&cfg->gpio_rst,
			GPIO_OUTPUT_HIGH | GPIO_OUTPUT_INIT_LOGICAL);
		if (ret) {
			goto sw_rst;
		}
		k_sleep(K_USEC(1));
		ret = gpio_pin_set_dt(&cfg->gpio_rst, 0U);
		if (ret) {
			goto sw_rst;
		}
		k_sleep(K_USEC(1));
		return;
	}

sw_rst:
	/** Warn that gpio configured but failed */
	if (cfg->gpio_rst.port != NULL) {
		LOG_WRN("gpio reset failed, fallback to soft reset");
	}
	/**
	 * Reset pin not connected, write reset value to registers
	 * No need to check return, as unsupported reg will return early with error
	 */
	gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_OUTPUT_PORT, reset_value_1);
	gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_CONFIGURATION, reset_value_1);
	if (cfg->part_cfg->flags & PCA_HAS_LATCH) {
		gpio_pca_series_reg_write(dev, PCA_REG_TYPE_2B_OUTPUT_DRIVE_STRENGTH,
			reset_value_1);
		gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_INPUT_LATCH, reset_value_0);
	}
	if (cfg->part_cfg->flags & PCA_HAS_PULL) {
		gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_PULL_ENABLE, reset_value_0);
		gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_PULL_SELECT, reset_value_1);
	}
	if (cfg->part_cfg->flags & PCA_HAS_OUT_CONFIG) {
		gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_OUTPUT_CONFIG, reset_value_0);
	}
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	if (cfg->part_cfg->flags & PCA_HAS_INT_MASK) {
		gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_INTERRUPT_MASK, reset_value_1);
	}
	if (cfg->part_cfg->flags & PCA_HAS_INT_EXTEND) {
		gpio_pca_series_reg_write(dev, PCA_REG_TYPE_2B_INTERRUPT_EDGE, reset_value_0);
	}
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
}

#ifdef GPIO_NXP_PCA_SERIES_DEBUG

/**
 * @brief Dump all available register and cache for debug purpose.
 *
 * @note This function does not consider cpu byteorder.
 */
void gpio_pca_series_debug_dump(const struct device *dev)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	struct gpio_pca_series_data *data = dev->data;
	int ret = 0;

	LOG_WRN("**** debug dump ****");
	LOG_WRN("device: %s", dev->name);
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	LOG_WRN("cache base addr: 0x%p size: 0x%2.2x",
		data->cache, cfg->part_cfg->cache_size);
#else
	LOG_WRN("cache base addr: 0x%p", data->cache);
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

	LOG_WRN("register profile:");
	LOG_WRN("type\t"
		"name\t\t\t"
		"addr\t"
		"reg_value\t\t\t"
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
		"cache\t"
		"cache_value\t\t"
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
	);
	for (int reg_type = 0; reg_type < PCA_REG_TYPE_COUNT; ++reg_type) {
		uint8_t reg = cfg->part_cfg->regs[reg_type];
		uint8_t reg_val[8];
		uint64_t *reg_val_p = (uint64_t *)&reg_val;
		uint32_t reg_size = gpio_pca_series_reg_size(dev, reg_type);
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
		uint8_t cache = gpio_pca_series_reg_cache_offset(dev, reg_type);
		uint8_t cache_val[8];
		uint64_t *cache_val_p = (uint64_t *)&cache_val;

		if (reg == PCA_REG_INVALID && cache == PCA_REG_INVALID)
#else
		if (reg == PCA_REG_INVALID)
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
		{
			continue;
		}

		if (reg != PCA_REG_INVALID) {
			*reg_val_p = 0;
			ret = gpio_pca_series_reg_read(dev, reg_type, reg_val);
			if (ret) {
				LOG_ERR("read reg error from reg type %d, invalidate this reg",
					reg_type);
				reg = PCA_REG_INVALID;
			}
		}
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
		if (cache != PCA_REG_INVALID) {
			*cache_val_p = 0;
			ret = gpio_pca_series_reg_cache_read(dev, reg_type, cache_val);
			if (ret) {
				LOG_ERR("read reg cache error from reg type %d, invalidate this "
					"reg cache",
					reg_type);
				reg = PCA_REG_INVALID;
			}
		}
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

		/** do_print */
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
		if (reg != PCA_REG_INVALID && cache != PCA_REG_INVALID) {
			LOG_WRN("%.2d\t%-24s\t0x%2.2x\t0x%16.16x\t0x%2.2x\t0x%16.16x\t"
				, reg_type, gpio_pca_series_reg_name[reg_type]
				, reg, *reg_val_p
				, cache, *cache_val_p
			);
			if (memcmp(reg_val, cache_val, reg_size)) {
				LOG_ERR("reg %d cache mismatch", reg_type);
			}
		} else if (reg == PCA_REG_INVALID && cache != PCA_REG_INVALID) {
			/**
			 * On devices without PCA_HAS_INT_EXTEND calabilitiy,
			 * PCA_REG_TYPE_2B_INTERRUPT_EDGE caches mask of rising and falling pins,
			 * while the actual register is not present. Account for that here:
			 */
			LOG_WRN("%.2d\t%-24s\tNone\tNone\t\t\t0x%2.2x\t0x%16.16x\t"
				, reg_type, gpio_pca_series_reg_name[reg_type]
				, cache, *cache_val_p
			);
		} else {
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
			LOG_WRN("%.2d\t%-24s\t0x%2.2x\t0x%16.16x\t"
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
				"None\tNone\t\t\t"
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
				, reg_type, gpio_pca_series_reg_name[reg_type]
				, reg, *reg_val_p
			);
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
		}
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
	}
	LOG_WRN("**** dump finish ****");
}

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL

/**
 * @brief Validate the cache api by filling data to the cache.
 */
void gpio_pca_series_cache_test(const struct device *dev)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	const uint8_t reset_value_0[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	const uint8_t reset_value_1[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	uint8_t buffer[8];
	uint8_t expected_offset = 0U;
	uint64_t *buffer_p = (uint64_t *)buffer;

	LOG_WRN("**** cache test ****");
	LOG_WRN("device: %s", dev->name);

	for (int reg_type = 0; reg_type < PCA_REG_TYPE_COUNT; ++reg_type) {
		uint8_t cache_offset = gpio_pca_series_reg_cache_offset(dev, reg_type);
		uint32_t cache_size = gpio_pca_series_reg_size(dev, reg_type);

		if (cache_offset == PCA_REG_INVALID) {
			LOG_WRN("skip reg %d: not present or non-cacheable", reg_type);
			continue;
		}

		if (cache_offset != expected_offset) {
			LOG_ERR("reg %d cache offset 0x%2.2x error, expected 0x%2.2x",
				reg_type, cache_offset, expected_offset);
			break;
		}

		expected_offset += cache_size;

		LOG_WRN("testing reg %d size %d", reg_type,  cache_size);
		(void)gpio_pca_series_reg_cache_update(dev, reg_type, reset_value_0);
		*buffer_p = 0;
		gpio_pca_series_reg_cache_read(dev, reg_type, buffer);
		LOG_WRN("fill 00, result: 0x%16.16x", *buffer_p);
		(void)gpio_pca_series_reg_cache_update(dev, reg_type, reset_value_1);
		*buffer_p = 0;
		gpio_pca_series_reg_cache_read(dev, reg_type, buffer);
		LOG_WRN("fill ff, result: 0x%16.16x", *buffer_p);
	}
	LOG_WRN("**** test finish ****");
}
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

#endif /* GPIO_NXP_PCA_SERIES_DEBUG */

/**
 * }
 * gpio_pca_custom_api
 */

/**
 * gpio_pca_zephyr_gpio_api
 * {
 */

#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
/** Forward declaration */
static inline void gpio_pca_series_interrupt_handler_standard(
		const struct device *dev, gpio_port_value_t *input_value);
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */

/**
 * @brief configure gpio port.
 *
 * @note This API applies to all supported part no. Capabilities (
 *       PCA_HAS_PULL and PCA_HAS_OUT_CONFIG) are evaluated and handled.
 *
 * @return int 0            if success
 *             -ENOTSUP     if unsupported config flags are provided
 */
static int gpio_pca_series_pin_configure(const struct device *dev,
		gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	struct gpio_pca_series_data *data = dev->data;
	uint32_t reg_value;
	int ret = 0;

	if ((flags & GPIO_INPUT) && (flags & GPIO_OUTPUT)) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_SINGLE_ENDED) &&
		(cfg->part_cfg->flags & PCA_HAS_OUT_CONFIG) == 0U) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_PULL_UP) || (flags & GPIO_PULL_DOWN)) {
		if ((cfg->part_cfg->flags & PCA_HAS_PULL) == 0U) {
			return -ENOTSUP;
		}
	} /* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	LOG_DBG("dev %s configure pin %d flag 0x%x", dev->name, pin, flags);

	k_sem_take(&data->lock, K_FOREVER);

	/**
	 * TODO: Only write 1 byte.
	 *
	 * The config api only configures 1 pin, so only need to write the
	 * modified byte. Need to create new register & cache api to provide
	 * single byte access.
	 * This applies to: pin_configure, pin_interrupt_configure
	 */
	if (cfg->part_cfg->flags & PCA_HAS_OUT_CONFIG) {
		/* configure PCA_REG_TYPE_1B_OUTPUT_CONFIG */
		ret = gpio_pca_series_reg_cache_read(dev,
			PCA_REG_TYPE_1B_OUTPUT_CONFIG, (uint8_t *)&reg_value);
		reg_value = sys_le32_to_cpu(reg_value);
		if (flags & GPIO_SINGLE_ENDED) {
			reg_value |= (BIT(pin)); /* set bit to set open-drain */
		} else {
			reg_value &= (~BIT(pin)); /* clear bit to set push-pull */
		}
		reg_value = sys_cpu_to_le32(reg_value);
		ret = gpio_pca_series_reg_write(dev,
			PCA_REG_TYPE_1B_OUTPUT_CONFIG, (uint8_t *)&reg_value);
	}

	if ((cfg->part_cfg->flags & PCA_HAS_PULL)) {
		if ((flags & GPIO_PULL_UP) || (flags & GPIO_PULL_DOWN)) {
			/* configure PCA_REG_TYPE_1B_PULL_SELECT */
			ret = gpio_pca_series_reg_cache_read(dev,
				PCA_REG_TYPE_1B_PULL_SELECT, (uint8_t *)&reg_value);
			reg_value = sys_le32_to_cpu(reg_value);
			if (flags & GPIO_PULL_UP) {
				reg_value |= (BIT(pin));
			} else {
				reg_value &= (~BIT(pin));
			}
			reg_value = sys_cpu_to_le32(reg_value);
			ret = gpio_pca_series_reg_write(dev,
				PCA_REG_TYPE_1B_PULL_SELECT, (uint8_t *)&reg_value);
		}
		/* configure PCA_REG_TYPE_1B_PULL_ENABLE */
		ret = gpio_pca_series_reg_cache_read(dev,
			PCA_REG_TYPE_1B_PULL_ENABLE, (uint8_t *)&reg_value);
		reg_value = sys_le32_to_cpu(reg_value);
		if ((flags & GPIO_PULL_UP) || (flags & GPIO_PULL_DOWN)) {
			reg_value |= (BIT(pin)); /* set bit to enable pull */
		} else {
			reg_value &= (~BIT(pin)); /* clear bit to disable pull */
		}
		reg_value = sys_cpu_to_le32(reg_value);
		ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_PULL_ENABLE,
						(uint8_t *)&reg_value);
	}

	/* configure PCA_REG_TYPE_1B_OUTPUT */
	if ((flags & GPIO_OUTPUT_INIT_HIGH) || (flags & GPIO_OUTPUT_INIT_LOW)) {
		uint32_t out_old;
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
		/* get output register old value from reg cache */
		ret = gpio_pca_series_reg_cache_read(dev, PCA_REG_TYPE_1B_OUTPUT_PORT,
							 (uint8_t *)&out_old);
		if (ret) {
			ret = -EINVAL; /* should never fail */
			goto out;
		}
		out_old = sys_le32_to_cpu(out_old);
#else  /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
		out_old = gpio_pca_series_reg_cache_mini_get(dev)->output;
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			reg_value = out_old | (BIT(pin));
		}
		if (flags & GPIO_OUTPUT_INIT_LOW) {
			reg_value = out_old & (~BIT(pin));
		}
		reg_value = sys_cpu_to_le32(reg_value);
		ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_OUTPUT_PORT,
						(uint8_t *)&reg_value);
		if (ret != 0) {
			goto out;
		}
#ifndef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
		/** update output register old value to void* cache raw value */
		gpio_pca_series_reg_cache_mini_get(dev)->output =
		sys_le32_to_cpu(reg_value);
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
	}

	/* configure PCA_REG_TYPE_1B_CONFIGURATION */
	ret = gpio_pca_series_reg_cache_read(dev,
		PCA_REG_TYPE_1B_CONFIGURATION, (uint8_t *)&reg_value);
	reg_value = sys_le32_to_cpu(reg_value);
	if (flags & GPIO_INPUT) {
		reg_value |= (BIT(pin)); /* set bit to set input */
	} else {
		reg_value &= (~BIT(pin)); /* clear bit to set output */
	}
	reg_value = sys_cpu_to_le32(reg_value);
	ret = gpio_pca_series_reg_write(dev,
		PCA_REG_TYPE_1B_CONFIGURATION, (uint8_t *)&reg_value);

out:
	k_sem_give(&data->lock);
	LOG_DBG("dev %s configure return %d", dev->name, ret);
	return ret;
}

/**
 * @brief read gpio port
 *
 * @note read input_port register will clear interrupt masks
 *       on supported devices. This API is used for part no
 *       without PCA_HAS_INT_EXTEND capability.
 *
 * @param dev
 * @param value
 * @return int 0            if success
 *             -EWOULDBLOCK if called from ISR context
 */
static int gpio_pca_series_port_read_standard(
		const struct device *dev, gpio_port_value_t *value)
{
	struct gpio_pca_series_data *data = dev->data;
	uint32_t input_data;
	int ret = 0;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	LOG_DBG("dev %s standard_read", dev->name);

#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	gpio_pca_series_interrupt_handler_standard(dev, value);
	ARG_UNUSED(data);
	ARG_UNUSED(input_data);
#else
	k_sem_take(&data->lock, K_FOREVER);

	/* Read Input Register */
	ret = gpio_pca_series_reg_read(dev,
		PCA_REG_TYPE_1B_INPUT_PORT, (uint8_t *)&input_data);
	if (ret) {
		LOG_ERR("port read error %d", ret);
	} else {
		value = sys_le32_to_cpu(input_data);
	}
	k_sem_give(&data->lock);
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */

	LOG_DBG("dev %s standard_read return %d result 0x%8.8x",
		dev->name, ret, (uint32_t) *value);
	return ret;
}

/**
 * @brief read gpio port
 *
 * @note This API is used for part no with PCA_HAS_INT_EXTEND capability.
 *       It read input_status register, which will NOT clear interrupt masks.
 *
 * @return int 0            if success
 *             -EWOULDBLOCK if called from ISR context
 */
static int gpio_pca_series_port_read_extended(
	const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	struct gpio_pca_series_data *data = dev->data;
	uint32_t input_data;
	int ret = 0;

#ifdef GPIO_NXP_PCA_SERIES_DEBUG
	/**
	 * Check the flags during runtime.
	 *
	 * The purpose is to check api assignment for developer who is adding
	 * new device support to this driver.
	 */
	const uint8_t check_flags = (PCA_HAS_LATCH | PCA_HAS_INT_MASK | PCA_HAS_INT_EXTEND);

	if ((cfg->part_cfg->flags & check_flags) != check_flags) {
		LOG_ERR("unsupported device trying to read gpio with extended api");
		return -ENOTSUP;
	}
#else
	ARG_UNUSED(cfg);
#endif /* GPIO_NXP_PCA_SERIES_DEBUG */

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	LOG_DBG("dev %s extended_read", dev->name);
	k_sem_take(&data->lock, K_FOREVER);

	/* Read Input Status Register */
	ret = gpio_pca_series_reg_read(dev, PCA_REG_TYPE_1B_INPUT_STATUS,
					   (uint8_t *)&input_data);
	if (ret) {
		LOG_ERR("port read error %d", ret);
	} else {
		*value = sys_le32_to_cpu(input_data);
	}

	k_sem_give(&data->lock);
	LOG_DBG("dev %s extended_read return %d result 0x%8.8x",
		dev->name, ret, (uint32_t) *value);
	return ret;
}

static int gpio_pca_series_port_write(const struct device *dev,
	gpio_port_pins_t mask, gpio_port_value_t value, gpio_port_value_t toggle)
{
	struct gpio_pca_series_data *data = dev->data;
	uint32_t out_old;
	uint32_t out;
	int ret = 0;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	LOG_DBG("dev %s write mask 0x%8.8x value 0x%8.8x toggle 0x%8.8x",
		dev->name, (uint32_t)mask, (uint32_t)value, (uint32_t)toggle);
	k_sem_take(&data->lock, K_FOREVER);

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	/** get output register old value from reg cache */
	ret = gpio_pca_series_reg_cache_read(dev, PCA_REG_TYPE_1B_OUTPUT_PORT,
						 (uint8_t *)&out_old);
	if (ret) {
		return -EINVAL; /** should never fail */
	}
	out_old = sys_le32_to_cpu(out_old);
#else  /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
	LOG_DBG("access address 0x%8.8x", (uint32_t)data->cache);
	out_old = gpio_pca_series_reg_cache_mini_get(dev)->output;
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

	out = ((out_old & ~mask) | (value & mask)) ^ toggle;
	out = sys_cpu_to_le32(out);

	ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_OUTPUT_PORT, (uint8_t *)&out);
	if (ret == 0) {
#ifndef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
		/** update output register old value to void* cache raw value */
		gpio_pca_series_reg_cache_mini_get(dev)->output = sys_le32_to_cpu(out);
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
	}

	k_sem_give(&data->lock);

	LOG_DBG("dev %s write return %d result 0x%8.8x", dev->name, ret, out);
	return ret;
}

static int gpio_pca_series_port_set_masked(const struct device *dev, gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	return gpio_pca_series_port_write(dev, mask, value, 0);
}

static int gpio_pca_series_port_set_bits(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_pca_series_port_write(dev, pins, pins, 0);
}

static int gpio_pca_series_port_clear_bits(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_pca_series_port_write(dev, pins, 0, 0);
}

static int gpio_pca_series_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_pca_series_port_write(dev, 0, 0, pins);
}

#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
/**
 * @brief Configure interrupt for device with software-compared interrupt edge
 *
 * @note This version is used by devices that does not have interrupt edge config
 *       (aka PCA_HAS_INT_EXTEND), and relies on software to check the edge.
 *       This applies to all pca(l)9xxx and pcal64xxa devices.
 *       This will also configure interrupt mask register if the device has it.
 *
 * @param dev
 * @param pin
 * @param mode
 * @param trig
 * @return int
 */
static int gpio_pca_series_pin_interrupt_configure_standard(
	const struct device *dev, gpio_pin_t pin, enum gpio_int_mode mode,
	enum gpio_int_trig trig)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	struct gpio_pca_series_data *data = dev->data;
	uint32_t int_rise, int_fall;
	uint32_t int_mask, input_latch;
	int ret = 0;

	if (cfg->gpio_int.port == NULL) {
		return -ENOTSUP;
	}
	/* Device does not support level-triggered interrupts. */
	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&data->lock, K_FOREVER);

	/**
	 * TODO: Only write 1 byte.
	 *
	 * The config api only configures 1 pin, so only need to write the
	 * modified byte. Need to create new register & cache api to provide
	 * single byte access.
	 * This applies to: pin_configure, pin_interrupt_configure
	 */

	/** get current interrupt config */
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	/** read from cache even if this register is not present on device */
	ret = gpio_pca_series_reg_cache_read(dev, PCA_REG_TYPE_1B_INTERRUPT_RISE,
						 (uint8_t *)&int_rise);
	if (ret) {
		goto out;
	}
	int_rise = sys_le32_to_cpu(int_rise);
	/** read from cache even if this register is not present on device */
	ret = gpio_pca_series_reg_cache_read(dev, PCA_REG_TYPE_1B_INTERRUPT_FALL,
						 (uint8_t *)&int_fall);
	if (ret) {
		goto out;
	}
	int_fall = sys_le32_to_cpu(int_fall);
#else
	int_rise = gpio_pca_series_reg_cache_mini_get(dev)->int_rise;
	int_fall = gpio_pca_series_reg_cache_mini_get(dev)->int_fall;
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

	if (mode == GPIO_INT_MODE_DISABLED) {
		int_fall &= ~BIT(pin);
		int_rise &= ~BIT(pin);
	} else {
		if (trig == GPIO_INT_TRIG_BOTH) {
			int_fall |= BIT(pin);
			int_rise |= BIT(pin);
		} else if (trig == GPIO_INT_TRIG_LOW) {
			int_fall |= BIT(pin);
			int_rise &= ~BIT(pin);
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			int_fall &= ~BIT(pin);
			int_rise |= BIT(pin);
		}
	}

	int_mask = int_fall | int_rise;
	input_latch = ~int_mask;

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	/** read from cache even if this register is not present on device */
	int_rise = sys_cpu_to_le32(int_rise);
	ret = gpio_pca_series_reg_cache_update(dev, PCA_REG_TYPE_1B_INTERRUPT_RISE,
						 (uint8_t *)&int_rise);
	if (ret) {
		goto out;
	}
	/** read from cache even if this register is not present on device */
	int_fall = sys_cpu_to_le32(int_fall);
	ret = gpio_pca_series_reg_cache_update(dev, PCA_REG_TYPE_1B_INTERRUPT_FALL,
						 (uint8_t *)&int_fall);
	if (ret) {
		goto out;
	}
#else
	gpio_pca_series_reg_cache_mini_get(dev)->int_rise = int_rise;
	gpio_pca_series_reg_cache_mini_get(dev)->int_fall = int_fall;
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

	/** enable latch if available, so we do not lost interrupt */
	if (cfg->part_cfg->flags & PCA_HAS_LATCH) {
		input_latch = sys_cpu_to_le32(input_latch);
		ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_INPUT_LATCH,
						(uint8_t *)&input_latch);
		if (ret) {
			goto out;
		}
	}
	/** update interrupt mask register if available */
	if (cfg->part_cfg->flags & PCA_HAS_INT_MASK) {
		int_mask = sys_cpu_to_le32(int_mask);
		ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_INTERRUPT_MASK,
						(uint8_t *)&int_mask);
		if (ret) {
			goto out;
		}
	}

out:
	k_sem_give(&data->lock);

	if (ret) {
		LOG_ERR("int config(s) error %d", ret);
	}
	return ret;
}

/**
 * @brief Configure interrupt for device with hardware-selected interrupt edge
 *
 * @note This version is used by devices that have interrupt edge config
 *       (aka PCA_HAS_INT_EXTEND), so interrupt only triggers on selected edge.
 *       This applies to all pcal65xx devices.
 *       This will configure interrupt mask register and interrupt edge register.
 *       (All devices that have PCA_HAS_INT_EXTEND flag should have PCA_HAS_INT_MASK
 *       flag. Otherwise, throw out error.)
 */
static int gpio_pca_series_pin_interrupt_configure_extended(
		const struct device *dev,
		gpio_pin_t pin, enum gpio_int_mode mode,
		enum gpio_int_trig trig)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	struct gpio_pca_series_data *data = dev->data;
	uint64_t int_edge;
	uint32_t int_mask, input_latch;
	int ret = 0;
	uint32_t edge_cfg_shift = pin << 1U;
	uint64_t edge_cfg_mask = 0b11 << edge_cfg_shift;

	if (cfg->gpio_int.port == NULL) {
		return -ENOTSUP;
	}
	/* Device does not support level-triggered interrupts. */
	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

#ifdef GPIO_NXP_PCA_SERIES_DEBUG
	/**
	 * Check the flags during runtime.
	 *
	 * The purpose is to check api assignment for developer who is adding
	 * new device support to this driver.
	 */
	const uint8_t check_flags = (PCA_HAS_LATCH | PCA_HAS_INT_MASK | PCA_HAS_INT_EXTEND);

	if ((cfg->part_cfg->flags & check_flags) != check_flags) {
		LOG_ERR("unsupported device trying to configure interrupt with extended api");
		return -ENOTSUP;
	}
#endif /* GPIO_NXP_PCA_SERIES_DEBUG */

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	LOG_DBG("int cfg(e) pin %d mode %d trig %d", pin, mode, trig);

	k_sem_take(&data->lock, K_FOREVER);

	/**
	 * TODO: Only write 1 byte.
	 *
	 * The config api only configures 1 pin, so only need to write the
	 * modified byte. Need to create new register & cache api to provide
	 * single byte access.
	 * This applies to: pin_configure, pin_interrupt_configure
	 */

	/** get current interrupt edge config */
	ret = gpio_pca_series_reg_cache_read(dev, PCA_REG_TYPE_2B_INTERRUPT_EDGE,
						 (uint8_t *)&int_edge);
	if (ret) {
		LOG_ERR("get current interrupt edge config fail [%d]", ret);
		goto out;
	}
	int_edge = sys_le64_to_cpu(int_edge);

	ret = gpio_pca_series_reg_cache_read(dev, PCA_REG_TYPE_1B_INTERRUPT_MASK,
						 (uint8_t *)&int_mask);
	if (ret) {
		goto out;
	}
	int_mask = sys_le32_to_cpu(int_mask);

	if (mode == GPIO_INT_MODE_DISABLED) {
		int_mask |= BIT(pin); /** set 1 to disable interrupt */
	} else {
		if (trig == GPIO_INT_TRIG_BOTH) {
			int_edge = (int_edge & (~edge_cfg_mask)) |
				   (PCA_INTERRUPT_EITHER_EDGE << edge_cfg_shift);
		} else if (trig == GPIO_INT_TRIG_LOW) {
			int_edge = (int_edge & (~edge_cfg_mask)) |
				   (PCA_INTERRUPT_FALLING_EDGE << edge_cfg_shift);
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			int_edge = (int_edge & (~edge_cfg_mask)) |
				   (PCA_INTERRUPT_RISING_EDGE << edge_cfg_shift);
		}
		int_mask &= ~BIT(pin); /** set 0 to enable interrupt */
	}

	/** update interrupt edge config */
	int_edge = sys_cpu_to_le64(int_edge);
	ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_2B_INTERRUPT_EDGE,
						 (uint8_t *)&int_edge);
	if (ret) {
		goto out;
	}
	/** enable latch, so we do not lost interrupt */
	input_latch = ~int_mask;
	input_latch = sys_cpu_to_le32(input_latch);
	ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_INPUT_LATCH,
					(uint8_t *)&input_latch);
	if (ret) {
		goto out;
	}
	/** update interrupt mask register */
	int_mask = sys_cpu_to_le32(int_mask);
	ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_INTERRUPT_MASK,
					(uint8_t *)&int_mask);
	if (ret) {
		goto out;
	}

out:
	k_sem_give(&data->lock);
	return ret;
}

static int gpio_pca_series_manage_callback(const struct device *dev,
		struct gpio_callback *callback, bool set)
{
	struct gpio_pca_series_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static void gpio_pca_series_interrupt_handler_standard(const struct device *dev,
		gpio_port_value_t *input_value)
{
	struct gpio_pca_series_data *data = dev->data;
	int ret = 0;
	uint32_t input_old, int_rise, int_fall;
	uint32_t input;
	uint32_t transitioned_pins;
	uint32_t int_status = 0;

	k_sem_take(&data->lock, K_FOREVER);

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	/** read from cache even if this register is not present on device */
	ret = gpio_pca_series_reg_cache_read(dev, PCA_REG_TYPE_1B_INPUT_HISTORY,
						 (uint8_t *)&input_old);
	if (ret) {
		goto out;
	}
	input_old = sys_le32_to_cpu(input_old);
	/** read from cache even if this register is not present on device */
	ret = gpio_pca_series_reg_cache_read(dev, PCA_REG_TYPE_1B_INTERRUPT_RISE,
						 (uint8_t *)&int_rise);
	if (ret) {
		goto out;
	}
	int_rise = sys_le32_to_cpu(int_rise);
	/** read from cache even if this register is not present on device */
	ret = gpio_pca_series_reg_cache_read(dev, PCA_REG_TYPE_1B_INTERRUPT_FALL,
						 (uint8_t *)&int_fall);
	if (ret) {
		goto out;
	}
	int_fall = sys_le32_to_cpu(int_fall);
#else
	input_old = gpio_pca_series_reg_cache_mini_get(dev)->input_old;
	int_rise = gpio_pca_series_reg_cache_mini_get(dev)->int_rise;
	int_fall = gpio_pca_series_reg_cache_mini_get(dev)->int_fall;
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

	/** check if any interrupt enabled */
	if ((!int_rise) && (!int_fall)) {
		goto out;
	}

	/** read current input value, and clear status if reg is present */
	ret = gpio_pca_series_reg_read(dev, PCA_REG_TYPE_1B_INPUT_PORT, (uint8_t *)&input);
	if (ret) {
		goto out;
	}
	input = sys_le32_to_cpu(input);
	/** compare input to input_old to get transitioned_pins */
	transitioned_pins = input_old ^ input;

	/** Mask gpio transactions with rising/falling edge interrupt config */
	int_status = (int_rise & transitioned_pins & input)
				| (int_fall & transitioned_pins & (~input));

	/** update current input to cache */
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	uint32_t input_le = sys_cpu_to_le32(input);

	ret = gpio_pca_series_reg_cache_update(dev, PCA_REG_TYPE_1B_INPUT_HISTORY,
						   (uint8_t *)&input_le);
#else
	gpio_pca_series_reg_cache_mini_get(dev)->input_old = input;
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

out:
	k_sem_give(&data->lock);

	if (input_value) {
		*input_value = input;
	}
	if ((ret == 0) && (int_status)) {
		gpio_fire_callbacks(&data->callbacks, dev, int_status);
	}
}

static void gpio_pca_series_interrupt_handler_extended(const struct device *dev)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	struct gpio_pca_series_data *data = dev->data;

	int ret = 0;
	uint32_t int_status = 0;

#ifdef GPIO_NXP_PCA_SERIES_DEBUG
	/**
	 * Check the flags during runtime.
	 *
	 * The purpose is to check api assignment for developer who is adding
	 * new device support to this driver.
	 */
	const uint8_t check_flags = (PCA_HAS_LATCH | PCA_HAS_INT_MASK | PCA_HAS_INT_EXTEND);

	if ((cfg->part_cfg->flags & check_flags) != check_flags) {
		LOG_ERR("unsupported device trying to read gpio with extended api");
		return;
	}
#else
	ARG_UNUSED(cfg);
#endif /* GPIO_NXP_PCA_SERIES_DEBUG */

	LOG_DBG("enter int handler(e)");

	k_sem_take(&data->lock, K_FOREVER);

	/** get transitioned_pins from interrupt_status register */
	ret = gpio_pca_series_reg_read(dev, PCA_REG_TYPE_1B_INTERRUPT_STATUS,
					   (uint8_t *)&int_status);
	if (ret) {
		goto out;
	}

	/** clear status */
	ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_INTERRUPT_CLEAR,
					(uint8_t *)&int_status);
	if (ret) {
		goto out;
	}

out:
	k_sem_give(&data->lock);

	if ((ret == 0) && (int_status)) {
		int_status = sys_le32_to_cpu(int_status);
		gpio_fire_callbacks(&data->callbacks, dev, int_status);
	}
}

static void gpio_pca_series_interrupt_worker_standard(struct k_work *work)
{
	struct gpio_pca_series_data *data =
		CONTAINER_OF(work, struct gpio_pca_series_data, int_work);
	const struct device *dev = data->self;

	gpio_pca_series_interrupt_handler_standard(dev, NULL);
}

static void gpio_pca_series_interrupt_worker_extended(struct k_work *work)
{
	struct gpio_pca_series_data *data =
		CONTAINER_OF(work, struct gpio_pca_series_data, int_work);
	const struct device *dev = data->self;

	gpio_pca_series_interrupt_handler_extended(dev);
}

static void gpio_pca_series_gpio_int_handler(const struct device *dev,
						 struct gpio_callback *gpio_cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	LOG_DBG("gpio_int trigger");

	struct gpio_pca_series_data *data =
		CONTAINER_OF(gpio_cb, struct gpio_pca_series_data, gpio_cb);

	k_work_submit(&data->int_work);
}

#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */

/**
 * }
 * gpio_pca_zephyr_gpio_api
 */

static DEVICE_API(gpio, gpio_pca_series_api_funcs_standard) = {
	.pin_configure = gpio_pca_series_pin_configure,
	.port_get_raw = gpio_pca_series_port_read_standard,
	.port_set_masked_raw = gpio_pca_series_port_set_masked,
	.port_set_bits_raw = gpio_pca_series_port_set_bits,
	.port_clear_bits_raw = gpio_pca_series_port_clear_bits,
	.port_toggle_bits = gpio_pca_series_port_toggle_bits,
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	.pin_interrupt_configure = gpio_pca_series_pin_interrupt_configure_standard,
	.manage_callback = gpio_pca_series_manage_callback,
#endif
};

static DEVICE_API(gpio, gpio_pca_series_api_funcs_extended) = {
	.pin_configure = gpio_pca_series_pin_configure,
	.port_get_raw = gpio_pca_series_port_read_extended, /* special version used */
	.port_set_masked_raw = gpio_pca_series_port_set_masked,
	.port_set_bits_raw = gpio_pca_series_port_set_bits,
	.port_clear_bits_raw = gpio_pca_series_port_clear_bits,
	.port_toggle_bits = gpio_pca_series_port_toggle_bits,
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	.pin_interrupt_configure = gpio_pca_series_pin_interrupt_configure_extended,
	.manage_callback = gpio_pca_series_manage_callback,
#endif
};

/**
 * @brief Initialization function of pca_series
 *
 * This sets initial input/ output configuration and output states.
 * The interrupt is configured if this is enabled.
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int gpio_pca_series_init(const struct device *dev)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	struct gpio_pca_series_data *data = dev->data;
	int ret = 0;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("i2c bus device not found");
		goto out_bus;
	}
#ifdef GPIO_NXP_PCA_SERIES_DEBUG
# ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	gpio_pca_series_cache_test(dev);
# endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
#endif /* GPIO_NXP_PCA_SERIES_DEBUG */

	/** Set cache to initial state */
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	ret = gpio_pca_series_reg_cache_reset(dev);
#else
	ret = gpio_pca_series_reg_cache_mini_reset(dev);
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
	if (ret) {
		LOG_ERR("cache init error %d", ret);
		goto out;
	}
	LOG_DBG("cache init done");

	/** device reset */
	gpio_pca_series_reset(dev);
	LOG_DBG("device reset done");

	/** configure interrupt */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	/** save dev pointer */
	data->self = dev;

	/** check the flags and init work obj */
	const uint8_t check_flags = (PCA_HAS_LATCH | PCA_HAS_INT_MASK | PCA_HAS_INT_EXTEND);

	if ((cfg->part_cfg->flags & check_flags) == check_flags) {
		k_work_init(&data->int_work, gpio_pca_series_interrupt_worker_extended);
	} else {
		k_work_init(&data->int_work, gpio_pca_series_interrupt_worker_standard);
	}

	/** Interrupt pin connected, enable interrupt */
	if (cfg->gpio_int.port != NULL) {
		if (!device_is_ready(cfg->gpio_int.port)) {
			LOG_ERR("Cannot get pointer to gpio interrupt device");
			ret = -EINVAL;
			goto out;
		}

		ret = gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
		if (ret) {
			goto out;
		}

		ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
		if (ret) {
			goto out;
		}

		gpio_init_callback(&data->gpio_cb, gpio_pca_series_gpio_int_handler,
				   BIT(cfg->gpio_int.pin));

		ret = gpio_add_callback(cfg->gpio_int.port, &data->gpio_cb);
	} else {
		LOG_WRN("pca interrupt enabled w/o int-gpios configured in dts");
	}
#else
	ARG_UNUSED(data);
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */

out:
#ifdef GPIO_NXP_PCA_SERIES_DEBUG
	gpio_pca_series_debug_dump(dev);
#endif /* GPIO_NXP_PCA_SERIES_DEBUG */

out_bus:
	if (ret) {
		LOG_ERR("%s init failed: %d", dev->name, ret);
	} else {
		LOG_INF("%s init ok", dev->name);
	}
	return ret;
}

/**
 * @brief get device description by part_no
 */
#define GPIO_PCA_GET_API_BY_PART_NO(part_no) ( \
	(part_no == PCA_PART_NO_PCAL6524) ? &gpio_pca_series_api_funcs_extended : \
	(part_no == PCA_PART_NO_PCAL6534) ? &gpio_pca_series_api_funcs_extended : \
	&gpio_pca_series_api_funcs_standard \
)
#define GPIO_PCA_GET_PORT_NO_CFG_BY_PART_NO(part_no) (GPIO_PCA_PORT_NO_##part_no)
#define GPIO_PCA_GET_PART_FLAG_BY_PART_NO(part_no) (GPIO_PCA_FLAG_##part_no)
#define GPIO_PCA_GET_PART_CFG_BY_PART_NO(part_no) (GPIO_PCA_PART_CFG_##part_no)

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL

/** Cache size increment by feature flags */
#define PCA_REG_HAS_LATCH	(3U) /* +2b_drive_strength, +1b_input_latch */
#define PCA_REG_HAS_PULL	(2U) /* +1b_pull_enable, +1b_pull_select */
#define PCA_REG_HAS_OUT_CONFIG	(1U) /* +1b_output_config */

#define GPIO_PCA_GET_CACHE_SIZE_BY_PART_NO_NO_INT(part_no) (( \
	2U /* basic: +output_port, +configuration */ \
	+ ((GPIO_PCA_GET_PART_FLAG_BY_PART_NO(part_no) & PCA_HAS_LATCH) ? \
		PCA_REG_HAS_LATCH : 0U) \
	+ ((GPIO_PCA_GET_PART_FLAG_BY_PART_NO(part_no) & PCA_HAS_PULL) ? \
		PCA_REG_HAS_PULL : 0U) \
	+ ((GPIO_PCA_GET_PART_FLAG_BY_PART_NO(part_no) & PCA_HAS_OUT_CONFIG) ? \
		PCA_REG_HAS_OUT_CONFIG : 0U) \
	) * GPIO_PCA_GET_PORT_NO_CFG_BY_PART_NO(part_no) \
)

#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT

/** Cache size increment by feature flags (continued) */
#define PCA_REG_HAS_INT_EXTEND	(3U) /* true: +2b_interrupt_edge, +1b_interrupt_mask */
#define PCA_REG_NO_INT_EXTEND	(3U) /* false: +1b_input_history, +1b_interrupt_rise,
				      * +1b_interrupt_fall
				      */

/**
 *  registers:
 *    1b_input_port
 *        - present on all devices
 *        - not used if PCA_HAS_OUT_CONFIG
 *        - non-cacheable
 *    1b_output_port
 *        - present on all devices
 *        - cacheable
 *    1b_configuration
 *        - present on all devices
 *        - cacheable
 *    2b_output_drive_strength
 *        - present if PCA_HAS_LATCH
 *        - cacheable if present
 *    1b_input_latch
 *        - present if PCA_HAS_LATCH
 *        - non-cacheable
 *    1b_pull_enable
 *        - present if PCA_HAS_PULL
 *        - cacheable if present
 *    1b_pull_select
 *        - present if PCA_HAS_PULL
 *        - cacheable if present
 *    1b_input_status
 *        - present if PCA_HAS_OUT_CONFIG
 *        - replaces 1b_input_port if present
 *        - non-cacheable
 *    1b_output_config
 *        - present if PCA_HAS_OUT_CONFIG
 *        - cacheable if present
 *    1b_interrupt_mask
 *        - present if PCA_HAS_INT_MASK
 *        - not present by default
 *        - cacheable if PCA_HAS_INT_EXTEND
 *    1b_interrupt_status
 *        - present if PCA_HAS_INT_MASK
 *        - not used if not PCA_HAS_INT_EXTEND
 *        - read only
 *        - non-cacheable
 *    2b_interrupt_edge
 *        - present if PCA_HAS_INT_EXTEND
 *        - cacheable if present
 *    1b_interrupt_clear
 *        - present if PCA_HAS_INT_EXTEND
 *        - write only
 *        - non-cacheable
 *    1b_input_history
 *        - not present on all devices (fake cache)
 *        - store last input value
 *        - cacheable (present) if not PCA_HAS_INT_EXTEND
 *    1b_interrupt_rise
 *        - not present on all devices (fake cache)
 *        - store pins interrupt on rising edge
 *        - cacheable (present) if not PCA_HAS_INT_EXTEND
 *    1b_interrupt_fall
 *        - not present on all devices (fake cache)
 *        - store pins interrupt on falling edge
 *        - cacheable (present) if not PCA_HAS_INT_EXTEND
 */

#define GPIO_PCA_GET_CACHE_SIZE_BY_PART_NO(part_no) ( \
	GPIO_PCA_GET_CACHE_SIZE_BY_PART_NO_NO_INT(part_no) \
	+ (((GPIO_PCA_GET_PART_FLAG_BY_PART_NO(part_no) & PCA_HAS_INT_EXTEND) ? \
			PCA_REG_HAS_INT_EXTEND : PCA_REG_NO_INT_EXTEND) \
	) * GPIO_PCA_GET_PORT_NO_CFG_BY_PART_NO(part_no) \
)
#else /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
#define GPIO_PCA_GET_CACHE_SIZE_BY_PART_NO(part_no) ( \
	GPIO_PCA_GET_CACHE_SIZE_BY_PART_NO_NO_INT(part_no) \
)
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

/**
 * @brief implement pca953x driver
 *
 * @note flags = 0U;
 *
 *       api set    :   standard
 *       ngpios     :   8, 16
 *       part_no    :   pca9534 pca9538 pca9535 pca9539
 */
#define GPIO_PCA_SERIES_FLAG_TYPE_0 (0U)

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
/**
 * cache map for flag = 0U
 */
static const uint8_t gpio_pca_series_cache_map_pca953x[] = {
	PCA_REG_INVALID, /** input_port if not PCA_HAS_OUT_CONFIG, non-cacheable */
	0x00, /** output_port */
/*	0x02,     polarity_inversion  (unused, omitted) */
	0x01, /** configuration */
	PCA_REG_INVALID, /** 2b_output_drive_strength if PCA_HAS_LATCH*/
	PCA_REG_INVALID, /** input_latch if PCA_HAS_LATCH*/
	PCA_REG_INVALID, /** pull_enable if PCA_HAS_PULL */
	PCA_REG_INVALID, /** pull_select if PCA_HAS_PULL */
	PCA_REG_INVALID, /** input_status if PCA_HAS_OUT_CONFIG, non-cacheable */
	PCA_REG_INVALID, /** output_config if PCA_HAS_OUT_CONFIG */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	PCA_REG_INVALID, /** interrupt_mask if PCA_HAS_INT_MASK,
			   * non-cacheable if not PCA_HAS_INT_EXTEND
			   */
	PCA_REG_INVALID, /** int_status if PCA_HAS_INT_MASK, non-cacheable */
	PCA_REG_INVALID, /** 2b_interrupt_edge if PCA_HAS_INT_EXTEND */
	PCA_REG_INVALID, /** interrupt_clear if PCA_HAS_INT_EXTEND, non-cacheable */
# ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	0x02, /** 1b_input_history if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
	0x03, /** 1b_interrupt_rise if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
	0x04, /** 1b_interrupt_fall if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
# endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

static const uint8_t gpio_pca_series_reg_pca9538[] = {
	0x00, /** input_port if not PCA_HAS_OUT_CONFIG, non-cacheable */
	0x01, /** output_port */
/*	0x02,     polarity_inversion  (unused, omitted) */
	0x03, /** configuration */
	PCA_REG_INVALID, /** 2b_output_drive_strength if PCA_HAS_LATCH*/
	PCA_REG_INVALID, /** input_latch if PCA_HAS_LATCH*/
	PCA_REG_INVALID, /** pull_enable if PCA_HAS_PULL */
	PCA_REG_INVALID, /** pull_select if PCA_HAS_PULL */
	PCA_REG_INVALID, /** input_status if PCA_HAS_OUT_CONFIG, non-cacheable */
	PCA_REG_INVALID, /** output_config if PCA_HAS_OUT_CONFIG */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	PCA_REG_INVALID, /** interrupt_mask if PCA_HAS_INT_MASK,
			   * non-cacheable if not PCA_HAS_INT_EXTEND
			   */
	PCA_REG_INVALID, /** int_status if PCA_HAS_INT_MASK */
	PCA_REG_INVALID, /** 2b_interrupt_edge if PCA_HAS_INT_EXTEND */
	PCA_REG_INVALID, /** interrupt_clear if PCA_HAS_INT_EXTEND, non-cacheable */
# ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	PCA_REG_INVALID, /** 1b_input_history if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
	PCA_REG_INVALID, /** 1b_interrupt_rise if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
	PCA_REG_INVALID, /** 1b_interrupt_fall if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
# endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};

#define GPIO_PCA_PORT_NO_PCA_PART_NO_PCA9538 (1U)
#define GPIO_PCA_FLAG_PCA_PART_NO_PCA9538 GPIO_PCA_SERIES_FLAG_TYPE_0
#define GPIO_PCA_PART_CFG_PCA_PART_NO_PCA9538 (&gpio_pca_series_part_cfg_pca9538)

const struct gpio_pca_series_part_config gpio_pca_series_part_cfg_pca9538 = {
	.port_no = GPIO_PCA_PORT_NO_PCA_PART_NO_PCA9538,
	.flags = GPIO_PCA_FLAG_PCA_PART_NO_PCA9538,
	.regs = gpio_pca_series_reg_pca9538,
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
# ifdef GPIO_NXP_PCA_SERIES_DEBUG
	.cache_size = GPIO_PCA_GET_CACHE_SIZE_BY_PART_NO(PCA_PART_NO_PCA9538),
# endif /* GPIO_NXP_PCA_SERIES_DEBUG */
	.cache_map = gpio_pca_series_cache_map_pca953x,
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
};

/**
 * pca9555 share the same register layout with pca9539, with
 * RESET pin repurposed to another address strapping pin.
 * no difference from driver perspective.
 */

#define GPIO_PCA_PORT_NO_PCA_PART_NO_PCA9554 GPIO_PCA_PORT_NO_PCA_PART_NO_PCA9538
#define GPIO_PCA_FLAG_PCA_PART_NO_PCA9554 GPIO_PCA_FLAG_PCA_PART_NO_PCA9538
#define GPIO_PCA_PART_CFG_PCA_PART_NO_PCA9554 (&gpio_pca_series_part_cfg_pca9554)

const struct gpio_pca_series_part_config gpio_pca_series_part_cfg_pca9554 = {
	.port_no = GPIO_PCA_PORT_NO_PCA_PART_NO_PCA9554,
	.flags = GPIO_PCA_FLAG_PCA_PART_NO_PCA9554,
	.regs = gpio_pca_series_reg_pca9538,
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
# ifdef GPIO_NXP_PCA_SERIES_DEBUG
	.cache_size = GPIO_PCA_GET_CACHE_SIZE_BY_PART_NO(PCA_PART_NO_PCA9554),
# endif /* GPIO_NXP_PCA_SERIES_DEBUG */
	.cache_map = gpio_pca_series_cache_map_pca953x,
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
};

static const uint8_t gpio_pca_series_reg_pca9539[] = {
	0x00, /** input_port if not PCA_HAS_OUT_CONFIG, non-cacheable */
	0x02, /** output_port */
/*	0x04,     polarity_inversion  (unused, omitted) */
	0x06, /** configuration */
	PCA_REG_INVALID, /** 2b_output_drive_strength if PCA_HAS_LATCH*/
	PCA_REG_INVALID, /** input_latch if PCA_HAS_LATCH*/
	PCA_REG_INVALID, /** pull_enable if PCA_HAS_PULL */
	PCA_REG_INVALID, /** pull_select if PCA_HAS_PULL */
	PCA_REG_INVALID, /** input_status if PCA_HAS_OUT_CONFIG, non-cacheable */
	PCA_REG_INVALID, /** output_config if PCA_HAS_OUT_CONFIG */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	PCA_REG_INVALID, /** interrupt_mask if PCA_HAS_INT_MASK,
			   * non-cacheable if not PCA_HAS_INT_EXTEND
			   */
	PCA_REG_INVALID, /** int_status if PCA_HAS_INT_MASK */
	PCA_REG_INVALID, /** 2b_interrupt_edge if PCA_HAS_INT_EXTEND */
	PCA_REG_INVALID, /** interrupt_clear if PCA_HAS_INT_EXTEND, non-cacheable */
# ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	PCA_REG_INVALID, /** 1b_input_history if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
	PCA_REG_INVALID, /** 1b_interrupt_rise if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
	PCA_REG_INVALID, /** 1b_interrupt_fall if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
# endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};

#define GPIO_PCA_PORT_NO_PCA_PART_NO_PCA9539 (2U)
#define GPIO_PCA_FLAG_PCA_PART_NO_PCA9539 GPIO_PCA_SERIES_FLAG_TYPE_0
#define GPIO_PCA_PART_CFG_PCA_PART_NO_PCA9539 (&gpio_pca_series_part_cfg_pca9539)

const struct gpio_pca_series_part_config gpio_pca_series_part_cfg_pca9539 = {
	.port_no = GPIO_PCA_PORT_NO_PCA_PART_NO_PCA9539,
	.flags = GPIO_PCA_FLAG_PCA_PART_NO_PCA9539,
	.regs = gpio_pca_series_reg_pca9539,
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
# ifdef GPIO_NXP_PCA_SERIES_DEBUG
	.cache_size = GPIO_PCA_GET_CACHE_SIZE_BY_PART_NO(PCA_PART_NO_PCA9539),
# endif /* GPIO_NXP_PCA_SERIES_DEBUG */
	.cache_map = gpio_pca_series_cache_map_pca953x,
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
};

/**
 * pca9555 share the same register layout with pca9539, with
 * RESET pin repurposed to another address strapping pin.
 * no difference from driver perspective.
 */

#define GPIO_PCA_PORT_NO_PCA_PART_NO_PCA9555 GPIO_PCA_PORT_NO_PCA_PART_NO_PCA9539
#define GPIO_PCA_FLAG_PCA_PART_NO_PCA9555 GPIO_PCA_FLAG_PCA_PART_NO_PCA9539
#define GPIO_PCA_PART_CFG_PCA_PART_NO_PCA9555 (&gpio_pca_series_part_cfg_pca9555)

const struct gpio_pca_series_part_config gpio_pca_series_part_cfg_pca9555 = {
	.port_no = GPIO_PCA_PORT_NO_PCA_PART_NO_PCA9555,
	.flags = GPIO_PCA_FLAG_PCA_PART_NO_PCA9555,
	.regs = gpio_pca_series_reg_pca9539,
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
# ifdef GPIO_NXP_PCA_SERIES_DEBUG
	.cache_size = GPIO_PCA_GET_CACHE_SIZE_BY_PART_NO(PCA_PART_NO_PCA9555),
# endif /* GPIO_NXP_PCA_SERIES_DEBUG */
	.cache_map = gpio_pca_series_cache_map_pca953x,
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
};

/**
 * @brief implement pcal65xx driver
 *
 * @note flags = PCA_HAS_LATCH
 *             | PCA_HAS_PULL
 *             | PCA_HAS_INT_MASK
 *             | PCA_HAS_INT_EXTEND
 *             | PCA_HAS_OUT_CONFIG
 *
 *       api set    :   pcal65xx
 *       ngpios     :   24, 32
 *       part_no    :   pcal6524 pcal6534
 */
#define GPIO_PCA_SERIES_FLAG_TYPE_3 (PCA_HAS_LATCH | PCA_HAS_PULL | PCA_HAS_INT_MASK \
		   | PCA_HAS_INT_EXTEND | PCA_HAS_OUT_CONFIG)

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
/**
 * cache map for flag = PCA_HAS_LATCH
 *                    | PCA_HAS_PULL
 *                    | PCA_HAS_INT_MASK
 *                    | PCA_HAS_INT_EXTEND
 *                    | PCA_HAS_OUT_CONFIG
 */
static const uint8_t gpio_pca_series_cache_map_pcal65xx[] = {
	PCA_REG_INVALID, /** input_port if not PCA_HAS_OUT_CONFIG, non-cacheable */
	0x00, /** output_port */
/*	0x02,     polarity_inversion  (unused, omitted) */
	0x01, /** configuration */
	0x02, /** 2b_output_drive_strength if PCA_HAS_LATCH*/
	0x04, /** input_latch if PCA_HAS_LATCH*/
	0x05, /** pull_enable if PCA_HAS_PULL */
	0x06, /** pull_select if PCA_HAS_PULL */
	PCA_REG_INVALID, /** input_status if PCA_HAS_OUT_CONFIG, non-cacheable */
	0x07, /** output_config if PCA_HAS_OUT_CONFIG */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	0x08, /** interrupt_mask if PCA_HAS_INT_MASK,
		* non-cacheable if not PCA_HAS_INT_EXTEND
		*/
	PCA_REG_INVALID, /** int_status if PCA_HAS_INT_MASK, non-cacheable */
	0x09, /** 2b_interrupt_edge if PCA_HAS_INT_EXTEND */
	PCA_REG_INVALID, /** interrupt_clear if PCA_HAS_INT_EXTEND, non-cacheable */
# ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	PCA_REG_INVALID, /** 1b_input_history if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
	PCA_REG_INVALID, /** 1b_interrupt_rise if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
	PCA_REG_INVALID, /** 1b_interrupt_fall if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
# endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

static const uint8_t gpio_pca_series_reg_pcal6524[] = {
	PCA_REG_INVALID, /** input_port if not PCA_HAS_OUT_CONFIG, non-cacheable */
	0x04, /** output_port */
/*	0x08,     polarity_inversion  (unused, omitted) */
	0x0c, /** configuration */
	0x40, /** 2b_output_drive_strength if PCA_HAS_LATCH*/
	0x48, /** input_latch if PCA_HAS_LATCH*/
	0x4c, /** pull_enable if PCA_HAS_PULL */
	0x50, /** pull_select if PCA_HAS_PULL */
	0x6c, /** input_status if PCA_HAS_OUT_CONFIG, non-cacheable */
	0x70, /** output_config if PCA_HAS_OUT_CONFIG */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	0x54, /** interrupt_mask if PCA_HAS_INT_MASK,
		* non-cacheable if not PCA_HAS_INT_EXTEND
		*/
	0x58, /** int_status if PCA_HAS_INT_MASK */
	0x60, /** 2b_interrupt_edge if PCA_HAS_INT_EXTEND */
	0x68, /** interrupt_clear if PCA_HAS_INT_EXTEND, non-cacheable */
# ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	PCA_REG_INVALID, /** 1b_input_history if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
	PCA_REG_INVALID, /** 1b_interrupt_rise if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
	PCA_REG_INVALID, /** 1b_interrupt_fall if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
# endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};

#define GPIO_PCA_PORT_NO_PCA_PART_NO_PCAL6524 (3U)
#define GPIO_PCA_FLAG_PCA_PART_NO_PCAL6524 GPIO_PCA_SERIES_FLAG_TYPE_3
#define GPIO_PCA_PART_CFG_PCA_PART_NO_PCAL6524 (&gpio_pca_series_part_cfg_pcal6524)

const struct gpio_pca_series_part_config gpio_pca_series_part_cfg_pcal6524 = {
	.port_no = GPIO_PCA_PORT_NO_PCA_PART_NO_PCAL6524,
	.flags = GPIO_PCA_FLAG_PCA_PART_NO_PCAL6524,
	.regs = gpio_pca_series_reg_pcal6524,
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
# ifdef GPIO_NXP_PCA_SERIES_DEBUG
	.cache_size = GPIO_PCA_GET_CACHE_SIZE_BY_PART_NO(PCA_PART_NO_PCAL6524),
# endif /* GPIO_NXP_PCA_SERIES_DEBUG */
	.cache_map = gpio_pca_series_cache_map_pcal65xx,
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
};

static const uint8_t gpio_pca_series_reg_pcal6534[] = {
	PCA_REG_INVALID, /** input_port if not PCA_HAS_OUT_CONFIG, non-cacheable */
	0x04, /** output_port */
/*	0x08,     polarity_inversion  (unused, omitted) */
	0x0c, /** configuration */
	0x40, /** 2b_output_drive_strength if PCA_HAS_LATCH*/
	0x48, /** input_latch if PCA_HAS_LATCH*/
	0x4c, /** pull_enable if PCA_HAS_PULL */
	0x50, /** pull_select if PCA_HAS_PULL */
	0x6c, /** input_status if PCA_HAS_OUT_CONFIG, non-cacheable */
	0x70, /** output_config if PCA_HAS_OUT_CONFIG */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	0x54, /** interrupt_mask if PCA_HAS_INT_MASK,
		* non-cacheable if not PCA_HAS_INT_EXTEND
		*/
	0x58, /** int_status if PCA_HAS_INT_MASK */
	0x60, /** 2b_interrupt_edge if PCA_HAS_INT_EXTEND */
	0x68, /** interrupt_clear if PCA_HAS_INT_EXTEND, non-cacheable */
# ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	PCA_REG_INVALID, /** 1b_input_history if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
	PCA_REG_INVALID, /** 1b_interrupt_rise if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
	PCA_REG_INVALID, /** 1b_interrupt_fall if PCA_HAS_LATCH and not PCA_HAS_INT_EXTEND */
# endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};

#define GPIO_PCA_PORT_NO_PCA_PART_NO_PCAL6534 (4U)
#define GPIO_PCA_FLAG_PCA_PART_NO_PCAL6534 GPIO_PCA_SERIES_FLAG_TYPE_3
#define GPIO_PCA_PART_CFG_PCA_PART_NO_PCAL6534 (&gpio_pca_series_part_cfg_pcal6534)

const struct gpio_pca_series_part_config gpio_pca_series_part_cfg_pcal6534 = {
	.port_no = GPIO_PCA_PORT_NO_PCA_PART_NO_PCAL6534,
	.flags = GPIO_PCA_FLAG_PCA_PART_NO_PCAL6534,
	.regs = gpio_pca_series_reg_pcal6534,
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
# ifdef GPIO_NXP_PCA_SERIES_DEBUG
	.cache_size = GPIO_PCA_GET_CACHE_SIZE_BY_PART_NO(PCA_PART_NO_PCAL6534),
# endif /* GPIO_NXP_PCA_SERIES_DEBUG */
	.cache_map = gpio_pca_series_cache_map_pcal65xx,
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
};

/**
 * @brief common device instance
 *
 */
#define GPIO_PCA_SERIES_DEVICE_INSTANCE(inst, part_no) \
	static const struct gpio_pca_series_config gpio_##part_no##_##inst##_cfg = { \
		.common = { \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst), \
		}, \
		.i2c = I2C_DT_SPEC_INST_GET(inst), \
		.part_cfg = GPIO_PCA_GET_PART_CFG_BY_PART_NO(part_no), \
		.gpio_rst = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}), \
		IF_ENABLED(CONFIG_GPIO_PCA_SERIES_INTERRUPT, \
			(.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {}),)) \
	}; \
	static uint8_t gpio_##part_no##_##inst##_reg_cache[COND_CODE_1( \
		CONFIG_GPIO_PCA_SERIES_CACHE_ALL, \
		(GPIO_PCA_GET_CACHE_SIZE_BY_PART_NO(part_no) /** true */\
		 ), \
		(sizeof(struct gpio_pca_series_reg_cache_mini) /** false */ \
		 ))]; \
	static struct gpio_pca_series_data gpio_##part_no##_##inst##_data = { \
		.lock = Z_SEM_INITIALIZER(gpio_##part_no##_##inst##_data.lock, 1, 1), \
		.cache = (void *)gpio_##part_no##_##inst##_reg_cache, \
	}; \
	DEVICE_DT_INST_DEFINE(inst, gpio_pca_series_init, NULL, \
		&gpio_##part_no##_##inst##_data, \
		&gpio_##part_no##_##inst##_cfg, POST_KERNEL, \
		CONFIG_GPIO_PCA_SERIES_INIT_PRIORITY, \
		GPIO_PCA_GET_API_BY_PART_NO(part_no));


#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pca9538
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_PCA_SERIES_DEVICE_INSTANCE, PCA_PART_NO_PCA9538)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pca9539
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_PCA_SERIES_DEVICE_INSTANCE, PCA_PART_NO_PCA9539)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pca9554
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_PCA_SERIES_DEVICE_INSTANCE, PCA_PART_NO_PCA9554)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pca9555
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_PCA_SERIES_DEVICE_INSTANCE, PCA_PART_NO_PCA9555)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pcal6524
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_PCA_SERIES_DEVICE_INSTANCE, PCA_PART_NO_PCAL6524)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pcal6534
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_PCA_SERIES_DEVICE_INSTANCE, PCA_PART_NO_PCAL6534)
