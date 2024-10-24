#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(i2c_xilinx_ps, CONFIG_I2C_LOG_LEVEL);

#include "i2c_xilinx_ps.h"
#include "i2c-priv.h"

struct i2c_xilinx_ps_config {
	mem_addr_t base;
	void (*irq_config_func)(const struct device *dev);
	/* Whether device has working dynamic read (broken prior to core rev. 2.1) */
	// bool dyn_read_working;
};

struct i2c_xilinx_ps_data {
	struct k_event irq_event;
	/* Serializes between ISR and other calls */
	struct k_spinlock lock;
	/* Provides exclusion against multiple concurrent requests */
	struct k_mutex mutex;

	struct i2c_msg *msgs;
	/* if received data larger than fifo size */
	bool more_data;

};

static void i2c_xilinx_ps_abort(const struct device *dev, const struct i2c_xilinx_ps_config *config)
{
	uint32_t reg_val;
	uint32_t irq_val;
	struct i2c_xilinx_ps_data *data = dev->data;
	const k_spinlock_key_t key = k_spin_lock(&data->lock);
	/*
	 * Enter a critical section, so disable the interrupts while we clear
	 * the FIFO and the status register.
	 */
	reg_val = sys_read32(config->base + REG_IMR);
	sys_write32(XIICPS_IXR_ALL_INTR_MASK, config->base + REG_IDR);

	/*
	 * Reset the settings in config register and clear the FIFOs.
	 */
	sys_write32( (uint32_t)XIICPS_CR_RESET_VALUE | (uint32_t)XIICPS_CR_CLR_FIFO_MASK, config->base + REG_CR);

	/*
	 * Read, then write the interrupt status to make sure there are no
	 * pending interrupts.
	 */
	irq_val = sys_read32(config->base + REG_ISR);
	sys_write32(irq_val, config->base + REG_ISR);

	/*
	 * Restore the interrupt state.
	 */
	reg_val = (uint32_t)XIICPS_IXR_ALL_INTR_MASK & (~reg_val);
	sys_write32(reg_val, config->base + REG_IER);
	k_spin_unlock(&data->lock, key);

}

static void i2c_xilinx_ps_reinit(struct i2c_xilinx_ps_data *data , const struct i2c_xilinx_ps_config *config)
{
	LOG_DBG("Controller reinit");
        /*
         * Abort any transfer that is in progress.
         */
	i2c_xilinx_ps_abort(data, config);

        /*
         * Reset any values so the software state matches the hardware device.
         */
	sys_write32(XIICPS_CR_RESET_VALUE, config->base + REG_CR);
	sys_write32(XIICPS_TO_RESET_VALUE, config->base + REG_TIME_OUT);
	sys_write32(XIICPS_IXR_ALL_INTR_MASK, config->base + REG_IDR);
}

