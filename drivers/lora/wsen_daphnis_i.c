/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_daphnis_i

/* Standard Includes */
#include <stdbool.h>

/* Zephyr RTOS includes */
#include <zephyr/drivers/lora.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>

/* WCON SDK includes */
#include <global/global.h>
#include <DaphnisI/DaphnisI.h>
#include <DaphnisI/ATCommands/P2P.h>
#include <DaphnisI/ATCommands/ATUserSettings.h>

LOG_MODULE_REGISTER(daphnis_i, CONFIG_LORA_LOG_LEVEL);

#define DAPHNISI_INST 0

/* Daphnis-I module */
#define DAPHNISI_CHANNEL_0_FREQ_HZ  (uint32_t)863000000
#define DAPHNISI_CHANNEL_SPACING_HZ (uint32_t)50000
#define DAPHNISI_MINIMUM_RF_CHANNEL (uint8_t)0
#define DAPHNISI_MAXIMUM_RF_CHANNEL (uint8_t)140
#define DAPHNISI_MINIMUM_TX_POWER   (uint8_t)0
#define DAPHNISI_MAXIMUM_TX_POWER   (uint8_t)14

#define DAPHNISI_SNR (int8_t)INT8_MIN

struct daphnisi_config {
	const struct device *uart;

	/* Pins */
	const struct gpio_dt_spec reset_gpio;
	const struct gpio_dt_spec wakeup_gpio;
	const struct gpio_dt_spec boot_gpio;

	/* User/Runtime Settings */
	const bool duty_cycle_restriction;
};

struct daphnisi_async_rx_data {
	lora_recv_cb async_rx_cb;
	void *async_user_data;
};

struct daphnisi_sync_rx_data {
	struct k_poll_signal *operation_done;
	uint8_t *buffer;
	uint8_t *size;
	int16_t *rssi;
	int8_t *snr;
};

struct daphnisi_data {
	WE_UART_t uart_def;
	DaphnisI_Pins_t pin_def;
	WE_UART_HandleRxByte_t *rx_byte_handler;

	/* Common Receive Data */
	uint8_t payload_buffer[DAPHNISI_P2P_MAX_PAYLOAD_SIZE];
	DaphnisI_P2P_RxData_t receive_data;

	/* Async Specific Receive Data */
	struct daphnisi_async_rx_data async_rx_data;

	/* Sync Specific Receive Data */
	struct daphnisi_sync_rx_data sync_rx_data;
};

typedef enum DaphnisI_Setting_Status_t {
	DaphnisI_Setting_Status_Unmodified = (1 << 0),
	DaphnisI_Setting_Status_Modified = (1 << 1),
	DaphnisI_Setting_Status_Failure = (1 << 2)
} DaphnisI_Setting_Status_t;

typedef struct DaphnisI_RF_Profile {
	enum lora_signal_bandwidth bandwidth;
	enum lora_datarate datarate;
} DaphnisI_RF_Profile;

/* UART functions */
static bool WE_UART_Init(uint32_t baudrate, WE_FlowControl_t flowControl, WE_Parity_t parity,
			 WE_UART_HandleRxByte_t *rxByteHandlerP);
static bool WE_UART_DeInit(void);
static void WE_UART_Receive(const struct device *dev, void *user_data);

/* Daphnis-I Init */
static int daphnisi_init(const struct device *dev);

/* Daphnis-I Helper functions*/
static bool DaphnisI_freq_to_channel(uint32_t frequency, uint8_t *rf_channel);
static DaphnisI_Setting_Status_t DaphnisI_Mode_Check_and_Set(DaphnisI_Mode_t new_mode);
static DaphnisI_Setting_Status_t DaphnisI_P2P_Role_Check_and_Set(DaphnisI_P2P_Role_t new_p2p_role);
static DaphnisI_Setting_Status_t DaphnisI_P2P_RF_Profile_Check_and_Set(uint8_t new_rf_profile);
static DaphnisI_Setting_Status_t
DaphnisI_P2P_Duty_Cycle_Restriction_Check_and_Set(bool new_duty_cycle_restriction);
static bool DaphnisI_Apply_Settings(DaphnisI_Setting_Status_t settings_statuses);

