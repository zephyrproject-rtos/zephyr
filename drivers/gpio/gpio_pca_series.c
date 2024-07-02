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

/* Feature flags */
#define PCA_HAS_LATCH		BIT(0)	/** + output_drive_strength, + input_latch */
#define PCA_HAS_PULL		BIT(1)	/** + pull_enable, + pull_select */
#define PCA_HAS_INT_MASK	BIT(2)	/** + interrupt_mask, + int_status */
#define PCA_HAS_INT_EXTEND	BIT(3)	/** + interrupt_edge, + interrupt_clear */
#define PCA_HAS_OUT_CONFIG	BIT(4)	/** + input_status, + output_config */

/* get port and pin from gpio_pin_t */
#define PCA_PORT(gpio_pin)		(gpio_pin >> 3)
#define PCA_PIN(gpio_pin)		(gpio_pin & GENMASK(2, 0))

#define PCA_REG_INVALID (0xff)

/**
 * @brief part number definition.
 *
 * @note must be consistent with @ref part_no in dt binding
 */
enum gpio_pca_series_part_no {
	PCA_PART_NO_PCA9538,
	PCA_PART_NO_PCA9539,
	PCA_PART_NO_PCA9574,
	PCA_PART_NO_PCA9575,
	PCA_PART_NO_PCAL9538,
	PCA_PART_NO_PCAL9539,
	PCA_PART_NO_PCAL6408A,
	PCA_PART_NO_PCAL6416A,
	PCA_PART_NO_PCAL6524,
	PCA_PART_NO_PCAL6534,
};

/**
 * @brief part name definition for debug.
 *
 * @note must be consistent with @ref part_no in dt binding
 */
const char *const gpio_pca_series_part_name[] = {
	"pca9538",
	"pca9539",
	"pca9574",
	"pca9575",
	"pcal9538",
	"pcal9539",
	"pcal6408a",
	"pcal6416a",
	"pcal6524",
	"pcal6534",
};

/**
 * Device reg layout types:
 * - Type 1: PCA953X, PCA955X
 * - Type 2: PCAL953X, PCAL955X, PCAL64XX
 * - Type 3: PCA957X
 * - Type 4: PCAL6524, PCAL6534
 */

enum gpio_pca_series_reg_type {			/** Type1 Type2 Type3 Type4 */
	PCA_REG_TYPE_1B_INPUT_PORT = 0U,	/**   x     x     x     x   */
	PCA_REG_TYPE_1B_OUTPUT_PORT,		/**   x     x     x     x   */
/*  PCA_REG_TYPE_1B_POLARITY_INVERSION,		/**   x     x     x     x   * (unused, omitted) */
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
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT	/**                         */
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

static inline uint8_t gpio_pca_series_reg_get_addr(const struct device *dev,
						   enum gpio_pca_series_reg_type reg_type)
{
	const struct gpio_pca_series_config *cfg = dev->config;

	return cfg->part_cfg->regs[reg_type];
}

/**
 * @brief get read size for register
 *
 * @param dev
 * @param reg_type
 * @return uint32_t size in bytes
 *                  0 if fail
 */
static inline uint32_t gpio_pca_series_reg_get_size(const struct device *dev,
							enum gpio_pca_series_reg_type reg_type)
{
	const struct gpio_pca_series_config *cfg = dev->config;

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
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
		return cfg->part_cfg->port_no;
	case PCA_REG_TYPE_2B_OUTPUT_DRIVE_STRENGTH:
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	case PCA_REG_TYPE_2B_INTERRUPT_EDGE:
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
		return (cfg->part_cfg->port_no * 2U);
	default:
		return 0; /** should never happen */
	}
}

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
/** forward declarations */
static inline int gpio_pca_series_reg_update_cache(struct device *dev,
						   enum gpio_pca_series_reg_type reg_type,
						   uint8_t *buf);
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

/**
 * @brief read register with i2c interface.
 *
 * @note if CONFIG_GPIO_PCA_SERIES_CACHE_ALL is enabled, this will
 *          also update reg cache.
 *
 * @param dev
 * @param reg_type
 * @param buf pointer to read data
 * @return int 0        if success
 *             -EFAULT  if register is not supported
 *             -EIO     if i2c failure
 */
static inline int gpio_pca_series_reg_read(const struct device *dev,
					   enum gpio_pca_series_reg_type reg_type, uint8_t *buf)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	int ret;
	uint32_t size = gpio_pca_series_reg_get_size(dev, reg_type);
	uint8_t addr = gpio_pca_series_reg_get_addr(dev, reg_type);

	LOG_DBG("reg read type %d addr 0x%x len %d", reg_type, addr, size);

	if (addr == PCA_REG_INVALID) {
		LOG_ERR("trying to read unsupported reg, reg type %d", reg_type);
		return -EFAULT;
	}

	ret = i2c_burst_read_dt(&cfg->i2c, addr, buf, size);
	if (ret) {
		LOG_ERR("i2c read error [%d]", ret);
		return ret;
	}

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	gpio_pca_series_reg_update_cache(dev, reg_type, buf);
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

	LOG_DBG("reg read return %d", ret);
	return ret;
}

/**
 * @brief write register with i2c interface.
 *
 * @note if CONFIG_GPIO_PCA_SERIES_CACHE_ALL is enabled, this will
 *          also update reg cache.
 *
 * @param dev
 * @param reg_type
 * @param buf pointer to read data
 * @return int 0        if success
 *             -EFAULT  if register is not supported
 *             -EIO     if i2c failure
 */
static inline int gpio_pca_series_reg_write(const struct device *dev,
					enum gpio_pca_series_reg_type reg_type, uint8_t *buf)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	int ret;
	uint32_t size = gpio_pca_series_reg_get_size(dev, reg_type);
	uint8_t addr = gpio_pca_series_reg_get_addr(dev, reg_type);

	LOG_DBG("reg write type %d addr 0x%x len %d", reg_type, addr, size);

	if (addr == PCA_REG_INVALID) {
		LOG_ERR("trying to write unsupported reg");
		return -EFAULT;
	}

	ret = i2c_burst_write_dt(&cfg->i2c, addr, buf, size);
	if (ret) {
		LOG_ERR("i2c write error [%d]", ret);
		return ret;
	}

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	gpio_pca_series_reg_update_cache(dev, reg_type, buf);
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

	LOG_DBG("reg write return %d", ret);
	return ret;
}

/**
 * }
 * gpio_pca_reg_access_api
 */

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
/**
 * gpio_pca_reg_cache_api
 * {
 */
/**
 * @brief get memory offset of register cache from register type
 *
 * @param dev
 * @param reg_type      the type of register
 * @param offset        offset in bytes related to cache pointer
 * @return int 0        if success
 *             -EACCES  if reg is not used or uncacheable
 */
static inline int gpio_pca_series_reg_cache_get_offset(struct device *dev,
							   enum gpio_pca_series_reg_type reg_type,
							   uint8_t *offset)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	struct gpio_pca_series_data *data = dev->data;

	if (cfg->part_cfg->cache_map[reg_type] == PCA_REG_INVALID) {
		return -EACCES;
	}
	*offset = cfg->part_cfg->cache_map[reg_type] * cfg->part_cfg->port_no;
	return 0;
}

/**
 * @brief read register value from reg cache
 *
 * @param dev
 * @param reg_type
 * @param buf pointer to read data
 * @return int 0        if success
 *             -EACCES  if register is uncacheable
 */
