/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Driver for PCA95XX I2C-based GPIO driver.
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(gpio_pca95xx);

/* Register definitions */
#define REG_INPUT_PORT0			0x00
#define REG_INPUT_PORT1			0x01
#define REG_OUTPUT_PORT0		0x02
#define REG_OUTPUT_PORT1		0x03
#define REG_POL_INV_PORT0		0x04
#define REG_POL_INV_PORT1		0x05
#define REG_CONF_PORT0			0x06
#define REG_CONG_PORT1			0x07
#define REG_OUT_DRV_STRENGTH_PORT0_L	0x40
#define REG_OUT_DRV_STRENGTH_PORT0_H	0x41
#define REG_OUT_DRV_STRENGTH_PORT1_L	0x42
#define REG_OUT_DRV_STRENGTH_PORT1_H	0x43
#define REG_INPUT_LATCH_PORT0		0x44
#define REG_INPUT_LATCH_PORT1		0x45
#define REG_PUD_EN_PORT0		0x46
#define REG_PUD_EN_PORT1		0x47
#define REG_PUD_SEL_PORT0		0x48
#define REG_PUD_SEL_PORT1		0x49
#define REG_INT_MASK_PORT0		0x4A
#define REG_INT_MASK_PORT1		0x4B
#define REG_INT_STATUS_PORT0		0x4C
#define REG_INT_STATUS_PORT1		0x4D
#define REG_OUTPUT_PORT_CONF		0x4F

/* Driver flags */
#define PCA_HAS_PUD			BIT(0)

/** Configuration data */
struct gpio_pca95xx_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;

	/** The master I2C device's name */
	const char * const i2c_master_dev_name;

	/** The slave address of the chip */
	u16_t i2c_slave_addr;

	u8_t capabilities;
};

/** Runtime driver data */
struct gpio_pca95xx_drv_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;

	/** Master I2C device */
	struct device *i2c_master;

	struct {
		u16_t output;
		u16_t dir;
		u16_t pud_en;
		u16_t pud_sel;
	} reg_cache;

	struct k_sem lock;
};

/**
 * @brief Read both port 0 and port 1 registers of certain register function.
 *
 * Given the register in reg, read the pair of port 0 and port 1.
 *
 * @param dev Device struct of the PCA95XX.
 * @param reg Register to read (the PORT0 of the pair of registers).
 * @param buf Buffer to read data into.
 *
 * @return 0 if successful, failed otherwise.
 */
static int read_port_regs(struct device *dev, u8_t reg, u16_t *buf)
{
	const struct gpio_pca95xx_config * const config =
		dev->config->config_info;
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->driver_data;
	struct device * const i2c_master = drv_data->i2c_master;
	u16_t i2c_addr = config->i2c_slave_addr;
	u16_t port_data;
	int ret;

	ret = i2c_burst_read(i2c_master, i2c_addr, reg,
			     (u8_t *)&port_data, sizeof(port_data));
	if (ret != 0) {
		LOG_ERR("PCA95XX[0x%X]: error reading register 0x%X (%d)",
			i2c_addr, reg, ret);
		return ret;
	}

	*buf = sys_le16_to_cpu(port_data);

	LOG_DBG("PCA95XX[0x%X]: Read: REG[0x%X] = 0x%X, REG[0x%X] = 0x%X",
		i2c_addr, reg, (*buf & 0xFF), (reg + 1), (*buf >> 8));

	return 0;
}

/**
 * @brief Write both port 0 and port 1 registers of certain register function.
 *
 * Given the register in reg, write the pair of port 0 and port 1.
 *
 * @param dev Device struct of the PCA95XX.
 * @param reg Register to write into (the PORT0 of the pair of registers).
 * @param cache Pointer to the cache to be updated after successful write.
 * @param value New value to set.
 *
 * @return 0 if successful, failed otherwise.
 */
