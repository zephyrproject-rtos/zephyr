/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_mspi_bitbang_controller

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(mspi_bitbang, CONFIG_MSPI_LOG_LEVEL);

#define MSPI_BITBANG_MAX_IO       16U
#define MSPI_BITBANG_MAX_CMD_LEN  sizeof(uint32_t)
#define MSPI_BITBANG_MAX_ADDR_LEN sizeof(uint32_t)

/*
 * Number of bytes one clock cycle carries in a phase, at least one. A phase
 * length must be a multiple of it.
 */
#define MSPI_BITBANG_UNIT_BYTES(lines, ddr) MAX(1U, ((lines) * ((ddr) ? 2U : 1U)) / BITS_PER_BYTE)

enum mspi_bitbang_phase_id {
	MSPI_BITBANG_PH_CMD,
	MSPI_BITBANG_PH_ADDR,
	MSPI_BITBANG_PH_DATA,
	MSPI_BITBANG_PHASES,
};

/* How one phase of a transfer is clocked. */
struct mspi_bitbang_phase {
	/* data lines used */
	uint8_t lines;
	/* bytes per clock cycle, at least one; phase lengths are multiples of it */
	uint8_t unit;
	/* dual data rate */
	bool ddr;
};

struct mspi_bitbang_config {
	struct gpio_dt_spec sck;
	const struct gpio_dt_spec *io;
	uint8_t num_io;
	/* One stored configuration per peripheral, indexed by dev_idx. */
	struct mspi_dev_cfg *dev_cfgs;
	uint8_t max_periph;
	struct mspi_cfg mspicfg;
};

struct mspi_bitbang_data {
	struct k_mutex lock;
	/* Peripheral that holds the controller and the thread that took it. */
	const struct mspi_dev_id *dev_id;
	k_tid_t owner;
	uint32_t max_freq;
	uint32_t num_periph;
	uint32_t quarter_period_us;
	int sck_active;
	int sck_idle;
	bool cpha;
	bool lsb_first;
	struct mspi_bitbang_phase phase[MSPI_BITBANG_PHASES];
	/* IO0 .. IO(num_out - 1) are outputs, the other data lines inputs. */
	uint8_t num_out;
};

/* Wait for a number of quarter clock periods. */
static inline void mspi_bitbang_delay(const struct mspi_bitbang_data *data, uint32_t quarters)
{
	k_busy_wait(quarters * data->quarter_period_us);
}

/*
 * Drive the data lines from bits 0 .. lines - 1 of val.
 *
 * Bit k goes to IOk, so a single-line phase drives IO0, the bus output line
 * (MOSI), and multi-line phases drive IO0 .. IO(lines - 1).
 */
static inline void mspi_bitbang_set_lines(const struct mspi_bitbang_config *cfg, uint8_t lines,
					  uint32_t val)
{
	for (uint8_t k = 0U; k < lines; k++) {
		(void)gpio_pin_set_dt(&cfg->io[k], (val >> k) & 0x1U);
	}
}

/*
 * Sample the data lines into bits 0 .. lines - 1 of the result.
 *
 * Multi-line phases use IO0 .. IO(lines - 1). A single-line phase uses the
 * bus input line: that is IO1 on a bus with separate output and input lines
 * (MOSI/MISO), and IO0 on a bus with only one data line.
 */
static inline uint32_t mspi_bitbang_get_lines(const struct mspi_bitbang_config *cfg, uint8_t lines)
{
	uint32_t val = 0U;

	if (lines == 1U) {
		uint8_t in = (cfg->num_io > 1U) ? 1U : 0U;

		return (gpio_pin_get_dt(&cfg->io[in]) > 0) ? 0x1U : 0U;
	}

	for (uint8_t k = 0U; k < lines; k++) {
		if (gpio_pin_get_dt(&cfg->io[k]) > 0) {
			val |= BIT(k);
		}
	}

	return val;
}

/*
 * One single data rate clock cycle. The output is set before the leading edge
 * for CPHA = 0 and after it for CPHA = 1. The input is sampled immediately
 * before the sampling edge (leading for CPHA = 0, trailing for CPHA = 1): the
 * device has driven it for half a period by then, and a device that releases
 * the line on that edge cannot be read too late.
 */
