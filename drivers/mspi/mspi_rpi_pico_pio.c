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
 * PIO program (30 instructions, fits one PIO program slot):
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
#include <zephyr/drivers/mspi/devicetree.h>
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
#define QSPI_CMD_BYTES    1 /* the instruction phase is one byte on IO0 */
#define QSPI_ADDR_BYTES   4 /* at most, the address is staged from a uint32_t */

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
	const struct device *clk_dev;
	clock_control_subsys_t clk_id;
	const struct device *dma_dev;
	uint8_t dma_tx_channel;
	uint8_t dma_rx_channel;
	/* Boot time controller configuration, from devicetree. */
	struct mspi_cfg mspicfg;
};

struct mspi_rpi_pico_pio_data {
	/* Controller configuration in force, as set through mspi_config(). */
	struct mspi_cfg mspicfg;
	/*
	 * Device holding the controller, from its mspi_dev_config() call until
	 * it hands the controller back with mspi_get_channel_status().
	 */
	const struct mspi_dev_id *dev_id;
	/* Device whose settings are loaded into the state machine. */
	const struct mspi_dev_id *cfg_dev_id;
	struct mspi_dev_cfg dev_cfg;
	/* Taken by the device that holds the controller. */
	struct k_sem cfg_lock;
	/* Serialises transfers against reprogramming the state machine. */
	struct k_sem ctx_lock;
	uint32_t clock_freq;
	PIO pio;
	size_t sm;
	uint32_t pio_offset;
	uint32_t io_base_pin;
	uint32_t io_pin_mask;
	/* Landing spot for the dummy byte a write-only transfer still clocks in. */
	uint8_t dma_sink;
	bool hw_ready;
};

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

/*
 * These spin on the PIO FIFO without yielding.  The PIO consumes/produces a
 * byte every couple of SCLK periods (tens of ns), which is far shorter than a
 * scheduler round-trip.  Calling k_yield() here would let the TX FIFO drain
 * empty (starving the state machine) and cap throughput at the yield rate
 * instead of the bus rate, so a tight busy-wait is intentional.
 */

/*
 * Every wait is bounded by the timeout of the request it serves.  Reading the
 * clock costs more than a poll of the FIFO or the DMA channel, though, and a
 * slower poll notices later that the hardware is done, which adds up over the
 * dozen transfers an Ethernet frame takes.  So the clock is only read once a
 * wait has gone on for a while: the deadline is set after the first batch of
 * polls and checked once per batch from then on.
 */
#define QSPI_POLLS_PER_CLOCK_READ 1024U

struct qspi_deadline {
	k_timeout_t timeout;
	k_timepoint_t end;
	uint32_t polls;
	bool started;
};

static bool qspi_expired(struct qspi_deadline *dl)
{
	if ((++dl->polls % QSPI_POLLS_PER_CLOCK_READ) != 0U) {
		return false;
	}

	if (!dl->started) {
		dl->end = sys_timepoint_calc(dl->timeout);
		dl->started = true;
		return false;
	}

	return sys_timepoint_expired(dl->end);
}

static int sm_put_byte(PIO pio, uint32_t sm, uint8_t v, struct qspi_deadline *dl)
{
	while (pio_sm_is_tx_fifo_full(pio, sm)) {
		if (qspi_expired(dl)) {
			return -ETIMEDOUT;
		}
	}

	/* Byte lane write: the value is replicated across the FIFO word. */
	*((io_rw_8 *)&pio->txf[sm]) = v;
	return 0;
}

static int sm_put_word(PIO pio, uint32_t sm, uint32_t v, struct qspi_deadline *dl)
{
	while (pio_sm_is_tx_fifo_full(pio, sm)) {
		if (qspi_expired(dl)) {
			return -ETIMEDOUT;
		}
	}

	pio->txf[sm] = v;
	return 0;
}

/* In quad mode a clock carries four bits, so two clocks make a byte. */
static inline size_t qspi_dummy_bytes(uint16_t dummy_clocks)
{
	return (size_t)dummy_clocks / 2U;
}

