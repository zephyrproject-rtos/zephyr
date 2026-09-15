/*
 * Copyright (c) 2026 WIZnet Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *
 * RP2350/RP2040 PIO-based MSPI controller driver.
 *
 * Implements MSPI_IO_MODE_QUAD_1_4_4: 1-wire serial instruction byte,
 * 4-wire quad address + dummy + data phases.  Designed for the W6300
 * QSPI register-access protocol.
 *
 * PIO program (32 instructions, fits one PIO program slot):
 *
 *  Each SCLK period is four PIO cycles: two high, then two low.  The extra
 *  resolution exists so the read phase can sample three quarters of the way
 *  through the period rather than at an edge (see phase 3).
 *
 *  Phase 1 – serial instruction (IO0 only, 8 SCLK periods):
 *   0: pull block          side 0        get instruction byte from TX FIFO
 *   1: set pindirs, 1      side 0        IO0 = output
 *   2: out pins, 1         side 0 [1]    bit 7, driven while SCLK is low
 *   3: nop                 side 1 [1]    SCLK high, device samples
 *   ...                                  repeated for bits 6..0
 *
 *  Phase 2 – quad write  (IO0-IO3, x+1 nibbles):
 *  18: set pindirs, 0xF    side 0        IO0-IO3 = outputs
 *  19: pull block          side 0        get x (nibble count - 1) from FIFO
 *  20: out x, 32           side 0
 *  21: pull block          side 0        get y (read nibble count - 1)
 *  22: out y, 32           side 0
 *  23: out pins, 4         side 0 [1]    drive nibble while SCLK is low
 *  24: jmp x--, 23         side 1 [1]    SCLK high, device samples
 *
 *  Phase 3 – quad read   (IO0-IO3, y+1 nibbles):
 *  25: set pins, 0         side 0        clear outputs
 *  26: set pindirs, 0      side 1 [1]    inputs; this edge launches nibble 0
 *  27: nop                 side 0        SCLK low
 *  28: in pins, 4          side 0        sample, 3/4 of a period after the edge
 *  29: jmp y--, 27         side 1 [1]    SCLK high, launches the next nibble
 *
 * The device launches a nibble on the rising edge and holds it until shortly
 * after the following rising edge, so both edges of that window are hostile.
 * Sampling on the falling edge (half a period after the launch) is clean up
 * to 25 MHz but corrupts about one byte in thirty at 33 MHz, where half a
 * period is 15 ns.  Sampling on the next rising edge instead runs into the
 * far end of the window and loses the last nibble of a transfer.  Splitting
 * the period into four PIO cycles puts the sample at 3/4 of a period, roughly
 * centred in the valid window, which is why phase 3 needs the priming edge at
 * instruction 26 to fill the pipeline before the loop starts.
 */

#define DT_DRV_COMPAT raspberrypi_pico_mspi_pio

#define LOG_LEVEL CONFIG_MSPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mspi_rpi_pico_pio);

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/dt-bindings/dma/rpi-pico-dma-common.h>
#include <zephyr/sys/util.h>
#include <hardware/pio.h>
#include <hardware/clocks.h>

#define QSPI_DATA_LINES   4
/*
 * Address and dummy bytes are staged on the stack before the payload, so the
 * controller only accepts requests whose combined header fits here.
 */
#define QSPI_HEADER_BYTES 5
#define QSPI_PIO_CYCLES   4 /* PIO cycles per SCLK period */

/* ------------------------------------------------------------------ */
/* PIO program                                                          */
/* ------------------------------------------------------------------ */

#define QSPI_MIX_WRAP_TARGET 0
#define QSPI_MIX_WRAP        29