static uint32_t mspi_bitbang_clock_sdr(const struct device *dev, uint32_t out, bool drive,
				       bool read, uint8_t lines)
{
	const struct mspi_bitbang_config *cfg = dev->config;
	const struct mspi_bitbang_data *data = dev->data;
	int active = data->sck_active;
	int idle = data->sck_idle;
	uint32_t in = 0U;

	if (data->cpha) {
		(void)gpio_pin_set_dt(&cfg->sck, active);
		if (drive) {
			mspi_bitbang_set_lines(cfg, lines, out);
		}
		mspi_bitbang_delay(data, 2U);
		if (read) {
			in = mspi_bitbang_get_lines(cfg, lines);
		}
		(void)gpio_pin_set_dt(&cfg->sck, idle);
		mspi_bitbang_delay(data, 2U);
	} else {
		if (drive) {
			mspi_bitbang_set_lines(cfg, lines, out);
		}
		mspi_bitbang_delay(data, 2U);
		if (read) {
			in = mspi_bitbang_get_lines(cfg, lines);
		}
		(void)gpio_pin_set_dt(&cfg->sck, active);
		mspi_bitbang_delay(data, 2U);
		(void)gpio_pin_set_dt(&cfg->sck, idle);
	}

	return in;
}

/*
 * One dual data rate clock cycle carrying two transfer instants. The output
 * changes a quarter period before each edge so it is centered on the edge
 * that samples it; the input is sampled a quarter period after each edge,
 * where a device that launches its data on the edge has settled. CPHA has no
 * meaning here.
 */
static void mspi_bitbang_clock_ddr(const struct device *dev, uint32_t out_a, uint32_t out_b,
				   bool drive, bool read, uint8_t lines, uint32_t *in_a,
				   uint32_t *in_b)
{
	const struct mspi_bitbang_config *cfg = dev->config;
	const struct mspi_bitbang_data *data = dev->data;
	int active = data->sck_active;
	int idle = data->sck_idle;

	if (drive) {
		mspi_bitbang_set_lines(cfg, lines, out_a);
	}
	mspi_bitbang_delay(data, 1U);
	(void)gpio_pin_set_dt(&cfg->sck, active);
	mspi_bitbang_delay(data, 1U);
	if (read) {
		*in_a = mspi_bitbang_get_lines(cfg, lines);
	}
	if (drive) {
		mspi_bitbang_set_lines(cfg, lines, out_b);
	}
	mspi_bitbang_delay(data, 1U);
	(void)gpio_pin_set_dt(&cfg->sck, idle);
	mspi_bitbang_delay(data, 1U);
	if (read) {
		*in_b = mspi_bitbang_get_lines(cfg, lines);
	}
}

/*
 * Transfer one unit of unit_bytes bytes over lines data lines. Within the
 * unit the bytes form one value, first byte in the most significant position
 * for MSB first and in the least significant one for LSB first; each
 * transfer instant takes the next lines bits of that value in the same order,
 * bit k of an instant on IOk.
 */
static void mspi_bitbang_xfer_unit(const struct device *dev, const uint8_t *out, uint8_t *in,
				   const struct mspi_bitbang_phase *ph)
{
	const struct mspi_bitbang_data *data = dev->data;
	uint8_t lines = ph->lines;
	bool ddr = ph->ddr;
	uint8_t unit_bytes = ph->unit;
	uint8_t unit_bits = 8U * unit_bytes;
	uint8_t instants = unit_bits / lines;
	uint32_t mask = BIT_MASK(lines);
	uint32_t out_val = 0U;
	uint32_t in_val = 0U;

	if (out != NULL) {
		for (uint8_t b = 0U; b < unit_bytes; b++) {
			uint8_t shift = data->lsb_first ? (8U * b) : (unit_bits - 8U * (b + 1U));

			out_val |= (uint32_t)out[b] << shift;
		}
	}

	for (uint8_t i = 0U; i < instants; i += ddr ? 2U : 1U) {
		uint8_t shift_a = data->lsb_first ? (i * lines) : (unit_bits - lines * (i + 1U));
		uint32_t in_a = 0U;

		if (ddr) {
			uint8_t shift_b = data->lsb_first ? ((i + 1U) * lines)
							  : (unit_bits - lines * (i + 2U));
			uint32_t in_b = 0U;

			mspi_bitbang_clock_ddr(dev, (out_val >> shift_a) & mask,
					       (out_val >> shift_b) & mask, out != NULL, in != NULL,
					       lines, &in_a, &in_b);
			in_val |= (in_a << shift_a) | (in_b << shift_b);
		} else {
			in_a = mspi_bitbang_clock_sdr(dev, (out_val >> shift_a) & mask, out != NULL,
						      in != NULL, lines);
			in_val |= in_a << shift_a;
		}
	}

	if (in != NULL) {
		for (uint8_t b = 0U; b < unit_bytes; b++) {
			uint8_t shift = data->lsb_first ? (8U * b) : (unit_bits - 8U * (b + 1U));

			in[b] = (uint8_t)(in_val >> shift);
		}
	}
}