static inline int gpio_pca_series_reg_read_cache(const struct device *dev,
						 enum gpio_pca_series_reg_type reg_type,
						 uint8_t *buf)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	struct gpio_pca_series_data *data = dev->data;
	int ret = 0;
	uint32_t offset;
	uint32_t size = gpio_pca_series_reg_get_size(dev, reg_type);

	LOG_DBG("cache read type %d len %d", reg_type, size);

	ret = gpio_pca_series_reg_cache_get_offset(dev, reg_type, &offset);
	if (ret != 0) {
		LOG_ERR("can not get noncacheable reg");
		ret = -EACCES;
		goto out;
	}
	memcpy(buf, ((uint8_t *)&data->cache) + offset, size);

out:
	LOG_DBG("cache read return %d", ret);
	return ret;
}

/**
 * @brief update register cache from device or existing value.
 *
 * @param dev
 * @param reg_type
 * @param buf pointer to new data to update from. if NULL, read reg value from device
 * @return int 0        if success
 *             -EACCES  if register is uncacheable
 *             -EFAULT  if register is unsupported when trying to read
 */
static inline int gpio_pca_series_reg_update_cache(const struct device *dev,
						   enum gpio_pca_series_reg_type reg_type,
						   const uint8_t *buf)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	struct gpio_pca_series_data *data = dev->data;
	int ret = 0;
	uint32_t offset;
	uint32_t size = gpio_pca_series_reg_get_size(dev, reg_type);
	struct i2c_msg msg[2];

	LOG_DBG("cache update type %d len %d", reg_type, size);

	ret = gpio_pca_series_reg_cache_get_offset(dev, reg_type, &offset);
	if (ret != 0) {
		LOG_ERR("can not update noncacheable reg");
		return -EACCES;
	}
	if (buf != NULL) {
		/** update cache from buf */
		memcpy(buf, ((uint8_t *)&data->cache) + offset, size);
		goto out;
	}

	uint8_t addr = gpio_pca_series_reg_get_addr(dev, reg_type);

	LOG_INF("reg read type %d addr 0x%x len %d", reg_type, addr, size);

	if (addr == PCA_REG_INVALID) {
		LOG_ERR("trying to read unsupported reg");
		ret = -EFAULT;
		goto out;
	}

	msg[0].buf = &addr;
	msg[0].len = 1;
	msg[0].flags = I2C_MSG_WRITE;
	msg[1].buf = buf;
	msg[1].len = size;
	msg[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	ret =  i2c_transfer(dev, msg, 2, addr);
	if (ret) {
		LOG_ERR("i2c read error [%d]", ret);
		goto out;
	}

out:
	LOG_DBG("cache read return %d", ret);
	return ret;
}

/**
 * @brief Split the value into two 32-bit buffer
 *
 * take 24-bit device as example:
 * buf[0]  B3 B2 B1 B0  -->  00 B2 B1 B0
 * buf[1]  00 00 B5 B4  -->  00 B5 B4 B3
 *
 * @param dev
 * @param buf
 */
static inline void gpio_pca_series_cache_2b_split(const struct device *dev, uint32_t buf[2])
{
	const struct gpio_pca_series_config *cfg = dev->config;
	uint32_t lshift = (cfg->part_cfg->port_no) << 3U;
	uint32_t hshift = (sizeof(uint32_t) - cfg->part_cfg->port_no) << 3U;

	buf[1] = (buf[1] << hshift) | (buf[0] >> lshift);
	buf[0] = (buf[0] << lshift) >> lshift;
}

/**
 * @brief Merge the value into low address
 *
 * take 24-bit device as example:
 * buf[0]  00 B2 B1 B0  -->  B3 B2 B1 B0
 * buf[1]  00 B5 B4 B3  -->  00 00 B5 B4
 *
 * @param dev
 * @param buf
 */
static inline void gpio_pca_series_cache_2b_merge(const struct device *dev, uint32_t buf[2])
{
	const struct gpio_pca_series_config *cfg = dev->config;
	uint32_t lshift = (cfg->part_cfg->port_no) << 3U;
	uint32_t hshift = (sizeof(uint32_t) - cfg->part_cfg->port_no) << 3U;

	buf[0] = (buf[0]) | (buf[1] << lshift);
	buf[1] = (buf[1] >> hshift);
}

#else /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

#define gpio_pca_series_reg_read_cache gpio_pca_series_reg_read

struct gpio_pca_series_reg_cache_mini {
	uint32_t output;   /** cache output value for faster output */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	uint32_t input;    /** only used when interrupt mask & edge config is not present */
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
	LOG_DBG("mini cache addr 0x%8.8x", cache);
	return cache;
}

static inline void gpio_pca_series_reg_cache_mini_reset(const struct device *dev)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	struct gpio_pca_series_reg_cache_mini *cache =
		gpio_pca_series_reg_cache_mini_get(dev);

	cache->output = cfg->common.port_pin_mask;
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	cache->input = 0;
	cache->int_rise = 0;
	cache->int_fall = 0;
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
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
 * reset value to device registers. It will not touch
 * cache.
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static inline void gpio_pca_series_reset(const struct device *dev)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	struct gpio_pca_series_data *data = dev->data;
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
	gpio_pca_series_reg_write(dev, PCA_REG_TYPE_2B_OUTPUT_DRIVE_STRENGTH, reset_value_1);
	gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_INPUT_LATCH, reset_value_0);
	gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_PULL_ENABLE, reset_value_0);
	gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_PULL_SELECT, reset_value_1);
	gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_OUTPUT_CONFIG, reset_value_0);
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_INTERRUPT_MASK, reset_value_1);
	gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_INTERRUPT_STATUS, reset_value_0);
	gpio_pca_series_reg_write(dev, PCA_REG_TYPE_2B_INTERRUPT_EDGE, reset_value_1);
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */

}

#ifdef GPIO_NXP_PCA_SERIES_DEBUG
void gpio_pca_series_register_dump(const struct device *dev)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	int ret;

	LOG_DBG("**** device debug dump ****");
	LOG_DBG("part_no: %s", gpio_pca_series_part_name[cfg->part_cfg->port_no]);
	LOG_DBG("register profile:");
	LOG_DBG("type\t"
		"name\t"
		"address\t"
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
		"cache\t"
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
	);
	for (int reg_type = 0; reg_type < PCA_REG_TYPE_COUNT; ++reg_type) {
		uint8_t reg = cfg->part_cfg->regs[reg_type];
		uint8_t reg_size = gpio_pca_series_reg_get_size(dev, reg_type);
		uint8_t reg_val[8];
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
		uint8_t cache;
		uint8_t cache_val[8];

		ret = gpio_pca_series_reg_cache_get_offset(dev, reg_type, &cache);
		if (ret != 0) {
			cache = PCA_REG_INVALID;
		}
		if (reg == PCA_REG_INVALID && cache == PCA_REG_INVALID)
#else
		if (reg == PCA_REG_INVALID)
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
		{
			continue;
		}

		if (reg != PCA_REG_INVALID) {
			ret = gpio_pca_series_reg_read(dev, reg_type, reg_val);
			if (ret != 0) {
				LOG_ERR("read reg error from reg type %d, invalidate this reg",
					reg_type);
				reg = PCA_REG_INVALID;
			}
		}
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
		if (cache != PCA_REG_INVALID) {
			ret = gpio_pca_series_reg_read_cache(dev, reg_type, cache_val);
			if (ret != 0) {
				LOG_ERR("read reg cache error from reg type %d, invalidate this "
					"reg cache",
					reg_type);
				reg = PCA_REG_INVALID;
			}
		}
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

		/** do_print */
		LOG_DBG("%.2d\t"
			"%s\t"
			"0x%2.2x\t"
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
			"0x%2.2x\t"
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
			,
			reg_type, gpio_pca_series_reg_name[reg_type], reg
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
			,
			cache
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
		);
	}
}
#endif /* GPIO_NXP_PCA_SERIES_DEBUG */