/* Daphnis-I LoRa API */
static int daphnisi_lora_config(const struct device *dev, struct lora_modem_config *config);
static int daphnisi_lora_send(const struct device *dev, uint8_t *data, uint32_t data_len);
static int daphnisi_lora_send_async(const struct device *dev, uint8_t *data, uint32_t data_len,
				    struct k_poll_signal *async);
static int daphnisi_lora_recv(const struct device *dev, uint8_t *data, uint8_t size,
			      k_timeout_t timeout, int16_t *rssi, int8_t *snr);
static int daphnisi_lora_recv_async(const struct device *dev, lora_recv_cb cb, void *user_data);

/* Event Callback Listener*/
static void DaphnisI_P2P_EventCallback(DaphnisI_ATEvent_t event, char *eventText);

static const struct daphnisi_config daphnisi_config = {
	.uart = DEVICE_DT_GET(DT_INST_PARENT(DAPHNISI_INST)),
	.reset_gpio = GPIO_DT_SPEC_INST_GET(DAPHNISI_INST, reset_gpios),
	.wakeup_gpio = GPIO_DT_SPEC_INST_GET(DAPHNISI_INST, wakeup_gpios),
	.boot_gpio = GPIO_DT_SPEC_INST_GET(DAPHNISI_INST, boot_gpios),
	.duty_cycle_restriction = DT_INST_PROP(DAPHNISI_INST, duty_cycle_restriction)};

static struct daphnisi_data daphnisi_data = {
	.receive_data = {.data = daphnisi_data.payload_buffer}};

static const DaphnisI_RF_Profile DaphnisI_rf_profiles[] = {
	{.bandwidth = BW_125_KHZ, .datarate = SF_12}, {.bandwidth = BW_125_KHZ, .datarate = SF_11},
	{.bandwidth = BW_125_KHZ, .datarate = SF_10}, {.bandwidth = BW_125_KHZ, .datarate = SF_9},
	{.bandwidth = BW_125_KHZ, .datarate = SF_8},  {.bandwidth = BW_125_KHZ, .datarate = SF_7},
	{.bandwidth = BW_250_KHZ, .datarate = SF_7},
};

#define DAPHNISI_RF_PROFILE_COUNT (sizeof(DaphnisI_rf_profiles) / sizeof(DaphnisI_RF_Profile))

static bool WE_UART_Init(uint32_t baudrate, WE_FlowControl_t flowControl, WE_Parity_t parity,
			 WE_UART_HandleRxByte_t *rxByteHandlerP)
{
	ARG_UNUSED(baudrate);
	ARG_UNUSED(flowControl);
	ARG_UNUSED(parity);
	daphnisi_data.rx_byte_handler = rxByteHandlerP;
	return true;
}

static bool WE_UART_DeInit(void)
{
	return true;
}

static bool WE_UART_Transmit(const uint8_t *data, uint16_t length)
{
	for (uint16_t i = 0; i < length; i++) {
		uart_poll_out(daphnisi_config.uart, data[i]);
	}
	return true;
}

static void WE_UART_Receive(const struct device *dev, void *user_data)
{
	uint8_t c;

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {

			int bytes_read;

			do {
				bytes_read = uart_fifo_read(dev, &c, 1);
				for (int i = 0; i < bytes_read; i++) {
					(*daphnisi_data.rx_byte_handler)(&c, 1);
				}
			} while (bytes_read > 0);
		}
	}
}

static DaphnisI_Setting_Status_t DaphnisI_Mode_Check_and_Set(DaphnisI_Mode_t new_mode)
{
	DaphnisI_Mode_t current_mode;

	if (!DaphnisI_GetModeRS(&current_mode)) {
		return DaphnisI_Setting_Status_Failure;
	}

	if (current_mode == new_mode) {
		return DaphnisI_Setting_Status_Unmodified;
	}

	if (!DaphnisI_SetModeUS(new_mode)) {
		return DaphnisI_Setting_Status_Failure;
	}

	return DaphnisI_Setting_Status_Modified;
}

