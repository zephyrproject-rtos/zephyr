/*
 * Copyright (c) 2026, Shontal Biton
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #define DT_DRV_COMPAT silabs_series2_radio

#include <stdio.h>
#include <zephyr/drivers/radio.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "xg28_rb4401c/rail_config.h"
// #include "pa_conversions_efr32.h" TODO

LOG_MODULE_REGISTER(radio_silabs, CONFIG_RADIO_LOG_LEVEL);

/* !!! These will be configs in the future !!! */
#define PACKET_BUFFER_SIZE (1024)

enum event_type {

	/* RX ended successfully */
	RX_DONE,

	/* Error accured while trying to receive data */
	RX_ERROR,

	/* Packet failed because of CRC */
	RX_ERROR_CRC,

	/* TX ended successfully */
	TX_DONE,

	/* Error accured while trying to send data */
	TX_ERROR,

	/* RSSI ended successfully, the data can be collected */
	RSSI_DONE,

	/* Number of events exist */
	NUMBER_OF_EVENTS
};

typedef enum raido_status {
    RADIO_SUCCESS = 0,
    RADIO_CONFIG_CHANNEL_FAILED,
	RADIO_START_CW_FAILED,
	RADIO_STOP_CW_FAILED,
	RADIO_WRITE_TX_FIFO_FAILED,
	RADIO_TX_CREATE_PACKET_FAILED,
	RADIO_START_TX_FAILED,
	RADIO_START_RX_FAILED,
	RADIO_SET_TX_POWER_FAILED,
	RADIO_CONFIG_RX_FAILED,
} radio_status_t;

typedef struct radio_event {

	/* The event that accured */
	enum event_type event;

	/* The data received if exist */
	uint8_t* data;

	/* Data buffer size */
	int data_size;

	/* RSSI of the packet */
	int16_t rssi;
} radio_event;

typedef void(*radio_event_callback)(struct radio_event* event);

struct radio_data {
	/* RAIL handle instance */
	RAIL_Handle_t rail_handle;

	/* Event for the user to use in callback*/
	radio_event event;

	/* The user callback to be called on events */
	radio_event_callback callback;

	/* The radio configuration to use */
	uint32_t configuration;

	/* The radio channel to use */
	uint16_t channel;

	/* Buffer to hold the variable length packet (packet and size) */
	uint8_t payload_to_send[CONFIG_RADIO_SILABS_TX_BUFFER_SIZE];

	/* Buffer to hold the received packet */
	uint8_t received_packet_buffer[CONFIG_RADIO_SILABS_RX_BUFFER_SIZE];

	/* Buffer given for RAIL to use for transmiting */
	uint8_t TX_buffer[CONFIG_RADIO_SILABS_TX_BUFFER_SIZE] __aligned(4);

	/* Buffers given for RAIL to use for receiving */
	uint8_t RX_buffer[CONFIG_RADIO_SILABS_RX_BUFFER_SIZE] __aligned(4);

	RAIL_ChannelConfigEntry_t channel_config_entry;
	RAIL_ChannelConfig_t channel_config;
} __packed;

#define VARIABLE_LENGTH_LENGTH_FIELD_SIZE (2)

#ifdef CONFIG_RADIO_SILABS
	/**
	 * From silabs rail efr32 documentation: https://docs.silabs.com/rail/latest/rail-api/efr32-main
	 * The chosen buffer size limits the maximum size of receive packets in packet mode and determines the size of the receive
	 * FIFO in FIFO mode. Because each receive packet has several bytes of overhead,
	 * you can only receive up to one (buffer size - overhead) byte packet without switching to FIFO mode.
	 * In FIFO mode, you must read out packet data as you approach this limit and store it off to construct the full packet later.
	 * This overhead is currently 8 bytes on all EFR32 Series-2 platforms. Note that this overhead may increase or decrease
	 * in future releases as the functionality is changed though large jumps are not expected in either direction.
	 *
	 * The RAIL overhead is added to each packet in RX FIFO
	 *
	 * Payload size most be smaller than FIFO_SIZE - EFR32_PACKET_STORAGE_OVERHEAD(8 bytes) - SIZE_FIELD_LENGTH(2 bytes)
	 */
	#define RADIO_SILABS_PACKET_STORAGE_OVERHEAD (8)
