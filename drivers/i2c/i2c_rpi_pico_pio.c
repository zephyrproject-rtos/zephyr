/*
 * Copyright (c) 2024 Yiding Jia <yiding.jia@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Portions Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>

#include <hardware/claim.h>

LOG_MODULE_REGISTER(i2c_pio);

#define DT_DRV_COMPAT raspberrypi_pico_pio_i2c

struct i2c_pio_config {
	const struct device *piodev;
	uint sm;
	const struct pinctrl_dev_config *pin_cfg;
	uint32_t bitrate;
};

struct i2c_pio_data {
	struct k_mutex lock;
	uint32_t config;
};

/**
 * @name I2C PIO Program
 * Based on pico example code:
 * https://github.com/raspberrypi/pico-examples/blob/master/pio/i2c/i2c.pio
 *
 * Some notes from the program reproduced here:
 *
 * @code{.unparsed}
 * TX Encoding:
 * | 15:10 | 9     | 8:1  | 0   |
 * | Instr | Final | Data | NAK |
 *
 * If Instr has a value n > 0, then this FIFO word has no
 * data payload, and the next n + 1 words will be executed as instructions.
 * Otherwise, shift out the 8 data bits, followed by the ACK bit.
 *
 * The Instr mechanism allows stop/start/repstart sequences to be programmed
 * by the processor, and then carried out by the state machine at defined points
 * in the datastream.
 *
 * The "Final" field should be set for the final byte in a transfer.
 * This tells the state machine to ignore a NAK: if this field is not
 * set, then any NAK will cause the state machine to halt and interrupt.
 *
 * Autopull should be enabled, with a threshold of 16.
 * Autopush should be enabled, with a threshold of 8.
 * The TX FIFO should be accessed with halfword writes, to ensure
 * the data is immediately available in the OSR.
 *
 * Pin mapping:
 * - Input pin 0 is SDA, 1 is SCL (if clock stretching used)
 * - Jump pin is SDA
 * - Side-set pin 0 is SCL
 * - Set pin 0 is SDA
 * - OUT pin 0 is SDA
 * - SCL must be SDA + 1 (for wait mapping)
 *
 * The OE outputs should be inverted in the system IO controls!
 * (It's possible for the inversion to be done in this program,
 * but costs 2 instructions: 1 for inversion, and one to cope
 * with the side effect of the MOV on TX shift counter.)
 * @endcode
 *
 */
/**@{*/

#define I2C_WRAP_TARGET        13
#define I2C_WRAP               18
#define I2C_OFFSET_ENTRY_POINT 13u
static const uint16_t i2c_program_instructions[] = {
	0x008d, /*  0: jmp    y--, 13 */
	0xc030, /*  1: irq    wait 0 rel */
	0xa0c3, /*  2: mov    isr, null */
	0xe027, /*  3: set    x, 7 */
	0x6781, /*  4: out    pindirs, 1             [7] */
	0xba42, /*  5: nop                    side 1 [2] */
	0x24a1, /*  6: wait   1 pin, 1               [4] */
	0x4701, /*  7: in     pins, 1                [7] */
	0x1744, /*  8: jmp    x--, 4          side 0 [7] */
	0x6781, /*  9: out    pindirs, 1             [7] */
	0xbf42, /* 10: nop                    side 1 [7] */
	0x27a1, /* 11: wait   1 pin, 1               [7] */
	0x12c0, /* 12: jmp    pin, 0          side 0 [2] */
	/*     .wrap_target */
	0x6026, /* 13: out    x, 6 */
	0x6041, /* 14: out    y, 1 */
	0x0022, /* 15: jmp    !x, 2 */
	0x6060, /* 16: out    null, 32 */
	0x60f0, /* 17: out    exec, 16 */
	0x0051, /* 18: jmp    x--, 17 */
		/*     .wrap */
};

static const struct pio_program i2c_program = {
	.instructions = i2c_program_instructions,
	.length = 19,
	.origin = -1,
};

static pio_sm_config i2c_program_get_default_config(uint offset)
{
	pio_sm_config c = pio_get_default_sm_config();

	sm_config_set_wrap(&c, offset + I2C_WRAP_TARGET, offset + I2C_WRAP);
	sm_config_set_sideset(&c, 2, true, true);

	return c;
}