static DaphnisI_Setting_Status_t DaphnisI_P2P_Role_Check_and_Set(DaphnisI_P2P_Role_t new_p2p_role)
{
	DaphnisI_P2P_Role_t current_p2p_role;

	if (!DaphnisI_GetP2PRoleRS(&current_p2p_role)) {
		return DaphnisI_Setting_Status_Failure;
	}

	if (current_p2p_role == new_p2p_role) {
		return DaphnisI_Setting_Status_Unmodified;
	}

	if (!DaphnisI_SetP2PRoleUS(new_p2p_role)) {
		return DaphnisI_Setting_Status_Failure;
	}

	return DaphnisI_Setting_Status_Modified;
}

static DaphnisI_Setting_Status_t DaphnisI_P2P_RF_Profile_Check_and_Set(uint8_t new_rf_profile)
{
	uint8_t current_rf_profile;

	if (!DaphnisI_GetP2PRFProfileRS(&current_rf_profile)) {
		return DaphnisI_Setting_Status_Failure;
	}

	if (current_rf_profile == new_rf_profile) {
		return DaphnisI_Setting_Status_Unmodified;
	}

	if (!DaphnisI_SetP2PRFProfileUS(new_rf_profile)) {
		return DaphnisI_Setting_Status_Failure;
	}

	return DaphnisI_Setting_Status_Modified;
}

static DaphnisI_Setting_Status_t
DaphnisI_P2P_Duty_Cycle_Restriction_Check_and_Set(bool new_duty_cycle_restriction)
{
	bool current_duty_cycle_restriction;

	if (!DaphnisI_GetP2PDutyCycleEnforceRS(&current_duty_cycle_restriction)) {
		return DaphnisI_Setting_Status_Failure;
	}

	if (current_duty_cycle_restriction == new_duty_cycle_restriction) {
		return DaphnisI_Setting_Status_Unmodified;
	}

	if (!DaphnisI_SetP2PDutyCycleEnforceUS(new_duty_cycle_restriction)) {
		return DaphnisI_Setting_Status_Failure;
	}

	return DaphnisI_Setting_Status_Modified;
}

static bool DaphnisI_Apply_Settings(DaphnisI_Setting_Status_t settings_statuses)
{
	if ((settings_statuses & DaphnisI_Setting_Status_Failure) ==
	    DaphnisI_Setting_Status_Failure) {
		LOG_DBG("One of the User/Runtime Settings couldn't be read or set.\r\n");
		return false;
	}

	if ((settings_statuses & DaphnisI_Setting_Status_Modified) ==
	    DaphnisI_Setting_Status_Modified) {
		if (!DaphnisI_PinReset()) {
			LOG_DBG("Failed to reset module.\r\n");
			return false;
		}
	}

	return true;
}

static bool DaphnisI_freq_to_channel(uint32_t frequency, uint8_t *rf_channel)
{
	if (rf_channel == NULL) {
		return false;
	}

	uint32_t offset = frequency - DAPHNISI_CHANNEL_0_FREQ_HZ;

	if (offset % DAPHNISI_CHANNEL_SPACING_HZ != 0) {
		return false; /* Not a valid channel frequency */
	}

	*rf_channel = (uint8_t)(offset / DAPHNISI_CHANNEL_SPACING_HZ);

	return (*rf_channel >= DAPHNISI_MINIMUM_RF_CHANNEL &&
		*rf_channel <= DAPHNISI_MAXIMUM_RF_CHANNEL);
}

