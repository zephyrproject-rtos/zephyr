/*
 * Copyright (c) 2023 Stephen Boylan <stephen.boylan@beechwoods.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_spi_pio

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_pico_pio);

#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include "spi_context.h"

#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>

#include <hardware/pio.h>
#include "hardware/clocks.h"

#define SPI_RPI_PICO_PIO_HALF_DUPLEX_ENABLED DT_ANY_INST_HAS_PROP_STATUS_OKAY(sio_gpios)

#define PIO_CYCLES     (4)
#define PIO_FIFO_DEPTH (4)

struct spi_pico_pio_config {
	const struct device *piodev;
	const struct pinctrl_dev_config *pin_cfg;
	struct gpio_dt_spec clk_gpio;
	struct gpio_dt_spec mosi_gpio;
	struct gpio_dt_spec miso_gpio;
	struct gpio_dt_spec sio_gpio;
	const struct device *clk_dev;
	clock_control_subsys_t clk_id;
};

struct spi_pico_pio_data {
	struct spi_context spi_ctx;
	uint32_t tx_count;
	uint32_t rx_count;
	PIO pio;
	size_t pio_sm;
	uint32_t pio_tx_offset;
	uint32_t pio_rx_offset;
	uint32_t pio_rx_wrap_target;
	uint32_t pio_rx_wrap;
	uint32_t bits;
	uint32_t dfs;
};

/* ------------ */
/* spi_mode_0_0 */
/* ------------ */

#define SPI_MODE_0_0_WRAP_TARGET 0
#define SPI_MODE_0_0_WRAP        1
#define SPI_MODE_0_0_CYCLES      4

RPI_PICO_PIO_DEFINE_PROGRAM(spi_mode_0_0, SPI_MODE_0_0_WRAP_TARGET, SPI_MODE_0_0_WRAP,
			    /*     .wrap_target */
			    0x6101, /*  0: out    pins, 1         side 0 [1] */
			    0x5101, /*  1: in     pins, 1         side 1 [1] */
				    /*     .wrap */
);

/* ------------ */
/* spi_mode_1_1 */
/* ------------ */

#define SPI_MODE_1_1_WRAP_TARGET 0
#define SPI_MODE_1_1_WRAP        2
#define SPI_MODE_1_1_CYCLES      4

RPI_PICO_PIO_DEFINE_PROGRAM(spi_mode_1_1, SPI_MODE_1_1_WRAP_TARGET, SPI_MODE_1_1_WRAP,
			    /*     .wrap_target */
			    0x7021, /*  0: out    x, 1            side 1 */
			    0xa101, /*  1: mov    pins, x         side 0 [1] */
			    0x5001, /*  2: in     pins, 1         side 1 */
				    /*     .wrap */
);

#if SPI_RPI_PICO_PIO_HALF_DUPLEX_ENABLED
/* ------------------- */
/* spi_sio_mode_0_0_tx */
/* ------------------- */

#define SPI_SIO_MODE_0_0_TX_WRAP_TARGET 0
#define SPI_SIO_MODE_0_0_TX_WRAP        2
#define SPI_SIO_MODE_0_0_TX_CYCLES      2

RPI_PICO_PIO_DEFINE_PROGRAM(spi_sio_mode_0_0_tx, SPI_SIO_MODE_0_0_TX_WRAP_TARGET,
			    SPI_SIO_MODE_0_0_TX_WRAP,
			    /*     .wrap_target */
			    0x80a0, /*  0: pull   block           side 0  */
			    0x6001, /*  1: out    pins, 1         side 0  */
			    0x10e1, /*  2: jmp    !osre, 1        side 1  */
				    /*     .wrap */
);

/* ------------------------- */
/* spi_sio_mode_0_0_rx */
/* ------------------------- */

#define SPI_SIO_MODE_0_0_RX_WRAP_TARGET 0
#define SPI_SIO_MODE_0_0_RX_WRAP        3
#define SPI_SIO_MODE_0_0_RX_CYCLES      2