/**@}*/

/* Instructions to manipulate clock and data lines to implement start/stop bits. */

/*  0: set    pindirs, 0      side 0 [7] */
#define I2C_SC0_SD0    0xf780
/*  1: set    pindirs, 1      side 0 [7] */
#define I2C_SC0_SD1    0xf781
/*  2: set    pindirs, 0      side 1 [7] */
#define I2C_SC1_SD0    0xff80
/*  3: set    pindirs, 1      side 1 [7] */
#define I2C_SC1_SD1    0xff81
/*  4: wait   1 pin, 1 */
#define I2C_WAIT_CLOCK 0x20a1

/**@}*/

#define PIO_I2C_ICOUNT_LSB 10
#define PIO_I2C_FINAL_LSB  9
#define PIO_I2C_DATA_LSB   1
#define PIO_I2C_NAK_LSB    0

static const uint16_t start_insts[] = {
	1u << PIO_I2C_ICOUNT_LSB, /* Escape code for 2 instruction sequence */
	I2C_SC1_SD0,              /* We are already in idle state, just pull SDA low */
	I2C_SC0_SD0,              /* Also pull clock low so we can present data */
};

static const uint16_t stop_insts[] = {
	3u << PIO_I2C_ICOUNT_LSB, /* Escape code for 4 instruction sequence */
	I2C_SC0_SD0,              /* SDA is unknown; pull it down */
	I2C_SC1_SD0,              /* Release clock */
	I2C_WAIT_CLOCK,           /* Wait for clock stretching */
	I2C_SC1_SD1,              /* Release SDA to return to idle state */
};

static const uint16_t repstart_insts[] = {
	4u << PIO_I2C_ICOUNT_LSB, /* Escape code for 5 instruction sequence */
	I2C_SC0_SD1,              /* SDA is unknown; pull it down */
	I2C_SC1_SD1,              /* Release clock */
	I2C_WAIT_CLOCK,           /* Release wait for clock stretching */
	I2C_SC1_SD0,              /* Pull SDA low */
	I2C_SC0_SD0,              /* Pull clock low */
};

static inline bool pio_i2c_check_error(PIO pio, uint sm)
{
	return pio_interrupt_get(pio, sm);
}

static void pio_i2c_resume_after_error(PIO pio, uint sm)
{
	pio_sm_set_enabled(pio, sm, false);
	pio_sm_drain_tx_fifo(pio, sm);
	pio_sm_restart(pio, sm);
	pio_sm_exec(pio, sm,
		    (pio->sm[sm].execctrl & PIO_SM0_EXECCTRL_WRAP_BOTTOM_BITS) >>
			    PIO_SM0_EXECCTRL_WRAP_BOTTOM_LSB);
	pio_interrupt_clear(pio, sm);
	pio_sm_set_enabled(pio, sm, true);
}

/**
 * Enable autopush to RX fifo. The program is always reading, but if autopush is
 * disabled the reads are not pushed out.
 */
static inline void pio_i2c_rx_enable(PIO pio, uint sm, bool en)
{
	if (en) {
		hw_set_bits(&pio->sm[sm].shiftctrl, PIO_SM0_SHIFTCTRL_AUTOPUSH_BITS);
	} else {
		hw_clear_bits(&pio->sm[sm].shiftctrl, PIO_SM0_SHIFTCTRL_AUTOPUSH_BITS);
	}
}

static void pio_i2c_put16(PIO pio, uint sm, uint16_t data)
{
	while (pio_sm_is_tx_fifo_full(pio, sm)) {
		k_yield();
	}

	*(io_rw_16 *)&pio->txf[sm] = data;
}

/* If I2C is ok, block and push data. Otherwise fall straight through. */
static void pio_i2c_put_or_err(PIO pio, uint sm, uint16_t data)
{
	while (pio_sm_is_tx_fifo_full(pio, sm)) {
		if (pio_i2c_check_error(pio, sm)) {
			return;
		}
		k_yield();
	}

	if (pio_i2c_check_error(pio, sm)) {
		return;
	}

	*(io_rw_16 *)&pio->txf[sm] = data;
}

static uint8_t pio_i2c_get(PIO pio, uint sm)
{
	return (uint8_t)pio_sm_get(pio, sm);
}