static int daphnisi_lora_config(const struct device *dev, struct lora_modem_config *config)
{
	/* Check frequency */
	uint8_t rf_channel;

	if (!DaphnisI_freq_to_channel(config->frequency, &rf_channel)) {
		return -ENOTSUP;
	}

	/* Check rf profile */
	uint8_t rf_profile = DAPHNISI_RF_PROFILE_COUNT;

	for (uint8_t i = 0; i < DAPHNISI_RF_PROFILE_COUNT; i++) {
		if (DaphnisI_rf_profiles[i].bandwidth != config->bandwidth) {
			continue;
		}

		if (DaphnisI_rf_profiles[i].datarate != config->datarate) {
			continue;
		}

		rf_profile = i;
	}

	if (rf_profile >= DAPHNISI_RF_PROFILE_COUNT) {
		return -ENOTSUP;
	}

	if (config->coding_rate != CR_4_5) {
		return -ENOTSUP;
	}

	if (config->preamble_len != 8) {
		return -ENOTSUP;
	}

	if (config->iq_inverted) {
		return -ENOTSUP;
	}

	if (config->public_network) {
		return -ENOTSUP;
	}

	if (config->tx) {
		if ((config->tx_power < DAPHNISI_MINIMUM_TX_POWER) ||
		    (config->tx_power > DAPHNISI_MAXIMUM_TX_POWER)) {
			return -ENOTSUP;
		}
	}

	DaphnisI_Setting_Status_t setting_status = DaphnisI_Setting_Status_Unmodified;

	setting_status |= DaphnisI_P2P_RF_Profile_Check_and_Set(rf_profile);

	if (!DaphnisI_Apply_Settings(setting_status)) {
		return -EIO;
	}

	if (!DaphnisI_SetP2PRFChannelRS(rf_channel)) {
		return -EIO;
	}

	if (config->tx) {
		if (!DaphnisI_SetP2PTXPowerRS((uint8_t)config->tx_power)) {
			return -EIO;
		}
	}

	return 0;
}

/**
 * @brief Is called when an event notification has been received.
 *
 * Note that this function is called from an interrupt - code in this function
 * should thus be kept simple.
 *
 * Note in particular that it is not possible to send AT commands to Daphnis
 * from within this event handler.
 *
 * Also note that not all calls of this handler necessarily correspond to valid
 * events (i.e. events from DaphnisI_ATEvent_t). Some events might in fact be responses
 * to AT commands that are not included in DaphnisI_ATEvent_t.
 */
static void DaphnisI_P2P_EventCallback(DaphnisI_ATEvent_t event, char *eventText)
{
	switch (event) {
	case DaphnisI_ATEvent_P2P_RxData: {
		if (!DaphnisI_P2P_ParseRXDataEvent(&eventText, &daphnisi_data.receive_data)) {
			return;
		}

		if (daphnisi_data.async_rx_data.async_rx_cb) {
			daphnisi_data.async_rx_data.async_rx_cb(
				DEVICE_DT_INST_GET(DAPHNISI_INST), daphnisi_data.receive_data.data,
				daphnisi_data.receive_data.data_length,
				daphnisi_data.receive_data.RSSI, DAPHNISI_SNR,
				daphnisi_data.async_rx_data.async_user_data);

			return;
		}

		if (daphnisi_data.receive_data.data_length < *daphnisi_data.sync_rx_data.size) {
			*daphnisi_data.sync_rx_data.size = daphnisi_data.receive_data.data_length;
		}
		/* Copy received data to output buffer */
		memcpy(daphnisi_data.sync_rx_data.buffer, daphnisi_data.receive_data.data,
		       *daphnisi_data.sync_rx_data.size);
		/* Output RSSI and SNR */
		if (daphnisi_data.sync_rx_data.rssi) {
			*daphnisi_data.sync_rx_data.rssi = daphnisi_data.receive_data.RSSI;
		}

		if (daphnisi_data.sync_rx_data.snr) {
			*daphnisi_data.sync_rx_data.snr = DAPHNISI_SNR;
		}

		struct k_poll_signal *sig = daphnisi_data.sync_rx_data.operation_done;

		daphnisi_data.sync_rx_data.operation_done = NULL;
		/* Notify caller RX is complete */
		k_poll_signal_raise(sig, 0);

		break;
	}
	default:
		break;
	}
}

static int daphnisi_lora_send(const struct device *dev, uint8_t *data, uint32_t data_len)
{

	if (!DaphnisI_P2P_TransmitBroadcast(data, (uint16_t)data_len)) {
		return -EIO;
	}

	return 0;
}

static int daphnisi_lora_send_async(const struct device *dev, uint8_t *data, uint32_t data_len,
				    struct k_poll_signal *async)
{
	return -ENOTSUP;
}