int32_t i2c_xilinx_ps_clock_config(const struct i2c_xilinx_ps_config *config, uint32_t FsclHz)
{
	uint32_t Div_a;
	uint32_t Div_b;
	uint32_t ActualFscl;
	uint32_t Temp;
	uint32_t TempLimit;
	uint32_t LastError;
	uint32_t BestError;
	uint32_t CurrentError;
	uint32_t ControlReg;
	uint32_t CalcDivA;
	uint32_t CalcDivB;
	uint32_t BestDivA;
	uint32_t BestDivB;
	uint32_t FsclHzVar = FsclHz;


	if (0U != sys_read32(config->base + REG_TRANS_SIZE)) {
		return -EPERM;
	}

	/*
	 * Assume Div_a is 0 and calculate (divisor_a+1) x (divisor_b+1).
	 */
	Temp = 99990005 / ((uint32_t)22U * FsclHzVar);

	/*
	 * If the answer is negative or 0, the Fscl input is out of range.
	 */
	if ((uint32_t)(0U) == Temp) {
		return -EDOM;
	}

	/*
	 * If frequency 400KHz is selected, 384.6KHz should be set.
	 * If frequency 100KHz is selected, 90KHz should be set.
	 * This is due to a hardware limitation.
	 */
	if(FsclHzVar > (uint32_t)384600U) {
		FsclHzVar = (uint32_t)384600U;
	}

	// if((FsclHzVar <= (uint32_t)100000U) && (FsclHzVar > (uint32_t)90000U)) {
	// 	FsclHzVar = (uint32_t)90000U;
	// }

	/*
	 * TempLimit helps in iterating over the consecutive value of Temp to
	 * find the closest clock rate achievable with divisors.
	 * Iterate over the next value only if fractional part is involved.
	 */
	TempLimit = (((99990005) %
			((uint32_t)22 * FsclHzVar)) != 	(uint32_t)0x0U) ?
						 (Temp + (uint32_t)1U) : Temp;
	BestError = FsclHzVar;

	BestDivA = 0U;
	BestDivB = 0U;
	for ( ; Temp <= TempLimit ; Temp++)
	{
		LastError = FsclHzVar;
		CalcDivA = 0U;
		CalcDivB = 0U;

		for (Div_b = 0U; Div_b < 64U; Div_b++) {

			Div_a = Temp / (Div_b + 1U);

			if (Div_a != 0U){
				Div_a = Div_a - (uint32_t)1U;
			}
			if (Div_a > 3U){
				continue;
			}
			ActualFscl = (99990005) /
						(22U * (Div_a + 1U) * (Div_b + 1U));

			if (ActualFscl > FsclHzVar){
				CurrentError = (ActualFscl - FsclHzVar);}
			else{
				CurrentError = (FsclHzVar - ActualFscl);}

			if (LastError > CurrentError) {
				CalcDivA = Div_a;
				CalcDivB = Div_b;
				LastError = CurrentError;
			}
		}

		/*
		 * Used to capture the best divisors.
		 */
		if (LastError < BestError) {
			BestError = LastError;
			BestDivA = CalcDivA;
			BestDivB = CalcDivB;
		}
	}


	/*
	 * Read the control register and mask the Divisors.
	 */
	ControlReg = sys_read32(config->base + REG_CR);
	ControlReg &= ~((uint32_t)XIICPS_CR_DIV_A_MASK | (uint32_t)XIICPS_CR_DIV_B_MASK);
	ControlReg |= (BestDivA << XIICPS_CR_DIV_A_SHIFT) |
		(BestDivB << XIICPS_CR_DIV_B_SHIFT);

	sys_write32(ControlReg, config->base + REG_CR);

	return 0;
}

static void i2c_xilinx_ps_isr(const struct device *dev)
{

	const struct i2c_xilinx_ps_config *config = dev->config;
	struct i2c_xilinx_ps_data *data = dev->data;
	const k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t int_enable = ~(sys_read32(config->base + REG_IMR));
	const uint32_t int_status = sys_read32(config->base + REG_ISR);
	uint32_t ints_to_clear = int_status;
	uint32_t ints_to_mask = int_status;

	printf("ISR called for 0x%08" PRIxPTR ", status 0x%08x", config->base, int_status);


	sys_write32(int_enable & ~ints_to_mask, config->base + REG_IER);
	/* Be careful, writing 1 to a bit that is not currently set in ISR will SET it! */
	sys_write32(ints_to_clear & sys_read32(config->base + REG_ISR), config->base + REG_ISR);

	k_spin_unlock(&data->lock, key);
	k_event_post(&data->irq_event, int_status);

}

static void i2c_xilinx_ps_clear_interrupt(const struct i2c_xilinx_ps_config *config,
					   struct i2c_xilinx_ps_data *data, uint32_t int_mask)
{
	const k_spinlock_key_t key = k_spin_lock(&data->lock);
	const uint32_t int_status = sys_read32(config->base + REG_ISR);

	if (int_status & int_mask) {
		sys_write32(int_status & int_mask, config->base + REG_ISR);
	}
	k_spin_unlock(&data->lock, key);
}



static void i2c_xilinx_ps_enable_interrupt(const struct i2c_xilinx_ps_config *config,
					   struct i2c_xilinx_ps_data *data, uint32_t int_mask)
{
	const k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (int_mask) {
		sys_write32(int_mask, config->base + REG_IER);
	}
	k_spin_unlock(&data->lock, key);
}