RPI_PICO_PIO_DEFINE_PROGRAM(spi_sio_mode_0_0_rx, SPI_SIO_MODE_0_0_RX_WRAP_TARGET,
			    SPI_SIO_MODE_0_0_RX_WRAP,
			    /*     .wrap_target */
			    0x80a0, /*  0: pull   block           side 0 */
			    0x6020, /*  1: out    x, 32           side 0 */
			    0x5001, /*  2: in     pins, 1         side 1 */
			    0x0042, /*  3: jmp    x--, 2          side 0 */
				    /*     .wrap */
);
#endif /* SPI_RPI_PICO_PIO_HALF_DUPLEX_ENABLED */

static float spi_pico_pio_clock_divisor(const uint32_t clock_freq, int cycles,
					uint32_t spi_frequency)
{
	return (float)clock_freq / (float)(cycles * spi_frequency);
}

static uint32_t spi_pico_pio_maximum_clock_frequency(const uint32_t clock_freq, int cycles)
{
	return clock_freq / cycles;
}

static uint32_t spi_pico_pio_minimum_clock_frequency(const uint32_t clock_freq, int cycles)
{
	return clock_freq / (cycles * 65536);
}

static inline bool spi_pico_pio_transfer_ongoing(struct spi_pico_pio_data *data)
{
	return spi_context_tx_on(&data->spi_ctx) || spi_context_rx_on(&data->spi_ctx);
}

static inline void spi_pico_pio_sm_put8(PIO pio, uint sm, uint8_t data)
{
	/* Do 8 bit accesses on FIFO, so that write data is byte-replicated. This */
	/* gets us the left-justification for free (for MSB-first shift-out) */
	io_rw_8 *txfifo = (io_rw_8 *)&pio->txf[sm];

	*txfifo = data;
}

static inline uint8_t spi_pico_pio_sm_get8(PIO pio, uint sm)
{
	/* Do 8 bit accesses on FIFO, so that write data is byte-replicated. This */
	/* gets us the left-justification for free (for MSB-first shift-out) */
	io_rw_8 *rxfifo = (io_rw_8 *)&pio->rxf[sm];

	return *rxfifo;
}

static inline void spi_pico_pio_sm_put16(PIO pio, uint sm, uint16_t data)
{
	/* Do 16 bit accesses on FIFO, so that write data is halfword-replicated. This */
	/* gets us the left-justification for free (for MSB-first shift-out) */
	io_rw_16 *txfifo = (io_rw_16 *)&pio->txf[sm];

	*txfifo = data;
}

static inline uint16_t spi_pico_pio_sm_get16(PIO pio, uint sm)
{
	io_rw_16 *rxfifo = (io_rw_16 *)&pio->rxf[sm];

	return *rxfifo;
}

static inline void spi_pico_pio_sm_put32(PIO pio, uint sm, uint32_t data)
{
	io_rw_32 *txfifo = (io_rw_32 *)&pio->txf[sm];

	*txfifo = data;
}

static inline uint32_t spi_pico_pio_sm_get32(PIO pio, uint sm)
{
	io_rw_32 *rxfifo = (io_rw_32 *)&pio->rxf[sm];

	return *rxfifo;
}

static inline int spi_pico_pio_sm_complete(struct spi_pico_pio_data *data)
{
	return ((data->pio->sm[data->pio_sm].addr == data->pio_tx_offset) &&
		pio_sm_is_tx_fifo_empty(data->pio, data->pio_sm));
}

static int spi_pico_pio_configure(const struct spi_pico_pio_config *dev_cfg,
				  struct spi_pico_pio_data *data, const struct spi_config *spi_cfg)
{
	const struct gpio_dt_spec *clk = NULL;
	pio_sm_config sm_config;
	bool lsb = false;
	uint32_t cpol = 0;
	uint32_t cpha = 0;
	uint32_t rc = 0;
	uint32_t clock_freq;