/* Transfer len bytes, a multiple of the phase unit, in one direction. */
static void mspi_bitbang_xfer_bytes(const struct device *dev, const uint8_t *out, uint8_t *in,
				    uint32_t len, const struct mspi_bitbang_phase *ph)
{
	for (uint32_t i = 0U; i < len; i += ph->unit) {
		mspi_bitbang_xfer_unit(dev, (out != NULL) ? &out[i] : NULL,
				       (in != NULL) ? &in[i] : NULL, ph);
	}
}

static void mspi_bitbang_dummy_cycles(const struct device *dev, uint16_t cycles)
{
	for (uint16_t i = 0U; i < cycles; i++) {
		(void)mspi_bitbang_clock_sdr(dev, 0U, false, false, 1U);
	}
}

/*
 * Make IO0 .. IO(num_out - 1) outputs and the other data lines inputs,
 * reconfiguring only the lines whose direction changes.
 */
static void mspi_bitbang_set_outputs(const struct device *dev, uint8_t num_out)
{
	const struct mspi_bitbang_config *cfg = dev->config;
	struct mspi_bitbang_data *data = dev->data;
	uint8_t lo = MIN(data->num_out, num_out);
	uint8_t hi = MAX(data->num_out, num_out);

	for (uint8_t k = lo; k < hi; k++) {
		(void)gpio_pin_configure_dt(&cfg->io[k],
					    (k < num_out) ? GPIO_OUTPUT_INACTIVE : GPIO_INPUT);
	}
	data->num_out = num_out;
}

static void mspi_bitbang_packet(const struct device *dev, const struct mspi_dev_id *dev_id,
				const struct mspi_xfer *xfer, const struct mspi_xfer_packet *pkt)
{
	const struct mspi_bitbang_config *cfg = dev->config;
	const struct mspi_bitbang_data *data = dev->data;
	const struct mspi_bitbang_phase *cmd = &data->phase[MSPI_BITBANG_PH_CMD];
	const struct mspi_bitbang_phase *addr = &data->phase[MSPI_BITBANG_PH_ADDR];
	const struct mspi_bitbang_phase *dat = &data->phase[MSPI_BITBANG_PH_DATA];
	/* the CE line named in ce_sw_ctrl when set, otherwise the device's own */
	const struct gpio_dt_spec *ce =
		(xfer->ce_sw_ctrl.gpio.port != NULL) ? &xfer->ce_sw_ctrl.gpio : &dev_id->ce;
	uint16_t dummy = (pkt->dir == MSPI_RX) ? xfer->rx_dummy : xfer->tx_dummy;
	uint8_t bytes[sizeof(uint32_t)];

	if (ce->port != NULL) {
		(void)gpio_pin_set_dt(ce, 1);
	}

	if (xfer->ce_sw_ctrl.delay > 0U) {
		k_busy_wait(xfer->ce_sw_ctrl.delay);
	}

	mspi_bitbang_delay(data, 2U);

	if (xfer->cmd_length > 0U) {
		mspi_bitbang_set_outputs(dev, cmd->lines);
		sys_put_be32(pkt->cmd, bytes);
		mspi_bitbang_xfer_bytes(dev, &bytes[sizeof(bytes) - xfer->cmd_length], NULL,
					xfer->cmd_length, cmd);
	}

	if (xfer->addr_length > 0U) {
		mspi_bitbang_set_outputs(dev, addr->lines);
		sys_put_be32(pkt->address, bytes);
		mspi_bitbang_xfer_bytes(dev, &bytes[sizeof(bytes) - xfer->addr_length], NULL,
					xfer->addr_length, addr);
	}

	if (pkt->dir == MSPI_RX) {
		/*
		 * Drive the data lines high through all but the last dummy
		 * cycle, then release them before the device starts driving.
		 * Devices that read continuous-read mode bits during the dummy
		 * cycles then see an all-ones mode byte, which keeps them in
		 * normal command mode.
		 */
		if (dummy > 1U) {
			mspi_bitbang_set_outputs(dev, dat->lines);
			mspi_bitbang_set_lines(cfg, dat->lines, BIT_MASK(dat->lines));
			mspi_bitbang_dummy_cycles(dev, dummy - 1U);
			dummy = 1U;
		}

		/*
		 * A single-line read on a bus with a separate input line keeps
		 * IO0 driven, every other read releases all lines.
		 */
		mspi_bitbang_set_outputs(dev, ((dat->lines == 1U) && (cfg->num_io > 1U)) ? 1U : 0U);
	} else {
		mspi_bitbang_set_outputs(dev, dat->lines);
	}

	mspi_bitbang_dummy_cycles(dev, dummy);

	if (pkt->dir == MSPI_TX) {
		mspi_bitbang_xfer_bytes(dev, pkt->data_buf, NULL, pkt->num_bytes, dat);
	} else {
		mspi_bitbang_xfer_bytes(dev, NULL, pkt->data_buf, pkt->num_bytes, dat);
	}

	mspi_bitbang_set_outputs(dev, 1U);

	if (xfer->hold_ce) {
		return;
	}

	mspi_bitbang_delay(data, 2U);

	if (xfer->ce_sw_ctrl.delay > 0U) {
		k_busy_wait(xfer->ce_sw_ctrl.delay);
	}

	if (ce->port != NULL) {
		(void)gpio_pin_set_dt(ce, 0);
	}
}

