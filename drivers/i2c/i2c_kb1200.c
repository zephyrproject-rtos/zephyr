/*
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb1200_i2c

#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_kb1200);

#define MAX_FSMBM  6

struct i2c_kb1200_pins {
	uint16_t scl;
	uint16_t sda;
};

static const struct i2c_kb1200_pins i2c_kb1200_pin_cfg[] = {
	{ FSMBM0_CLK_GPIO_Num, FSMBM0_DAT_GPIO_Num },   // scl:GPIO2C, sda:GPIO2D
	{ FSMBM1_CLK_GPIO_Num, FSMBM1_DAT_GPIO_Num },   // scl:GPIO2E, sda:GPIO2F
	{ FSMBM2_CLK_GPIO_Num, FSMBM2_DAT_GPIO_Num },   // scl:GPIO32, sda:GPIO33
	{ FSMBM3_CLK_GPIO_Num, FSMBM3_DAT_GPIO_Num },   // scl:GPIO34, sda:GPIO35
	{ FSMBM4_CLK_GPIO_Num, FSMBM4_DAT_GPIO_Num },   // scl:GPIO38, sda:GPIO39
	{ FSMBM5_CLK_GPIO_Num, FSMBM5_DAT_GPIO_Num },   // scl:GPIO4A, sda:GPIO4B
	{ FSMBM6_CLK_GPIO_Num, FSMBM6_DAT_GPIO_Num },   // scl:GPIO4C, sda:GPIO4D
	{ FSMBM7_CLK_GPIO_Num, FSMBM7_DAT_GPIO_Num },   // scl:GPIO50, sda:GPIO51
};

struct i2c_kb1200_config {
	uintptr_t base_addr;
	uint32_t port_num;
};

#define STATE_IDLE       0
#define STATE_SENDING    1
#define STATE_RECEIVING  2
#define STATE_COMPLETE   3

struct i2c_kb1200_data {
	struct k_sem mutex;
	volatile uint8_t *msg_buf;
	volatile uint32_t msg_len;
	volatile uint8_t  msg_flags;
	volatile int state;
	volatile uint32_t index;
	volatile int err_code;
};

static int i2c_kb1200_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_kb1200_config *config = dev->config;
	FSMBM_T* fsmbm = (FSMBM_T *)config->base_addr;
	//printk("%s base_addr = %p, port_num = %d, config = 0x%08x \n", 
	//	    __func__, (void*)config->base_addr, config->port_num, dev_config);

	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		return -ENOTSUP;
	}

	if (dev_config & I2C_ADDR_10_BITS) {
		return -ENOTSUP;
	}

	uint32_t speed = I2C_SPEED_GET(dev_config);
	//printk("speed = %d\n", speed);
	switch (speed) {
	case I2C_SPEED_STANDARD:  // 100KHz OFH = OFL = 9
		fsmbm->FSMBMCFG = (9 << 24) | (9 << 16);
		break;
	case I2C_SPEED_FAST:      // 400KHZ OFH = 1, OFL = 2
		fsmbm->FSMBMCFG = (1 << 24) | (2 << 16);
		break;
	case I2C_SPEED_FAST_PLUS: // 1MHz OFH = OFL = 0
		fsmbm->FSMBMCFG = (0 << 24) | (0 << 16);
		break;
	default:
		return -EINVAL;
	}

	fsmbm->FSMBMCFG |= 0x0010; // HW reset
	return 0;
}

static void i2c_kb1200_isr(const struct device *dev)
{
	const struct i2c_kb1200_config *config = dev->config;
	struct i2c_kb1200_data *data = dev->data;
	FSMBM_T* fsmbm = (FSMBM_T *)config->base_addr;

	if (data->state == STATE_SENDING)
	{
		if (fsmbm->FSMBMPF & 0x01)   // complete
		{
			fsmbm->FSMBMPF = 0x01;
			uint32_t status = fsmbm->FSMBMSTS;
			data->err_code = (int)(status & 0x1F);
			data->state = STATE_COMPLETE;
		}
		if (fsmbm->FSMBMPF & 0x04)   // continue block
		{
			uint32_t remain = data->msg_len - data->index;
			uint32_t send_bytes = remain > 32 ? 32 : remain;
			memcpy((void*)&fsmbm->FSMBMDAT[0], (void*)&data->msg_buf[data->index], send_bytes);
			data->index += send_bytes;
			fsmbm->FSMBMPF = 0x04;
		}
	}
	else if (data->state == STATE_RECEIVING)
	{
		uint32_t remain = data->msg_len - data->index;
		uint32_t receive_bytes = remain > 32 ? 32 : remain;

		memcpy((void*)&data->msg_buf[data->index], (void*)&fsmbm->FSMBMDAT[0], receive_bytes);
		data->index += receive_bytes;
		if (fsmbm->FSMBMPF & 0x01)   // complete
		{
			fsmbm->FSMBMPF = 0x01;
			uint32_t status = fsmbm->FSMBMSTS;
			data->err_code = (int)(status & 0x1F);
			data->state = STATE_COMPLETE;
		}
		if (fsmbm->FSMBMPF & 0x04)   // continue block
		{
			fsmbm->FSMBMPF = 0x04;
		}
	}
	else if (data->state == STATE_COMPLETE)
	{
		fsmbm->FSMBMPF = 0x05;
	}
}

static int i2c_kb1200_poll_write(const struct device *dev, struct i2c_msg msg, uint16_t addr)
{
	const struct i2c_kb1200_config *config = dev->config;
	struct i2c_kb1200_data *data = dev->data;
	FSMBM_T* fsmbm = (FSMBM_T *)config->base_addr;

	fsmbm->FSMBMCMD = 0;
	fsmbm->FSMBMADR = (addr << 1) & (~1);
	fsmbm->FSMBMPF = 0x07;
	fsmbm->FSMBMCFG |= 0x0003; // Enable FSMBM function, Timeout function

	if (msg.flags & I2C_MSG_STOP)
		fsmbm->FSMBMFRT = 0x01;  // No CMD, No CNT, No PEC, with STOP
	else
		fsmbm->FSMBMFRT = 0x00;  // No CMD, No CNT, No PEC, no STOP

	data->msg_len = msg.len;
	data->msg_buf = msg.buf;
	data->msg_flags = msg.flags;

	data->state = STATE_IDLE;
	data->index = 0;
	data->err_code = 0;

	//printk("Info@%s write %d bytes @0x%02x\n", __func__, msg.len, addr);

	fsmbm->FSMBMPRTC_C = (uint8_t)data->msg_len;
	uint32_t send_bytes = data->msg_len > 32 ? 32 : data->msg_len;
	memcpy((void*)&fsmbm->FSMBMDAT[0], (void*)&data->msg_buf[data->index], send_bytes);
	data->state = STATE_SENDING;
	data->index += send_bytes;
	fsmbm->FSMBMIE = 0x05;
	fsmbm->FSMBMPRTC_P = 0x7F;

	while (data->state != STATE_COMPLETE);
	data->state = STATE_IDLE;
	if (data->err_code != 0)
	{
		//printk("Error@%s err_code = 0x%02x\n", __func__, data->err_code);
		fsmbm->FSMBMCFG |= 0x0010; // HW reset
		return data->err_code;
	}
	return 0;
}

static int i2c_kb1200_poll_read(const struct device *dev, struct i2c_msg msg, uint16_t addr)
{
	const struct i2c_kb1200_config *config = dev->config;
	struct i2c_kb1200_data *data = dev->data;
	FSMBM_T* fsmbm = (FSMBM_T *)config->base_addr;

	fsmbm->FSMBMCMD = 0;
	fsmbm->FSMBMADR = (addr << 1) | 1;
	fsmbm->FSMBMPF = 0x07;
	fsmbm->FSMBMCFG |= 0x0003; // Enable FSMBM function, Timeout function

	if (msg.flags & I2C_MSG_STOP)
		fsmbm->FSMBMFRT = 0x01;  // No CMD, No CNT, No PEC, with STOP
	else
		fsmbm->FSMBMFRT = 0x00;  // No CMD, No CNT, No PEC, no STOP

	data->msg_len = msg.len;
	data->msg_buf = msg.buf;
	data->msg_flags = msg.flags;

	data->state = STATE_IDLE;
	data->index = 0;
	data->err_code = 0;

	fsmbm->FSMBMPRTC_C = (uint8_t)data->msg_len;
	data->state = STATE_RECEIVING;
	fsmbm->FSMBMIE = 0x05;
	fsmbm->FSMBMPRTC_P = 0x7F;
	while (data->state != STATE_COMPLETE);
	data->state = STATE_IDLE;
	if (data->err_code != 0)
	{
		//printk("Error@%s err_code = 0x%02x\n", __func__, data->err_code);
		fsmbm->FSMBMCFG |= 0x0010; // HW reset
		return data->err_code;
	}
	return 0;
}

static int i2c_kb1200_transfer(const struct device *dev, struct i2c_msg *msgs,
			    uint8_t num_msgs, uint16_t addr)
{
	const struct i2c_kb1200_config *config = dev->config;
	FSMBM_T* fsmbm = (FSMBM_T *)config->base_addr;
	struct i2c_kb1200_data *data = dev->data;
	int ret = 0;

	//printk("%s base_addr = %p, port_num = %d, slave = %d \n", 
	//		__func__, (void*)config->base_addr, config->port_num, addr);
	/* get the mutex */
	k_sem_take(&data->mutex, K_FOREVER);

	fsmbm->FSMBMCFG |= 0x0003; // Enable FSMBM function, Timeout function
	fsmbm->FSMBMIE = 0x00;

	for (int i = 0U; i < num_msgs; i++) 
	{
		//if (i == num_msgs-1)   // the last message
		//	msgs[i].flags |= I2C_MSG_STOP;
		//else
		//	msgs[i].flags &= ~I2C_MSG_STOP;

		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = i2c_kb1200_poll_write(dev, msgs[i], addr);
			if (ret) {
				LOG_ERR("%s Write error: %d", dev->name, ret);
				break;
			}
		} else {
			ret = i2c_kb1200_poll_read(dev, msgs[i], addr);
			if (ret) {
				LOG_ERR("%s Read error: %d", dev->name, ret);
				break;
			}
		}
	}
	/* release the mutex */
	k_sem_give(&data->mutex);

	return ret;
}