/* Dummy cycles go out as whole bytes staged in the header. */
static inline bool qspi_dummy_ok(uint16_t dummy_clocks)
{
	return (dummy_clocks % 2U) == 0U && qspi_dummy_bytes(dummy_clocks) <= QSPI_HEADER_BYTES;
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

/*
 * The PIO divider is 16.8 fixed point.  Round it up so the clock never comes
 * out above the frequency the device asked for, and reject what 16.8 cannot
 * express instead of letting the SDK truncate it.
 */
static int qspi_clkdiv(uint32_t clock_freq, uint32_t freq, uint32_t *div_int, uint8_t *div_frac8)
{
	uint64_t d256;

	if (freq == 0U) {
		return -EINVAL;
	}

	d256 = DIV_ROUND_UP((uint64_t)clock_freq * 256U, (uint64_t)QSPI_PIO_CYCLES * freq);

	if (d256 < 256U || d256 > 0xFFFFFFU) {
		return -EINVAL;
	}

	*div_int = (uint32_t)(d256 / 256U);
	*div_frac8 = (uint8_t)(d256 % 256U);

	return 0;
}

/* The state machine is stopped between transfers, so this cannot cut one. */
static int qspi_set_freq(struct mspi_rpi_pico_pio_data *data, uint32_t freq)
{
	uint32_t div_int;
	uint8_t div_frac8;
	int rc;

	rc = qspi_clkdiv(data->clock_freq, freq, &div_int, &div_frac8);
	if (rc < 0) {
		return rc;
	}

	pio_sm_set_enabled(data->pio, data->sm, false);
	pio_sm_set_clkdiv_int_frac8(data->pio, data->sm, div_int, div_frac8);

	return 0;
}

/* Claim the state machine, load the program and set up pins and DMA. */
static int qspi_hw_init(const struct mspi_rpi_pico_pio_config *cfg,
			struct mspi_rpi_pico_pio_data *data, uint32_t freq)
{
	uint32_t div_int;
	uint8_t div_frac8;
	pio_sm_config sm_cfg;
	int rc;

	rc = qspi_clkdiv(data->clock_freq, freq, &div_int, &div_frac8);
	if (rc < 0) {
		return rc;
	}

	rc = pio_rpi_pico_allocate_sm(cfg->piodev, &data->sm);
	if (rc < 0) {
		return rc;
	}

	data->pio = pio_rpi_pico_get_pio(cfg->piodev);

	if (!pio_can_add_program(data->pio, RPI_PICO_PIO_GET_PROGRAM(qspi_mix))) {
		LOG_ERR("No PIO program space");
		rc = -ENOMEM;
		goto err_sm;
	}
	data->pio_offset = pio_add_program(data->pio, RPI_PICO_PIO_GET_PROGRAM(qspi_mix));

	sm_cfg = pio_get_default_sm_config();
	sm_config_set_clkdiv_int_frac8(&sm_cfg, div_int, div_frac8);
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

	for (uint32_t p = cfg->io0_gpio.pin; p <= cfg->io3_gpio.pin; p++) {
		gpio_set_pulls(p, false, true);
		gpio_set_input_hysteresis_enabled(p, true);
	}
	gpio_pull_down(cfg->clk_gpio.pin);

	pio_sm_init(data->pio, data->sm, data->pio_offset, &sm_cfg);
	pio_sm_exec(data->pio, data->sm, pio_encode_set(pio_pins, 1));

	/* The data request lines depend on the state machine we just got. */
	rc = qspi_dma_configure(cfg, data, true);
	if (rc < 0) {
		goto err_program;
	}

	rc = qspi_dma_configure(cfg, data, false);
	if (rc < 0) {
		goto err_program;
	}

	data->hw_ready = true;
	return 0;

err_program:
	pio_remove_program(data->pio, RPI_PICO_PIO_GET_PROGRAM(qspi_mix), data->pio_offset);
err_sm:
	pio_sm_unclaim(data->pio, (uint)data->sm);
	return rc;
}

/* ------------------------------------------------------------------ */
/* PIO transfer                                                         */
/* ------------------------------------------------------------------ */

/*
 * dma_stop() only raises the abort bit, so a channel can still be busy when it
 * returns.  A channel left running is still paced by the state machine and
 * would feed the next transfer, so wait for it to retire.  An abort completes
 * within a few bus cycles; the budget only stops a broken channel from hanging
 * the caller after its own timeout has already run out.
 */
#define QSPI_DMA_ABORT_POLL_LIMIT 1000000U

static int qspi_dma_wait_idle(const struct mspi_rpi_pico_pio_config *cfg, uint8_t channel)
{
	struct dma_status stat;
	uint32_t polls = QSPI_DMA_ABORT_POLL_LIMIT;
	int rc;

	while (polls-- > 0U) {
		rc = dma_get_status(cfg->dma_dev, channel, &stat);
		if (rc < 0) {
			return rc;
		}

		if (!stat.busy) {
			return 0;
		}
	}

	return -ETIMEDOUT;
}

static int pio_do_transfer(const struct mspi_rpi_pico_pio_config *cfg,
			   struct mspi_rpi_pico_pio_data *data, uint8_t cmd_byte, uint8_t addr_len,
			   uint32_t address, size_t dummy_bytes, const uint8_t *tx_data,
			   size_t tx_len, uint8_t *rx_data, size_t rx_len, struct qspi_deadline *dl)
{
	PIO pio = data->pio;
	uint32_t sm = data->sm;
	uint32_t offset = data->pio_offset;
	size_t total_quad_tx = addr_len + dummy_bytes + tx_len;
	uint32_t quad_nibbles = total_quad_tx * 2;
	/* A write-only transfer still clocks one byte through the read phase. */
	size_t rx_count = rx_len > 0 ? rx_len : 1;
	uint8_t *rx_dst = rx_len > 0 ? rx_data : &data->dma_sink;
	uint8_t hdr[QSPI_HEADER_BYTES];
	size_t hdr_len = 0;
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
	ret = sm_put_byte(pio, sm, cmd_byte, dl);
	if (ret < 0) {
		goto out;
	}

	ret = sm_put_word(pio, sm, quad_nibbles - 1, dl);
	if (ret < 0) {
		goto out;
	}

	ret = sm_put_word(pio, sm, rx_count * 2 - 1, dl);
	if (ret < 0) {
		goto out;
	}

	/*
	 * Address and dummy bytes go in by hand: there are at most a few of
	 * them, and splicing them onto the caller's payload would mean copying
	 * the payload.
	 */
	for (size_t i = 0; i < hdr_len; i++) {
		ret = sm_put_byte(pio, sm, hdr[i], dl);
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
	while (true) {
		struct dma_status stat;

		ret = dma_get_status(cfg->dma_dev, cfg->dma_rx_channel, &stat);
		if (ret < 0) {
			goto out;
		}

		if (!stat.busy) {
			break;
		}

		if (qspi_expired(dl)) {
			ret = -ETIMEDOUT;
			goto out;
		}
	}

out:
	if (ret < 0) {
		LOG_ERR("transfer failed %d (cmd 0x%02x, tx %zu, rx %zu)", ret, cmd_byte, tx_len,
			rx_len);
		(void)dma_stop(cfg->dma_dev, cfg->dma_rx_channel);
		(void)dma_stop(cfg->dma_dev, cfg->dma_tx_channel);
		(void)qspi_dma_wait_idle(cfg, cfg->dma_rx_channel);
		(void)qspi_dma_wait_idle(cfg, cfg->dma_tx_channel);
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
/* Chip enable                                                          */
/* ------------------------------------------------------------------ */

static int qspi_ce_set(const struct gpio_dt_spec *ce, uint32_t delay_us, bool active)
{
	int rc;

	if (!active && delay_us > 0U) {
		k_busy_wait(delay_us);
	}

	rc = gpio_pin_set_dt(ce, active ? 1 : 0);

	if (rc == 0 && active && delay_us > 0U) {
		k_busy_wait(delay_us);
	}

	return rc;
}

static int qspi_ce_init(const struct mspi_cfg *mcfg)
{
	int rc;

	for (uint32_t i = 0; i < mcfg->num_ce_gpios; i++) {
		if (!gpio_is_ready_dt(&mcfg->ce_group[i])) {
			return -ENODEV;
		}

		rc = gpio_pin_configure_dt(&mcfg->ce_group[i], GPIO_OUTPUT_INACTIVE);
		if (rc < 0) {
			return rc;
		}
	}

	return 0;
}

/* ------------------------------------------------------------------ */
/* Configuration checks                                                 */
/* ------------------------------------------------------------------ */

static int qspi_check_cfg(const struct mspi_rpi_pico_pio_data *data, const struct mspi_cfg *mcfg,
			  uint32_t *max_freq)
{
	uint32_t limit = data->clock_freq / QSPI_PIO_CYCLES;

	if (mcfg->op_mode != MSPI_OP_MODE_CONTROLLER) {
		LOG_ERR("Only controller mode is supported");
		return -ENOTSUP;
	}

	if (mcfg->duplex != MSPI_HALF_DUPLEX) {
		LOG_ERR("The data lines are shared, so only half duplex is supported");
		return -ENOTSUP;
	}

	if (mcfg->dqs_support) {
		LOG_ERR("There is no DQS input");
		return -ENOTSUP;
	}

	/* There is no hardware chip enable, so every peripheral needs a GPIO. */
	if (mcfg->num_periph > mcfg->num_ce_gpios ||
	    (mcfg->num_ce_gpios > 0U && mcfg->ce_group == NULL)) {
		LOG_ERR("Every peripheral needs a chip enable in ce-gpios");
		return -EINVAL;
	}

	/* Moving between peripherals is done in software, in mspi_dev_config(). */
	if (mcfg->num_periph > 1U && !mcfg->sw_multi_periph) {
		LOG_ERR("More than one peripheral needs software-multiperipheral");
		return -ENOTSUP;
	}

	if (mcfg->max_freq > limit) {
		LOG_ERR("%u Hz is above the %u Hz the state machine can clock", mcfg->max_freq,
			limit);
		return -ENOTSUP;
	}

	*max_freq = mcfg->max_freq != 0U ? mcfg->max_freq : limit;

	return 0;
}

static int qspi_verify_device(const struct mspi_rpi_pico_pio_data *data,
			      const struct mspi_dev_id *dev_id)
{
	const struct gpio_dt_spec *ce;

	if (dev_id->dev_idx >= data->mspicfg.num_periph) {
		LOG_ERR("There is no device %u on this controller", dev_id->dev_idx);
		return -ENODEV;
	}

	ce = &data->mspicfg.ce_group[dev_id->dev_idx];
	if (dev_id->ce.port != ce->port || dev_id->ce.pin != ce->pin ||
	    dev_id->ce.dt_flags != ce->dt_flags) {
		LOG_ERR("Device %u does not match its chip enable", dev_id->dev_idx);
		return -ENODEV;
	}

	return 0;
}

/* Check the fields selected by the mask and copy them into @p next. */
static int qspi_check_dev_cfg(const struct mspi_rpi_pico_pio_data *data,
			      const struct mspi_dev_id *dev_id, enum mspi_dev_cfg_mask mask,
			      const struct mspi_dev_cfg *cfg, struct mspi_dev_cfg *next)
{
	const struct gpio_dt_spec *ce = &data->mspicfg.ce_group[dev_id->dev_idx];
	uint32_t div_int;
	uint8_t div_frac8;

	if ((mask & MSPI_DEVICE_CONFIG_CE_NUM) != 0U) {
		/* Chip enables are GPIOs picked by the device index. */
		if (cfg->ce_num != 0U) {
			LOG_ERR("There is no hardware chip enable to select");
			return -ENOTSUP;
		}
		next->ce_num = cfg->ce_num;
	}

	if ((mask & MSPI_DEVICE_CONFIG_FREQUENCY) != 0U) {
		if (cfg->freq > data->mspicfg.max_freq ||
		    qspi_clkdiv(data->clock_freq, cfg->freq, &div_int, &div_frac8) < 0) {
			LOG_ERR("%u Hz is not reachable, the limit is %u Hz", cfg->freq,
				data->mspicfg.max_freq);
			return -ENOTSUP;
		}
		next->freq = cfg->freq;
	}

	if ((mask & MSPI_DEVICE_CONFIG_IO_MODE) != 0U) {
		if (cfg->io_mode != MSPI_IO_MODE_QUAD_1_4_4) {
			LOG_ERR("Only MSPI_IO_MODE_QUAD_1_4_4 is supported");
			return -ENOTSUP;
		}
		next->io_mode = cfg->io_mode;
	}

	if ((mask & MSPI_DEVICE_CONFIG_DATA_RATE) != 0U) {
		if (cfg->data_rate != MSPI_DATA_RATE_SINGLE) {
			LOG_ERR("Only single data rate is supported");
			return -ENOTSUP;
		}
		next->data_rate = cfg->data_rate;
	}

	if ((mask & MSPI_DEVICE_CONFIG_CPP) != 0U) {
		if (cfg->cpp != MSPI_CPP_MODE_0) {
			LOG_ERR("Only clock mode 0 is supported");
			return -ENOTSUP;
		}
		next->cpp = cfg->cpp;
	}

	/*
	 * Data moves a byte at a time in buffer order, so there is no word whose
	 * byte order this could change: both settings give the same bus traffic.
	 */
	if ((mask & MSPI_DEVICE_CONFIG_ENDIAN) != 0U) {
		next->endian = cfg->endian;
	}

	if ((mask & MSPI_DEVICE_CONFIG_CE_POL) != 0U) {
		bool active_low = (ce->dt_flags & GPIO_ACTIVE_LOW) != 0U;

		if ((cfg->ce_polarity == MSPI_CE_ACTIVE_LOW) != active_low) {
			LOG_ERR("The chip enable polarity disagrees with ce-gpios");
			return -EINVAL;
		}
		next->ce_polarity = cfg->ce_polarity;
	}

	if ((mask & MSPI_DEVICE_CONFIG_DQS) != 0U) {
		if (cfg->dqs_enable) {
			LOG_ERR("There is no DQS input");
			return -ENOTSUP;
		}
		next->dqs_enable = cfg->dqs_enable;
	}

	if ((mask & MSPI_DEVICE_CONFIG_RX_DUMMY) != 0U) {
		if (!qspi_dummy_ok(cfg->rx_dummy)) {
			LOG_ERR("%u RX dummy cycles do not make whole header bytes", cfg->rx_dummy);
			return -ENOTSUP;
		}
		next->rx_dummy = cfg->rx_dummy;
	}

	if ((mask & MSPI_DEVICE_CONFIG_TX_DUMMY) != 0U) {
		if (!qspi_dummy_ok(cfg->tx_dummy)) {
			LOG_ERR("%u TX dummy cycles do not make whole header bytes", cfg->tx_dummy);
			return -ENOTSUP;
		}
		next->tx_dummy = cfg->tx_dummy;
	}

	if ((mask & MSPI_DEVICE_CONFIG_READ_CMD) != 0U) {
		if (cfg->read_cmd > UINT8_MAX) {
			LOG_ERR("The instruction is one byte");
			return -ENOTSUP;
		}
		next->read_cmd = cfg->read_cmd;
	}

	if ((mask & MSPI_DEVICE_CONFIG_WRITE_CMD) != 0U) {
		if (cfg->write_cmd > UINT8_MAX) {
			LOG_ERR("The instruction is one byte");
			return -ENOTSUP;
		}
		next->write_cmd = cfg->write_cmd;
	}

	if ((mask & MSPI_DEVICE_CONFIG_CMD_LEN) != 0U) {
		if (cfg->cmd_length > QSPI_CMD_BYTES) {
			LOG_ERR("The instruction is one byte");
			return -ENOTSUP;
		}
		next->cmd_length = cfg->cmd_length;
	}

	if ((mask & MSPI_DEVICE_CONFIG_ADDR_LEN) != 0U) {
		if (cfg->addr_length > QSPI_ADDR_BYTES) {
			LOG_ERR("The address is at most %u bytes", QSPI_ADDR_BYTES);
			return -ENOTSUP;
		}
		next->addr_length = cfg->addr_length;
	}

	if ((mask & MSPI_DEVICE_CONFIG_MEM_BOUND) != 0U) {
		if (cfg->mem_boundary != 0U) {
			LOG_ERR("Splitting transfers at a memory boundary is not supported");
			return -ENOTSUP;
		}
		next->mem_boundary = cfg->mem_boundary;
	}

	if ((mask & MSPI_DEVICE_CONFIG_BREAK_TIME) != 0U) {
		if (cfg->time_to_break != 0U) {
			LOG_ERR("Splitting transfers after a time is not supported");
			return -ENOTSUP;
		}
		next->time_to_break = cfg->time_to_break;
	}

	return 0;
}

static int qspi_check_xfer(const struct mspi_xfer *req)
{
	if (req->async) {
		LOG_ERR("Only synchronous transfers are supported");
		return -ENOTSUP;
	}

	if (req->xfer_mode != MSPI_DMA) {
		LOG_ERR("The state machine FIFOs are always moved by DMA");
		return -ENOTSUP;
	}

	if (req->cmd_length != QSPI_CMD_BYTES) {
		LOG_ERR("The instruction phase is fixed at one byte");
		return -ENOTSUP;
	}

	if (req->addr_length > QSPI_ADDR_BYTES || !qspi_dummy_ok(req->tx_dummy) ||
	    !qspi_dummy_ok(req->rx_dummy) ||
	    req->addr_length +
			    MAX(qspi_dummy_bytes(req->tx_dummy), qspi_dummy_bytes(req->rx_dummy)) >
		    QSPI_HEADER_BYTES) {
		LOG_ERR("Address (%u) and dummy cycles (%u/%u) do not fit the header",
			req->addr_length, req->tx_dummy, req->rx_dummy);
		return -ENOTSUP;
	}

	if (req->packets == NULL || req->num_packet == 0U) {
		return -EINVAL;
	}

	for (uint32_t i = 0; i < req->num_packet; i++) {
		const struct mspi_xfer_packet *pkt = &req->packets[i];
		size_t dummy =
			qspi_dummy_bytes(pkt->dir == MSPI_RX ? req->rx_dummy : req->tx_dummy);

		if ((pkt->dir != MSPI_RX && pkt->dir != MSPI_TX) || pkt->cmd > UINT8_MAX ||
		    (pkt->num_bytes > 0U && pkt->data_buf == NULL)) {
			return -EINVAL;
		}

		/*
		 * The quad write loop is a do-while in the state machine, so it
		 * always emits one nibble.  A packet with nothing to write would
		 * stall it.
		 */
		if (req->addr_length + dummy + (pkt->dir == MSPI_TX ? pkt->num_bytes : 0U) == 0U) {
			LOG_ERR("A packet needs an address, dummy cycles or data to write");
			return -EINVAL;
		}
	}

	return 0;
}

/* ------------------------------------------------------------------ */
/* MSPI API                                                             */
/* ------------------------------------------------------------------ */

/*
 * Validates the controller configuration and programs the state machine for
 * it.  The first call, from init, claims the PIO and DMA resources.  A later
 * call has to set re_init and waits for the bus to be free; every device then
 * has to load its settings again.
 */
static int api_config(const struct mspi_dt_spec *spec)
{
	const struct mspi_rpi_pico_pio_config *cfg = spec->bus->config;
	struct mspi_rpi_pico_pio_data *data = spec->bus->data;
	const struct mspi_cfg *mcfg = &spec->config;
	bool reinit = data->hw_ready;
	uint32_t max_freq;
	int rc;

	rc = qspi_check_cfg(data, mcfg, &max_freq);
	if (rc < 0) {
		return rc;
	}

	if (reinit) {
		if (!mcfg->re_init) {
			LOG_ERR("The controller is initialised already, set re_init");
			return -EALREADY;
		}

		if (k_sem_take(&data->cfg_lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE)) !=
		    0) {
			LOG_ERR("The controller is held by a device");
			return -EBUSY;
		}
	}

	(void)k_sem_take(&data->ctx_lock, K_FOREVER);

	rc = qspi_ce_init(mcfg);
	if (rc == 0) {
		rc = reinit ? qspi_set_freq(data, max_freq) : qspi_hw_init(cfg, data, max_freq);
	}

	if (rc == 0) {
		data->mspicfg = *mcfg;
		data->mspicfg.max_freq = max_freq;
		data->cfg_dev_id = NULL;
	}

	k_sem_give(&data->ctx_lock);

	if (reinit) {
		k_sem_give(&data->cfg_lock);
	}

	return rc;
}

/*
 * Takes the controller for a device and loads the settings selected by the
 * mask.  The device holds the controller until it calls
 * mspi_get_channel_status().  Anything short of a complete configuration
 * builds on the settings loaded, so it is only taken from the device whose
 * settings those are; MSPI_DEVICE_CONFIG_NONE then just takes the controller.
 */
static int api_dev_config(const struct device *controller, const struct mspi_dev_id *dev_id,
			  const enum mspi_dev_cfg_mask param_mask, const struct mspi_dev_cfg *cfg)
{
	struct mspi_rpi_pico_pio_data *data = controller->data;
	struct mspi_dev_cfg next;
	bool acquired = false;
	int rc;

	if (dev_id == NULL || (param_mask & ~MSPI_DEVICE_CONFIG_ALL) != 0U ||
	    (param_mask != MSPI_DEVICE_CONFIG_NONE && cfg == NULL)) {
		return -EINVAL;
	}

	if (dev_id != data->dev_id) {
		if (k_sem_take(&data->cfg_lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE)) !=
		    0) {
			LOG_ERR("The controller is held by another device");
			return -EBUSY;
		}
		acquired = true;

		rc = qspi_verify_device(data, dev_id);
		if (rc < 0) {
			goto out;
		}
	}

	if (param_mask != MSPI_DEVICE_CONFIG_ALL && data->cfg_dev_id != dev_id) {
		LOG_ERR("Device %u has to load a complete configuration first", dev_id->dev_idx);
		rc = -ENOTSUP;
		goto out;
	}

	if (param_mask != MSPI_DEVICE_CONFIG_NONE) {
		next = data->dev_cfg;
		rc = qspi_check_dev_cfg(data, dev_id, param_mask, cfg, &next);
		if (rc < 0) {
			goto out;
		}

		/* Reprogramming the clock cannot overlap a transfer. */
		(void)k_sem_take(&data->ctx_lock, K_FOREVER);
		rc = qspi_set_freq(data, next.freq);
		if (rc == 0) {
			data->dev_cfg = next;
			data->cfg_dev_id = dev_id;
		}
		k_sem_give(&data->ctx_lock);

		if (rc < 0) {
			goto out;
		}
	}

	data->dev_id = dev_id;
	return 0;

out:
	/* A failed call leaves the controller held, or not, as it found it. */
	if (acquired) {
		k_sem_give(&data->cfg_lock);
	}

	return rc;
}

/* Hands the controller back from the device that took it. */
static int api_get_channel_status(const struct device *controller, uint8_t ch)
{
	struct mspi_rpi_pico_pio_data *data = controller->data;

	/* There is one channel per controller instance. */
	ARG_UNUSED(ch);

	/* A transfer still in flight finishes before the controller changes hands. */
	(void)k_sem_take(&data->ctx_lock, K_FOREVER);

	if (data->dev_id != NULL) {
		data->dev_id = NULL;
		k_sem_give(&data->cfg_lock);
	}

	k_sem_give(&data->ctx_lock);

	return 0;
}

static int api_transceive(const struct device *controller, const struct mspi_dev_id *dev_id,
			  const struct mspi_xfer *req)
{
	const struct mspi_rpi_pico_pio_config *cfg = controller->config;
	struct mspi_rpi_pico_pio_data *data = controller->data;
	const struct gpio_dt_spec *ce;
	uint32_t ce_delay = 0;
	struct qspi_deadline dl = {
		.timeout = K_MSEC(req->timeout),
	};
	int ret;

	if (dev_id == NULL || dev_id != data->dev_id) {
		LOG_ERR("The controller is not configured for this device");
		return -ESTALE;
	}

	ret = qspi_check_xfer(req);
	if (ret < 0) {
		return ret;
	}

	if (k_sem_take(&data->ctx_lock, dl.timeout) != 0) {
		return -EBUSY;
	}

	/* A software chip enable from the request replaces the device's own. */
	if (req->ce_sw_ctrl.gpio.port != NULL) {
		ce = &req->ce_sw_ctrl.gpio;
		ce_delay = req->ce_sw_ctrl.delay;
	} else {
		ce = &data->mspicfg.ce_group[dev_id->dev_idx];
	}

	for (uint32_t i = 0; i < req->num_packet; i++) {
		const struct mspi_xfer_packet *pkt = &req->packets[i];
		size_t dummy_bytes =
			qspi_dummy_bytes(pkt->dir == MSPI_RX ? req->rx_dummy : req->tx_dummy);
		const uint8_t *tx_data = NULL;
		size_t tx_len = 0;
		uint8_t *rx_data = NULL;
		size_t rx_len = 0;
		int rc;

		if (pkt->dir == MSPI_TX) {
			tx_data = pkt->data_buf;
			tx_len = pkt->num_bytes;
		} else {
			rx_data = pkt->data_buf;
			rx_len = pkt->num_bytes;
		}

		/*
		 * A held chip enable frames the whole request and stays
		 * asserted after it, which is what callers that split a
		 * transaction over several calls rely on.  Otherwise each
		 * packet is framed on its own.
		 */
		if (i == 0 || !req->hold_ce) {
			ret = qspi_ce_set(ce, ce_delay, true);
			if (ret < 0) {
				break;
			}
		}

		ret = pio_do_transfer(cfg, data, (uint8_t)pkt->cmd, req->addr_length, pkt->address,
				      dummy_bytes, tx_data, tx_len, rx_data, rx_len, &dl);

		/* A failed packet also has to release a held chip enable. */
		if (ret < 0 || !req->hold_ce) {
			rc = qspi_ce_set(ce, ce_delay, false);
			if (ret == 0) {
				ret = rc;
			}
		}

		if (ret < 0) {
			break;
		}
	}

	k_sem_give(&data->ctx_lock);
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
	const struct mspi_dt_spec spec = {
		.bus = dev,
		.config = cfg->mspicfg,
	};
	int rc;

	k_sem_init(&data->cfg_lock, 1, 1);
	k_sem_init(&data->ctx_lock, 1, 1);

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

	if (!device_is_ready(cfg->dma_dev)) {
		LOG_ERR("DMA controller not ready");
		return -ENODEV;
	}

	rc = clock_control_on(cfg->clk_dev, cfg->clk_id);
	if (rc < 0) {
		return rc;
	}

	rc = clock_control_get_rate(cfg->clk_dev, cfg->clk_id, &data->clock_freq);
	if (rc < 0) {
		return rc;
	}

	return api_config(&spec);
}

/* ------------------------------------------------------------------ */
/* Instantiation                                                        */
/* ------------------------------------------------------------------ */

/* clang-format off */
#define MSPI_RPI_PICO_PIO_MSPI_CFG(n) \
	{ \
		.channel_num = 0, \
		.op_mode = DT_INST_ENUM_IDX_OR(n, op_mode, MSPI_OP_MODE_CONTROLLER), \
		.duplex = DT_INST_ENUM_IDX_OR(n, duplex, MSPI_HALF_DUPLEX), \
		.dqs_support = DT_INST_PROP(n, dqs_support), \
		.sw_multi_periph = DT_INST_PROP(n, software_multiperipheral), \
		.ce_group = mspi_ce_gpios_##n, \
		.num_ce_gpios = ARRAY_SIZE(mspi_ce_gpios_##n), \
		.num_periph = DT_INST_CHILD_NUM(n), \
		.max_freq = DT_INST_PROP_OR(n, clock_frequency, 0), \
		.re_init = false, \
	}

#define MSPI_RPI_PICO_PIO_INIT(n) \
	PINCTRL_DT_INST_DEFINE(n); \
	static struct gpio_dt_spec mspi_ce_gpios_##n[] = MSPI_CE_GPIOS_DT_SPEC_INST_GET(n); \
	static const struct mspi_rpi_pico_pio_config mspi_cfg_##n = { \
		.piodev  = DEVICE_DT_GET(DT_INST_PARENT(n)), \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n), \
		.clk_gpio = GPIO_DT_SPEC_INST_GET(n, clk_gpios), \
		.io0_gpio = GPIO_DT_SPEC_INST_GET(n, io0_gpios), \
		.io1_gpio = GPIO_DT_SPEC_INST_GET(n, io1_gpios), \
		.io2_gpio = GPIO_DT_SPEC_INST_GET(n, io2_gpios), \
		.io3_gpio = GPIO_DT_SPEC_INST_GET(n, io3_gpios), \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)), \
		.clk_id  = (clock_control_subsys_t) \
			DT_INST_PHA_BY_IDX(n, clocks, 0, clk_id), \
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, tx)), \
		.dma_tx_channel = DT_INST_DMAS_CELL_BY_NAME(n, tx, channel), \
		.dma_rx_channel = DT_INST_DMAS_CELL_BY_NAME(n, rx, channel), \
		.mspicfg = MSPI_RPI_PICO_PIO_MSPI_CFG(n), \
	}; \
	static struct mspi_rpi_pico_pio_data mspi_data_##n; \
	DEVICE_DT_INST_DEFINE(n, mspi_rpi_pico_pio_init, NULL, \
			      &mspi_data_##n, &mspi_cfg_##n, \
			      POST_KERNEL, CONFIG_MSPI_INIT_PRIORITY, \
			      &mspi_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_RPI_PICO_PIO_INIT)
/* clang-format on */