static int mspi_bitbang_xfer_check(const struct device *dev, const struct mspi_xfer *xfer)
{
	const struct mspi_bitbang_data *data = dev->data;
	uint8_t unit_cmd = data->phase[MSPI_BITBANG_PH_CMD].unit;
	uint8_t unit_addr = data->phase[MSPI_BITBANG_PH_ADDR].unit;
	uint8_t unit_data = data->phase[MSPI_BITBANG_PH_DATA].unit;

	if (xfer->async) {
		LOG_ERR("Asynchronous transfers are not supported");
		return -ENOTSUP;
	}

	if (xfer->xfer_mode != MSPI_PIO) {
		LOG_ERR("Only PIO transfers are supported");
		return -ENOTSUP;
	}

	if ((xfer->packets == NULL) || (xfer->num_packet == 0U)) {
		return -EINVAL;
	}

	if (xfer->cmd_length > MSPI_BITBANG_MAX_CMD_LEN) {
		LOG_ERR("Invalid cmd_length %u", xfer->cmd_length);
		return -EINVAL;
	}

	if ((xfer->cmd_length % unit_cmd) != 0U) {
		LOG_ERR("cmd_length %u is not a multiple of the bytes per cycle", xfer->cmd_length);
		return -EINVAL;
	}

	if (xfer->addr_length > MSPI_BITBANG_MAX_ADDR_LEN) {
		LOG_ERR("Invalid addr_length %u", xfer->addr_length);
		return -EINVAL;
	}

	if ((xfer->addr_length % unit_addr) != 0U) {
		LOG_ERR("addr_length %u is not a multiple of the bytes per cycle",
			xfer->addr_length);
		return -EINVAL;
	}

	for (uint32_t i = 0U; i < xfer->num_packet; i++) {
		const struct mspi_xfer_packet *pkt = &xfer->packets[i];

		if ((pkt->dir != MSPI_RX) && (pkt->dir != MSPI_TX)) {
			return -EINVAL;
		}

		if ((pkt->num_bytes > 0U) && (pkt->data_buf == NULL)) {
			return -EINVAL;
		}

		if ((pkt->num_bytes % unit_data) != 0U) {
			LOG_ERR("num_bytes %u is not a multiple of the bytes per cycle",
				pkt->num_bytes);
			return -EINVAL;
		}
	}

	return 0;
}

static int mspi_bitbang_transceive(const struct device *dev, const struct mspi_dev_id *dev_id,
				   const struct mspi_xfer *xfer)
{
	struct mspi_bitbang_data *data = dev->data;
	int ret;

	if (dev_id != data->dev_id) {
		LOG_ERR("Device ID mismatch");
		return -ESTALE;
	}

	ret = mspi_bitbang_xfer_check(dev, xfer);
	if (ret < 0) {
		return ret;
	}

	if (xfer->ce_sw_ctrl.gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&xfer->ce_sw_ctrl.gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure the transfer CE GPIO: %d", ret);
			return ret;
		}
	}

	for (uint32_t i = 0U; i < xfer->num_packet; i++) {
		mspi_bitbang_packet(dev, dev_id, xfer, &xfer->packets[i]);
	}

	return 0;
}