	rc = clock_control_on(dev_cfg->clk_dev, dev_cfg->clk_id);
	if (rc < 0) {
		LOG_ERR("Failed to enable the clock");
		return rc;
	}

	rc = clock_control_get_rate(dev_cfg->clk_dev, dev_cfg->clk_id, &clock_freq);
	if (rc < 0) {
		LOG_ERR("Failed to get clock frequency");
		return rc;
	}

	if (spi_context_configured(&data->spi_ctx, spi_cfg)) {
		return 0;
	}

	if (spi_cfg->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	/* Note that SPI_TRANSFER_LSB controls the direction of shift, not the */
	/* "endianness" of the data.  In MSB mode, the high-order bit of the   */
	/* most significant byte is sent first;  in LSB mode, the low-order    */
	/* bit of the least-significant byte is sent first.                    */
	if (spi_cfg->operation & SPI_TRANSFER_LSB) {
		lsb = true;
	}

#if defined(CONFIG_SPI_EXTENDED_MODES)
	if (spi_cfg->operation & (SPI_LINES_DUAL | SPI_LINES_QUAD | SPI_LINES_OCTAL)) {
		LOG_ERR("Unsupported configuration");
		return -ENOTSUP;
	}
#endif /* CONFIG_SPI_EXTENDED_MODES */

	data->bits = SPI_WORD_SIZE_GET(spi_cfg->operation);

	if ((data->bits != 8) && (data->bits != 16) && (data->bits != 32)) {
		LOG_ERR("Only 8, 16, and 32 bit word sizes are supported");
		return -ENOTSUP;
	}

	data->dfs = ((data->bits - 1) / 8) + 1;

	if (spi_cfg->operation & SPI_CS_ACTIVE_HIGH) {
		gpio_set_outover(data->spi_ctx.config->cs.gpio.pin, GPIO_OVERRIDE_INVERT);
	}

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL) {
		cpol = 1;
	}
	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA) {
		cpha = 1;
	}
	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_LOOP) {
		LOG_ERR("Loopback not supported");
		return -ENOTSUP;
	}

#if SPI_RPI_PICO_PIO_HALF_DUPLEX_ENABLED
	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		if ((cpol != 0) || (cpha != 0)) {
			LOG_ERR("Only mode (0, 0) supported in 3-wire SIO");
			return -ENOTSUP;
		}

		if ((spi_cfg->frequency > spi_pico_pio_maximum_clock_frequency(
						  clock_freq, SPI_SIO_MODE_0_0_TX_CYCLES)) ||
		    (spi_cfg->frequency < spi_pico_pio_minimum_clock_frequency(
						  clock_freq, SPI_SIO_MODE_0_0_TX_CYCLES))) {
			LOG_ERR("clock-frequency out of range");
			return -EINVAL;
		}
	} else if (dev_cfg->sio_gpio.port) {
		LOG_ERR("SPI_HALF_DUPLEX operation needed for sio-gpios");
		return -EINVAL;
	}
#else
	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("No sio-gpios defined, half-duplex not enabled");
		return -EINVAL;
	}