/* clang-format off */
RPI_PICO_PIO_DEFINE_PROGRAM(qspi_mix, QSPI_MIX_WRAP_TARGET, QSPI_MIX_WRAP,
	0x80A0, /*  0: pull block          side 0      */
	0xE081, /*  1: set pindirs, 1      side 0      */
	0x6101, /*  2: out pins, 1         side 0 [1] */
	0xB142, /*  3: nop                 side 1 [1] */
	0x6101, /*  4: out pins, 1         side 0 [1] */
	0xB142, /*  5: nop                 side 1 [1] */
	0x6101, /*  6: out pins, 1         side 0 [1] */
	0xB142, /*  7: nop                 side 1 [1] */
	0x6101, /*  8: out pins, 1         side 0 [1] */
	0xB142, /*  9: nop                 side 1 [1] */
	0x6101, /* 10: out pins, 1         side 0 [1] */
	0xB142, /* 11: nop                 side 1 [1] */
	0x6101, /* 12: out pins, 1         side 0 [1] */
	0xB142, /* 13: nop                 side 1 [1] */
	0x6101, /* 14: out pins, 1         side 0 [1] */
	0xB142, /* 15: nop                 side 1 [1] */
	0x6101, /* 16: out pins, 1         side 0 [1] */
	0xB142, /* 17: nop                 side 1 [1] */
	0xE08F, /* 18: set pindirs, 0xF    side 0      */
	0x80A0, /* 19: pull block          side 0      */
	0x6020, /* 20: out x, 32           side 0      */
	0x80A0, /* 21: pull block          side 0      */
	0x6040, /* 22: out y, 32           side 0      */
	0x6104, /* 23: out pins, 4         side 0 [1] */
	0x1157, /* 24: jmp x--, 23         side 1 [1] */
	0xE000, /* 25: set pins, 0         side 0      */
	0xF180, /* 26: set pindirs, 0      side 1 [1] */
	0xA042, /* 27: nop                 side 0      */
	0x4004, /* 28: in pins, 4          side 0      */
	0x119B  /* 29: jmp y--, 27         side 1 [1] */
);
/* clang-format on */

/* ------------------------------------------------------------------ */
/* Driver structs                                                       */
/* ------------------------------------------------------------------ */

struct mspi_rpi_pico_pio_config {
	const struct device *piodev;
	const struct pinctrl_dev_config *pin_cfg;
	struct gpio_dt_spec clk_gpio;
	struct gpio_dt_spec io0_gpio;
	struct gpio_dt_spec io1_gpio;
	struct gpio_dt_spec io2_gpio;
	struct gpio_dt_spec io3_gpio;
	const struct gpio_dt_spec *ce_gpios;
	uint8_t ce_gpios_len;
	const struct device *clk_dev;
	clock_control_subsys_t clk_id;
	const struct device *dma_dev;
	uint8_t dma_tx_channel;
	uint8_t dma_rx_channel;
};

struct mspi_rpi_pico_pio_data {
	struct mspi_dev_cfg dev_cfg;
	PIO pio;
	uint sm;
	uint32_t pio_offset;
	uint32_t io_base_pin;
	uint32_t io_pin_mask;
	struct k_sem lock;
	/* Landing spot for the dummy byte a write-only transfer still clocks in. */
	uint8_t dma_sink;
	bool sm_ready;
};

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

/*
 * These spin on the PIO FIFO without yielding.  The PIO consumes/produces a
 * byte every couple of SCLK periods (tens of ns), which is far shorter than a
 * scheduler round-trip.  Calling k_yield() here would let the TX FIFO drain
 * empty (starving the state machine) and cap throughput at the yield rate
 * instead of the bus rate, so a tight busy-wait is intentional.  Transfers are
 * hardware-bounded and run under the controller lock, so the busy window is
 * just the transfer time.
 */
#define QSPI_SPIN_LIMIT 1000000U

/*
 * Every wait in the transfer path is bounded by an iteration budget rather
 * than a timer read: the controller comes up before the boot banner, so a
 * wedged state machine would otherwise hang the boot with nothing on the
 * console to say why.
 */
static int sm_put_byte(PIO pio, uint sm, uint8_t v)
{
	uint32_t spins = QSPI_SPIN_LIMIT;

	while (pio_sm_is_tx_fifo_full(pio, sm)) {
		if (--spins == 0) {
			return -ETIMEDOUT;
		}
	}

	/* Byte lane write: the value is replicated across the FIFO word. */
	*((io_rw_8 *)&pio->txf[sm]) = v;
	return 0;
}