static int mspi_bitbang_dev_cfg_check(const struct device *dev, const struct mspi_dev_id *dev_id,
				      const enum mspi_dev_cfg_mask param_mask,
				      const struct mspi_dev_cfg *cfg)
{
	const struct mspi_bitbang_config *bb_cfg = dev->config;
	const struct mspi_bitbang_data *data = dev->data;

	if (dev_id->dev_idx >= data->num_periph) {
		LOG_ERR("Invalid device index %u", dev_id->dev_idx);
		return -ENODEV;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_IO_MODE) {
		uint8_t lines;

		switch (cfg->io_mode) {
		case MSPI_IO_MODE_SINGLE:
			lines = 1U;
			break;
		case MSPI_IO_MODE_DUAL:
		case MSPI_IO_MODE_DUAL_1_1_2:
		case MSPI_IO_MODE_DUAL_1_2_2:
			lines = 2U;
			break;
		case MSPI_IO_MODE_QUAD:
		case MSPI_IO_MODE_QUAD_1_1_4:
		case MSPI_IO_MODE_QUAD_1_4_4:
			lines = 4U;
			break;
		case MSPI_IO_MODE_OCTAL:
		case MSPI_IO_MODE_OCTAL_1_1_8:
		case MSPI_IO_MODE_OCTAL_1_8_8:
			lines = 8U;
			break;
		case MSPI_IO_MODE_HEX:
		case MSPI_IO_MODE_HEX_8_8_16:
		case MSPI_IO_MODE_HEX_8_16_16:
			lines = 16U;
			break;
		default:
			LOG_ERR("Unsupported io_mode %d", cfg->io_mode);
			return -ENOTSUP;
		}

		if (bb_cfg->num_io < lines) {
			LOG_ERR("io_mode %d needs more data lines than wired", cfg->io_mode);
			return -ENOTSUP;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) {
		switch (cfg->data_rate) {
		case MSPI_DATA_RATE_SINGLE:
		case MSPI_DATA_RATE_S_S_D:
		case MSPI_DATA_RATE_S_D_D:
		case MSPI_DATA_RATE_DUAL:
			break;
		default:
			LOG_ERR("Invalid data_rate %d", cfg->data_rate);
			return -EINVAL;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CPP) {
		switch (cfg->cpp) {
		case MSPI_CPP_MODE_0:
		case MSPI_CPP_MODE_1:
		case MSPI_CPP_MODE_2:
		case MSPI_CPP_MODE_3:
			break;
		default:
			LOG_ERR("Invalid cpp %d", cfg->cpp);
			return -EINVAL;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_ENDIAN) {
		switch (cfg->endian) {
		case MSPI_XFER_LITTLE_ENDIAN:
		case MSPI_XFER_BIG_ENDIAN:
			break;
		default:
			LOG_ERR("Invalid endian %d", cfg->endian);
			return -EINVAL;
		}
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_CE_POL) && (dev_id->ce.port != NULL)) {
		bool gpio_active_low = (dev_id->ce.dt_flags & GPIO_ACTIVE_LOW) != 0U;
		bool cfg_active_low = (cfg->ce_polarity == MSPI_CE_ACTIVE_LOW);

		if (gpio_active_low != cfg_active_low) {
			LOG_ERR("ce_polarity does not match the CE GPIO flags");
			return -EINVAL;
		}
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_DQS) && cfg->dqs_enable) {
		LOG_ERR("DQS is not supported");
		return -ENOTSUP;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) && (data->max_freq > 0U) &&
	    (cfg->freq > data->max_freq)) {
		LOG_ERR("Frequency %u exceeds controller maximum %u", cfg->freq, data->max_freq);
		return -ENOTSUP;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_CMD_LEN) &&
	    (cfg->cmd_length > MSPI_BITBANG_MAX_CMD_LEN)) {
		LOG_ERR("Invalid cmd_length %u", cfg->cmd_length);
		return -EINVAL;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_ADDR_LEN) &&
	    (cfg->addr_length > MSPI_BITBANG_MAX_ADDR_LEN)) {
		LOG_ERR("Invalid addr_length %u", cfg->addr_length);
		return -EINVAL;
	}

	return 0;
}

static void mspi_bitbang_dev_cfg_store(struct mspi_dev_cfg *dst,
				       const enum mspi_dev_cfg_mask param_mask,
				       const struct mspi_dev_cfg *cfg)
{
	if (param_mask & MSPI_DEVICE_CONFIG_CE_NUM) {
		dst->ce_num = cfg->ce_num;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) {
		dst->freq = cfg->freq;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_IO_MODE) {
		dst->io_mode = cfg->io_mode;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) {
		dst->data_rate = cfg->data_rate;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_CPP) {
		dst->cpp = cfg->cpp;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_ENDIAN) {
		dst->endian = cfg->endian;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_CE_POL) {
		dst->ce_polarity = cfg->ce_polarity;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_DQS) {
		dst->dqs_enable = cfg->dqs_enable;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_RX_DUMMY) {
		dst->rx_dummy = cfg->rx_dummy;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_TX_DUMMY) {
		dst->tx_dummy = cfg->tx_dummy;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_READ_CMD) {
		dst->read_cmd = cfg->read_cmd;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_WRITE_CMD) {
		dst->write_cmd = cfg->write_cmd;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_CMD_LEN) {
		dst->cmd_length = cfg->cmd_length;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_ADDR_LEN) {
		dst->addr_length = cfg->addr_length;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_MEM_BOUND) {
		dst->mem_boundary = cfg->mem_boundary;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_BREAK_TIME) {
		dst->time_to_break = cfg->time_to_break;
	}
}

