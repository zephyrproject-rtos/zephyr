/*
 * Copyright (c) 2026 Embeint Inc
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/lora/sx126x_emul.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/dt-bindings/lora/sx126x.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define LORA_NODE DT_ALIAS(lora0)
#define IS_SX1261 DT_NODE_HAS_COMPAT(LORA_NODE, semtech_sx1261)
#define IS_LLCC68 DT_NODE_HAS_COMPAT(LORA_NODE, semtech_llcc68)

#define REST_MODE                                                                                  \
	(IS_ENABLED(CONFIG_LORA_SX126X_NATIVE_SLEEP) ? SX126X_EMUL_MODE_SLEEP                      \
						     : SX126X_EMUL_MODE_STDBY_RC)

/* IRQs the driver routes to DIO1: TX done, RX done, CRC error and timeout */
#define DRIVER_IRQ_MASK (BIT(0) | BIT(1) | BIT(6) | BIT(9))

/* Register addresses from the datasheet */
#define REG_RETENTION_LIST 0x029F
#define REG_RX_GAIN        0x08AC
#define REG_TX_MODULATION  0x0889

/* Opcodes and IRQs from the datasheet */
#define OP_SET_STANDBY 0x80
#define OP_SET_TX      0x83
#define IRQ_TX_DONE    BIT(0)
#define IRQ_RX_DONE    BIT(1)

#define RX_GAIN_BOOSTED     0x96
#define TX_MODULATION_BIT   BIT(2)
#define IQ_POLARITY_BIT     BIT(2)
#define RAMP_200_US         0x04
#define GFSK_PULSE_BT_05    0x09
#define GFSK_PREAMBLE_DET_8 0x04
#define GFSK_CRC_2_BYTE_INV 0x06

static const struct device *const lora_dev = DEVICE_DT_GET(LORA_NODE);
static const struct emul *const lora_emul = EMUL_DT_GET(LORA_NODE);
static const struct gpio_dt_spec tx_enable = GPIO_DT_SPEC_GET(LORA_NODE, tx_enable_gpios);
static const struct gpio_dt_spec rx_enable = GPIO_DT_SPEC_GET(LORA_NODE, rx_enable_gpios);
static const struct gpio_dt_spec busy = GPIO_DT_SPEC_GET(LORA_NODE, busy_gpios);

/* Captured before the first test clears the counter */
static uint32_t boot_errors;

struct recv_ctx {
	struct k_sem sem;
	uint8_t data[SX126X_EMUL_MAX_PAYLOAD];
	uint16_t len;
	int16_t rssi;
	int8_t snr;
};

static void recv_cb(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi,
		    int8_t snr, void *user_data)
{
	struct recv_ctx *ctx = user_data;

	ARG_UNUSED(dev);

	memcpy(ctx->data, data, MIN(size, sizeof(ctx->data)));
	ctx->len = size;
	ctx->rssi = rssi;
	ctx->snr = snr;
	k_sem_give(&ctx->sem);
}

static void lora_cfg_default(struct lora_modem_config *cfg)
{
	*cfg = (struct lora_modem_config){
		.frequency = 868100000,
		.bandwidth = BW_125_KHZ,
		.datarate = SF_7,
		.coding_rate = CR_4_5,
		.preamble_len = 8,
		.tx_power = 14,
		.tx = true,
		.public_network = true,
	};
}

static void lora_configure_default(void)
{
	struct lora_modem_config cfg;

	lora_cfg_default(&cfg);
	zassert_ok(lora_config(lora_dev, &cfg));
}

static void rx_packet_queue(const char *payload, int16_t rssi, int8_t snr, bool crc_error)
{
	struct sx126x_emul_rx_packet pkt = {
		.len = strlen(payload),
		.rssi = rssi,
		.snr = snr,
		.crc_error = crc_error,
	};

	memcpy(pkt.data, payload, pkt.len);
	zassert_ok(sx126x_emul_rx_packet_set(lora_emul, &pkt));
}

/* Raises IRQs once, as the next instance of an opcode completes */
struct cmd_hook {
	uint8_t opcode;
	uint16_t irq;
	uint32_t seen;
	uint8_t params[4];
	bool fired;
};