static void pio_i2c_put16_array_or_err(PIO pio, uint sm, const uint16_t *data, uint len)
{
	for (uint i = 0; i < len; i++) {
		pio_i2c_put_or_err(pio, sm, data[i]);
	}
}

static void pio_i2c_start(PIO pio, uint sm)
{
	pio_i2c_put16_array_or_err(pio, sm, start_insts, ARRAY_SIZE(start_insts));
}

static void pio_i2c_stop(PIO pio, uint sm)
{
	pio_i2c_put16_array_or_err(pio, sm, stop_insts, ARRAY_SIZE(stop_insts));
};

static void pio_i2c_repstart(PIO pio, uint sm)
{
	pio_i2c_put16_array_or_err(pio, sm, repstart_insts, ARRAY_SIZE(repstart_insts));
}

static void pio_i2c_wait_idle(PIO pio, uint sm)
{
	/* Finished when TX runs dry or SM hits an IRQ */
	pio->fdebug = BIT(PIO_FDEBUG_TXSTALL_LSB + sm);
	while (!(pio->fdebug & BIT(PIO_FDEBUG_TXSTALL_LSB + sm) || pio_i2c_check_error(pio, sm))) {
		k_yield();
	}
}

static int pio_i2c_write_blocking(PIO pio, uint sm, uint8_t addr, uint8_t *txbuf, uint len,
				  uint8_t flags, bool need_start)
{
	int err = 0;

	bool sent_start_or_restart;

	if (flags & I2C_MSG_RESTART) {
		pio_i2c_repstart(pio, sm);
		sent_start_or_restart = true;
	} else if (need_start) {
		pio_i2c_start(pio, sm);
		sent_start_or_restart = true;
	} else {
		sent_start_or_restart = false;
	}

	pio_i2c_rx_enable(pio, sm, false);
	if (sent_start_or_restart) {
		pio_i2c_put16(pio, sm, (addr << 2) | BIT(PIO_I2C_NAK_LSB));
	}

	while (len && !pio_i2c_check_error(pio, sm)) {
		if (!pio_sm_is_tx_fifo_full(pio, sm)) {
			/*
			 * Always release SDA (set NAK bit to 1) to let peripheral pull-down for
			 * ACK.
			 */
			uint16_t cmd = (*txbuf << PIO_I2C_DATA_LSB) | BIT(PIO_I2C_NAK_LSB);

			pio_i2c_put_or_err(pio, sm, cmd);
			txbuf++;
			len--;
		} else {
			k_yield();
		}
	}

	if (flags & I2C_MSG_STOP) {
		pio_i2c_stop(pio, sm);
	}

	pio_i2c_wait_idle(pio, sm);

	if (pio_i2c_check_error(pio, sm)) {
		err = -1;
		pio_i2c_resume_after_error(pio, sm);
		pio_i2c_stop(pio, sm);
	}

	return err;
}

static int pio_i2c_read_blocking(PIO pio, uint sm, uint8_t addr, uint8_t *rxbuf, uint len,
				 uint8_t flags, bool need_start)
{
	bool sent_start_or_restart;

	if (flags & I2C_MSG_RESTART) {
		pio_i2c_repstart(pio, sm);
		sent_start_or_restart = true;
	} else if (need_start) {
		pio_i2c_start(pio, sm);
		sent_start_or_restart = true;
	} else {
		sent_start_or_restart = false;
	}

	pio_i2c_rx_enable(pio, sm, true);
	while (!pio_sm_is_rx_fifo_empty(pio, sm)) {
		(void)pio_i2c_get(pio, sm);
	}
	if (sent_start_or_restart) {
		pio_i2c_put16(pio, sm, (addr << 2) | 2u | BIT(PIO_I2C_NAK_LSB));
	}
	uint32_t tx_remain = len; /* Need to stuff 0xff bytes in to get clocks */

	bool first = true;

	while ((tx_remain || len) && !pio_i2c_check_error(pio, sm)) {
		if (tx_remain && !pio_sm_is_tx_fifo_full(pio, sm)) {
			uint16_t cmd;

			--tx_remain;
			cmd = 0xffu << PIO_I2C_DATA_LSB;
			/*
			 * For the final receiving byte, send a NACK to indicate finished
			 * reading, and set Final so PIO doesn't error on the NACK we sent.
			 */
			cmd |= !tx_remain ? (BIT(PIO_I2C_FINAL_LSB) | BIT(PIO_I2C_NAK_LSB)) : 0;
			pio_i2c_put16(pio, sm, cmd);
		}
		if (!pio_sm_is_rx_fifo_empty(pio, sm)) {
			if (first) {
				/* Ignore returned address byte */
				(void)pio_i2c_get(pio, sm);
				first = false;
			} else {
				--len;
				*rxbuf = pio_i2c_get(pio, sm);
				rxbuf++;
			}
		} else {
			k_yield();
		}
	}

	if (flags & I2C_MSG_STOP) {
		pio_i2c_stop(pio, sm);
	}

	pio_i2c_wait_idle(pio, sm);

	int err = 0;

	if (pio_i2c_check_error(pio, sm)) {
		err = -1;
		pio_i2c_resume_after_error(pio, sm);
		pio_i2c_stop(pio, sm);
	}

	return err;
}