static void mspi_bitbang_dev_cfg_apply(const struct device *dev, const struct mspi_dev_cfg *dst)
{
	const struct mspi_bitbang_config *cfg = dev->config;
	struct mspi_bitbang_data *data = dev->data;
	struct mspi_bitbang_phase *cmd = &data->phase[MSPI_BITBANG_PH_CMD];
	struct mspi_bitbang_phase *addr = &data->phase[MSPI_BITBANG_PH_ADDR];
	struct mspi_bitbang_phase *dat = &data->phase[MSPI_BITBANG_PH_DATA];
	bool cpol;

	cpol = (dst->cpp == MSPI_CPP_MODE_2) || (dst->cpp == MSPI_CPP_MODE_3);
	data->sck_active = cpol ? 0 : 1;
	data->sck_idle = cpol ? 1 : 0;
	data->cpha = (dst->cpp == MSPI_CPP_MODE_1) || (dst->cpp == MSPI_CPP_MODE_3);
	data->lsb_first = (dst->endian == MSPI_XFER_LITTLE_ENDIAN);

	/* The idle level follows the clock polarity of the peripheral. */
	(void)gpio_pin_set_dt(&cfg->sck, data->sck_idle);

	cmd->ddr = (dst->data_rate == MSPI_DATA_RATE_DUAL);
	addr->ddr = cmd->ddr || (dst->data_rate == MSPI_DATA_RATE_S_D_D);
	dat->ddr = (dst->data_rate != MSPI_DATA_RATE_SINGLE);

	/* Data lines of the command, address and data phases. */
	switch (dst->io_mode) {
	case MSPI_IO_MODE_DUAL:
		cmd->lines = 2U;
		addr->lines = 2U;
		dat->lines = 2U;
		break;
	case MSPI_IO_MODE_DUAL_1_1_2:
		cmd->lines = 1U;
		addr->lines = 1U;
		dat->lines = 2U;
		break;
	case MSPI_IO_MODE_DUAL_1_2_2:
		cmd->lines = 1U;
		addr->lines = 2U;
		dat->lines = 2U;
		break;
	case MSPI_IO_MODE_QUAD:
		cmd->lines = 4U;
		addr->lines = 4U;
		dat->lines = 4U;
		break;
	case MSPI_IO_MODE_QUAD_1_1_4:
		cmd->lines = 1U;
		addr->lines = 1U;
		dat->lines = 4U;
		break;
	case MSPI_IO_MODE_QUAD_1_4_4:
		cmd->lines = 1U;
		addr->lines = 4U;
		dat->lines = 4U;
		break;
	case MSPI_IO_MODE_OCTAL:
		cmd->lines = 8U;
		addr->lines = 8U;
		dat->lines = 8U;
		break;
	case MSPI_IO_MODE_OCTAL_1_1_8:
		cmd->lines = 1U;
		addr->lines = 1U;
		dat->lines = 8U;
		break;
	case MSPI_IO_MODE_OCTAL_1_8_8:
		cmd->lines = 1U;
		addr->lines = 8U;
		dat->lines = 8U;
		break;
	case MSPI_IO_MODE_HEX:
		cmd->lines = 16U;
		addr->lines = 16U;
		dat->lines = 16U;
		break;
	case MSPI_IO_MODE_HEX_8_8_16:
		cmd->lines = 8U;
		addr->lines = 8U;
		dat->lines = 16U;
		break;
	case MSPI_IO_MODE_HEX_8_16_16:
		cmd->lines = 8U;
		addr->lines = 16U;
		dat->lines = 16U;
		break;
	case MSPI_IO_MODE_SINGLE:
	default:
		cmd->lines = 1U;
		addr->lines = 1U;
		dat->lines = 1U;
		break;
	}

	cmd->unit = MSPI_BITBANG_UNIT_BYTES(cmd->lines, cmd->ddr);
	addr->unit = MSPI_BITBANG_UNIT_BYTES(addr->lines, addr->ddr);
	dat->unit = MSPI_BITBANG_UNIT_BYTES(dat->lines, dat->ddr);

	/* The requested frequency is an upper bound; a device that leaves it
	 * unset gets the controller maximum. Without any bound there is no
	 * delay and the GPIO access time alone sets the clock rate.
	 */
	if (dst->freq > 0U) {
		data->quarter_period_us = DIV_ROUND_UP(USEC_PER_SEC, 4U * dst->freq);
	} else if (data->max_freq > 0U) {
		data->quarter_period_us = DIV_ROUND_UP(USEC_PER_SEC, 4U * data->max_freq);
	} else {
		data->quarter_period_us = 0U;
	}
}