#else
	#define RADIO_SILABS_PACKET_STORAGE_OVERHEAD (0)
#endif

/**
 * This is the size of the actual data from the user. In the
 * driver, the payload size (2 bytes) and rail overhead (8 bytes) are added to the payload.
 */
#define RADIO_SILABS_MAX_PACKET_LENGTH (CONFIG_RADIO_SILABS_TX_BUFFER_SIZE - RADIO_SILABS_PACKET_STORAGE_OVERHEAD - VARIABLE_LENGTH_LENGTH_FIELD_SIZE)

typedef enum communication_status {

	/* Status default value */
	UNINITIALIZED = -1,

	/* The packet successfully sent */
	RX_SUCCESS,

	/* packet received size larger then the RX buffer size */
	RX_PACKET_TOO_LARGE,

	/* The RX rail handler invalid */
	RX_HANDLER_INVALID,

} communication_status_t;

/**
 * Send continuous wave in the channel configured in the driver
 *
 * @return Status code indicates error.
 *
 */
int radio_silabs_start_cw(const struct device* dev)
{
	RAIL_Status_t status;
	struct radio_data* data = (struct radio_data*)dev->data;

	status = RAIL_StartTxStream(data->rail_handle, data->channel, RAIL_STREAM_CARRIER_WAVE);
	if (status != RAIL_STATUS_NO_ERROR)
	{
		return RADIO_START_CW_FAILED;
	}

	return RADIO_SUCCESS;
}

/**
 * Stop the continuous wave send.
 *
 * @return Status code indicates error.
 *
 */
int radio_silabs_stop_cw(const struct device* dev)
{
	RAIL_Status_t status;
	struct radio_data* data = (struct radio_data*)dev->data;

	status = RAIL_StopTxStream(data->rail_handle);
	if (status != RAIL_STATUS_NO_ERROR)
	{
		return RADIO_STOP_CW_FAILED;
	}

	return RADIO_SUCCESS;
}

/**
 * In RAIL variable length mode, we can create packets in different
 * sizes. Variable length packet format requires the first 2 bytes to be the payload length
 * and after that the data is inserted.
 * The function get the payload and size, and create the packet buffer in the correct variable
 * length format.
 *
 * @param[in,out] payload_to_send A buffer for the variable length packet buffer.
 * @param[in] payload The data to send.
 * @param[in] len Payload size.
 *
 *
 * @return Status code indicates error.
 *
 */
static int create_radio_packet(uint8_t* payload_to_send, const uint8_t* payload, uint16_t len)
{
	*(uint16_t*)payload_to_send = sys_cpu_to_le16(len);
	void* ret = memcpy(payload_to_send + VARIABLE_LENGTH_LENGTH_FIELD_SIZE, payload, len);
	if (ret == NULL)
	{
		return RADIO_TX_CREATE_PACKET_FAILED;
	}

	return RADIO_SUCCESS;
}

/**
 * Transmits the given data through the given channel.
 *
 * @param[in] channel The channel to use.
 * @param[in] payload The data to send.
 * @param[in] len Payload size.
 * @param[in] clear_fifo Whether to clear the transmit FIFO before inserting new data.
 * (NOT recommended if previous data may still exist)
 *
 * @return Status code indicates error.
 *
 */