static int i2c_pio_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_pio_data *data = dev->data;

	if (I2C_SPEED_GET(dev_config) != I2C_SPEED_DT) {
		return -ENOTSUP;
	}

	data->config = dev_config;

	return 0;
}

static int i2c_pio_get_config(const struct device *dev, uint32_t *config)
{
	struct i2c_pio_data *data = dev->data;

	return data->config;
}

static int i2c_pio_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			    uint16_t addr)
{
	const struct i2c_pio_config *cfg = dev->config;
	struct i2c_pio_data *data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(cfg->piodev);
	int rc;

	k_mutex_lock(&data->lock, K_FOREVER);

	bool need_start = true;

	for (int i = 0; i < num_msgs; i++) {
		/* per i2c_transfer(): last message gets an implicit STOP */
		uint8_t flags = msgs[i].flags | ((i == num_msgs - 1) ? I2C_MSG_STOP : 0);

		if (msgs[i].flags & I2C_MSG_READ) {
			rc = pio_i2c_read_blocking(pio, cfg->sm, addr, msgs[i].buf, msgs[i].len,
						   flags, need_start);
		} else {
			rc = pio_i2c_write_blocking(pio, cfg->sm, addr, msgs[i].buf, msgs[i].len,
						    flags, need_start);
		}

		if (rc) {
			rc = -EIO;
			goto done;
		}

		need_start = flags & I2C_MSG_STOP;
	}

	rc = 0;
done:
	k_mutex_unlock(&data->lock);

	return rc;
}

static int i2c_pio_recover_bus(const struct device *dev)
{
	const struct i2c_pio_config *cfg = dev->config;
	struct i2c_pio_data *data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(cfg->piodev);

	k_mutex_lock(&data->lock, K_FOREVER);

	pio_i2c_resume_after_error(pio, cfg->sm);

	k_mutex_unlock(&data->lock);

	return 0;
}

static DEVICE_API(i2c, api) = {
	.configure = i2c_pio_configure,
	.get_config = i2c_pio_get_config,
	.transfer = i2c_pio_transfer,
	.recover_bus = i2c_pio_recover_bus,
};

#if NUM_PIOS == 2
/* RP2040 has 2 PIO */
static uint8_t i2c_pio_program_offset[NUM_PIOS] = {255, 255};
static inline int get_pio_idx(PIO pio)
{
	return pio == pio0 ? 0 : 1;
}
#elif NUM_PIOS == 3
/* RP2350 has 3 PIO */
static uint8_t i2c_pio_program_offset[NUM_PIOS] = {255, 255, 255};
static inline int get_pio_idx(PIO pio)
{
	return pio == pio0 ? 0 : pio == pio1 ? 1 : 2;
}
#else
BUILD_ASSERT(0, "Unsupported number of PIOs");
#endif
K_MUTEX_DEFINE(i2c_pio_program_loader_mtx);

/**
 * Load i2c program once per pio, if using multiple instances of this driver.
 *
 * @return offset of the program in the PIO.
 */