/*
 * Acquire the controller for a peripheral and update its stored configuration.
 * The stored configuration of the peripheral becomes active, so a peripheral
 * that acquires the controller with MSPI_DEVICE_CONFIG_NONE gets back what it
 * configured earlier. mspi_get_channel_status() releases the controller.
 */
static int mspi_bitbang_dev_config(const struct device *dev, const struct mspi_dev_id *dev_id,
				   const enum mspi_dev_cfg_mask param_mask,
				   const struct mspi_dev_cfg *cfg)
{
	const struct mspi_bitbang_config *bb_cfg = dev->config;
	struct mspi_bitbang_data *data = dev->data;
	struct mspi_dev_cfg *dst;
	int ret;

	ret = mspi_bitbang_dev_cfg_check(dev, dev_id, param_mask, cfg);
	if (ret < 0) {
		return ret;
	}

	if (data->dev_id != dev_id) {
		/* A second lock from the owning thread would succeed and never be released. */
		if ((data->dev_id != NULL) && (data->owner == k_current_get())) {
			LOG_ERR("Device %u has not released the controller", data->dev_id->dev_idx);
			return -EBUSY;
		}
		ret = k_mutex_lock(&data->lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE));
		if (ret != 0) {
			LOG_ERR("Controller is busy");
			return -EBUSY;
		}
		data->dev_id = dev_id;
		data->owner = k_current_get();
	}

	dst = &bb_cfg->dev_cfgs[dev_id->dev_idx];
	mspi_bitbang_dev_cfg_store(dst, param_mask, cfg);
	mspi_bitbang_dev_cfg_apply(dev, dst);

	return 0;
}

static int mspi_bitbang_get_channel_status(const struct device *dev, uint8_t ch)
{
	struct mspi_bitbang_data *data = dev->data;

	ARG_UNUSED(ch);

	if (data->dev_id != NULL) {
		data->dev_id = NULL;
		data->owner = NULL;
		return k_mutex_unlock(&data->lock);
	}

	return 0;
}

static int mspi_bitbang_pin_init(const struct gpio_dt_spec *spec, gpio_flags_t flags)
{
	if (!gpio_is_ready_dt(spec)) {
		return -ENODEV;
	}

	return gpio_pin_configure_dt(spec, flags);
}

static int mspi_bitbang_config(const struct mspi_dt_spec *spec)
{
	const struct device *dev = spec->bus;
	const struct mspi_bitbang_config *cfg = dev->config;
	struct mspi_bitbang_data *data = dev->data;
	const struct mspi_cfg *mspicfg = &spec->config;
	int ret;

	if (mspicfg->op_mode != MSPI_OP_MODE_CONTROLLER) {
		LOG_ERR("Only controller mode is supported");
		return -ENOTSUP;
	}

	if (mspicfg->duplex != MSPI_HALF_DUPLEX) {
		LOG_ERR("Only half-duplex transfers are supported");
		return -ENOTSUP;
	}

	if (mspicfg->dqs_support) {
		LOG_ERR("DQS is not supported");
		return -ENOTSUP;
	}

	if (mspicfg->num_periph > cfg->max_periph) {
		LOG_ERR("Only %u peripherals can be configured", cfg->max_periph);
		return -ENOTSUP;
	}

	if ((mspicfg->num_ce_gpios != 0U) && (mspicfg->num_ce_gpios < mspicfg->num_periph)) {
		LOG_ERR("Fewer CE GPIOs than peripherals");
		return -EINVAL;
	}

	if (mspicfg->re_init) {
		ret = k_mutex_lock(&data->lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE));
		if (ret != 0) {
			LOG_ERR("Controller is busy");
			return -EBUSY;
		}
	}

	ret = mspi_bitbang_pin_init(&cfg->sck, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure the SCK GPIO: %d", ret);
		goto out;
	}

	for (uint8_t k = 0U; k < cfg->num_io; k++) {
		ret = mspi_bitbang_pin_init(&cfg->io[k],
					    (k == 0U) ? GPIO_OUTPUT_INACTIVE : GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure the IO%u GPIO: %d", k, ret);
			goto out;
		}
	}

	for (uint32_t i = 0U; i < mspicfg->num_ce_gpios; i++) {
		ret = mspi_bitbang_pin_init(&mspicfg->ce_group[i], GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure the CE%u GPIO: %d", i, ret);
			goto out;
		}
	}

	data->num_out = 1U;
	data->max_freq = mspicfg->max_freq;
	data->num_periph = mspicfg->num_periph;