static uint32_t i2c_xilinx_ps_wait_interrupt(const struct i2c_xilinx_ps_config *config,
					      struct i2c_xilinx_ps_data *data, uint32_t int_mask)
{
	const k_spinlock_key_t key = k_spin_lock(&data->lock);
	const uint32_t int_enable = sys_read32(config->base + REG_IER) | int_mask;
	uint32_t events;

	LOG_DBG("Set IER to 0x%02x", int_enable);
	sys_write32(int_enable, config->base + REG_IER);
	k_event_clear(&data->irq_event, int_mask);
	k_spin_unlock(&data->lock, key);

	events = k_event_wait(&data->irq_event, int_mask, false, K_MSEC(1000));

	LOG_DBG("Got ISR events 0x%02x", events);
	if (!events) {
		LOG_ERR("Timeout waiting for ISR events 0x%02x, SR 0x%02x, ISR 0x%02x", int_mask,
			sys_read32(config->base + REG_SR), sys_read32(config->base + REG_ISR));
	}
	return events;
}
static int i2c_xilinx_ps_wait_rx_full(const struct i2c_xilinx_ps_config *config,
				       struct i2c_xilinx_ps_data *data, uint32_t read_bytes)
{
	uint32_t events;

	i2c_xilinx_ps_clear_interrupt(config, data, ISR_RX_OVF);

	events = i2c_xilinx_ps_wait_interrupt(config, data, ISR_RX_OVF | ISR_ARB_LOST | ISR_TX_COMP);
	if (!events) {
		return -ETIMEDOUT;
	}
	if ((events & ISR_ARB_LOST) | (events & ISR_RX_OVF)) {
		LOG_ERR("Arbitration lost on RX");
		return -ENXIO;
	}
	return 0;
}
static int i2c_xilinx_ps_wait_not_busy(const struct i2c_xilinx_ps_config *config,
					struct i2c_xilinx_ps_data *data)
{
	if (sys_read32(config->base + REG_SR) & XIICPS_SR_BA_MASK) {
		uint32_t events = i2c_xilinx_ps_wait_interrupt(config, data, ISR_TX_COMP);
		printf("123\n");
		if (events != ISR_TX_COMP) {
			LOG_ERR("Bus stuck busy");
			i2c_xilinx_ps_reinit(data, config);
			return -EBUSY;
		}
	}
	return 0;
}

static int i2c_xilinx_ps_wait_tx_done(const struct i2c_xilinx_ps_config *config,
				       struct i2c_xilinx_ps_data *data)
{

	uint32_t events = i2c_xilinx_ps_wait_interrupt(
		config, data, ISR_TX_COMP | ISR_ARB_LOST | ISR_TIMEOUT | ISR_NACK);
	if (!(events & ISR_TX_COMP)) {
		if (events & ISR_TIMEOUT) {
			return -ETIMEDOUT;
		}
		if (events & ISR_ARB_LOST) {
			LOG_ERR("Arbitration lost on TX");
			return -EAGAIN;
		}
		if (events & ISR_NACK) {
			LOG_ERR("TX received NAK");
			return -ENXIO;
		}
	}
	return 0;
}