static uint16_t cmd_hook_cb(const struct emul *target, uint8_t opcode, const uint8_t *params,
			    size_t len, void *user_data)
{
	struct cmd_hook *hook = user_data;

	ARG_UNUSED(target);

	if (opcode != hook->opcode) {
		return 0;
	}

	hook->seen++;
	memcpy(hook->params, params, MIN(len, sizeof(hook->params)));
	if (hook->fired) {
		return 0;
	}

	hook->fired = true;
	return hook->irq;
}

static void noop_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
}

/* Interrupt work left behind by the driver must not hold up the workqueue */
static void workq_assert_responsive(void)
{
	struct k_work work;
	struct k_work_sync sync;
	int64_t start = k_uptime_get();

	k_work_init(&work, noop_work_handler);
	zassert_true(k_work_submit(&work) >= 0);
	(void)k_work_flush(&work, &sync);
	zassert_true(k_uptime_get() - start < 100, "System workqueue stalled for %lld ms",
		     k_uptime_get() - start);
}

static void radio_assert_rested(void)
{
	/* Let any interrupt work still in flight finish */
	k_msleep(10);

	zassert_equal(sx126x_emul_mode_get(lora_emul), REST_MODE, "Radio in mode %d",
		      sx126x_emul_mode_get(lora_emul));
	zassert_equal(gpio_emul_output_get_dt(&tx_enable), 0, "TX path left enabled");
	zassert_equal(gpio_emul_output_get_dt(&rx_enable), 0, "RX path left enabled");
	zassert_equal(sx126x_emul_errors_get(lora_emul), 0, "Emulator reported protocol errors");
}