out:
	if (mspicfg->re_init) {
		(void)k_mutex_unlock(&data->lock);
	}

	return ret;
}

static int mspi_bitbang_init(const struct device *dev)
{
	const struct mspi_bitbang_config *cfg = dev->config;
	struct mspi_bitbang_data *data = dev->data;
	const struct mspi_dt_spec spec = {
		.bus = dev,
		.config = cfg->mspicfg,
	};

	k_mutex_init(&data->lock);

	return mspi_bitbang_config(&spec);
}

static DEVICE_API(mspi, mspi_bitbang_api) = {
	.config = mspi_bitbang_config,
	.dev_config = mspi_bitbang_dev_config,
	.get_channel_status = mspi_bitbang_get_channel_status,
	.transceive = mspi_bitbang_transceive,
};

#define MSPI_BITBANG_IO_SPEC(node_id, prop, idx) GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx),

#define MSPI_BITBANG_NUM_CE(inst) DT_INST_PROP_LEN_OR(inst, ce_gpios, 0)

/* Peripherals are indexed by reg, so their reg values must be 0 .. children - 1. */
#define MSPI_BITBANG_CHECK_CHILD(node_id)                                                          \
	BUILD_ASSERT(DT_REG_ADDR(node_id) < DT_CHILD_NUM(DT_PARENT(node_id)),                      \
		     "peripheral reg must be below the number of children");

#define MSPI_BITBANG_MSPICFG(inst)                                                                 \
	{                                                                                          \
		.channel_num = inst,                                                               \
		.op_mode = DT_INST_ENUM_IDX_OR(inst, op_mode, MSPI_OP_MODE_CONTROLLER),            \
		.duplex = DT_INST_ENUM_IDX_OR(inst, duplex, MSPI_HALF_DUPLEX),                     \
		.dqs_support = DT_INST_PROP(inst, dqs_support),                                    \
		.sw_multi_periph = DT_INST_PROP(inst, software_multiperipheral),                   \
		.ce_group = mspi_bitbang_ce_##inst,                                                \
		.num_ce_gpios = ARRAY_SIZE(mspi_bitbang_ce_##inst),                                \
		.num_periph = DT_INST_CHILD_NUM(inst),                                             \
		.max_freq = DT_INST_PROP_OR(inst, clock_frequency, 0),                             \
		.re_init = false,                                                                  \
	}

#define MSPI_BITBANG_DEFINE(inst)                                                                  \
	BUILD_ASSERT(DT_INST_PROP_LEN(inst, io_gpios) <= MSPI_BITBANG_MAX_IO,                      \
		     "io-gpios supports at most 16 data lines");                                   \
	BUILD_ASSERT((MSPI_BITBANG_NUM_CE(inst) == 0) ||                                           \
			     (MSPI_BITBANG_NUM_CE(inst) >= DT_INST_CHILD_NUM(inst)),               \
		     "ce-gpios needs one entry per peripheral");                                   \
	DT_INST_FOREACH_CHILD(inst, MSPI_BITBANG_CHECK_CHILD)                                      \
                                                                                                   \
	static const struct gpio_dt_spec mspi_bitbang_io_##inst[] = {                              \
		DT_INST_FOREACH_PROP_ELEM(inst, io_gpios, MSPI_BITBANG_IO_SPEC)                    \
	};                                                                                         \
	static struct gpio_dt_spec mspi_bitbang_ce_##inst[] =                                      \
		MSPI_CE_GPIOS_DT_SPEC_INST_GET(inst);                                              \
                                                                                                   \
	static struct mspi_dev_cfg mspi_bitbang_dev_cfgs_##inst[MAX(1, DT_INST_CHILD_NUM(inst))];  \
                                                                                                   \
	static const struct mspi_bitbang_config mspi_bitbang_config_##inst = {                     \
		.sck = GPIO_DT_SPEC_INST_GET(inst, sck_gpios),                                     \
		.io = mspi_bitbang_io_##inst,                                                      \
		.num_io = DT_INST_PROP_LEN(inst, io_gpios),                                        \
		.dev_cfgs = mspi_bitbang_dev_cfgs_##inst,                                          \
		.max_periph = ARRAY_SIZE(mspi_bitbang_dev_cfgs_##inst),                            \
		.mspicfg = MSPI_BITBANG_MSPICFG(inst),                                             \
	};                                                                                         \
                                                                                                   \
	static struct mspi_bitbang_data mspi_bitbang_data_##inst;                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mspi_bitbang_init, NULL, &mspi_bitbang_data_##inst,            \
			      &mspi_bitbang_config_##inst, POST_KERNEL, CONFIG_MSPI_INIT_PRIORITY, \
			      &mspi_bitbang_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_BITBANG_DEFINE)