static int sm_put_word(PIO pio, uint sm, uint32_t v)
{
	uint32_t spins = QSPI_SPIN_LIMIT;

	while (pio_sm_is_tx_fifo_full(pio, sm)) {
		if (--spins == 0) {
			return -ETIMEDOUT;
		}
	}

	pio->txf[sm] = v;
	return 0;
}

/*
 * Direction, data request line and transfer width never change once the state
 * machine is up, so the channels are configured once here.  Each transfer then
 * only has to hand over the new addresses through dma_reload(), which is far
 * cheaper than rebuilding the whole channel configuration: the driver issues
 * around a dozen transfers per Ethernet frame, so this cost is paid often.
 */
static int qspi_dma_configure(const struct mspi_rpi_pico_pio_config *cfg,
			      struct mspi_rpi_pico_pio_data *data, bool is_tx)
{
	struct dma_block_config blk = {0};
	struct dma_config dma_cfg = {0};

	/* The memory side address and the size are supplied per transfer. */
	blk.block_size = 1;

	if (is_tx) {
		blk.dest_address = (uintptr_t)&data->pio->txf[data->sm];
		blk.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	} else {
		blk.source_address = (uintptr_t)&data->pio->rxf[data->sm];
		blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		blk.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	}

	/*
	 * The state machine is handed out at runtime, so the data request line
	 * cannot be fixed in devicetree: derive it from the machine we got.
	 */
	dma_cfg.dma_slot = RPI_PICO_DMA_DREQ_TO_SLOT(pio_get_dreq(data->pio, data->sm, is_tx));
	dma_cfg.source_data_size = 1;
	dma_cfg.dest_data_size = 1;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &blk;

	return dma_config(cfg->dma_dev, is_tx ? cfg->dma_tx_channel : cfg->dma_rx_channel,
			  &dma_cfg);
}