int radio_silabs_send(const struct device *dev, uint16_t channel, const uint8_t* payload, int len, bool clear_fifo)
{
	RAIL_Status_t status;
	int ret;
	struct radio_data* data = (struct radio_data*)dev->data;

	if (len > RADIO_SILABS_MAX_PACKET_LENGTH || len <= 0)
	{
		LOG_ERR("radio_send() invalid payload length %d", len);
		return RADIO_TX_CREATE_PACKET_FAILED;
	}

	ret = create_radio_packet(data->payload_to_send, payload, len);
	if (ret != RADIO_SUCCESS)
	{
		return RADIO_TX_CREATE_PACKET_FAILED;
	}

	ret = RAIL_WriteTxFifo(data->rail_handle, data->payload_to_send, len + VARIABLE_LENGTH_LENGTH_FIELD_SIZE, clear_fifo);
	if (ret != len + VARIABLE_LENGTH_LENGTH_FIELD_SIZE) {
		LOG_ERR("RAIL_WriteTxFifo(): %d", ret);
		return RADIO_WRITE_TX_FIFO_FAILED;
	}

	status = RAIL_StartTx(data->rail_handle, channel, RAIL_TX_OPTIONS_DEFAULT, NULL);
	if (status) {
		LOG_ERR("RAIL_StartTx(): %d ", status);
		return RADIO_START_TX_FAILED;
	}
	LOG_HEXDUMP_INF(data->payload_to_send, len + VARIABLE_LENGTH_LENGTH_FIELD_SIZE, "tx data:");

	return RADIO_SUCCESS;
}

int handle_rx_packets(const struct device *dev, RAIL_Handle_t rail_handle)
{
	RAIL_RxPacketHandle_t handle;
	RAIL_RxPacketInfo_t info;
	RAIL_RxPacketDetails_t details;
	RAIL_Status_t status;
	struct radio_data* data = (struct radio_data*)dev->data;

	handle = RAIL_GetRxPacketInfo(rail_handle, RAIL_RX_PACKET_HANDLE_OLDEST_COMPLETE,
						&info);
	if (handle == RAIL_RX_PACKET_HANDLE_INVALID) {
		return RX_HANDLER_INVALID;
	}

	if (info.packetBytes < sizeof(data->received_packet_buffer)) {
		RAIL_CopyRxPacket(data->received_packet_buffer, &info);
	}

	status = RAIL_GetRxPacketDetails(rail_handle, RAIL_RX_PACKET_HANDLE_OLDEST_COMPLETE, &details);
	if (status) {
		return RX_HANDLER_INVALID;
	}

	data->event.rssi = details.rssi;

	status = RAIL_ReleaseRxPacket(rail_handle, handle);
	if (status) {
		LOG_ERR("RAIL_ReleaseRxPacket(): %d", status);
	}
	if (info.packetBytes >= sizeof(data->received_packet_buffer)) {
		return RX_PACKET_TOO_LARGE;
	}

	return RX_SUCCESS;
}

/**
 * Set the frequency of the radio driver channel.
 *
 * @param[in] frequency The new frequency to set.
 *
 * @return Status code indicates error.
 *
 */
int radio_silabs_set_frequency(const struct device *dev, uint32_t frequency)
{
	struct radio_data* data = (struct radio_data*)dev->data;

	uint16_t status = RAIL_OverrideDebugFrequency(data->rail_handle, frequency);
	if (status < 0) {
		LOG_ERR("Failed to set frequency: %d", status);
	}

	return RADIO_SUCCESS;
}

int radio_silabs_set_tx_power(const struct device *dev, int16_t dbm)
{
	struct radio_data* data = (struct radio_data*)dev->data;

	RAIL_TxPowerLevel_t raw_power = RAIL_ConvertDbmToRaw(data->rail_handle, RAIL_TX_POWER_MODE_SUBGIG_HP, dbm * 10);
	RAIL_Status_t status = RAIL_SetTxPower(data->rail_handle, raw_power);

	if (status) {
		return RADIO_SET_TX_POWER_FAILED;
	}

	return RADIO_SUCCESS;
}

static void rail_on_rf_ready(RAIL_Handle_t rail_handle)
{
	LOG_INF("radio is ready %p", rail_handle);
}

/**
 * Radio initialization function.
 * This function init all the default radio configurations.
 *
 * @param[in] radio_dev The radio device object.
 *
 * @return 0 when the funcion is over.
 *
 */