static int write_port_regs(struct device *dev, u8_t reg,
			   u16_t *cache, u16_t value)
{
	const struct gpio_pca95xx_config * const config =
		dev->config->config_info;
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->driver_data;
	struct device * const i2c_master = drv_data->i2c_master;
	u16_t i2c_addr = config->i2c_slave_addr;
	u16_t port_data;
	int ret;

	LOG_DBG("PCA95XX[0x%X]: Write: REG[0x%X] = 0x%X, REG[0x%X] = "
		"0x%X", i2c_addr, reg, (value & 0xFF),
		(reg + 1), (value >> 8));

	port_data = sys_cpu_to_le16(value);

	ret = i2c_burst_write(i2c_master, i2c_addr, reg,
			      (u8_t *)&port_data, sizeof(port_data));
	if (ret == 0) {
		*cache = value;
	} else {
		LOG_ERR("PCA95XX[0x%X]: error writing to register 0x%X "
			"(%d)", i2c_addr, reg, ret);
	}

	return ret;
}

static inline int update_output_regs(struct device *dev, u16_t value)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->driver_data;

	return write_port_regs(dev, REG_OUTPUT_PORT0,
			       &drv_data->reg_cache.output, value);
}

static inline int update_direction_regs(struct device *dev, u16_t value)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->driver_data;

	return write_port_regs(dev, REG_CONF_PORT0,
			       &drv_data->reg_cache.dir, value);
}

static inline int update_pul_sel_regs(struct device *dev, u16_t value)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->driver_data;

	return write_port_regs(dev, REG_PUD_SEL_PORT0,
			       &drv_data->reg_cache.pud_sel, value);
}

static inline int update_pul_en_regs(struct device *dev, u16_t value)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->driver_data;

	return write_port_regs(dev, REG_PUD_EN_PORT0,
			       &drv_data->reg_cache.pud_en, value);
}

/**
 * @brief Setup the pin direction (input or output)
 *
 * @param dev Device struct of the PCA95XX
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int setup_pin_dir(struct device *dev, u32_t pin, int flags)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->driver_data;
	u16_t reg_dir = drv_data->reg_cache.dir;
	u16_t reg_out = drv_data->reg_cache.output;
	int ret;

	/* For each pin, 0 == output, 1 == input */
	if ((flags & GPIO_OUTPUT) != 0U) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			reg_out |= BIT(pin);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			reg_out &= ~BIT(pin);
		}
		reg_dir &= ~BIT(pin);
	} else {
		reg_dir |= BIT(pin);
	}

	ret = update_output_regs(dev, reg_out);
	if (ret != 0) {
		return ret;
	}

	ret = update_direction_regs(dev, reg_dir);

	return ret;
}

/**
 * @brief Setup the pin pull up/pull down status
 *
 * @param dev Device struct of the PCA95XX
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int setup_pin_pullupdown(struct device *dev, u32_t pin, int flags)
{
	const struct gpio_pca95xx_config * const config =
		dev->config->config_info;
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->driver_data;
	u16_t reg_pud;
	int ret;

	if ((config->capabilities & PCA_HAS_PUD) == 0) {
		/* Chip does not support pull up/pull down */
		if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U) {
			return -ENOTSUP;
		}

		/* If both GPIO_PULL_UP and GPIO_PULL_DOWN are not set,
		 * we should disable them in hardware. But need to skip
		 * if the chip does not support pull up/pull down.
		 */
		return 0;
	}

	/* If disabling pull up/down, there is no need to set the selection
	 * register. Just go straight to disabling.
	 */
	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U) {
		/* Setup pin pull up or pull down */
		reg_pud = drv_data->reg_cache.pud_sel;

		/* pull down == 0, pull up == 1 */
		WRITE_BIT(reg_pud, pin, (flags & GPIO_PULL_UP) != 0U);

		ret = update_pul_sel_regs(dev, reg_pud);
		if (ret) {
			return ret;
		}
	}

	/* enable/disable pull up/down */
	reg_pud = drv_data->reg_cache.pud_en;

	WRITE_BIT(reg_pud, pin,
		  (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U);

	ret = update_pul_en_regs(dev, reg_pud);

	return ret;
}