#endif /* SPI_RPI_PICO_PIO_HALF_DUPLEX_ENABLED */

	clk = &dev_cfg->clk_gpio;
	data->pio = pio_rpi_pico_get_pio(dev_cfg->piodev);
	rc = pio_rpi_pico_allocate_sm(dev_cfg->piodev, &data->pio_sm);
	if (rc < 0) {
		return rc;
	}

	if (dev_cfg->sio_gpio.port) {
#if SPI_RPI_PICO_PIO_HALF_DUPLEX_ENABLED
		const struct gpio_dt_spec *sio = &dev_cfg->sio_gpio;

		float clock_div = spi_pico_pio_clock_divisor(clock_freq, SPI_SIO_MODE_0_0_TX_CYCLES,
							     spi_cfg->frequency);

		data->pio_tx_offset =
			pio_add_program(data->pio, RPI_PICO_PIO_GET_PROGRAM(spi_sio_mode_0_0_tx));

		data->pio_rx_offset =
			pio_add_program(data->pio, RPI_PICO_PIO_GET_PROGRAM(spi_sio_mode_0_0_rx));
		data->pio_rx_wrap_target =
			data->pio_rx_offset + RPI_PICO_PIO_GET_WRAP_TARGET(spi_sio_mode_0_0_rx);
		data->pio_rx_wrap =
			data->pio_rx_offset + RPI_PICO_PIO_GET_WRAP(spi_sio_mode_0_0_rx);

		sm_config = pio_get_default_sm_config();

		sm_config_set_clkdiv(&sm_config, clock_div);
		sm_config_set_in_pins(&sm_config, sio->pin);
		sm_config_set_in_shift(&sm_config, lsb, true, data->bits);
		sm_config_set_out_pins(&sm_config, sio->pin, 1);
		sm_config_set_out_shift(&sm_config, lsb, false, data->bits);
		hw_set_bits(&data->pio->input_sync_bypass, 1u << sio->pin);

		sm_config_set_sideset_pins(&sm_config, clk->pin);
		sm_config_set_sideset(&sm_config, 1, false, false);
		sm_config_set_wrap(
			&sm_config,
			data->pio_tx_offset + RPI_PICO_PIO_GET_WRAP_TARGET(spi_sio_mode_0_0_tx),
			data->pio_tx_offset + RPI_PICO_PIO_GET_WRAP(spi_sio_mode_0_0_tx));

		pio_sm_set_pindirs_with_mask(data->pio, data->pio_sm,
					     (BIT(clk->pin) | BIT(sio->pin)),
					     (BIT(clk->pin) | BIT(sio->pin)));
		pio_sm_set_pins_with_mask(data->pio, data->pio_sm, 0,
					  BIT(clk->pin) | BIT(sio->pin));
		pio_gpio_init(data->pio, sio->pin);
		pio_gpio_init(data->pio, clk->pin);

		pio_sm_init(data->pio, data->pio_sm, data->pio_tx_offset, &sm_config);
		pio_sm_set_enabled(data->pio, data->pio_sm, true);
#else
		LOG_ERR("SIO pin requires half-duplex support");
		return -EINVAL;
#endif /* SPI_RPI_PICO_PIO_HALF_DUPLEX_ENABLED */
	} else {
		/* 4-wire mode */
		const struct gpio_dt_spec *miso = miso = &dev_cfg->miso_gpio;
		const struct gpio_dt_spec *mosi = &dev_cfg->mosi_gpio;
		const pio_program_t *program;
		uint32_t wrap_target;
		uint32_t wrap;
		int cycles;

		if ((cpol == 0) && (cpha == 0)) {
			program = RPI_PICO_PIO_GET_PROGRAM(spi_mode_0_0);
			wrap_target = RPI_PICO_PIO_GET_WRAP_TARGET(spi_mode_0_0);
			wrap = RPI_PICO_PIO_GET_WRAP(spi_mode_0_0);
			cycles = SPI_MODE_0_0_CYCLES;
		} else if ((cpol == 1) && (cpha == 1)) {
			program = RPI_PICO_PIO_GET_PROGRAM(spi_mode_1_1);
			wrap_target = RPI_PICO_PIO_GET_WRAP_TARGET(spi_mode_1_1);
			wrap = RPI_PICO_PIO_GET_WRAP(spi_mode_1_1);
			cycles = SPI_MODE_1_1_CYCLES;
		} else {
			LOG_ERR("Not supported:  cpol=%d, cpha=%d\n", cpol, cpha);
			return -ENOTSUP;
		}

		if ((spi_cfg->frequency >
		     spi_pico_pio_maximum_clock_frequency(clock_freq, cycles)) ||
		    (spi_cfg->frequency <
		     spi_pico_pio_minimum_clock_frequency(clock_freq, cycles))) {
			LOG_ERR("clock-frequency out of range");
			return -EINVAL;
		}

		float clock_div =
			spi_pico_pio_clock_divisor(clock_freq, cycles, spi_cfg->frequency);

		if (!pio_can_add_program(data->pio, program)) {
			return -EBUSY;
		}

		data->pio_tx_offset = pio_add_program(data->pio, program);
		sm_config = pio_get_default_sm_config();

		sm_config_set_clkdiv(&sm_config, clock_div);
		sm_config_set_in_pins(&sm_config, miso->pin);
		sm_config_set_in_shift(&sm_config, lsb, true, data->bits);
		sm_config_set_out_pins(&sm_config, mosi->pin, 1);
		sm_config_set_out_shift(&sm_config, lsb, true, data->bits);
		sm_config_set_sideset_pins(&sm_config, clk->pin);
		sm_config_set_sideset(&sm_config, 1, false, false);
		sm_config_set_wrap(&sm_config, data->pio_tx_offset + wrap_target,
				   data->pio_tx_offset + wrap);

		pio_sm_set_consecutive_pindirs(data->pio, data->pio_sm, miso->pin, 1, false);
		pio_sm_set_pindirs_with_mask(data->pio, data->pio_sm,
					     (BIT(clk->pin) | BIT(mosi->pin)),
					     (BIT(clk->pin) | BIT(mosi->pin)));
		pio_sm_set_pins_with_mask(data->pio, data->pio_sm, (cpol << clk->pin),
					  BIT(clk->pin) | BIT(mosi->pin));
		pio_gpio_init(data->pio, mosi->pin);
		pio_gpio_init(data->pio, miso->pin);
		pio_gpio_init(data->pio, clk->pin);

		pio_sm_init(data->pio, data->pio_sm, data->pio_tx_offset, &sm_config);
		pio_sm_set_enabled(data->pio, data->pio_sm, true);
	}

	data->spi_ctx.config = spi_cfg;

	return 0;
}