static int radio_silabs_init(const struct device* dev)
{
	struct radio_data* data = (struct radio_data*)dev->data;

	// sl_rail_util_pa_init(); TODO

	RAIL_Config_t rail_config = {};
	RAIL_DataConfig_t data_config = {
		.txSource = TX_PACKET_DATA,
		.rxSource = RX_PACKET_DATA,
		.txMethod = PACKET_MODE,
		.rxMethod = PACKET_MODE,
	};
	RAIL_StateTransitions_t transitions = {
		.success = RAIL_RF_STATE_RX,
		.error   = RAIL_RF_STATE_RX,
	};

	RAIL_Status_t status;
	int ret;
	uint16_t RX_buffer_size = CONFIG_RADIO_SILABS_RX_BUFFER_SIZE;

	data->rail_handle = RAIL_Init(&rail_config, &rail_on_rf_ready);
	if (!data->rail_handle) {
		LOG_ERR("RAIL_Init() failed");
	}
	LOG_INF("*** rail handle in init: %p data: %p ***\n", &data->rail_handle, data);

	status = RAIL_ConfigData(data->rail_handle, &data_config);
	if (status) {
		LOG_ERR("RAIL_ConfigData(): %d", status);
	}
	status = RAIL_SetPtiProtocol(data->rail_handle, RAIL_PTI_PROTOCOL_CUSTOM);
	if (status) {
		LOG_ERR("RAIL_SetPtiProtocol(): %d", status);
	}
	status = RAIL_ConfigCal(data->rail_handle, RAIL_CAL_TEMP | RAIL_CAL_ONETIME);
	if (status) {
		LOG_ERR("RAIL_ConfigCal(): %d", status);
	}
	status = RAIL_ConfigEvents(data->rail_handle, RAIL_EVENTS_ALL,
				   RAIL_EVENTS_RX_COMPLETION |
				   RAIL_EVENTS_TX_COMPLETION |
				   RAIL_EVENTS_TXACK_COMPLETION |
				   RAIL_EVENT_CAL_NEEDED);
	if (status) {
		LOG_ERR("RAIL_ConfigEvents(): %d", status);
	}
	status = RAIL_SetRxTransitions(data->rail_handle, &transitions);
	if (status) {
		LOG_ERR("RAIL_SetRxTransitions(): %d", status);
	}
	ret = RAIL_SetTxFifo(data->rail_handle, data->TX_buffer, 0, CONFIG_RADIO_SILABS_TX_BUFFER_SIZE);
	if (ret != CONFIG_RADIO_SILABS_TX_BUFFER_SIZE) {
		LOG_ERR("RAIL_SetTxFifo(): %d != %d", ret, CONFIG_RADIO_SILABS_TX_BUFFER_SIZE);
	}
	status = RAIL_SetRxFifo(data->rail_handle, &(data->RX_buffer)[0], &RX_buffer_size);
	if (status || RX_buffer_size != CONFIG_RADIO_SILABS_RX_BUFFER_SIZE) {
		LOG_ERR("RAIL_SetRxFifo(): status: %d, sizes: %d != %d", status, RX_buffer_size, CONFIG_RADIO_SILABS_RX_BUFFER_SIZE);
	}

	RAIL_TxPowerConfig_t pa_config = {
		.mode = RAIL_TX_POWER_MODE_SUBGIG_HP,
		.voltage = 3300,
		.rampTime = 10,
	};

	status = RAIL_ConfigTxPower(data->rail_handle, &pa_config);
	if (status) {
		LOG_ERR("Failed to config tx power, %d", status);
		return RADIO_START_TX_FAILED;
	}

	return 0;
}

static DEVICE_API(radio, radio_silabs_driver_api) = {
	.start_cw = radio_silabs_start_cw,
	.stop_cw = radio_silabs_stop_cw,
	.send = radio_silabs_send,
	.set_frequency = radio_silabs_set_frequency,
	.set_tx_power = radio_silabs_set_tx_power,
};

#define RADIO_SILABS_DEFINE(inst)                                   \
	static struct radio_data radio_##inst##_data = {0};            \
                                                                   \
    DEVICE_DT_INST_DEFINE(inst,                                    \
                  radio_silabs_init,                               \
                  NULL,                                            \
                  &radio_##inst##_data,                            \
                  NULL,                                       	   \
                  POST_KERNEL,                                     \
                  CONFIG_RADIO_INIT_PRIORITY,                      \
                  &radio_silabs_driver_api);					   \


DT_INST_FOREACH_STATUS_OKAY(RADIO_SILABS_DEFINE)