ZTEST(sx126x_emul, test_init)
{
	struct sx126x_emul_radio radio;
	uint8_t retention[5];

	zassert_equal(boot_errors, 0, "Protocol errors during driver init");

	sx126x_emul_radio_get(lora_emul, &radio);
	zassert_equal(radio.packet_type, SX126X_EMUL_PACKET_TYPE_LORA);
	zassert_true(radio.dcdc, "DC-DC regulator not selected");
	zassert_true(radio.tcxo, "TCXO not configured");
	zassert_equal(radio.tcxo_voltage, SX126X_DIO3_TCXO_1V8);
	zassert_false(radio.dio2_rf_switch);
	zassert_equal(radio.irq_mask, DRIVER_IRQ_MASK);
	zassert_equal(radio.dio1_mask, DRIVER_IRQ_MASK);

	/* RX gain and TX modulation survive warm-start sleep */
	zassert_ok(sx126x_emul_reg_read(lora_emul, REG_RETENTION_LIST, retention,
					sizeof(retention)));
	zassert_equal(retention[0], 2);
	zassert_equal(sys_get_be16(&retention[1]), REG_RX_GAIN);
	zassert_equal(sys_get_be16(&retention[3]), REG_TX_MODULATION);

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_config_lora)
{
	struct lora_modem_config cfg;
	struct sx126x_emul_radio radio;

	lora_cfg_default(&cfg);
	zassert_ok(lora_config(lora_dev, &cfg));

	sx126x_emul_radio_get(lora_emul, &radio);
	zassert_equal(radio.packet_type, SX126X_EMUL_PACKET_TYPE_LORA);
	zassert_within(radio.frequency, 868100000, 1);
	zassert_equal(radio.lora.sf, 7);
	zassert_equal(radio.lora.bandwidth, 125000);
	zassert_equal(radio.lora.cr, 5);
	zassert_false(radio.lora.ldro);
	zassert_equal(radio.lora.sync_word, 0x3444);
	zassert_equal(radio.tx_power, 14);
	zassert_equal(radio.ramp_time, RAMP_200_US);
	if (IS_SX1261) {
		zassert_equal(radio.pa.duty_cycle, 0x04);
		zassert_equal(radio.pa.hp_max, 0x00);
		zassert_equal(radio.pa.device_sel, 1);
	} else {
		zassert_equal(radio.pa.duty_cycle, 0x04);
		zassert_equal(radio.pa.hp_max, 0x07);
		zassert_equal(radio.pa.device_sel, 0);
	}
	zassert_equal(radio.pa.pa_lut, 1);
	/* Read after the driver put the radio back to sleep: retained */
	zassert_equal(radio.rx_gain, RX_GAIN_BOOSTED);

	cfg.public_network = false;
	zassert_ok(lora_config(lora_dev, &cfg));
	sx126x_emul_radio_get(lora_emul, &radio);
	zassert_equal(radio.lora.sync_word, 0x1424);

	cfg.sync_word = 0x56;
	zassert_ok(lora_config(lora_dev, &cfg));
	sx126x_emul_radio_get(lora_emul, &radio);
	zassert_equal(radio.lora.sync_word, 0x5464);

	/* Output power is clamped to what the PA can deliver */
	cfg.tx_power = 30;
	zassert_ok(lora_config(lora_dev, &cfg));
	sx126x_emul_radio_get(lora_emul, &radio);
	zassert_equal(radio.tx_power, IS_SX1261 ? 14 : 22);

	if (!IS_LLCC68) {
		cfg.datarate = SF_12;
		zassert_ok(lora_config(lora_dev, &cfg));
		sx126x_emul_radio_get(lora_emul, &radio);
		zassert_equal(radio.lora.sf, 12);
		zassert_true(radio.lora.ldro, "LDRO required at SF12/125 kHz");
	} else {
		cfg.datarate = SF_12;
		zassert_equal(lora_config(lora_dev, &cfg), -EINVAL);
	}

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_send)
{
	static const uint8_t payload[] = "hello sx126x";
	struct sx126x_emul_tx_packet pkt;
	struct sx126x_emul_stats stats;

	lora_configure_default();
	zassert_ok(lora_send(lora_dev, (uint8_t *)payload, sizeof(payload)));

	zassert_ok(sx126x_emul_last_tx_get(lora_emul, &pkt));
	zassert_equal(pkt.len, sizeof(payload));
	zassert_mem_equal(pkt.data, payload, sizeof(payload));
	zassert_equal(pkt.radio.packet_type, SX126X_EMUL_PACKET_TYPE_LORA);
	zassert_within(pkt.radio.frequency, 868100000, 1);
	zassert_equal(pkt.radio.lora.preamble_len, 8);
	zassert_false(pkt.radio.lora.implicit_header);
	zassert_equal(pkt.radio.lora.payload_len, sizeof(payload));
	zassert_true(pkt.radio.lora.crc);
	zassert_false(pkt.radio.lora.iq_inverted);
	zassert_true((pkt.radio.tx_modulation & TX_MODULATION_BIT) != 0);
	zassert_true((pkt.radio.iq_polarity & IQ_POLARITY_BIT) != 0);
	zassert_not_equal(pkt.timeout, 0, "No TX timeout armed");

	sx126x_emul_stats_get(lora_emul, &stats);
	zassert_equal(stats.tx_starts, 1);
	zassert_equal(stats.tx_done, 1);

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_send_bw500)
{
	static const uint8_t payload[] = {0x01, 0x02, 0x03};
	struct lora_modem_config cfg;
	struct sx126x_emul_tx_packet pkt;

	lora_cfg_default(&cfg);
	cfg.bandwidth = BW_500_KHZ;
	zassert_ok(lora_config(lora_dev, &cfg));
	zassert_ok(lora_send(lora_dev, (uint8_t *)payload, sizeof(payload)));

	zassert_ok(sx126x_emul_last_tx_get(lora_emul, &pkt));
	zassert_equal(pkt.radio.lora.bandwidth, 500000);
	zassert_true((pkt.radio.tx_modulation & TX_MODULATION_BIT) == 0,
		     "500 kHz workaround not applied");

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_send_implicit_header)
{
	static const uint8_t payload[] = {0xA0, 0xA1, 0xA2, 0xA3};
	struct lora_modem_config cfg;
	struct sx126x_emul_tx_packet pkt;

	lora_cfg_default(&cfg);
	cfg.explicit_header_disable = true;
	cfg.implicit_packet_length = sizeof(payload);
	zassert_ok(lora_config(lora_dev, &cfg));

	zassert_equal(lora_send(lora_dev, (uint8_t *)payload, sizeof(payload) - 1), -EINVAL);
	zassert_ok(lora_send(lora_dev, (uint8_t *)payload, sizeof(payload)));

	zassert_ok(sx126x_emul_last_tx_get(lora_emul, &pkt));
	zassert_true(pkt.radio.lora.implicit_header);
	zassert_equal(pkt.radio.lora.payload_len, sizeof(payload));
	zassert_mem_equal(pkt.data, payload, sizeof(payload));

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_send_async)
{
	static const uint8_t payload[] = {0x10, 0x20, 0x30, 0x40, 0x50};
	struct k_poll_signal sig;
	struct k_poll_event evt =
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &sig);
	unsigned int signaled;
	int result;

	k_poll_signal_init(&sig);
	lora_configure_default();

	zassert_ok(lora_send_async(lora_dev, (uint8_t *)payload, sizeof(payload), &sig));
	zassert_ok(k_poll(&evt, 1, K_SECONDS(1)));
	k_poll_signal_check(&sig, &signaled, &result);
	zassert_equal(signaled, 1);
	zassert_ok(result);

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_recv)
{
	static const char payload[] = "downlink";
	struct lora_modem_config cfg;
	struct sx126x_emul_radio radio;
	uint8_t buf[32];
	int16_t rssi;
	int8_t snr;

	lora_cfg_default(&cfg);
	cfg.tx = false;
	cfg.iq_inverted = true;
	zassert_ok(lora_config(lora_dev, &cfg));

	rx_packet_queue(payload, -87, 6, false);
	zassert_equal(lora_recv(lora_dev, buf, sizeof(buf), K_SECONDS(1), &rssi, &snr),
		      strlen(payload));
	zassert_mem_equal(buf, payload, strlen(payload));
	zassert_equal(rssi, -87);
	zassert_equal(snr, 6);

	zassert_ok(sx126x_emul_last_rx_get(lora_emul, &radio));
	zassert_true(radio.lora.iq_inverted);
	zassert_true((radio.iq_polarity & IQ_POLARITY_BIT) == 0, "Inverted IQ workaround");
	zassert_equal(radio.rx_gain, RX_GAIN_BOOSTED, "RX gain lost across sleep");

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_recv_timeout)
{
	uint8_t buf[16];

	lora_configure_default();
	zassert_equal(lora_recv(lora_dev, buf, sizeof(buf), K_MSEC(50), NULL, NULL), -EAGAIN);

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_recv_crc_error)
{
	uint8_t buf[16];

	lora_configure_default();
	rx_packet_queue("corrupt", -100, -5, true);
	zassert_equal(lora_recv(lora_dev, buf, sizeof(buf), K_SECONDS(1), NULL, NULL), -EIO);

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_recv_forever)
{
	static const char payload[] = "no timeout";
	uint8_t buf[16];

	lora_configure_default();
	rx_packet_queue(payload, -60, 10, false);
	zassert_equal(lora_recv(lora_dev, buf, sizeof(buf), K_FOREVER, NULL, NULL),
		      strlen(payload));
	zassert_mem_equal(buf, payload, strlen(payload));

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_recv_async)
{
	static const char *const payloads[] = {"first", "second packet"};
	struct sx126x_emul_stats stats;
	struct recv_ctx ctx;

	k_sem_init(&ctx.sem, 0, 1);
	lora_configure_default();

	zassert_ok(lora_recv_async(lora_dev, recv_cb, &ctx));
	zassert_equal(sx126x_emul_mode_get(lora_emul), SX126X_EMUL_MODE_RX);
	zassert_equal(gpio_emul_output_get_dt(&rx_enable), 1, "RX path not enabled");
	zassert_equal(gpio_emul_output_get_dt(&tx_enable), 0, "TX path enabled while receiving");

	ARRAY_FOR_EACH(payloads, i) {
		rx_packet_queue(payloads[i], -70 - (int16_t)i, 5, false);
		zassert_ok(k_sem_take(&ctx.sem, K_SECONDS(1)), "Packet %zu not delivered", i);
		zassert_equal(ctx.len, strlen(payloads[i]));
		zassert_mem_equal(ctx.data, payloads[i], ctx.len);
		zassert_equal(ctx.rssi, -70 - (int16_t)i);
		zassert_equal(ctx.snr, 5);
	}

	/* A corrupted packet is dropped and reception continues */
	rx_packet_queue("bad", -70, 5, true);
	zassert_equal(k_sem_take(&ctx.sem, K_MSEC(50)), -EAGAIN);
	zassert_equal(sx126x_emul_mode_get(lora_emul), SX126X_EMUL_MODE_RX);

	/* Continuous reception carries on without being restarted */
	sx126x_emul_stats_get(lora_emul, &stats);
	zassert_equal(stats.rx_starts, 1);

	zassert_ok(lora_recv_async(lora_dev, NULL, NULL));

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_rssi)
{
	struct recv_ctx ctx;
	int16_t rssi;

	k_sem_init(&ctx.sem, 0, 1);
	lora_configure_default();

	zassert_ok(lora_recv_async(lora_dev, recv_cb, &ctx));
	sx126x_emul_rssi_set(lora_emul, -95);
	zassert_ok(lora_rssi(lora_dev, &rssi));
	zassert_equal(rssi, -95);
	zassert_ok(lora_recv_async(lora_dev, NULL, NULL));

	sx126x_emul_rssi_set(lora_emul, -60);
	zassert_equal(lora_energy_detect(lora_dev, -80, K_MSEC(20)), 1);
	sx126x_emul_rssi_set(lora_emul, -110);
	zassert_equal(lora_energy_detect(lora_dev, -80, K_MSEC(20)), 0);

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_duty_cycle)
{
	static const char payload[] = "wake up";
	struct sx126x_emul_stats stats;
	uint8_t buf[16];
	int16_t rssi;
	int8_t snr;

	lora_configure_default();
	rx_packet_queue(payload, -90, 3, false);
	zassert_equal(lora_recv_duty_cycle(lora_dev, K_MSEC(10), K_MSEC(90), buf, sizeof(buf),
					   K_SECONDS(1), &rssi, &snr),
		      strlen(payload));
	zassert_mem_equal(buf, payload, strlen(payload));
	zassert_equal(rssi, -90);
	zassert_equal(snr, 3);

	sx126x_emul_stats_get(lora_emul, &stats);
	zassert_equal(stats.rx_starts, 1);
	zassert_equal(stats.rx_done, 1);

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_duty_cycle_timeout)
{
	uint8_t buf[16];

	lora_configure_default();
	zassert_equal(lora_recv_duty_cycle(lora_dev, K_MSEC(10), K_MSEC(90), buf, sizeof(buf),
					   K_MSEC(50), NULL, NULL),
		      -EAGAIN);

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_duty_cycle_async)
{
	static const char *const payloads[] = {"one", "two"};
	struct sx126x_emul_stats stats;
	struct recv_ctx ctx;

	k_sem_init(&ctx.sem, 0, 1);
	lora_configure_default();

	zassert_ok(lora_recv_duty_cycle_async(lora_dev, K_MSEC(10), K_MSEC(90), recv_cb, &ctx));
	zassert_equal(sx126x_emul_mode_get(lora_emul), SX126X_EMUL_MODE_RX_DUTY_CYCLE);

	ARRAY_FOR_EACH(payloads, i) {
		rx_packet_queue(payloads[i], -75, 4, false);
		zassert_ok(k_sem_take(&ctx.sem, K_SECONDS(1)), "Packet %zu not delivered", i);
		zassert_equal(ctx.len, strlen(payloads[i]));
		zassert_mem_equal(ctx.data, payloads[i], ctx.len);
		/* The radio re-enters the duty cycle after every packet */
		k_msleep(5);
		zassert_equal(sx126x_emul_mode_get(lora_emul), SX126X_EMUL_MODE_RX_DUTY_CYCLE);
	}

	zassert_ok(lora_recv_duty_cycle_async(lora_dev, K_NO_WAIT, K_NO_WAIT, NULL, NULL));

	sx126x_emul_stats_get(lora_emul, &stats);
	zassert_equal(stats.rx_done, ARRAY_SIZE(payloads));

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_cw)
{
	struct sx126x_emul_radio radio;

	lora_configure_default();
	zassert_ok(lora_test_cw(lora_dev, 868500000, 10, 1));

	sx126x_emul_radio_get(lora_emul, &radio);
	zassert_within(radio.frequency, 868500000, 1);
	zassert_equal(radio.tx_power, 10);

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_gfsk)
{
	static const uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x42};
	struct lora_modem_config_gfsk cfg = {
		.frequency = 868300000,
		.bitrate = 50000,
		.freq_deviation = 25000,
		.bandwidth = 117300,
		.pulse_shape = LORA_GFSK_PULSE_SHAPE_BT_0_5,
		.sync_word = {0xC1, 0x94, 0xC1},
		.sync_word_len = 3,
		.preamble_len = 5,
		.tx_power = 14,
		.tx = true,
		.whitening = true,
	};
	struct sx126x_emul_tx_packet pkt;
	uint8_t buf[16];
	int16_t rssi;
	int8_t snr;

	Z_TEST_SKIP_IFNDEF(CONFIG_LORA_GFSK);

	zassert_ok(lora_config_gfsk(lora_dev, &cfg));
	zassert_ok(lora_send(lora_dev, (uint8_t *)payload, sizeof(payload)));

	zassert_ok(sx126x_emul_last_tx_get(lora_emul, &pkt));
	zassert_equal(pkt.radio.packet_type, SX126X_EMUL_PACKET_TYPE_GFSK);
	zassert_within(pkt.radio.frequency, 868300000, 1);
	zassert_equal(pkt.radio.gfsk.bitrate, 50000);
	zassert_within(pkt.radio.gfsk.freq_deviation, 25000, 1);
	zassert_equal(pkt.radio.gfsk.bandwidth, 117300);
	zassert_equal(pkt.radio.gfsk.pulse_shape, GFSK_PULSE_BT_05);
	zassert_equal(pkt.radio.gfsk.preamble_bits, 40);
	zassert_equal(pkt.radio.gfsk.preamble_detect, GFSK_PREAMBLE_DET_8);
	zassert_equal(pkt.radio.gfsk.sync_word_bits, 24);
	zassert_mem_equal(pkt.radio.gfsk.sync_word, cfg.sync_word, 3);
	zassert_true(pkt.radio.gfsk.variable_len);
	zassert_equal(pkt.radio.gfsk.payload_len, sizeof(payload));
	zassert_equal(pkt.radio.gfsk.crc_type, GFSK_CRC_2_BYTE_INV);
	zassert_equal(pkt.radio.gfsk.crc_init, 0x1D0F);
	zassert_equal(pkt.radio.gfsk.crc_poly, 0x1021);
	zassert_true(pkt.radio.gfsk.whitening);
	zassert_equal(pkt.radio.gfsk.whitening_init, 0x01FF);
	zassert_true((pkt.radio.tx_modulation & TX_MODULATION_BIT) != 0);
	zassert_equal(pkt.len, sizeof(payload));
	zassert_mem_equal(pkt.data, payload, sizeof(payload));

	rx_packet_queue("gfsk rx", -70, 0, false);
	zassert_equal(lora_recv(lora_dev, buf, sizeof(buf), K_SECONDS(1), &rssi, &snr),
		      strlen("gfsk rx"));
	zassert_mem_equal(buf, "gfsk rx", strlen("gfsk rx"));
	zassert_equal(rssi, -70);
	zassert_equal(snr, 0);

	/* Switching back to LoRa reselects the modem */
	lora_configure_default();
	zassert_ok(lora_send(lora_dev, (uint8_t *)payload, sizeof(payload)));
	zassert_ok(sx126x_emul_last_tx_get(lora_emul, &pkt));
	zassert_equal(pkt.radio.packet_type, SX126X_EMUL_PACKET_TYPE_LORA);

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_emulator_flags_misuse)
{
	const struct spi_dt_spec spi =
		SPI_DT_SPEC_GET(LORA_NODE, SPI_WORD_SET(8) | SPI_TRANSFER_MSB);
	/* An opcode the radio does not have */
	uint8_t bogus[] = {0xFF};
	/* SetSleep, warm start */
	uint8_t sleep[] = {0x84, 0x04};
	struct spi_buf buf = {.buf = bogus, .len = sizeof(bogus)};
	struct spi_buf_set set = {.buffers = &buf, .count = 1};

	/*
	 * Asleep, the command is lost waking the radio; awake, it is
	 * rejected. Either way the emulator must notice.
	 */
	zassert_ok(spi_write_dt(&spi, &set));
	zassert_equal(sx126x_emul_errors_get(lora_emul), 1);
	sx126x_emul_errors_clear(lora_emul);

	if (IS_ENABLED(CONFIG_LORA_SX126X_NATIVE_SLEEP)) {
		/* Put the radio back to sleep behind the driver's back, once it has woken */
		zassert_true(WAIT_FOR(gpio_pin_get_dt(&busy) == 0, 100000, k_msleep(1)));
		buf.buf = sleep;
		buf.len = sizeof(sleep);
		zassert_ok(spi_write_dt(&spi, &set));
	}

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_cmd_hook_observe)
{
	/* Static: the emulator may still call the hook once the test has returned */
	static struct cmd_hook hook;
	uint8_t payload[] = "observed";

	hook = (struct cmd_hook){.opcode = OP_SET_TX};
	lora_configure_default();
	sx126x_emul_cmd_cb_set(lora_emul, cmd_hook_cb, &hook);
	zassert_ok(lora_send(lora_dev, payload, sizeof(payload)));
	zassert_equal(hook.seen, 1);
	/* 10 s TX timeout in 15.625 us steps */
	zassert_equal(sys_get_be24(hook.params), 10000 * 64);

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_irq_tx_done_on_set_tx)
{
	static struct cmd_hook hook;
	uint8_t payload[] = "instant";

	hook = (struct cmd_hook){.opcode = OP_SET_TX, .irq = IRQ_TX_DONE};
	/* The radio finishes before the driver has returned from SetTx */
	lora_configure_default();
	sx126x_emul_cmd_cb_set(lora_emul, cmd_hook_cb, &hook);
	zassert_ok(lora_send(lora_dev, payload, sizeof(payload)));
	zassert_true(hook.fired);

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_irq_rx_done_while_stopping_async)
{
	static struct cmd_hook hook;
	uint8_t payload[] = "next";
	struct recv_ctx ctx;

	hook = (struct cmd_hook){.opcode = OP_SET_STANDBY, .irq = IRQ_RX_DONE};
	k_sem_init(&ctx.sem, 0, 1);
	lora_configure_default();
	zassert_ok(lora_recv_async(lora_dev, recv_cb, &ctx));

	/* A packet completes as the driver stops the radio */
	sx126x_emul_cmd_cb_set(lora_emul, cmd_hook_cb, &hook);
	zassert_ok(lora_recv_async(lora_dev, NULL, NULL));
	zassert_true(hook.fired);
	workq_assert_responsive();
	zassert_equal(k_sem_take(&ctx.sem, K_MSEC(50)), -EAGAIN, "Callback after stop");
	radio_assert_rested();

	/* The driver is still usable */
	zassert_ok(lora_send(lora_dev, payload, sizeof(payload)));

	radio_assert_rested();
}