static uint8_t load_program_once(PIO pio)
{
	int pionum = get_pio_idx(pio);

	k_mutex_lock(&i2c_pio_program_loader_mtx, K_FOREVER);

	if (i2c_pio_program_offset[pionum] == 255) {
		i2c_pio_program_offset[pionum] = pio_add_program(pio, &i2c_program);
	}
	uint8_t offset = i2c_pio_program_offset[pionum];

	k_mutex_unlock(&i2c_pio_program_loader_mtx);

	return offset;
}

static int i2c_pio_init(const struct device *dev)
{
	const struct i2c_pio_config *cfg = dev->config;
	struct i2c_pio_data *data = dev->data;
	int rc;

	if (!device_is_ready(cfg->piodev)) {
		return -ENODEV;
	}

	__ASSERT(!is_spin_locked(spin_lock_instance(PICO_SPINLOCK_ID_HARDWARE_CLAIM)),
		 "hardware claim lock should not be locked right now.");

	k_mutex_init(&data->lock);
	data->config = I2C_SPEED_SET(I2C_SPEED_DT);

	PIO pio = pio_rpi_pico_get_pio(cfg->piodev);

	pio_sm_claim(pio, cfg->sm);

	uint8_t program_offset = load_program_once(pio);

	if (program_offset == 255) {
		return -ENOMEM;
	}

	const struct pinctrl_state *pinctrl;

	rc = pinctrl_lookup_state(cfg->pin_cfg, PINCTRL_STATE_DEFAULT, &pinctrl);
	if (rc < 0) {
		return rc;
	}

	__ASSERT(pinctrl->pin_cnt == 2, "Expected two pins in pinctrl state.");

	uint pin_sda = pinctrl->pins[0].pin_num;
	uint pin_scl = pinctrl->pins[1].pin_num;

	__ASSERT(pin_scl == pin_sda + 1, "SCL pin must be SDA + 1");

	/* Largely taken from official example's i2c.pio */

	pio_sm_config sm_cfg = i2c_program_get_default_config(program_offset);

	sm_config_set_out_pins(&sm_cfg, pin_sda, 1);
	sm_config_set_set_pins(&sm_cfg, pin_sda, 1);
	sm_config_set_in_pins(&sm_cfg, pin_sda);
	sm_config_set_sideset_pins(&sm_cfg, pin_scl);
	sm_config_set_jmp_pin(&sm_cfg, pin_sda);

	sm_config_set_out_shift(&sm_cfg, false, true, 16);
	sm_config_set_in_shift(&sm_cfg, false, true, 8);

	float clkdiv = (float)sys_clock_hw_cycles_per_sec() / (32 * cfg->bitrate);

	sm_config_set_clkdiv(&sm_cfg, clkdiv);

	/* Configure pinctrl for use with SM. */
	uint32_t both_pins = BIT(pin_sda) | BIT(pin_scl);

	pio_sm_set_pins_with_mask(pio, cfg->sm, both_pins, both_pins);
	pio_sm_set_pindirs_with_mask(pio, cfg->sm, both_pins, both_pins);

	rc = pinctrl_apply_state(cfg->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		return rc;
	}

	pio_sm_set_pins_with_mask(pio, cfg->sm, 0, both_pins);

	/* IRQ used as status flag, ensure it doesn't trigger system interrupt. */
	pio_set_irq0_source_enabled(pio, pis_interrupt0 + cfg->sm, false);
	pio_set_irq1_source_enabled(pio, pis_interrupt0 + cfg->sm, false);
	pio_interrupt_clear(pio, cfg->sm);

	pio_sm_init(pio, cfg->sm, I2C_OFFSET_ENTRY_POINT + program_offset, &sm_cfg);
	pio_sm_set_enabled(pio, cfg->sm, true);

	return 0;
}

#define DEFINE_I2C_PIO(inst)                                                                       \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static struct i2c_pio_data i2c_pio_dev_data_##inst;                                        \
                                                                                                   \
	static const struct i2c_pio_config i2c_pio_dev_cfg_##inst = {                              \
		.piodev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
		.sm = DT_INST_REG_ADDR(inst),                                                      \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                   \
		.bitrate = DT_INST_PROP(inst, clock_frequency),                                    \
	};                                                                                         \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(inst, i2c_pio_init, NULL, &i2c_pio_dev_data_##inst,              \
				  &i2c_pio_dev_cfg_##inst, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,  \
				  &api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_I2C_PIO)