static int setup_sm(const struct mspi_rpi_pico_pio_config *cfg, struct mspi_rpi_pico_pio_data *data)
{
	uint32_t clock_freq;
	float clock_div;
	pio_sm_config sm_cfg;
	int rc;

	rc = clock_control_on(cfg->clk_dev, cfg->clk_id);
	if (rc < 0) {
		return rc;
	}

	rc = clock_control_get_rate(cfg->clk_dev, cfg->clk_id, &clock_freq);
	if (rc < 0) {
		return rc;
	}

	/*
	 * The divider has to land in [1, 65536): a zero frequency would divide
	 * by zero and anything above clk_sys / QSPI_PIO_CYCLES would need a
	 * divider below one.
	 */
	if (data->dev_cfg.freq == 0 || data->dev_cfg.freq > clock_freq / QSPI_PIO_CYCLES) {
		LOG_ERR("%u Hz is not reachable from a %u Hz clock", data->dev_cfg.freq,
			clock_freq);
		return -EINVAL;
	}

	if (data->sm_ready) {
		/* Reconfigure: update clock divider only */
		clock_div = (float)clock_freq / (float)(QSPI_PIO_CYCLES * data->dev_cfg.freq);
		pio_sm_set_enabled(data->pio, data->sm, false);
		pio_sm_set_clkdiv(data->pio, data->sm, clock_div);
		pio_sm_set_enabled(data->pio, data->sm, true);
		return 0;
	}

	rc = pio_rpi_pico_allocate_sm(cfg->piodev, &data->sm);
	if (rc < 0) {
		return rc;
	}

	data->pio = pio_rpi_pico_get_pio(cfg->piodev);

	if (!pio_can_add_program(data->pio, RPI_PICO_PIO_GET_PROGRAM(qspi_mix))) {
		LOG_ERR("No PIO program space");
		return -ENOMEM;
	}
	data->pio_offset = pio_add_program(data->pio, RPI_PICO_PIO_GET_PROGRAM(qspi_mix));

	clock_div = (float)clock_freq / (float)(QSPI_PIO_CYCLES * data->dev_cfg.freq);

	sm_cfg = pio_get_default_sm_config();
	sm_config_set_clkdiv(&sm_cfg, clock_div);
	sm_config_set_out_pins(&sm_cfg, cfg->io0_gpio.pin, QSPI_DATA_LINES);
	sm_config_set_in_pins(&sm_cfg, cfg->io0_gpio.pin);
	sm_config_set_set_pins(&sm_cfg, cfg->io0_gpio.pin, QSPI_DATA_LINES);
	sm_config_set_out_shift(&sm_cfg, false, true, 8);
	sm_config_set_in_shift(&sm_cfg, false, true, 8);
	sm_config_set_sideset_pins(&sm_cfg, cfg->clk_gpio.pin);
	sm_config_set_sideset(&sm_cfg, 1, false, false);
	sm_config_set_wrap(&sm_cfg, data->pio_offset + qspi_mix_wrap_target,
			   data->pio_offset + qspi_mix_wrap);

	data->io_base_pin = cfg->io0_gpio.pin;
	data->io_pin_mask = BIT(cfg->io0_gpio.pin) | BIT(cfg->io1_gpio.pin) |
			    BIT(cfg->io2_gpio.pin) | BIT(cfg->io3_gpio.pin);

	hw_set_bits(&data->pio->input_sync_bypass, data->io_pin_mask);

	pio_sm_set_pindirs_with_mask(data->pio, data->sm,
				     BIT(cfg->clk_gpio.pin) | data->io_pin_mask,
				     BIT(cfg->clk_gpio.pin) | data->io_pin_mask);
	pio_sm_set_pins_with_mask(data->pio, data->sm, 0,
				  BIT(cfg->clk_gpio.pin) | data->io_pin_mask);

	pio_gpio_init(data->pio, cfg->io0_gpio.pin);
	pio_gpio_init(data->pio, cfg->io1_gpio.pin);
	pio_gpio_init(data->pio, cfg->io2_gpio.pin);
	pio_gpio_init(data->pio, cfg->io3_gpio.pin);
	pio_gpio_init(data->pio, cfg->clk_gpio.pin);

	for (uint p = cfg->io0_gpio.pin; p <= cfg->io3_gpio.pin; p++) {
		gpio_set_pulls(p, false, true);
		gpio_set_input_hysteresis_enabled(p, true);
	}
	gpio_pull_down(cfg->clk_gpio.pin);

	pio_sm_init(data->pio, data->sm, data->pio_offset, &sm_cfg);
	pio_sm_exec(data->pio, data->sm, pio_encode_set(pio_pins, 1));

	/* The data request lines depend on the state machine we just got. */
	rc = qspi_dma_configure(cfg, data, true);
	if (rc < 0) {
		return rc;
	}

	rc = qspi_dma_configure(cfg, data, false);
	if (rc < 0) {
		return rc;
	}

	data->sm_ready = true;
	return 0;
}

/* ------------------------------------------------------------------ */
/* PIO transfer                                                         */
/* ------------------------------------------------------------------ */

/*
 * Bound on how long a transfer may take, expressed as poll iterations rather
 * than a timer read.  Unlike the CPU driven path this loop only watches the
 * DMA channel, so its speed has no bearing on whether the transfer is correct;
 * the budget exists purely so a wedged state machine cannot hang the caller.
 */
#define QSPI_DMA_POLL_LIMIT 1000000U