static const struct i2c_driver_api i2c_kb1200_api = {
	.configure = i2c_kb1200_configure,
	.transfer  = i2c_kb1200_transfer,
};

static struct device* fsmbm_device[MAX_FSMBM] = { NULL };
static int fsmbm_device_count = 0;
static void i2c_kb1200_isr_wrap(const struct device *dev)
{
	//#define KB1200_SERTBUF 0x40310014
	//(*((volatile uint32_t*)KB1200_SERTBUF)) = '^';  // write to serial port
	for (int i = 0; i < fsmbm_device_count; i++)
	{
		struct device* dev_ = fsmbm_device[i];
		const struct i2c_kb1200_config *config = dev_->config;
		FSMBM_T* fsmbm = (FSMBM_T *)config->base_addr;
		if (fsmbm->FSMBMIE & fsmbm->FSMBMPF)
			i2c_kb1200_isr(dev_);
	}
}

static int i2c_kb1200_init(const struct device *dev)
{
	const struct i2c_kb1200_config *config = dev->config;
	struct i2c_kb1200_data *data = dev->data;
	//printk("%s base_addr = %p, port_num = %d \n", __func__, (void*)config->base_addr, config->port_num);

	PINMUX_DEV_T scl_pinmux = gpio_pinmux(i2c_kb1200_pin_cfg[config->port_num].scl);
	PINMUX_DEV_T sda_pinmux = gpio_pinmux(i2c_kb1200_pin_cfg[config->port_num].sda);

	gpio_pinmux_set(scl_pinmux.port, scl_pinmux.pin, PINMUX_FUNC_B);
	gpio_pinmux_set(sda_pinmux.port, sda_pinmux.pin, PINMUX_FUNC_B);
	gpio_pinmux_pullup(scl_pinmux.port, scl_pinmux.pin, 1);
	gpio_pinmux_pullup(sda_pinmux.port, sda_pinmux.pin, 1);

	GPIO_T *gpio = (GPIO_T *)GPIO_BASE;
	gpio->GPIOOExx[scl_pinmux.port] |= (1lu << scl_pinmux.pin);
	gpio->GPIOOExx[sda_pinmux.port] |= (1lu << sda_pinmux.pin);
	gpio->GPIOIExx[scl_pinmux.port] |= (1lu << scl_pinmux.pin);
	gpio->GPIOIExx[sda_pinmux.port] |= (1lu << sda_pinmux.pin);
	gpio->GPIOODxx[scl_pinmux.port] &= ~(1lu << scl_pinmux.pin);
	gpio->GPIOODxx[sda_pinmux.port] &= ~(1lu << sda_pinmux.pin);
	gpio->GPIOPUxx[scl_pinmux.port] |= (1lu << scl_pinmux.pin);
	gpio->GPIOPUxx[sda_pinmux.port] |= (1lu << sda_pinmux.pin);

	/* init mutex */
	k_sem_init(&data->mutex, 1, 1);

	//printk("%s Record dev = %p, port = %d\n", __func__, dev, config->port_num);
	fsmbm_device[fsmbm_device_count] = (struct device *)dev;
	fsmbm_device_count++;
	/* connect irq for once */
	static bool irq_connected = false;
	if (!irq_connected)
	{
		IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
			    i2c_kb1200_isr_wrap, DEVICE_DT_INST_GET(0), 0);
		irq_enable(DT_INST_IRQN(0));
		irq_connected = true;
	}

	return 0;
}

#define I2C_KB1200_DEVICE(n)						\
	static struct i2c_kb1200_data i2c_kb1200_data_##n;			\
	static const struct i2c_kb1200_config i2c_kb1200_config_##n = {	\
		.base_addr = (uintptr_t)DT_INST_REG_ADDR(n),				\
		.port_num = (uint32_t)DT_INST_PROP(n, port_num),		\
	};								\
	DEVICE_DT_INST_DEFINE(n, &i2c_kb1200_init, NULL,			\
		&i2c_kb1200_data_##n, &i2c_kb1200_config_##n,			\
		PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
		&i2c_kb1200_api);					\

DT_INST_FOREACH_STATUS_OKAY(I2C_KB1200_DEVICE)