/**
 * @brief Configure pin or port
 *
 * @param dev Device struct of the PCA95XX
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_pca95xx_config(struct device *dev, int access_op,
				 u32_t pin, int flags)
{
	int ret;
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->driver_data;

#if (CONFIG_GPIO_LOG_LEVEL >= LOG_LEVEL_DEBUG)
	const struct gpio_pca95xx_config * const config =
		dev->config->config_info;
	u16_t i2c_addr = config->i2c_slave_addr;
#endif

	/* only support config by pin */
	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	/* Does not support disconnected pin */
	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == GPIO_DISCONNECTED) {
		return -ENOTSUP;
	}

	/* Open-drain support is per port, not per pin.
	 * So can't really support the API as-is.
	 */
	if ((flags & GPIO_SINGLE_ENDED) != 0U) {
		return -ENOTSUP;
	}

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	ret = setup_pin_dir(dev, pin, flags);
	if (ret) {
		LOG_ERR("PCA95XX[0x%X]: error setting pin direction (%d)",
			i2c_addr, ret);
		goto done;
	}

	ret = setup_pin_pullupdown(dev, pin, flags);
	if (ret) {
		LOG_ERR("PCA95XX[0x%X]: error setting pin pull up/down "
			"(%d)", i2c_addr, ret);
		goto done;
	}

done:
	k_sem_give(&drv_data->lock);
	return ret;
}

/**
 * @brief Set the pin or port output
 *
 * @param dev Device struct of the PCA95XX
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param value Value to set (0 or 1)
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_pca95xx_write(struct device *dev, int access_op,
				u32_t pin, u32_t value)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->driver_data;
	u16_t reg_out;
	int ret;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	reg_out = drv_data->reg_cache.output;

	/* Invert input value for pins configurated as active low. */
	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		if (value) {
			reg_out |= BIT(pin);
		} else {
			reg_out &= ~BIT(pin);
		}
		break;
	case GPIO_ACCESS_BY_PORT:
		reg_out = value;
		break;
	default:
		ret = -ENOTSUP;
		goto done;
	}

	ret = update_output_regs(dev, reg_out);

done:
	k_sem_give(&drv_data->lock);
	return ret;
}

/**
 * @brief Read the pin or port status
 *
 * @param dev Device struct of the PCA95XX
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param value Value of input pin(s)
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_pca95xx_read(struct device *dev, int access_op,
			       u32_t pin, u32_t *value)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->driver_data;
	u16_t buf;
	int ret;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	ret = read_port_regs(dev, REG_INPUT_PORT0, &buf);
	if (ret != 0) {
		goto done;
	}

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		*value = (buf >> pin) & 0x01;
		break;
	case GPIO_ACCESS_BY_PORT:
		*value = buf;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

done:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int gpio_pca95xx_port_get_raw(struct device *dev, u32_t *value)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->driver_data;
	u16_t buf;
	int ret;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	ret = read_port_regs(dev, REG_INPUT_PORT0, &buf);
	if (ret != 0) {
		goto done;
	}

	*value = buf;

done:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int gpio_pca95xx_port_set_masked_raw(struct device *dev,
					      u32_t mask, u32_t value)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->driver_data;
	u16_t reg_out;
	int ret;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	reg_out = drv_data->reg_cache.output;
	reg_out = (reg_out & ~mask) | (mask & value);

	ret = update_output_regs(dev, reg_out);

	k_sem_give(&drv_data->lock);

	return ret;
}

static int gpio_pca95xx_port_set_bits_raw(struct device *dev, u32_t mask)
{
	return gpio_pca95xx_port_set_masked_raw(dev, mask, mask);
}

static int gpio_pca95xx_port_clear_bits_raw(struct device *dev, u32_t mask)
{
	return gpio_pca95xx_port_set_masked_raw(dev, mask, 0);
}

static int gpio_pca95xx_port_toggle_bits(struct device *dev, u32_t mask)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->driver_data;
	u16_t reg_out;
	int ret;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	reg_out = drv_data->reg_cache.output;
	reg_out ^= mask;

	ret = update_output_regs(dev, reg_out);

	k_sem_give(&drv_data->lock);

	return ret;
}

static int gpio_pca95xx_pin_interrupt_configure(struct device *dev,
						  unsigned int pin,
						  enum gpio_int_mode mode,
						  enum gpio_int_trig trig)
{
	return -ENOTSUP;
}

static const struct gpio_driver_api gpio_pca95xx_drv_api_funcs = {
	.config = gpio_pca95xx_config,
	.write = gpio_pca95xx_write,
	.read = gpio_pca95xx_read,
	.port_get_raw = gpio_pca95xx_port_get_raw,
	.port_set_masked_raw = gpio_pca95xx_port_set_masked_raw,
	.port_set_bits_raw = gpio_pca95xx_port_set_bits_raw,
	.port_clear_bits_raw = gpio_pca95xx_port_clear_bits_raw,
	.port_toggle_bits = gpio_pca95xx_port_toggle_bits,
	.pin_interrupt_configure = gpio_pca95xx_pin_interrupt_configure,
};

/**
 * @brief Initialization function of PCA95XX
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int gpio_pca95xx_init(struct device *dev)
{
	const struct gpio_pca95xx_config * const config =
		dev->config->config_info;
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->driver_data;
	struct device *i2c_master;

	/* Find out the device struct of the I2C master */
	i2c_master = device_get_binding((char *)config->i2c_master_dev_name);
	if (!i2c_master) {
		return -EINVAL;
	}
	drv_data->i2c_master = i2c_master;

	k_sem_init(&drv_data->lock, 1, 1);

	return 0;
}