static int pio_do_transfer(const struct mspi_rpi_pico_pio_config *cfg,
			   struct mspi_rpi_pico_pio_data *data, uint8_t cmd_byte, uint8_t addr_len,
			   uint32_t address, size_t dummy_bytes, const uint8_t *tx_data,
			   size_t tx_len, uint8_t *rx_data, size_t rx_len)
{
	PIO pio = data->pio;
	uint sm = data->sm;
	uint32_t offset = data->pio_offset;
	size_t total_quad_tx = addr_len + dummy_bytes + tx_len;
	uint32_t quad_nibbles = total_quad_tx * 2;
	/* A write-only transfer still clocks one byte through the read phase. */
	size_t rx_count = rx_len > 0 ? rx_len : 1;
	uint8_t *rx_dst = rx_len > 0 ? rx_data : &data->dma_sink;
	uint8_t hdr[QSPI_HEADER_BYTES];
	size_t hdr_len = 0;
	uint32_t polls;
	int ret = 0;

	for (int i = (int)addr_len - 1; i >= 0; i--) {
		hdr[hdr_len++] = (uint8_t)(address >> (8 * i));
	}
	for (size_t i = 0; i < dummy_bytes; i++) {
		hdr[hdr_len++] = 0;
	}

	pio_sm_set_enabled(pio, sm, false);
	pio_sm_set_pindirs_with_mask(pio, sm, data->io_pin_mask, data->io_pin_mask);
	pio_sm_clear_fifos(pio, sm);
	pio_sm_restart(pio, sm);
	pio_sm_clkdiv_restart(pio, sm);
	pio_sm_exec(pio, sm, pio_encode_jmp(offset));

	/* Arm the receive side before the machine can produce anything. */
	ret = dma_reload(cfg->dma_dev, cfg->dma_rx_channel, (uintptr_t)&pio->rxf[sm],
			 (uintptr_t)rx_dst, rx_count);
	if (ret < 0) {
		goto out;
	}

	pio_sm_set_enabled(pio, sm, true);

	/*
	 * Priming words: instruction byte, the quad write nibble count and the
	 * read byte count.  The FIFO is four deep and was just cleared, so
	 * these cannot block.
	 */
	ret = sm_put_byte(pio, sm, cmd_byte);
	if (ret < 0) {
		goto out;
	}

	ret = sm_put_word(pio, sm, quad_nibbles > 0 ? quad_nibbles - 1 : 0);
	if (ret < 0) {
		goto out;
	}

	ret = sm_put_word(pio, sm, rx_count * 2 - 1);
	if (ret < 0) {
		goto out;
	}

	/*
	 * Address and dummy bytes go in by hand: there are at most a few of
	 * them, and splicing them onto the caller's payload would mean copying
	 * the payload.
	 */
	for (size_t i = 0; i < hdr_len; i++) {
		ret = sm_put_byte(pio, sm, hdr[i]);
		if (ret < 0) {
			goto out;
		}
	}

	if (tx_len > 0) {
		ret = dma_reload(cfg->dma_dev, cfg->dma_tx_channel, (uintptr_t)tx_data,
				 (uintptr_t)&pio->txf[sm], tx_len);
		if (ret < 0) {
			goto out;
		}
	}

	/*
	 * The read phase is last in the program, so the receive channel
	 * finishing means the whole transfer is done.
	 */
	polls = QSPI_DMA_POLL_LIMIT;
	while (true) {
		struct dma_status stat;

		ret = dma_get_status(cfg->dma_dev, cfg->dma_rx_channel, &stat);
		if (ret < 0) {
			goto out;
		}

		if (!stat.busy) {
			break;
		}

		if (--polls == 0) {
			ret = -ETIMEDOUT;
			goto out;
		}
	}

out:
	if (ret < 0) {
		LOG_ERR("transfer failed %d (cmd 0x%02x, tx %zu, rx %zu)", ret, cmd_byte, tx_len,
			rx_len);
		(void)dma_stop(cfg->dma_dev, cfg->dma_rx_channel);
		if (tx_len > 0) {
			(void)dma_stop(cfg->dma_dev, cfg->dma_tx_channel);
		}
	}

	pio_sm_set_enabled(pio, sm, false);

	while (!pio_sm_is_rx_fifo_empty(pio, sm)) {
		(void)(pio->rxf[sm]);
	}

	pio_sm_set_consecutive_pindirs(pio, sm, data->io_base_pin, 4, false);
	pio_sm_exec(pio, sm, pio_encode_mov(pio_pins, pio_null));

	return ret;
}

/* ------------------------------------------------------------------ */
/* CE helpers                                                           */
/* ------------------------------------------------------------------ */

static void ce_assert(const struct mspi_rpi_pico_pio_config *cfg, uint8_t ce_num)
{
	if (ce_num < cfg->ce_gpios_len) {
		gpio_pin_set_dt(&cfg->ce_gpios[ce_num], 1);
	}
}