static void spi_pico_pio_txrx_4_wire(const struct device *dev)
{
	struct spi_pico_pio_data *data = dev->data;
	const size_t chunk_len = spi_context_max_continuous_chunk(&data->spi_ctx);
	const uint8_t *txbuf = data->spi_ctx.tx_buf;
	uint8_t *rxbuf = data->spi_ctx.rx_buf;
	uint32_t txrx;
	size_t fifo_cnt = 0;

	data->tx_count = 0;
	data->rx_count = 0;

	pio_sm_clear_fifos(data->pio, data->pio_sm);

	while (data->rx_count < chunk_len || data->tx_count < chunk_len) {
		/* Fill up fifo with available TX data */
		while ((!pio_sm_is_tx_fifo_full(data->pio, data->pio_sm)) &&
		       data->tx_count < chunk_len && fifo_cnt < PIO_FIFO_DEPTH) {
			/* Send 0 in the case of read only operation */
			txrx = 0;

			switch (data->dfs) {
			case 4: {
				if (txbuf) {
					txrx = sys_get_be32(txbuf + (data->tx_count * 4));
				}
				spi_pico_pio_sm_put32(data->pio, data->pio_sm, txrx);
			} break;

			case 2: {
				if (txbuf) {
					txrx = sys_get_be16(txbuf + (data->tx_count * 2));
				}
				spi_pico_pio_sm_put16(data->pio, data->pio_sm, txrx);
			} break;

			case 1: {
				if (txbuf) {
					txrx = ((uint8_t *)txbuf)[data->tx_count];
				}
				spi_pico_pio_sm_put8(data->pio, data->pio_sm, txrx);
			} break;

			default:
				LOG_ERR("Support fot %d bits not enabled", (data->dfs * 8));
				break;
			}
			data->tx_count++;
			fifo_cnt++;
		}

		while ((!pio_sm_is_rx_fifo_empty(data->pio, data->pio_sm)) &&
		       data->rx_count < chunk_len && fifo_cnt > 0) {
			switch (data->dfs) {
			case 4: {
				txrx = spi_pico_pio_sm_get32(data->pio, data->pio_sm);

				/* Discard received data if rx buffer not assigned */
				if (rxbuf) {
					sys_put_be32(txrx, rxbuf + (data->rx_count * 4));
				}
			} break;

			case 2: {
				txrx = spi_pico_pio_sm_get16(data->pio, data->pio_sm);

				/* Discard received data if rx buffer not assigned */
				if (rxbuf) {
					sys_put_be16(txrx, rxbuf + (data->rx_count * 2));
				}
			} break;

			case 1: {
				txrx = spi_pico_pio_sm_get8(data->pio, data->pio_sm);

				/* Discard received data if rx buffer not assigned */
				if (rxbuf) {
					((uint8_t *)rxbuf)[data->rx_count] = (uint8_t)txrx;
				}
			} break;

			default:
				LOG_ERR("Support fot %d bits not enabled", (data->dfs * 8));
				break;
			}
			data->rx_count++;
			fifo_cnt--;
		}
	}
}