static int i2c_xilinx_ps_write(const struct i2c_xilinx_ps_config *config,
				struct i2c_xilinx_ps_data *data, const struct i2c_msg *msg,
				uint16_t addr)
{
	const uint8_t *write_ptr = msg->buf;
	uint8_t bytes_left = msg->len;
	uint8_t fifo_space; /* account for address being written */
	sys_write32(XIICPS_CR_CLR_FIFO_MASK | XIICPS_CR_ACKEN_MASK | XIICPS_CR_CLR_FIFO_MASK |
		XIICPS_CR_NEA_MASK | XIICPS_CR_MS_MASK | sys_read32(config->base + REG_CR), config->base + REG_CR);
	fifo_space = (uint8_t) FIFO_SIZE - (uint8_t)sys_read32(config->base + REG_TRANS_SIZE);
	sys_write32((uint32_t)~(XIICPS_CR_RD_WR_MASK) & sys_read32(config->base + REG_CR), config->base + REG_CR);
	if (!msg->flags & I2C_MSG_RESTART) {
		sys_write32((uint32_t) ~(XIICPS_CR_HOLD_MASK) & sys_read32(config->base + REG_CR), config->base + REG_CR);
	}
	sys_write32(XIICPS_IXR_ALL_INTR_MASK, config->base + REG_IDR);
	i2c_xilinx_ps_clear_interrupt(config, data, XIICPS_IXR_ALL_INTR_MASK);
		
	i2c_xilinx_ps_enable_interrupt(config, data, (uint32_t)XIICPS_IXR_NACK_MASK | (uint32_t)XIICPS_IXR_COMP_MASK |
	(uint32_t)XIICPS_IXR_ARB_LOST_MASK | (uint32_t)XIICPS_IXR_TO_MASK | (uint32_t)XIICPS_IXR_DATA_MASK | XIICPS_IXR_TX_OVR_MASK);
	while (bytes_left) {
		uint8_t bytes_to_send = bytes_left;
		const k_spinlock_key_t key = k_spin_lock(&data->lock);
		int ret;
		
		if (bytes_to_send > fifo_space) {
			bytes_to_send = fifo_space;
		}
		while (bytes_to_send) {
			uint8_t write_byte = *write_ptr++;
			printf("bytes_to_send %u\n", msg->buf);
			sys_write32(write_byte, config->base + REG_DATA);
			bytes_to_send--;
			bytes_left--;
		}
		
		sys_write32(addr, config->base + REG_ADDR);
		if(bytes_left <= 0)
		{
			sys_write32((uint32_t)(~XIICPS_CR_HOLD_MASK) & sys_read32(config->base + REG_CR), config->base + REG_CR);
		}
		k_spin_unlock(&data->lock, key);



		ret = i2c_xilinx_ps_wait_tx_done(config, data);
		if (ret) {
			return ret;
		}
	}
	return 0;
}


static int i2c_xilinx_ps_read(const struct i2c_xilinx_ps_config *config,
				      struct i2c_xilinx_ps_data *data, struct i2c_msg *msg,
				      uint16_t addr)
{
	uint8_t *read_ptr = msg->buf;
	uint32_t bytes_left = msg->len;

	
	if(bytes_left > FIFO_SIZE || msg->flags & I2C_MSG_RESTART) {
		sys_write32((uint32_t) XIICPS_CR_HOLD_MASK | sys_read32(config->base + REG_CR), config->base + REG_CR);
	}
	sys_write32(XIICPS_CR_ACKEN_MASK | XIICPS_CR_CLR_FIFO_MASK |
		XIICPS_CR_NEA_MASK | XIICPS_CR_MS_MASK | XIICPS_CR_RD_WR_MASK | sys_read32(config->base + REG_CR), config->base + REG_CR);
	sys_write32(XIICPS_IXR_ALL_INTR_MASK, config->base + REG_IDR);

	if(bytes_left > XIICPS_MAX_TRANSFER_SIZE) {
		sys_write32(XIICPS_MAX_TRANSFER_SIZE, config->base + REG_TRANS_SIZE);
		data->more_data = 1;
	} else {
		sys_write32(bytes_left, config->base + REG_TRANS_SIZE);
	}

	
	i2c_xilinx_ps_enable_interrupt(config, data, (uint32_t)XIICPS_IXR_NACK_MASK | (uint32_t)XIICPS_IXR_DATA_MASK | (uint32_t)XIICPS_IXR_RX_OVR_MASK |
	(uint32_t)XIICPS_IXR_COMP_MASK | (uint32_t)XIICPS_IXR_ARB_LOST_MASK | XIICPS_IXR_TO_MASK);

	while(bytes_left) {
		sys_write32(addr, config->base + REG_ADDR);
		int ret = i2c_xilinx_ps_wait_rx_full(config, data, bytes_left > XIICPS_MAX_TRANSFER_SIZE? XIICPS_MAX_TRANSFER_SIZE: bytes_left);
		while(sys_read32(config->base + REG_SR) & XIICPS_SR_RXDV_MASK) {
			uint8_t data = sys_read32(config->base + REG_DATA);
			*msg->buf = data;
			msg->buf++;
			bytes_left--;
		}
		if(bytes_left < FIFO_SIZE)  {
			sys_write32(~(XIICPS_CR_HOLD_MASK) & sys_read32(config->base + REG_CR), config->base + REG_CR);
		}
	}

	return 0;
}