#define GPIO_PCA95XX_DEVICE_INSTANCE(inst)				\
static const struct gpio_pca95xx_config gpio_pca95xx_##inst##_cfg = {	\
	.common = {							\
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_##inst##_NXP_PCA95XX_NGPIOS), \
	},								\
	.i2c_master_dev_name = DT_INST_##inst##_NXP_PCA95XX_BUS_NAME,	\
	.i2c_slave_addr = DT_INST_##inst##_NXP_PCA95XX_BASE_ADDRESS,	\
	.capabilities =							\
		(DT_INST_##inst##_NXP_PCA95XX_HAS_PUD ?			\
			PCA_HAS_PUD : 0) |				\
		0,							\
};									\
									\
static struct gpio_pca95xx_drv_data gpio_pca95xx_##inst##_drvdata = {	\
	.reg_cache.output = 0xFFFF,					\
	.reg_cache.dir = 0xFFFF,					\
	.reg_cache.pud_en = 0x0,					\
	.reg_cache.pud_sel = 0xFFFF,					\
};									\
									\
DEVICE_AND_API_INIT(gpio_pca95xx_##inst,				\
	DT_INST_##inst##_NXP_PCA95XX_LABEL,				\
	gpio_pca95xx_init,						\
	&gpio_pca95xx_##inst##_drvdata,					\
	&gpio_pca95xx_##inst##_cfg,					\
	POST_KERNEL, CONFIG_GPIO_PCA95XX_INIT_PRIORITY,			\
	&gpio_pca95xx_drv_api_funcs)


#ifdef DT_INST_0_NXP_PCA95XX
GPIO_PCA95XX_DEVICE_INSTANCE(0);
#endif /* DT_INST_0_NXP_PCA95XX */

#ifdef DT_INST_1_NXP_PCA95XX
GPIO_PCA95XX_DEVICE_INSTANCE(1);
#endif /* DT_INST_1_NXP_PCA95XX */

#ifdef DT_INST_2_NXP_PCA95XX
GPIO_PCA95XX_DEVICE_INSTANCE(2);
#endif /* DT_INST_2_NXP_PCA95XX */

#ifdef DT_INST_3_NXP_PCA95XX
GPIO_PCA95XX_DEVICE_INSTANCE(3);
#endif /* DT_INST_3_NXP_PCA95XX */