static void spi_pico_pio_txrx_3_wire(const struct device *dev)
{
#if SPI_RPI_PICO_PIO_HALF_DUPLEX_ENABLED
	struct spi_pico_pio_data *data = dev->data;
	const struct spi_pico_pio_config *dev_cfg = dev->config;
	const uint8_t *txbuf = data->spi_ctx.tx_buf;
	uint8_t *rxbuf = data->spi_ctx.rx_buf;
	uint32_t txrx;
	int sio_pin = dev_cfg->sio_gpio.pin;
	uint32_t tx_size = data->spi_ctx.tx_len; /* Number of WORDS to send */
	uint32_t rx_size = data->spi_ctx.rx_len; /* Number of WORDS to receive */

	data->tx_count = 0;
	data->rx_count = 0;

	if (txbuf) {
		pio_sm_set_enabled(data->pio, data->pio_sm, false);
		pio_sm_set_wrap(data->pio, data->pio_sm,
				data->pio_tx_offset +
					RPI_PICO_PIO_GET_WRAP_TARGET(spi_sio_mode_0_0_tx),
				data->pio_tx_offset + RPI_PICO_PIO_GET_WRAP(spi_sio_mode_0_0_tx));
		pio_sm_clear_fifos(data->pio, data->pio_sm);
		pio_sm_set_pindirs_with_mask(data->pio, data->pio_sm, BIT(sio_pin), BIT(sio_pin));
		pio_sm_restart(data->pio, data->pio_sm);
		pio_sm_clkdiv_restart(data->pio, data->pio_sm);
		pio_sm_exec(data->pio, data->pio_sm, pio_encode_jmp(data->pio_tx_offset));
		pio_sm_set_enabled(data->pio, data->pio_sm, true);

		while (data->tx_count < tx_size) {
			/* Fill up fifo with available TX data */
			while ((!pio_sm_is_tx_fifo_full(data->pio, data->pio_sm)) &&
			       data->tx_count < tx_size) {

				switch (data->dfs) {
				case 4: {
					txrx = sys_get_be32(txbuf + (data->tx_count * 4));
					spi_pico_pio_sm_put32(data->pio, data->pio_sm, txrx);
				} break;

				case 2: {
					txrx = sys_get_be16(txbuf + (data->tx_count * 2));
					spi_pico_pio_sm_put16(data->pio, data->pio_sm, txrx);
				} break;

				case 1: {
					txrx = ((uint8_t *)txbuf)[data->tx_count];
					spi_pico_pio_sm_put8(data->pio, data->pio_sm, txrx);
				} break;

				default:
					LOG_ERR("Support fot %d bits not enabled", (data->dfs * 8));
					break;
				}
				data->tx_count++;
			}
		}
		/* Wait for the state machine to complete the cycle */
		/* before resetting the PIO for reading.            */
		while ((!pio_sm_is_tx_fifo_empty(data->pio, data->pio_sm)) ||
		       (!spi_pico_pio_sm_complete(data)))
			;
	}

	if (rxbuf) {
		pio_sm_set_enabled(data->pio, data->pio_sm, false);
		pio_sm_set_wrap(data->pio, data->pio_sm, data->pio_rx_wrap_target,
				data->pio_rx_wrap);
		pio_sm_clear_fifos(data->pio, data->pio_sm);
		pio_sm_set_pindirs_with_mask(data->pio, data->pio_sm, 0, BIT(sio_pin));
		pio_sm_restart(data->pio, data->pio_sm);
		pio_sm_clkdiv_restart(data->pio, data->pio_sm);
		pio_sm_put(data->pio, data->pio_sm, (rx_size * data->bits) - 1);
		pio_sm_exec(data->pio, data->pio_sm, pio_encode_jmp(data->pio_rx_offset));
		pio_sm_set_enabled(data->pio, data->pio_sm, true);

		while (data->rx_count < rx_size) {
			while ((!pio_sm_is_rx_fifo_empty(data->pio, data->pio_sm)) &&
			       data->rx_count < rx_size) {

				switch (data->dfs) {
				case 4: {
					txrx = spi_pico_pio_sm_get32(data->pio, data->pio_sm);
					sys_put_be32(txrx, rxbuf + (data->rx_count * 4));
				} break;

				case 2: {
					txrx = spi_pico_pio_sm_get16(data->pio, data->pio_sm);
					sys_put_be16(txrx, rxbuf + (data->rx_count * 2));
				} break;

				case 1: {
					txrx = spi_pico_pio_sm_get8(data->pio, data->pio_sm);
					rxbuf[data->rx_count] = (uint8_t)txrx;
				} break;

				default:
					LOG_ERR("Support fot %d bits not enabled", (data->dfs * 8));
					break;
				}
				data->rx_count++;
			}
		}
	}
#else
	LOG_ERR("SIO pin requires half-duplex support");
#endif /* SPI_RPI_PICO_PIO_HALF_DUPLEX_ENABLED */
}