/**
 * }
 * gpio_pca_custom_api
 */

/**
 * gpio_pca_zephyr_gpio_api
 * {
 */

/**
 * @brief configure gpio port.
 *
 * @note This API applies to all supported part no. Capabilities (
 *       PCA_HAS_PULL and PCA_HAS_OUT_CONFIG) are evaluated and handled.
 *
 * @param dev
 * @param pin
 * @param flags
 * @return int 0            if success
 *             -ENOTSUP     if unsupported config flags are provided
 */
static int gpio_pca_series_pin_configure(const struct device *dev,
		gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	struct gpio_pca_series_data *data = dev->data;
	uint32_t reg_value;
	int ret;

	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	if (((flags & GPIO_SINGLE_ENDED) != 0U) &&
		(cfg->part_cfg->flags & PCA_HAS_OUT_CONFIG) == 0U) {
		return -ENOTSUP;
	}

	if (((flags & GPIO_PULL_UP) != 0) || ((flags & GPIO_PULL_DOWN) != 0)) {
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

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		if (cfg->part_cfg->flags & PCA_HAS_OUT_CONFIG) {
			/* configure PCA_REG_TYPE_1B_OUTPUT_CONFIG */
			ret = gpio_pca_series_reg_read_cache(dev,
				PCA_REG_TYPE_1B_OUTPUT_CONFIG, (uint8_t *)&reg_value);
			reg_value |= (BIT(pin)); /* set bit to set open-drain */
			ret = gpio_pca_series_reg_write(dev,
				PCA_REG_TYPE_1B_OUTPUT_CONFIG, (uint8_t *)&reg_value);
		}
	} else {
		if (cfg->part_cfg->flags & PCA_HAS_OUT_CONFIG) {
			ret = gpio_pca_series_reg_read_cache(dev,
				PCA_REG_TYPE_1B_OUTPUT_CONFIG, (uint8_t *)&reg_value);
			reg_value &= (~BIT(pin)); /* clear bit to set push-pull */
			ret = gpio_pca_series_reg_write(dev,
			 PCA_REG_TYPE_1B_OUTPUT_CONFIG, (uint8_t *)&reg_value);
		}
	}

	if (((flags & GPIO_PULL_UP) != 0) || ((flags & GPIO_PULL_DOWN) != 0)) {
		if (cfg->part_cfg->flags & PCA_HAS_PULL) {
			/* configure PCA_REG_TYPE_1B_PULL_SELECT */
			ret = gpio_pca_series_reg_read_cache(dev,
				PCA_REG_TYPE_1B_PULL_SELECT, (uint8_t *)&reg_value);
			if (flags & GPIO_PULL_UP) {
				reg_value |= (BIT(pin));
			} else {
				reg_value &= (~BIT(pin));
			}
			ret = gpio_pca_series_reg_write(dev,
				PCA_REG_TYPE_1B_PULL_SELECT, (uint8_t *)&reg_value);

			/* configure PCA_REG_TYPE_1B_PULL_ENABLE */
			ret = gpio_pca_series_reg_read_cache(dev,
				PCA_REG_TYPE_1B_PULL_ENABLE, (uint8_t *)&reg_value);
			reg_value &= (~BIT(pin)); /* clear bit to disable pull */
			ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_PULL_ENABLE,
							(uint8_t *)&reg_value);
		} else {
			ret = -ENOTSUP;
			goto out;
		}
	} else {
		if (cfg->part_cfg->flags & PCA_HAS_PULL) {
			ret = gpio_pca_series_reg_read_cache(dev,
				PCA_REG_TYPE_1B_PULL_ENABLE, (uint8_t *)&reg_value);
			reg_value &= (~BIT(pin)); /* clear bit to disable pull */
			ret = gpio_pca_series_reg_write(dev,
				PCA_REG_TYPE_1B_PULL_ENABLE, (uint8_t *)&reg_value);
		}
	}

	if (flags & GPIO_INPUT) {
		/* configure PCA_REG_TYPE_1B_CONFIGURATION */
		ret = gpio_pca_series_reg_read_cache(dev,
			PCA_REG_TYPE_1B_CONFIGURATION, (uint8_t *)&reg_value);
		reg_value |= (BIT(pin)); /* set bit to set input */
		ret = gpio_pca_series_reg_write(dev,
			PCA_REG_TYPE_1B_CONFIGURATION, (uint8_t *)&reg_value);
	} else {
		uint32_t out_old, out;
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
		/* get output register old value from reg cache */
		ret = gpio_pca_series_reg_read_cache(dev, PCA_REG_TYPE_1B_OUTPUT_PORT,
							 (uint8_t *)&out_old);
		if (ret) {
			ret = -EINVAL; /* should never fail */
			goto out;
		}
#else  /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
		out_old = gpio_pca_series_reg_cache_mini_get(dev)->output;
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			out = out_old | BIT(pin);
			ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_OUTPUT_PORT,
							(uint8_t *)&out);
		}
		if (flags & GPIO_OUTPUT_INIT_LOW) {
			out = out_old & (~BIT(pin));
			ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_OUTPUT_PORT,
							(uint8_t *)&out);
		}
		if (ret == 0) {
#ifndef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
			/** update output register old value to void* cache raw value */
			gpio_pca_series_reg_cache_mini_get(dev)->output = out;
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
		}
		/* configure PCA_REG_TYPE_1B_CONFIGURATION */
		ret = gpio_pca_series_reg_read_cache(dev, PCA_REG_TYPE_1B_CONFIGURATION,
							 (uint8_t *)&reg_value);
		reg_value &= (~BIT(pin)); /* clear bit to set output */
		ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_CONFIGURATION,
						(uint8_t *)&reg_value);
	}

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
	k_sem_take(&data->lock, K_FOREVER);

	/* Read Input Register */
	ret = gpio_pca_series_reg_read(dev,
		PCA_REG_TYPE_1B_INPUT_PORT, (uint8_t *)&input_data);
	if (ret) {
		goto out;
	}

	*value = input_data;
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
#ifndef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	/** cache input for interrupt handling */
	gpio_pca_series_reg_cache_mini_get(dev)->input = input_data;
	/* FIXME: handle interrupt here, fire callbacks as needed */
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */

out:
	k_sem_give(&data->lock);
	LOG_DBG("dev %s standard_read return %d result 0x%8.8lx",
		dev->name, ret, input_data);
	return ret;
}

/**
 * @brief read gpio port
 *
 * @note This API is used for part no with PCA_HAS_INT_EXTEND capability.
 *       It read input_status register, which will NOT clear interrupt masks.
 *
 * @param dev
 * @param value
 * @return int 0            if success
 *             -EWOULDBLOCK if called from ISR context
 */
static int gpio_pca_series_port_read_extended(
	const struct device *dev, gpio_port_value_t *value)
{
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
	LOG_DBG("read %x got %d", input_data, ret);
	if (ret == 0) {
		*value = input_data;
	}

	k_sem_give(&data->lock);
	LOG_DBG("dev %s extended_read return %d result 0x%8.8lx",
		dev->name, ret, input_data);
	return ret;
}

