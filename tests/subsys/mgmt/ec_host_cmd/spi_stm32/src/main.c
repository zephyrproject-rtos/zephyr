/*
 * Copyright (c) 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stm32_ll_spi.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/mgmt/ec_host_cmd/backend.h>
#include <zephyr/mgmt/ec_host_cmd/ec_host_cmd.h>
#include <zephyr/ztest.h>

#define CS_PIN             0U
#define CMD_TEST_ID        0x0020
#define EC_SPI_RX_READY    0x78
#define EC_SPI_PROCESSING  0xfa

uint8_t mock_spi_last_status;
int mock_rx_reload_count;
dma_callback_t mock_tx_dma_cb;
void *mock_tx_dma_user_data;
struct k_sem tx_dma_started_sem;

static const struct device *const cs_gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
static int rx_reload_count_in_handler;
static uint8_t status_after_handler_cs_assert;
static bool toggle_cs_in_handler;

static enum ec_host_cmd_status test_cmd_handler(struct ec_host_cmd_handler_args *args)
{
	const uint8_t *req = args->input_buf;
	uint8_t *resp = args->output_buf;

	if (toggle_cs_in_handler) {
		/* Deassert CS, then unexpectedly assert and deassert CS during PROCESSING. */
		gpio_emul_input_set(cs_gpio_dev, CS_PIN, 1);
		gpio_emul_input_set(cs_gpio_dev, CS_PIN, 0);
		status_after_handler_cs_assert = mock_spi_last_status;
		gpio_emul_input_set(cs_gpio_dev, CS_PIN, 1);
		rx_reload_count_in_handler = mock_rx_reload_count;
	}

	*resp = (uint8_t)(*req * 2U);
	args->output_buf_size = sizeof(*resp);
	return EC_HOST_CMD_SUCCESS;
}
EC_HOST_CMD_HANDLER(CMD_TEST_ID, test_cmd_handler, BIT(0), uint8_t, uint8_t);

static void send_spi_request(uint8_t val)
{
	struct {
		struct ec_host_cmd_request_header hdr;
		uint8_t val;
	} __packed pkt = {
		.hdr = {
			.prtcl_ver = 3,
			.checksum = (uint8_t)(-(0x24U + val)),
			.cmd_id = CMD_TEST_ID,
			.data_len = sizeof(uint8_t),
		},
		.val = val,
	};

	memcpy(ec_host_cmd_get_hc()->rx_ctx.buf, &pkt, sizeof(pkt));
	gpio_emul_input_set(cs_gpio_dev, CS_PIN, 0);
}

static void verify_tx_complete_and_response(uint8_t expected_val)
{
	const struct ec_host_cmd_response_header *resp_hdr =
		(const struct ec_host_cmd_response_header *)ec_host_cmd_get_hc()->tx.buf;
	const uint8_t *resp_val = (const uint8_t *)resp_hdr + sizeof(*resp_hdr);

	mock_tx_dma_cb(cs_gpio_dev, mock_tx_dma_user_data, MOCK_DMA_CH_tx, 0);
	zassert_equal(mock_rx_reload_count, 1, "RX DMA must re-arm once TX DMA finishes");
	zassert_equal(mock_spi_last_status, EC_SPI_RX_READY);
	zassert_equal(resp_hdr->result, EC_HOST_CMD_SUCCESS);
	zassert_equal(*resp_val, expected_val);
}

static void *spi_test_setup(void)
{
	struct gpio_dt_spec cs = { .port = cs_gpio_dev, .pin = CS_PIN };

	k_sem_init(&tx_dma_started_sem, 0, 1);
	zassert_ok(gpio_pin_configure(cs_gpio_dev, CS_PIN, GPIO_INPUT));
	zassert_ok(gpio_emul_input_set(cs_gpio_dev, CS_PIN, 1));
	zassert_ok(ec_host_cmd_init(ec_host_cmd_backend_get_spi(&cs)));
	return NULL;
}

static void spi_test_before(void *fixture)
{
	ARG_UNUSED(fixture);
	toggle_cs_in_handler = false;
	k_sem_reset(&tx_dma_started_sem);
	gpio_emul_input_set(cs_gpio_dev, CS_PIN, 1);
	mock_rx_reload_count = 0;
}

ZTEST(ec_host_cmd_spi_stm32, test_spi_unexpected_cs_toggles_during_processing)
{
	const uint8_t req_val = 0x12;

	toggle_cs_in_handler = true;
	send_spi_request(req_val);

	zassert_ok(k_sem_take(&tx_dma_started_sem, K_SECONDS(1)));
	zassert_equal(status_after_handler_cs_assert, EC_SPI_PROCESSING,
		      "Unexpected CS assert during PROCESSING must not clobber TX status");
	zassert_equal(rx_reload_count_in_handler, 0,
		      "CS deassert after unexpected CS assert must not re-arm RX DMA in PROCESSING");
	zassert_equal(mock_rx_reload_count, 0);
	verify_tx_complete_and_response(req_val * 2U);
}

ZTEST(ec_host_cmd_spi_stm32, test_spi_unexpected_cs_toggles_during_sending)
{
	const uint8_t req_val = 0x25;

	send_spi_request(req_val);
	zassert_ok(k_sem_take(&tx_dma_started_sem, K_SECONDS(1)));

	/* Backend is now in SENDING state. Toggle CS deassert -> assert -> deassert. */
	gpio_emul_input_set(cs_gpio_dev, CS_PIN, 1);
	zassert_equal(mock_rx_reload_count, 0, "CS deassert in SENDING must defer RX re-arm");
	gpio_emul_input_set(cs_gpio_dev, CS_PIN, 0);
	gpio_emul_input_set(cs_gpio_dev, CS_PIN, 1);
	zassert_equal(mock_rx_reload_count, 0,
		      "Unexpected CS toggle in SENDING must not re-arm RX before TX DMA finishes");
	verify_tx_complete_and_response(req_val * 2U);
}

ZTEST_SUITE(ec_host_cmd_spi_stm32, NULL, spi_test_setup, spi_test_before, NULL, NULL);