static void spi_pico_pio_txrx(const struct device *dev)
{
	const struct spi_pico_pio_config *dev_cfg = dev->config;

	/* 3-wire or 4-wire mode? */
	if (dev_cfg->sio_gpio.port) {
		spi_pico_pio_txrx_3_wire(dev);
	} else {
		spi_pico_pio_txrx_4_wire(dev);
	}
}

static int spi_pico_pio_transceive_impl(const struct device *dev, const struct spi_config *spi_cfg,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs, bool asynchronous,
					spi_callback_t cb, void *userdata)
{
	const struct spi_pico_pio_config *dev_cfg = dev->config;
	struct spi_pico_pio_data *data = dev->data;
	struct spi_context *spi_ctx = &data->spi_ctx;
	int rc = 0;

	spi_context_lock(spi_ctx, asynchronous, cb, userdata, spi_cfg);

	rc = spi_pico_pio_configure(dev_cfg, data, spi_cfg);
	if (rc < 0) {
		goto error;
	}

	spi_context_buffers_setup(spi_ctx, tx_bufs, rx_bufs, data->dfs);
	spi_context_cs_control(spi_ctx, true);

	do {
		spi_pico_pio_txrx(dev);
		spi_context_update_tx(spi_ctx, 1, data->tx_count);
		spi_context_update_rx(spi_ctx, 1, data->rx_count);
	} while (spi_pico_pio_transfer_ongoing(data));

	spi_context_cs_control(spi_ctx, false);

error:
	spi_context_release(spi_ctx, rc);

	return rc;
}

static int spi_pico_pio_transceive(const struct device *dev, const struct spi_config *spi_cfg,
				   const struct spi_buf_set *tx_bufs,
				   const struct spi_buf_set *rx_bufs)
{
	return spi_pico_pio_transceive_impl(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

int spi_pico_pio_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_pico_pio_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->spi_ctx);

	return 0;
}