static int gpio_pca_series_port_write(const struct device *dev,
	gpio_port_pins_t mask, gpio_port_value_t value, gpio_port_value_t toggle)
{
	struct gpio_pca_series_data *data = dev->data;
	uint32_t out_old;
	uint32_t out;
	int ret;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	LOG_DBG("dev %s write mask 0x%8.8lx value 0x%8.8lx toggle 0x%8.8lx",
		dev->name, mask, value, toggle);
	k_sem_take(&data->lock, K_FOREVER);

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	/** get output register old value from reg cache */
	ret = gpio_pca_series_reg_read_cache(dev, PCA_REG_TYPE_1B_OUTPUT_PORT,
						 (uint8_t *)&out_old);
	if (ret) {
		return -EINVAL; /** should never fail */
	}
#else  /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
	LOG_DBG("access address 0x%8.8lx", (uint32_t)data->cache);
	out_old = gpio_pca_series_reg_cache_mini_get(dev)->output;
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

	out = ((out_old & ~mask) | (value & mask)) ^ toggle;

	ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_OUTPUT_PORT, (uint8_t *)&out);
	if (ret == 0) {
#ifndef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
		/** update output register old value to void* cache raw value */
		gpio_pca_series_reg_cache_mini_get(dev)->output = out;
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
	}

	k_sem_give(&data->lock);

	LOG_DBG("dev %s write return %d result 0x%8.8lx", dev->name, ret, out);
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
	uint32_t *int_rise, *int_fall;
	uint32_t int_mask, input_latch;
	int ret;

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
	uint32_t buf[2];

	/** read from cache even if this register is not present on device */
	ret = gpio_pca_series_reg_read_cache(dev, PCA_REG_TYPE_2B_INTERRUPT_EDGE,
						 (uint8_t *)&buf);
	if (ret) {
		LOG_ERR("get current interrupt config fail [%d]", ret);
		goto out;
	}
	/** split the value into two 32-bit buffer */
	gpio_pca_series_cache_2b_split(dev, buf);

	int_rise = &buf[0];
	int_fall = &buf[1];
#else
	int_rise = gpio_pca_series_reg_cache_mini_get(dev)->int_rise;
	int_fall = gpio_pca_series_reg_cache_mini_get(dev)->int_fall;
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

	if (cfg->part_cfg->flags & PCA_HAS_INT_MASK) {
		ret = gpio_pca_series_reg_read_cache(dev, PCA_REG_TYPE_1B_INTERRUPT_MASK,
							 (uint8_t *)&int_mask);
		if (ret) {
			goto out;
		}
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		(*int_fall) &= ~BIT(pin);
		(*int_rise) &= ~BIT(pin);
		if (cfg->part_cfg->flags & PCA_HAS_INT_MASK) {
			int_mask |= BIT(pin); /** set 1 to disable interrupt */
		}
	} else {
		if (trig == GPIO_INT_TRIG_BOTH) {
			(*int_fall) |= BIT(pin);
			(*int_rise) |= BIT(pin);
		} else if (trig == GPIO_INT_TRIG_LOW) {
			(*int_fall) |= BIT(pin);
			(*int_rise) &= ~BIT(pin);
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			(*int_fall) &= ~BIT(pin);
			(*int_rise) |= BIT(pin);
		}
		if (cfg->part_cfg->flags & PCA_HAS_INT_MASK) {
			int_mask &= ~BIT(pin); /** set 0 to enable interrupt */
		}
	}

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	/** merge two buffer into low address */
	gpio_pca_series_cache_2b_merge(dev, buf);
	/** save config to cache even if this register is not present on device */
	ret = gpio_pca_series_reg_update_cache(dev, PCA_REG_TYPE_2B_INTERRUPT_EDGE,
						   (uint8_t *)&buf);
	if (ret) {
		LOG_ERR("get current interrupt config fail [%d]", ret);
		goto out;
	}
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

	/** enable latch if available, so we do not lost interrupt */
	if (cfg->part_cfg->flags & PCA_HAS_LATCH) {
		input_latch = ~int_mask;
		ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_INPUT_LATCH,
						(uint8_t *)&input_latch);
		if (ret) {
			goto out;
		}
	}
	/** update interrupt mask register if available */
	if (cfg->part_cfg->flags & PCA_HAS_INT_MASK) {
		ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_INTERRUPT_MASK,
						(uint8_t *)&int_mask);
		if (ret) {
			goto out;
		}
	}

out:
	k_sem_give(&data->lock);
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
 *
 * @param dev
 * @param pin
 * @param mode
 * @param trig
 * @return int
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
	int ret;
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

	LOG_INF("int cfg(e) pin %d mode %d trig %d", pin, mode, trig);

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
	ret = gpio_pca_series_reg_read_cache(dev, PCA_REG_TYPE_2B_INTERRUPT_EDGE,
						 (uint8_t *)&int_edge);
	if (ret) {
		LOG_ERR("get current interrupt edge config fail [%d]", ret);
		goto out;
	}

	ret = gpio_pca_series_reg_read_cache(dev, PCA_REG_TYPE_1B_INTERRUPT_MASK,
						 (uint8_t *)&int_mask);
	if (ret) {
		goto out;
	}

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
	ret = gpio_pca_series_reg_read_cache(dev, PCA_REG_TYPE_2B_INTERRUPT_EDGE,
						 (uint8_t *)&int_edge);
	if (ret) {
		LOG_ERR("get current interrupt config fail [%d]", ret);
		goto out;
	}

	/** enable latch, so we do not lost interrupt */
	input_latch = ~int_mask;
	ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_INPUT_LATCH,
					(uint8_t *)&input_latch);
	if (ret) {
		goto out;
	}
	/** update interrupt mask register */
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

static void gpio_pca_series_interrupt_handler_standard(const struct device *dev)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	struct gpio_pca_series_data *data = dev->data;
	int ret;
	uint32_t input_old;
	uint32_t input;
	uint32_t transitioned_pins;
	uint32_t int_status = 0;
	uint32_t *int_rise, *int_fall;

	k_sem_take(&data->lock, K_FOREVER);

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	uint32_t buf[2];
	/** read from cache even if this register is not present on device */
	ret = gpio_pca_series_reg_read_cache(dev, PCA_REG_TYPE_2B_INTERRUPT_EDGE,
						 (uint8_t *)&buf);
	if (ret) {
		LOG_ERR("get current interrupt config fail [%d]", ret);
		goto out;
	}
	/** split the value into two 32-bit buffer */
	gpio_pca_series_cache_2b_split(dev, buf);

	int_rise = &buf[0];
	int_fall = &buf[1];