static int i2c_xilinx_ps_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				   uint16_t addr)
{
	const struct i2c_xilinx_ps_config *config = dev->config;
	struct i2c_xilinx_ps_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->mutex, K_FOREVER);

	/**
	 * Reinitializing before each transfer shouldn't technically be needed, but
	 * seems to improve general reliability. The Linux driver also does this.
	 */
	// i2c_xilinx_ps_reinit(data, config);

	ret = i2c_xilinx_ps_wait_not_busy(config, data);
	if (ret) {
		goto out_unlock;
	}

	if (!num_msgs) {
		goto out_unlock;
	}

	do {
		if (msgs->flags & I2C_MSG_ADDR_10_BITS) {
			/* Optionally supported in core, but not implemented in driver yet */
			ret = -EOPNOTSUPP;
			goto out_unlock;
		}
		if (msgs->flags & I2C_MSG_READ) {
			ret = i2c_xilinx_ps_read(config, data, msgs, addr);
		} else {
			printf("test\n");
			ret = i2c_xilinx_ps_write(config, data, msgs, addr);
			ret = i2c_xilinx_ps_wait_not_busy(config, data);
		}
		if (!ret && (msgs->flags & I2C_MSG_STOP)) {
			ret = i2c_xilinx_ps_wait_not_busy(config, data);
		}
		if (ret) {
			goto out_unlock;
		}
		msgs++;
		num_msgs--;
	} while (num_msgs);

out_unlock:
	k_mutex_unlock(&data->mutex);
	return ret;
}


 
static int i2c_xilinx_ps_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_xilinx_ps_config *config = dev->config;

	LOG_INF("Configuring %s at 0x%08" PRIxPTR, dev->name, config->base);	
	i2c_xilinx_ps_reinit(dev->data, config);
	return 0;
}

static int i2c_xilinx_ps_init(const struct device *dev)
{
	const struct i2c_xilinx_ps_config *config = dev->config;
	struct i2c_xilinx_ps_data *data = dev->data;
	int error;

	k_event_init(&data->irq_event);
	k_mutex_init(&data->mutex);

	error = i2c_xilinx_ps_configure(dev, I2C_MODE_CONTROLLER);
	if (error) {
		return error;
	}
	error = i2c_xilinx_ps_clock_config(config, 100000);
	if (error) {
		return error;
	}
	config->irq_config_func(dev);

	LOG_INF("initialized");
	return 0;
}

static const struct i2c_driver_api i2c_xilinx_ps_driver_api = {
	.configure = i2c_xilinx_ps_configure,
	.transfer = i2c_xilinx_ps_transfer,
};


#define I2C_XILINX_PS_INIT(n) \
	static void i2c_xilinx_ps_config_func_##n(const struct device *dev);\
	static const struct i2c_xilinx_ps_config i2c_xilinx_ps_config_##n = {         \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.irq_config_func = i2c_xilinx_ps_config_func_##n,                      \
	};\
	static struct i2c_xilinx_ps_data i2c_xilinx_ps_data_##n;                      \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_xilinx_ps_init, NULL,                                    \
				  &i2c_xilinx_ps_data_##n,                             \
				  &i2c_xilinx_ps_config_##n, POST_KERNEL,              \
				  CONFIG_I2C_INIT_PRIORITY, &i2c_xilinx_ps_driver_api);           \
	static void i2c_xilinx_ps_config_func_##n(const struct device *dev)            \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		printf("irq config %d\n", DT_INST_IRQN(n));\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2c_xilinx_ps_isr,         \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define DT_DRV_COMPAT xlnx_xps_iic
DT_INST_FOREACH_STATUS_OKAY(I2C_XILINX_PS_INIT)