static void ce_deassert(const struct mspi_rpi_pico_pio_config *cfg, uint8_t ce_num)
{
	if (ce_num < cfg->ce_gpios_len) {
		gpio_pin_set_dt(&cfg->ce_gpios[ce_num], 0);
	}
}

/* ------------------------------------------------------------------ */
/* MSPI API                                                             */
/* ------------------------------------------------------------------ */

static int api_config(const struct mspi_dt_spec *spec)
{
	ARG_UNUSED(spec);
	return 0;
}

static int api_dev_config(const struct device *controller, const struct mspi_dev_id *dev_id,
			  const enum mspi_dev_cfg_mask param_mask, const struct mspi_dev_cfg *cfg)
{
	struct mspi_rpi_pico_pio_data *data = controller->data;

	ARG_UNUSED(dev_id);
	ARG_UNUSED(param_mask);

	if (cfg->io_mode != MSPI_IO_MODE_QUAD_1_4_4) {
		LOG_ERR("Only MSPI_IO_MODE_QUAD_1_4_4 supported");
		return -ENOTSUP;
	}

	data->dev_cfg = *cfg;

	return setup_sm(controller->config, data);
}

/* In quad mode a clock carries four bits, so two clocks make a byte. */
static inline size_t qspi_dummy_bytes(uint16_t dummy_clocks)
{
	return ((size_t)dummy_clocks + 1) / 2;
}

static int api_get_channel_status(const struct device *controller, uint8_t ch)
{
	ARG_UNUSED(controller);
	ARG_UNUSED(ch);
	return 0;
}

static int api_transceive(const struct device *controller, const struct mspi_dev_id *dev_id,
			  const struct mspi_xfer *req)
{
	const struct mspi_rpi_pico_pio_config *cfg = controller->config;
	struct mspi_rpi_pico_pio_data *data = controller->data;
	int ret = 0;

	if (req->async) {
		return -ENOTSUP;
	}

	if (!data->sm_ready) {
		return -EAGAIN;
	}

	if (req->addr_length > sizeof(uint32_t) ||
	    req->addr_length +
			    MAX(qspi_dummy_bytes(req->tx_dummy), qspi_dummy_bytes(req->rx_dummy)) >
		    QSPI_HEADER_BYTES) {
		LOG_ERR("address (%u) and dummy bytes do not fit the header", req->addr_length);
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	for (uint32_t i = 0; i < req->num_packet; i++) {
		const struct mspi_xfer_packet *pkt = &req->packets[i];

		uint8_t cmd_byte = (uint8_t)(pkt->cmd & 0xFF);

		size_t dummy_bytes =
			qspi_dummy_bytes((pkt->dir == MSPI_RX) ? req->rx_dummy : req->tx_dummy);

		const uint8_t *tx_data = NULL;
		size_t tx_len = 0;
		uint8_t *rx_data = NULL;
		size_t rx_len = 0;

		if (pkt->dir == MSPI_TX) {
			tx_data = pkt->data_buf;
			tx_len = pkt->num_bytes;
		} else {
			rx_data = pkt->data_buf;
			rx_len = pkt->num_bytes;
		}

		if (i == 0) {
			ce_assert(cfg, dev_id->dev_idx);
		}

		ret = pio_do_transfer(cfg, data, cmd_byte, req->addr_length, pkt->address,
				      dummy_bytes, tx_data, tx_len, rx_data, rx_len);

		/* A failed packet also has to release a held chip enable. */
		if (ret < 0 || !req->hold_ce || i == req->num_packet - 1) {
			ce_deassert(cfg, dev_id->dev_idx);
		}

		if (ret < 0) {
			break;
		}
	}

	k_sem_give(&data->lock);
	return ret;
}

static DEVICE_API(mspi, mspi_api) = {
	.config = api_config,
	.dev_config = api_dev_config,
	.get_channel_status = api_get_channel_status,
	.transceive = api_transceive,
};

/* ------------------------------------------------------------------ */
/* Init                                                                 */
/* ------------------------------------------------------------------ */

static int mspi_rpi_pico_pio_init(const struct device *dev)
{
	const struct mspi_rpi_pico_pio_config *cfg = dev->config;
	struct mspi_rpi_pico_pio_data *data = dev->data;
	int rc;

	rc = pinctrl_apply_state(cfg->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		LOG_ERR("pinctrl failed: %d", rc);
		return rc;
	}

	if (!gpio_is_ready_dt(&cfg->clk_gpio) || !gpio_is_ready_dt(&cfg->io0_gpio) ||
	    !gpio_is_ready_dt(&cfg->io1_gpio) || !gpio_is_ready_dt(&cfg->io2_gpio) ||
	    !gpio_is_ready_dt(&cfg->io3_gpio)) {
		return -ENODEV;
	}

	if ((cfg->io1_gpio.pin != cfg->io0_gpio.pin + 1) ||
	    (cfg->io2_gpio.pin != cfg->io0_gpio.pin + 2) ||
	    (cfg->io3_gpio.pin != cfg->io0_gpio.pin + 3)) {
		LOG_ERR("IO0-IO3 must be consecutive GPIO pins");
		return -EINVAL;
	}

	for (uint8_t i = 0; i < cfg->ce_gpios_len; i++) {
		if (!gpio_is_ready_dt(&cfg->ce_gpios[i])) {
			return -ENODEV;
		}
		gpio_pin_configure_dt(&cfg->ce_gpios[i], GPIO_OUTPUT_INACTIVE);
	}

	if (!device_is_ready(cfg->dma_dev)) {
		LOG_ERR("DMA controller not ready");
		return -ENODEV;
	}

	k_sem_init(&data->lock, 1, 1);

	return 0;
}

/* ------------------------------------------------------------------ */
/* Instantiation                                                        */
/* ------------------------------------------------------------------ */

/* clang-format off */
#define FOREACH_CE_GPIOS_ELEM(inst) \
	DT_INST_FOREACH_PROP_ELEM_SEP(inst, ce_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))