#else
	int_rise = gpio_pca_series_reg_cache_mini_get(dev)->int_rise;
	int_fall = gpio_pca_series_reg_cache_mini_get(dev)->int_fall;
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

	/** check if any interrupt enabled */
	if (!(*int_rise) && !(*int_fall)) {
		ret = -EINVAL;
		goto out;
	}
	/** get transitioned_pins */
	if (cfg->part_cfg->flags &
		PCA_HAS_INT_MASK) { /** get transitioned_pins from interrupt_status register */
		ret = gpio_pca_series_reg_read(dev, PCA_REG_TYPE_1B_INTERRUPT_STATUS,
						   (uint8_t *)&transitioned_pins);
		if (ret) {
			goto out;
		}
	}

	/** read current input value, and clear status if needed */
	ret = gpio_pca_series_reg_read(dev, PCA_REG_TYPE_1B_INPUT_PORT, (uint8_t *)&input);
	if (ret != 0) {
		goto out;
	}
	if (cfg->part_cfg->flags & PCA_HAS_INT_MASK) {
		/** if either-edge interrupt happens only, send out directly */
		if ((transitioned_pins & (*int_rise)) == (transitioned_pins & (*int_fall))) {
			int_status = transitioned_pins;
			goto cache;
		}
	}
	/** compare input to input_old to get transitioned_pins */
	/** get previous input state */
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	ret = gpio_pca_series_reg_read_cache(dev, PCA_REG_TYPE_1B_INPUT_PORT,
						 (uint8_t *)&input_old);
	if (ret) {
		goto cache;
	}
#else
	input_old = gpio_pca_series_reg_cache_mini_get(dev)->input;
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
	/** Find out which input pins have changed state */
	transitioned_pins = input_old ^ input;

	/** Mask gpio transactions with rising/falling edge interrupt config */
	int_status = ((*int_rise) & transitioned_pins & input);
	int_status |= ((*int_fall) & transitioned_pins & input_old);

cache:
	/** update current input to cache */
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	ret = gpio_pca_series_reg_update_cache(dev, PCA_REG_TYPE_1B_INPUT_PORT,
						   (uint8_t *)&input);
	if (ret) {
		goto out;
	}
#else
	gpio_pca_series_reg_cache_mini_get(dev)->input = input;
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

out:
	k_sem_give(&data->lock);

	if ((ret == 0) && (int_status)) {
		gpio_fire_callbacks(&data->callbacks, dev, int_status);
	}
}

static void gpio_pca_series_interrupt_handler_extended(const struct device *dev)
{
	const struct gpio_pca_series_config *cfg = dev->config;
	struct gpio_pca_series_data *data = dev->data;

	int ret;
	uint32_t int_status;

	k_sem_take(&data->lock, K_FOREVER);

	/** get transitioned_pins */
	if (cfg->part_cfg->flags &
		PCA_HAS_INT_MASK) { /** get transitioned_pins from interrupt_status register */
		ret = gpio_pca_series_reg_read(dev, PCA_REG_TYPE_1B_INTERRUPT_STATUS,
						   (uint8_t *)&int_status);
		if (ret) {
			goto out;
		}
	}

	/** clear status  */
	ret = gpio_pca_series_reg_write(dev, PCA_REG_TYPE_1B_INTERRUPT_CLEAR,
					(uint8_t *)&int_status);
	if (ret != 0) {
		goto out;
	}

out:
	k_sem_give(&data->lock);

	if ((ret == 0) && (int_status)) {
		gpio_fire_callbacks(&data->callbacks, dev, int_status);
	}
}

static void gpio_pca_series_interrupt_worker(struct k_work *work)
{
	struct gpio_pca_series_data *data =
		CONTAINER_OF(work, struct gpio_pca_series_data, int_work);
	const struct device *dev = data->self;
	const struct gpio_pca_series_config *cfg = dev->config;

	/** check the flags */
	const uint8_t check_flags = (PCA_HAS_LATCH | PCA_HAS_INT_MASK | PCA_HAS_INT_EXTEND);

	if ((cfg->part_cfg->flags & check_flags) == check_flags) {
		gpio_pca_series_interrupt_handler_extended(dev);
	} else {
		gpio_pca_series_interrupt_handler_standard(dev);
	}
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

static const struct gpio_driver_api gpio_pca_series_api_funcs_standard = {
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

static const struct gpio_driver_api gpio_pca_series_api_funcs_pcal65xx = {
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
		goto out;
	}

	LOG_WRN("pca");

	/** device reset */
	gpio_pca_series_reset(dev);

	LOG_WRN("pca");

	/** Set cache to initial state */
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	for (int reg_type = 0; reg_type < PCA_REG_TYPE_COUNT; ++reg_type) {

#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
		/**
		 * On devices without PCA_HAS_INT_EXTEND calabilitiy,
		 * PCA_REG_TYPE_2B_INTERRUPT_EDGE caches mask of rising and falling pins,
		 * while the actual register is not present. Account for that here:
		 */
		const uint8_t reset_value_0[] = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

		if ((reg_type == PCA_REG_TYPE_2B_INTERRUPT_EDGE) &&
			(cfg->flags & PCA_HAS_INT_EXTEND) == 0U) {
			gpio_pca_series_reg_update_cache(dev,
				PCA_REG_TYPE_2B_INTERRUPT_EDGE, reset_value_0);
			continue;
		}
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

		ret = gpio_pca_series_reg_update_cache(dev, reg_type, NULL);
		if (ret == -EACCESS) {
			/** The reg is marked as non-cacheable, not a error */
			ret = 0;
		}
		if (ret == 0) {
			continue;
		} else {
			/** Trying to cache a reg which is not present, error */
			break;
		}
	}
	if (ret) {
		LOG_ERR("update cache fail %d", ret);
		goto out;
	}
#else
	gpio_pca_series_reg_cache_mini_reset(dev);
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */


	LOG_WRN("pca");

	/** configure interrupt */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
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
	}
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */

#ifdef GPIO_NXP_PCA_SERIES_DEBUG
	gpio_pca_series_register_dump(dev);
#endif /* GPIO_NXP_PCA_SERIES_DEBUG */

out:
	if (ret) {
		LOG_ERR("%s init failed: %d", dev->name, ret);
	} else {
		LOG_INF("%s init ok", dev->name);
	}
	return ret;
}

/**
 * @brief implement pca953x driver
 *
 * @note flags = 0U;
 *
 *       api set    :   standard
 *       ngpios     :   8, 16
 *       part_no    :   pca9534 pca9538 pca9535 pca9539
 */

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
/**
 * cache map for flag = 0U
 */