static DEVICE_API(spi, spi_pico_pio_api) = {
	.transceive = spi_pico_pio_transceive,
	.release = spi_pico_pio_release,
};

static int config_gpio(const struct gpio_dt_spec *gpio, const char *tag, int mode)
{
	int rc = 0;

	if (!device_is_ready(gpio->port)) {
		LOG_ERR("GPIO port for %s pin is not ready", tag);
		return -ENODEV;
	}

	rc = gpio_pin_configure_dt(gpio, mode);
	if (rc < 0) {
		LOG_ERR("Couldn't configure %s pin; (%d)", tag, rc);
		return rc;
	}

	return 0;
}

int spi_pico_pio_init(const struct device *dev)
{
	const struct spi_pico_pio_config *dev_cfg = dev->config;
	struct spi_pico_pio_data *data = dev->data;
	int rc;

	rc = pinctrl_apply_state(dev_cfg->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (rc) {
		LOG_ERR("Failed to apply pinctrl state");
		return rc;
	}

	rc = config_gpio(&dev_cfg->clk_gpio, "clk", GPIO_OUTPUT_ACTIVE);
	if (rc < 0) {
		return rc;
	}

	if (dev_cfg->mosi_gpio.port != NULL) {
		rc = config_gpio(&dev_cfg->mosi_gpio, "mosi", GPIO_OUTPUT);
		if (rc < 0) {
			return rc;
		}
	}

	if (dev_cfg->miso_gpio.port != NULL) {
		rc = config_gpio(&dev_cfg->miso_gpio, "miso", GPIO_INPUT);
		if (rc < 0) {
			return rc;
		}
	}

	rc = spi_context_cs_configure_all(&data->spi_ctx);
	if (rc < 0) {
		LOG_ERR("Failed to configure CS pins: %d", rc);
		return rc;
	}

	spi_context_unlock_unconditionally(&data->spi_ctx);

	return 0;
}

#define SPI_PICO_PIO_INIT(inst)                                                                    \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static struct spi_pico_pio_config spi_pico_pio_config_##inst = {                           \
		.piodev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                   \
		.clk_gpio = GPIO_DT_SPEC_INST_GET(inst, clk_gpios),                                \
		.mosi_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mosi_gpios, {0}),                      \
		.miso_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, miso_gpios, {0}),                      \
		.sio_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, sio_gpios, {0}),                        \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                               \
		.clk_id = (clock_control_subsys_t)DT_INST_PHA_BY_IDX(inst, clocks, 0, clk_id),     \
	};                                                                                         \
	static struct spi_pico_pio_data spi_pico_pio_data_##inst = {                               \
		SPI_CONTEXT_INIT_LOCK(spi_pico_pio_data_##inst, spi_ctx),                          \
		SPI_CONTEXT_INIT_SYNC(spi_pico_pio_data_##inst, spi_ctx),                          \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(inst), spi_ctx)};                      \
	DEVICE_DT_INST_DEFINE(inst, spi_pico_pio_init, NULL, &spi_pico_pio_data_##inst,            \
			      &spi_pico_pio_config_##inst, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,  \
			      &spi_pico_pio_api);                                                  \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, clk_gpios), "Missing clock GPIO");                \
	BUILD_ASSERT(((DT_INST_NODE_HAS_PROP(inst, mosi_gpios) ||                                  \
		       DT_INST_NODE_HAS_PROP(inst, miso_gpios)) &&                                 \
		      (!DT_INST_NODE_HAS_PROP(inst, sio_gpios))) ||                                \
			     (DT_INST_NODE_HAS_PROP(inst, sio_gpios) &&                            \
			      !(DT_INST_NODE_HAS_PROP(inst, mosi_gpios) ||                         \
				DT_INST_NODE_HAS_PROP(inst, miso_gpios))),                         \
		     "Invalid GPIO Configuration");

DT_INST_FOREACH_STATUS_OKAY(SPI_PICO_PIO_INIT)