ZTEST(sx126x_emul, test_irq_rx_done_during_recv_timeout)
{
	static struct cmd_hook hook;
	static const char payload[] = "fresh";
	uint8_t buf[16];

	hook = (struct cmd_hook){.opcode = OP_SET_STANDBY, .irq = IRQ_RX_DONE};
	/* A packet completes as the driver gives up waiting */
	lora_configure_default();
	sx126x_emul_cmd_cb_set(lora_emul, cmd_hook_cb, &hook);
	zassert_equal(lora_recv(lora_dev, buf, sizeof(buf), K_MSEC(50), NULL, NULL), -EAGAIN);
	zassert_true(hook.fired);
	workq_assert_responsive();
	radio_assert_rested();

	/* The next reception gets its own packet */
	rx_packet_queue(payload, -60, 10, false);
	zassert_equal(lora_recv(lora_dev, buf, sizeof(buf), K_SECONDS(1), NULL, NULL),
		      strlen(payload));
	zassert_mem_equal(buf, payload, strlen(payload));

	radio_assert_rested();
}

static void *sx126x_emul_setup(void)
{
	zassert_true(device_is_ready(lora_dev));
	boot_errors = sx126x_emul_errors_get(lora_emul);

	return NULL;
}

static void sx126x_emul_before(void *fixture)
{
	ARG_UNUSED(fixture);

	sx126x_emul_cmd_cb_set(lora_emul, NULL, NULL);
	sx126x_emul_errors_clear(lora_emul);
	sx126x_emul_stats_clear(lora_emul);
}

ZTEST_SUITE(sx126x_emul, NULL, sx126x_emul_setup, sx126x_emul_before, NULL, NULL);