static int daphnisi_lora_recv(const struct device *dev, uint8_t *data, uint8_t size,
			      k_timeout_t timeout, int16_t *rssi, int8_t *snr)
{
	struct k_poll_signal done = K_POLL_SIGNAL_INITIALIZER(done);
	struct k_poll_event evt =
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &done);
	int ret;

	struct daphnisi_data *dev_data = dev->data;

	dev_data->async_rx_data.async_rx_cb = NULL;
	/* Store operation signal */
	dev_data->sync_rx_data.operation_done = &done;
	/* Set data output location */
	dev_data->sync_rx_data.buffer = data;
	dev_data->sync_rx_data.size = &size;
	dev_data->sync_rx_data.rssi = rssi;
	dev_data->sync_rx_data.snr = snr;

	/* Start reception */
	if (!DaphnisI_P2P_SetRXEnabled(true)) {
		return -EIO;
	}

	ret = k_poll(&evt, 1, timeout);

	if (!DaphnisI_P2P_SetRXEnabled(false)) {
		return -EIO;
	}

	return (ret < 0) ? ret : size;
}

static int daphnisi_lora_recv_async(const struct device *dev, lora_recv_cb cb, void *user_data)
{
	/* Cancel ongoing reception */
	if (cb == NULL) {

		if (!DaphnisI_P2P_SetRXEnabled(false)) {
			return -EIO;
		}

		return 0;
	}

	struct daphnisi_data *data = dev->data;

	/* Store parameters */
	data->async_rx_data.async_rx_cb = cb;
	data->async_rx_data.async_user_data = user_data;

	/* Start reception */
	if (!DaphnisI_P2P_SetRXEnabled(true)) {
		return -EIO;
	}

	return 0;
}

static int daphnisi_init(const struct device *dev)
{
	LOG_DBG("Initializing Daphnis-I");

	const struct daphnisi_config *const config = dev->config;
	struct daphnisi_data *data = dev->data;

	if (!device_is_ready(config->uart)) {
		LOG_ERR("UART device not ready\n");
		return -EIO;
	}

	if (uart_irq_callback_user_data_set(config->uart, WE_UART_Receive, NULL) < 0) {
		return -EIO;
	}

	uart_irq_rx_enable(config->uart);

	data->uart_def.baudrate = DAPHNISI_DEFAULT_BAUDRATE;
	data->uart_def.flowControl = WE_FlowControl_NoFlowControl;
	data->uart_def.parity = WE_Parity_None;

	data->uart_def.uartInit = WE_UART_Init;
	data->uart_def.uartDeinit = WE_UART_DeInit;
	data->uart_def.uartTransmit = WE_UART_Transmit;

	data->pin_def.DaphnisI_Pin_Reset = WE_PIN((void *)&config->reset_gpio);
	data->pin_def.DaphnisI_Pin_Boot = WE_PIN((void *)&config->boot_gpio);
	data->pin_def.DaphnisI_Pin_WakeUp = WE_PIN((void *)&config->wakeup_gpio);

	if (!DaphnisI_Init(&data->uart_def, &data->pin_def, DaphnisI_P2P_EventCallback)) {
		return -EIO;
	}

	DaphnisI_Setting_Status_t setting_status = DaphnisI_Setting_Status_Unmodified;

	setting_status |= DaphnisI_Mode_Check_and_Set(DaphnisI_Mode_P2P);
	setting_status |= DaphnisI_P2P_Role_Check_and_Set(DaphnisI_P2P_Role_Transceiver);
	setting_status |=
		DaphnisI_P2P_Duty_Cycle_Restriction_Check_and_Set(config->duty_cycle_restriction);

	if (!DaphnisI_Apply_Settings(setting_status)) {
		return -EIO;
	}

	LOG_DBG("Daphnis-I initialized successfully");

	return 0;
}

static DEVICE_API(lora, daphnisi_lora_api) = {
	.config = daphnisi_lora_config,
	.send = daphnisi_lora_send,
	.send_async = daphnisi_lora_send_async,
	.recv = daphnisi_lora_recv,
	.recv_async = daphnisi_lora_recv_async,
	.test_cw = NULL,
};

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) != 1
#error "Expected exactly one instance with status 'okay'"
#endif

DEVICE_DT_INST_DEFINE(DAPHNISI_INST, &daphnisi_init, NULL, &daphnisi_data, &daphnisi_config,
		      POST_KERNEL, CONFIG_LORA_INIT_PRIORITY, &daphnisi_lora_api)