#define MSPI_RPI_PICO_PIO_CE_GPIOS(inst) \
	.ce_gpios = (const struct gpio_dt_spec []) { FOREACH_CE_GPIOS_ELEM(inst) }, \
	.ce_gpios_len = DT_INST_PROP_LEN(inst, ce_gpios)

#define MSPI_RPI_PICO_PIO_INIT(n) \
	PINCTRL_DT_INST_DEFINE(n); \
	static struct mspi_rpi_pico_pio_config mspi_cfg_##n = { \
		.piodev  = DEVICE_DT_GET(DT_INST_PARENT(n)), \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n), \
		.clk_gpio = GPIO_DT_SPEC_INST_GET(n, clk_gpios), \
		.io0_gpio = GPIO_DT_SPEC_INST_GET(n, io0_gpios), \
		.io1_gpio = GPIO_DT_SPEC_INST_GET(n, io1_gpios), \
		.io2_gpio = GPIO_DT_SPEC_INST_GET(n, io2_gpios), \
		.io3_gpio = GPIO_DT_SPEC_INST_GET(n, io3_gpios), \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, ce_gpios), \
			   (MSPI_RPI_PICO_PIO_CE_GPIOS(n),)) \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)), \
		.clk_id  = (clock_control_subsys_t) \
			DT_INST_PHA_BY_IDX(n, clocks, 0, clk_id), \
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, tx)), \
		.dma_tx_channel = DT_INST_DMAS_CELL_BY_NAME(n, tx, channel), \
		.dma_rx_channel = DT_INST_DMAS_CELL_BY_NAME(n, rx, channel), \
	}; \
	static struct mspi_rpi_pico_pio_data mspi_data_##n; \
	DEVICE_DT_INST_DEFINE(n, mspi_rpi_pico_pio_init, NULL, \
			      &mspi_data_##n, &mspi_cfg_##n, \
			      POST_KERNEL, CONFIG_MSPI_INIT_PRIORITY, \
			      &mspi_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_RPI_PICO_PIO_INIT)
/* clang-format on */