static const uint8_t gpio_pca_series_cache_map_pca953x[] = {
	0x00, /** input_port, cache for interrupt */
	0x01, /** output_port */
/*  0x02, /** polarity_inversion  (unused, omitted) */
	0x02, /** configuration */
	PCA_REG_INVALID, /** 2b_output_drive_strength */
	PCA_REG_INVALID, /** input_latch, never cache */
	PCA_REG_INVALID, /** pull_enable */
	PCA_REG_INVALID, /** pull_select */
	PCA_REG_INVALID, /** input_status */
	PCA_REG_INVALID, /** output_config */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	PCA_REG_INVALID, /** interrupt_mask */
	PCA_REG_INVALID, /** int_status, non-cacheable */
	0x08, /** 2b_interrupt_edge, fake cache for storage only */
	PCA_REG_INVALID, /** interrupt_clear */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

static const uint8_t gpio_pca_series_reg_pca9538[] = {
	0x00, /** input_port */
	0x01, /** output_port */
/*  0x02, /** polarity_inversion  (unused, omitted) */
	0x03, /** configuration */
	PCA_REG_INVALID, /** 2b_output_drive_strength */
	PCA_REG_INVALID, /** input_latch */
	PCA_REG_INVALID, /** pull_enable */
	PCA_REG_INVALID, /** pull_select */
	PCA_REG_INVALID, /** input_status */
	PCA_REG_INVALID, /** output_config */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	PCA_REG_INVALID, /** interrupt_mask */
	PCA_REG_INVALID, /** int_status */
	PCA_REG_INVALID, /** 2b_interrupt_edge */
	PCA_REG_INVALID, /** interrupt_clear */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};

const struct gpio_pca_series_part_config gpio_pca_series_part_cfg_pca9538 = {
	.port_no = 1U,
	.flags = 0U,
	.regs = gpio_pca_series_reg_pca9538,
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	.cache_map = gpio_pca_series_cache_map_pca953x,
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
};

static const uint8_t gpio_pca_series_reg_pca9539[] = {
	0x00, /** input_port */
	0x02, /** output_port */
/*  0x04, /** polarity_inversion  (unused, omitted) */
	0x06, /** configuration */
	PCA_REG_INVALID, /** 2b_output_drive_strength */
	PCA_REG_INVALID, /** input_latch */
	PCA_REG_INVALID, /** pull_enable */
	PCA_REG_INVALID, /** pull_select */
	PCA_REG_INVALID, /** input_status */
	PCA_REG_INVALID, /** output_config */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	PCA_REG_INVALID, /** interrupt_mask */
	PCA_REG_INVALID, /** int_status */
	PCA_REG_INVALID, /** 2b_interrupt_edge */
	PCA_REG_INVALID, /** interrupt_clear */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};

const struct gpio_pca_series_part_config gpio_pca_series_part_cfg_pca9539 = {
	.port_no = 2U,
	.flags = 0U,
	.regs = gpio_pca_series_reg_pca9539,
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	.cache_map = gpio_pca_series_cache_map_pca953x,
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
};

/**
 * @brief implement pca957x driver
 *
 * @note flags = PCA_HAS_PULL
 *             | PCA_HAS_INT_MASK
 *
 *       api set    :   standard
 *       ngpios     :   8, 16
 *       part_no    :   pca9574 pca9575
 */

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
/**
 * cache map for flag = PCA_HAS_PULL
 *                    | PCA_HAS_INT_MASK
 */
static const uint8_t gpio_pca_series_cache_map_pca957x[] = {
	0x00, /** input_port, cache for interrupt */
	0x01, /** output_port */
/*  0x02, /** polarity_inversion  (unused, omitted) */
	0x02, /** configuration */
	PCA_REG_INVALID, /** 2b_output_drive_strength */
	PCA_REG_INVALID, /** input_latch */
	0x03, /** pull_enable */
	0x04, /** pull_select */
	PCA_REG_INVALID, /** input_status, non-cacheable */
	0x05, /** output_config */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	0x06, /** interrupt_mask */
	PCA_REG_INVALID, /** int_status, non-cacheable */
	0x07, /** 2b_interrupt_edge, fake cache for storage only */
	PCA_REG_INVALID, /** interrupt_clear, non-cacheable */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

static const uint8_t gpio_pca_series_reg_pca9574[] = {
	0x00, /** input_port */
	0x05, /** output_port */
/*  0x01, /** polarity_inversion  (unused, omitted) */
	0x04, /** configuration */
	PCA_REG_INVALID, /** 2b_output_drive_strength */
	PCA_REG_INVALID, /** input_latch, never cache */
	0x02, /** pull_enable */
	0x03, /** pull_select */
	PCA_REG_INVALID, /** input_status */
	PCA_REG_INVALID, /** output_config */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	0x06, /** interrupt_mask */
	0x07, /** int_status */
	PCA_REG_INVALID, /** 2b_interrupt_edge */
	PCA_REG_INVALID, /** interrupt_clear */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};

const struct gpio_pca_series_part_config gpio_pca_series_part_cfg_pca9574 = {
	.port_no = 1U,
	.flags = PCA_HAS_PULL | PCA_HAS_INT_MASK,
	.regs = gpio_pca_series_reg_pca9574,
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	.cache_map = gpio_pca_series_cache_map_pca957x,
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
};

static const uint8_t gpio_pca_series_reg_pca9575[] = {
	0x00, /** input_port */
	0x0a, /** output_port */
/*  0x02, /** polarity_inversion  (unused, omitted) */
	0x08, /** configuration */
	PCA_REG_INVALID, /** 2b_output_drive_strength */
	PCA_REG_INVALID, /** input_latch, never cache */
	0x04, /** pull_enable */
	0x06, /** pull_select */
	PCA_REG_INVALID, /** input_status */
	PCA_REG_INVALID, /** output_config */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	0x0c, /** interrupt_mask */
	0x0e, /** int_status */
	PCA_REG_INVALID, /** 2b_interrupt_edge */
	PCA_REG_INVALID, /** interrupt_clear */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};

const struct gpio_pca_series_part_config gpio_pca_series_part_cfg_pca9575 = {
	.port_no = 2U,
	.flags = PCA_HAS_PULL | PCA_HAS_INT_MASK,
	.regs = gpio_pca_series_reg_pca9575,
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	.cache_map = gpio_pca_series_cache_map_pca957x,
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
};

/**
 * @brief implement pcal953x and pcal64xxa driver
 *
 * @note flags = PCA_HAS_LATCH
 *             | PCA_HAS_PULL
 *             | PCA_HAS_INT_MASK
 *
 *       api set    :   standard
 *
 *       ngpios     :   8, 16;
 *       part_no    :   pcal9534 pcal9538 pcal6408a
 *                      pcal9535 pcal9539 pcal6416a
 */

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
/**
 * cache map for flag = PCA_HAS_LATCH
 *                    | PCA_HAS_PULL
 *                    | PCA_HAS_INT_MASK
 */
static const uint8_t gpio_pca_series_cache_map_pcal953x[] = {
	0x00, /** input_port, cache for interrupt */
	0x01, /** output_port */
/*  0x02, /** polarity_inversion  (unused, omitted) */
	0x02, /** configuration */
	0x03, /** 2b_output_drive_strength */
	PCA_REG_INVALID, /** input_latch, never cache */
	0x05, /** pull_enable */
	0x06, /** pull_select */
	PCA_REG_INVALID, /** input_status */
	PCA_REG_INVALID, /** output_config */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	0x07, /** interrupt_mask */
	PCA_REG_INVALID, /** int_status, non-cacheable */
	0x08, /** 2b_interrupt_edge, fake cache for storage only */
	PCA_REG_INVALID, /** interrupt_clear */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

static const uint8_t gpio_pca_series_reg_pcal9538[] = {
	0x00, /** input_port */
	0x01, /** output_port */
/*  0x02, /** polarity_inversion  (unused, omitted) */
	0x03, /** configuration */
	0x40, /** 2b_output_drive_strength */
	0x42, /** input_latch */
	0x43, /** pull_enable */
	0x44, /** pull_select */
	PCA_REG_INVALID, /** input_status */
	PCA_REG_INVALID, /** output_config */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	0x45, /** interrupt_mask */
	0x46, /** int_status */
	PCA_REG_INVALID, /** 2b_interrupt_edge */
	PCA_REG_INVALID, /** interrupt_clear */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};

const struct gpio_pca_series_part_config gpio_pca_series_part_cfg_pcal9538 = {
	.port_no = 1U,
	.flags = PCA_HAS_LATCH | PCA_HAS_PULL | PCA_HAS_INT_MASK,
	.regs = gpio_pca_series_reg_pcal9538,
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	.cache_map = gpio_pca_series_cache_map_pcal953x,
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
};

/**
 * pcal6408a share the same register layout with pca9538, with
 * additional voltage level translation capability.
 * no difference from driver perspective.
 */
const struct gpio_pca_series_part_config gpio_pca_series_part_cfg_pcal6408a = {
	.port_no = 1U,
	.flags = PCA_HAS_LATCH | PCA_HAS_PULL | PCA_HAS_INT_MASK,
	.regs = gpio_pca_series_reg_pcal9538,
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	.cache_map = gpio_pca_series_cache_map_pcal953x,
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
};

static const uint8_t gpio_pca_series_reg_pcal9539[] = {
	0x00, /** input_port */
	0x02, /** output_port */
/*  0x04, /** polarity_inversion  (unused, omitted) */
	0x06, /** configuration */
	0x40, /** 2b_output_drive_strength */
	0x44, /** input_latch */
	0x46, /** pull_enable */
	0x48, /** pull_select */
	PCA_REG_INVALID, /** input_status */
	PCA_REG_INVALID, /** output_config */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	0x4a, /** interrupt_mask */
	0x4c, /** int_status */
	PCA_REG_INVALID, /** 2b_interrupt_edge */
	PCA_REG_INVALID, /** interrupt_clear */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};

const struct gpio_pca_series_part_config gpio_pca_series_part_cfg_pcal9539 = {
	.port_no = 2U,
	.flags = PCA_HAS_LATCH | PCA_HAS_PULL | PCA_HAS_INT_MASK,
	.regs = gpio_pca_series_reg_pcal9539,
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	.cache_map = gpio_pca_series_cache_map_pcal953x,
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
};

/**
 * pcal6416a share the same register layout with pca9539, with
 * additional voltage level translation capability.
 * no difference from driver perspective.
 */
const struct gpio_pca_series_part_config gpio_pca_series_part_cfg_pcal6416a = {
	.port_no = 2U,
	.flags = PCA_HAS_LATCH | PCA_HAS_PULL | PCA_HAS_INT_MASK,
	.regs = gpio_pca_series_reg_pcal9539,
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	.cache_map = gpio_pca_series_cache_map_pcal953x,
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

#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
/**
 * cache map for flag = PCA_HAS_LATCH
 *                    | PCA_HAS_PULL
 *                    | PCA_HAS_INT_MASK
 *                    | PCA_HAS_INT_EXTEND
 *                    | PCA_HAS_OUT_CONFIG
 */
static const uint8_t gpio_pca_series_cache_map_pcal65xx[] = {
	PCA_REG_INVALID, /** input_port, non-cacheable */
	0x00, /** output_port */
/*  0x02, /** polarity_inversion  (unused, omitted) */
	0x01, /** configuration */
	0x02, /** 2b_output_drive_strength */
	PCA_REG_INVALID, /** input_latch, never cache */
	0x04, /** pull_enable */
	0x05, /** pull_select */
	PCA_REG_INVALID, /** input_status, non-cacheable */
	0x06, /** output_config */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	0x07, /** interrupt_mask */
	PCA_REG_INVALID, /** int_status, non-cacheable */
	0x08, /** 2b_interrupt_edge */
	PCA_REG_INVALID, /** interrupt_clear, non-cacheable */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

static const uint8_t gpio_pca_series_reg_pcal6524[] = {
	PCA_REG_INVALID/*0x00, use input_status instead*/, /** input_port */
	0x04, /** output_port */
/*  0x08, /** polarity_inversion  (unused, omitted) */
	0x0c, /** configuration */
	0x40, /** 2b_output_drive_strength */
	0x48, /** input_latch */
	0x4c, /** pull_enable */
	0x50, /** pull_select */
	0x6c, /** input_status */
	0x70, /** output_config */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	0x54, /** interrupt_mask */
	0x58, /** int_status */
	0x60, /** 2b_interrupt_edge */
	0x68, /** interrupt_clear */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};

const struct gpio_pca_series_part_config gpio_pca_series_part_cfg_pcal6524 = {
	.port_no = 3U,
	.flags = PCA_HAS_LATCH | PCA_HAS_PULL | PCA_HAS_INT_MASK
		   | PCA_HAS_INT_EXTEND | PCA_HAS_OUT_CONFIG,
	.regs = gpio_pca_series_reg_pcal6524,
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	.cache_map = gpio_pca_series_cache_map_pcal65xx,
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
};

static const uint8_t gpio_pca_series_reg_pcal6534[] = {
	PCA_REG_INVALID/*0x00, use input_status instead*/, /** input_port */
	0x04, /** output_port */
/*  0x08, /** polarity_inversion  (unused, omitted) */
	0x0c, /** configuration */
	0x40, /** 2b_output_drive_strength */
	0x48, /** input_latch */
	0x4c, /** pull_enable */
	0x50, /** pull_select */
	0x6c, /** input_status */
	0x70, /** output_config */
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT
	0x54, /** interrupt_mask */
	0x58, /** int_status */
	0x60, /** 2b_interrupt_edge */
	0x68, /** interrupt_clear */
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
};

const struct gpio_pca_series_part_config gpio_pca_series_part_cfg_pcal6534 = {
	.port_no = 4U,
	.flags = PCA_HAS_LATCH | PCA_HAS_PULL | PCA_HAS_INT_MASK
		   | PCA_HAS_INT_EXTEND | PCA_HAS_OUT_CONFIG,
	.regs = gpio_pca_series_reg_pcal6534,
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
	.cache_map = gpio_pca_series_cache_map_pcal65xx,
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */
};

/**
 * @brief get device description by part_no
 */
#define GPIO_PCA_GET_API_BY_PART_NO(part_no) ( \
	(part_no == PCA_PART_NO_PCAL6524) ? &gpio_pca_series_api_funcs_pcal65xx : \
	(part_no == PCA_PART_NO_PCAL6534) ? &gpio_pca_series_api_funcs_pcal65xx : \
	&gpio_pca_series_api_funcs_standard \
)
#define GPIO_PCA_GET_PART_CFG_BY_PART_NO(part_no) ( \
	(part_no == PCA_PART_NO_PCA9538) ? &gpio_pca_series_part_cfg_pca9538 : \
	(part_no == PCA_PART_NO_PCA9575) ? &gpio_pca_series_part_cfg_pca9575 : \
	(part_no == PCA_PART_NO_PCAL9538) ? &gpio_pca_series_part_cfg_pcal9538 : \
	(part_no == PCA_PART_NO_PCAL6408A) ? &gpio_pca_series_part_cfg_pcal6408a : \
	(part_no == PCA_PART_NO_PCAL6416A) ? &gpio_pca_series_part_cfg_pcal6416a : \
	(part_no == PCA_PART_NO_PCAL6524) ? &gpio_pca_series_part_cfg_pcal6524 : \
	(part_no == PCA_PART_NO_PCAL6534) ? &gpio_pca_series_part_cfg_pcal6534 : \
	NULL \
)
#ifdef CONFIG_GPIO_PCA_SERIES_CACHE_ALL
#ifdef CONFIG_GPIO_PCA_SERIES_INTERRUPT

/** Cache size increment by feature flags */
#define PCA_REG_HAS_LATCH	(2U) /* +drive_strength */
#define PCA_REG_HAS_PULL	(2U) /* +pull_enable, +pull_select */
#define PCA_REG_HAS_OUT_CONFIG	(1U) /* +output_config */
#define PCA_REG_HAS_INT_MASK	(1U) /* +interrupt_mask */
#define PCA_REG_HAS_INT_EXTEND	(2U) /* true: +interrupt_edge */
#define PCA_REG_NO_INT_EXTEND	(3U) /* false: +input_port, +interrupt_edge
				      * (with alternative definition)
				      */

/**
 * @brief
 *
 *  registers:
 *    1b_input_port
 *        - non-cacheable by default
 *        - only cacheable if PCA_HAS_INT_EXTEND is not set and
 *          interrupt is enabled
 *    1b_output_port
 *        - cacheable
 *    1b_configuration
 *        - cacheable
 *    2b_output_drive_strength
 *        - not present by default
 *        - cacheable if present (PCA_HAS_LATCH is set)
 *    1b_input_latch
 *        - not present by default
 *        - non-cacheable
 *    1b_pull_enable
 *        - not present by default
 *        - cacheable if present (PCA_HAS_PULL is set)
 *    1b_pull_select
 *        - not present by default
 *        - cacheable if present (PCA_HAS_PULL is set)
 *    1b_input_status
 *        - not present by default
 *        - non-cacheable
 *    1b_output_config
 *        - not present by default
 *        - cacheable if present (PCA_HAS_OUT_CONFIG is set)
 *    1b_interrupt_mask
 *        - not present by default
 *        - cacheable if present (PCA_HAS_INT_MASK is set)
 *    1b_interrupt_status
 *        - not present by default
 *        - non-cacheable
 *    2b_interrupt_edge
 *        - always cacheable, despite not present by default
 *        - save int_rise and int_fall value if PCA_HAS_INT_EXTEND
 *          is not set (register is not present)
 *          value need to be split before use, and merge back before
 *          save to cache.
 *        - save actual register value if PCA_HAS_INT_EXTEND is set
 *          every 2 bits represent interrupt config defined by
 *          enum PCA_INTERRUPT_config_extended.
 *    1b_interrupt_clear
 *        - not present by default
 *        - non-cacheable
 *
 */
#define GPIO_PCA_GET_CACHE_SIZE_BY_PART_NO(part_no) ( \
	(2U /* basic: +output_port, +configuration */ + ( \
		((GPIO_PCA_GET_PART_CFG_BY_PART_NO(part_no)->flags | PCA_HAS_LATCH) ? \
			PCA_REG_HAS_LATCH : 0U) \
		+ ((GPIO_PCA_GET_PART_CFG_BY_PART_NO(part_no)->flags | PCA_HAS_PULL) ? \
			PCA_REG_HAS_PULL : 0U) \
		+ ((GPIO_PCA_GET_PART_CFG_BY_PART_NO(part_no)->flags | PCA_HAS_OUT_CONFIG) ? \
			PCA_REG_HAS_OUT_CONFIG  : 0U) \
		+ ((GPIO_PCA_GET_PART_CFG_BY_PART_NO(part_no)->flags | PCA_HAS_INT_MASK) ? \
			PCA_REG_HAS_INT_MASK : 0U) \
		+ ((GPIO_PCA_GET_PART_CFG_BY_PART_NO(part_no)->flags | PCA_HAS_INT_EXTEND) ? \
			PCA_REG_HAS_INT_EXTEND : PCA_REG_NO_INT_EXTEND) \
	)) * (GPIO_PCA_GET_PART_CFG_BY_PART_NO(part_no)->port_no) \
)
#else /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
#define GPIO_PCA_GET_CACHE_SIZE_BY_PART_NO(part_no) ( \
		(2U /* basic: +output_port, +configuration */ + ( \
		((GPIO_PCA_GET_PART_CFG_BY_PART_NO(part_no)->flags | PCA_HAS_LATCH) ? \
			PCA_REG_HAS_LATCH : 0U) \
		+ ((GPIO_PCA_GET_PART_CFG_BY_PART_NO(part_no)->flags | PCA_HAS_PULL) ? \
			PCA_REG_HAS_PULL : 0U) \
		+ ((GPIO_PCA_GET_PART_CFG_BY_PART_NO(part_no)->flags | PCA_HAS_OUT_CONFIG) ? \
			PCA_REG_HAS_OUT_CONFIG : 0U) \
	)) * (GPIO_PCA_GET_PART_CFG_BY_PART_NO(part_no)->port_no) \
)
#endif /* CONFIG_GPIO_PCA_SERIES_INTERRUPT */
#endif /* CONFIG_GPIO_PCA_SERIES_CACHE_ALL */

/**
 * @brief common device instance
 *
 */
#define GPIO_PCA_SERIES_DEVICE_INSTANCE(inst, part_no) \
	static const struct gpio_pca_series_config gpio_pca_series_##inst##_cfg = { \
		.common = { \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst), \
		}, \
		.i2c = I2C_DT_SPEC_INST_GET(inst), \
		.part_cfg = GPIO_PCA_GET_PART_CFG_BY_PART_NO(part_no), \
		.gpio_rst = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}), \
		IF_ENABLED(CONFIG_GPIO_PCA_SERIES_INTERRUPT, \
			(.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, interrupt_gpios, {}),))}; \
	static uint8_t gpio_pca_series_##inst##_reg_cache[COND_CODE_1( \
		CONFIG_GPIO_PCA_SERIES_CACHE_ALL, \
		(GPIO_PCA_GET_CACHE_SIZE_BY_PART_NO(part_no) /** true */\
		 ), \
		(sizeof(struct gpio_pca_series_reg_cache_mini) /** false */ \
		 ))]; \
	static struct gpio_pca_series_data gpio_pca_series_##inst##_data = { \
		.lock = Z_SEM_INITIALIZER(gpio_pca_series_##inst##_data.lock, 1, 1), \
		.cache = (void *)gpio_pca_series_##inst##_reg_cache, \
		IF_ENABLED(CONFIG_GPIO_PCA_SERIES_INTERRUPT, \
			(.self = DEVICE_DT_INST_GET(inst), \
			.int_work = Z_WORK_INITIALIZER(gpio_pca_series_interrupt_worker),))}; \
	DEVICE_DT_INST_DEFINE(inst, gpio_pca_series_init, NULL, \
		&gpio_pca_series_##inst##_data, \
		&gpio_pca_series_##inst##_cfg, POST_KERNEL, \
		CONFIG_GPIO_PCA_SERIES_INIT_PRIORITY, \
		GPIO_PCA_GET_API_BY_PART_NO(part_no));

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pca9538
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_PCA_SERIES_DEVICE_INSTANCE, PCA_PART_NO_PCA9538)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pca9539
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_PCA_SERIES_DEVICE_INSTANCE, PCA_PART_NO_PCA9539)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pca9574
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_PCA_SERIES_DEVICE_INSTANCE, PCA_PART_NO_PCA9574)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pca9575
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_PCA_SERIES_DEVICE_INSTANCE, PCA_PART_NO_PCA9575)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pcal9538
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_PCA_SERIES_DEVICE_INSTANCE, PCA_PART_NO_PCAL9538)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pcal9539
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_PCA_SERIES_DEVICE_INSTANCE, PCA_PART_NO_PCAL9539)
/**
 * #undef DT_DRV_COMPAT
 * #define DT_DRV_COMPAT nxp_pcal6408a
 * DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_PCA_SERIES_DEVICE_INSTANCE, PCA_PART_NO_PCAL6408A)
 *
 * #undef DT_DRV_COMPAT
 * #define DT_DRV_COMPAT nxp_pcal6416a
 * DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_PCA_SERIES_DEVICE_INSTANCE, PCA_PART_NO_PCAL6416A)
 */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pcal6524
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_PCA_SERIES_DEVICE_INSTANCE, PCA_PART_NO_PCAL6524)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pcal6534
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_PCA_SERIES_DEVICE_INSTANCE, PCA_PART_NO_PCAL6534)
