/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2024 NXP
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include "opendroneid.h"

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define BT_LE_ADV_PARAM_NCONN BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY, 0x20, 0x20, NULL)

static uint8_t payload[29] = {
	0xFA, 0xFF, /* 0xFFFA = ASTM International, ASTM Remote ID. */
	0x0D,       /* AD Application Code within the ASTM address space = ODID. */
	0x00,       /* message counter starting at 0x00 and wrapping around at 0xFF. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 /* 25-bytes data */
};

static const struct bt_data ad[] = {
	{
		.type = BT_DATA_SVC_DATA16,
		.data_len = ARRAY_SIZE(payload),
		.data = payload,
	},
};

#define MINIMUM(a, b) (((a) < (b)) ? (a) : (b))

#define BASIC_ID_POS_ZERO 0
#define BASIC_ID_POS_ONE  1

static void fill_example_data(struct ODID_UAS_Data *uasData)
{
	uasData->BasicID[BASIC_ID_POS_ZERO].UAType = ODID_UATYPE_HELICOPTER_OR_MULTIROTOR;
	uasData->BasicID[BASIC_ID_POS_ZERO].IDType = ODID_IDTYPE_SERIAL_NUMBER;
	char uas_id[] = "112624150A90E3AE1EC0";

	strncpy(uasData->BasicID[BASIC_ID_POS_ZERO].UASID, uas_id,
		MINIMUM(sizeof(uas_id), sizeof(uasData->BasicID[BASIC_ID_POS_ZERO].UASID)));

	uasData->BasicID[BASIC_ID_POS_ONE].UAType = ODID_UATYPE_HELICOPTER_OR_MULTIROTOR;
	uasData->BasicID[BASIC_ID_POS_ONE].IDType = ODID_IDTYPE_SPECIFIC_SESSION_ID;
	char uas_caa_id[] = "FD3454B778E565C24B70";

	strncpy(uasData->BasicID[BASIC_ID_POS_ONE].UASID, uas_caa_id,
		MINIMUM(sizeof(uas_caa_id), sizeof(uasData->BasicID[BASIC_ID_POS_ONE].UASID)));

	uasData->Auth[0].AuthType = ODID_AUTH_UAS_ID_SIGNATURE;
	uasData->Auth[0].DataPage = 0;
	uasData->Auth[0].LastPageIndex = 2;
	uasData->Auth[0].Length = 63;
	uasData->Auth[0].Timestamp = 28000000;
	char auth0_data[] = "12345678901234567";

	memcpy(uasData->Auth[0].AuthData, auth0_data,
	       MINIMUM(sizeof(auth0_data), sizeof(uasData->Auth[0].AuthData)));

	uasData->Auth[1].AuthType = ODID_AUTH_UAS_ID_SIGNATURE;
	uasData->Auth[1].DataPage = 1;
	char auth1_data[] = "12345678901234567890123";

	memcpy(uasData->Auth[1].AuthData, auth1_data,
	       MINIMUM(sizeof(auth1_data), sizeof(uasData->Auth[1].AuthData)));

	uasData->Auth[2].AuthType = ODID_AUTH_UAS_ID_SIGNATURE;
	uasData->Auth[2].DataPage = 2;
	char auth2_data[] = "12345678901234567890123";

	memcpy(uasData->Auth[2].AuthData, auth2_data,
	       MINIMUM(sizeof(auth2_data), sizeof(uasData->Auth[2].AuthData)));

	uasData->SelfID.DescType = ODID_DESC_TYPE_TEXT;
	char description[] = "Drone ID test flight---";

	strncpy(uasData->SelfID.Desc, description,
		MINIMUM(sizeof(description), sizeof(uasData->SelfID.Desc)));

	uasData->System.OperatorLocationType = ODID_OPERATOR_LOCATION_TYPE_TAKEOFF;
	uasData->System.ClassificationType = ODID_CLASSIFICATION_TYPE_EU;
	uasData->System.OperatorLatitude = uasData->Location.Latitude + 0.001;
	uasData->System.OperatorLongitude = uasData->Location.Longitude - 0.001;
	uasData->System.AreaCount = 1;
	uasData->System.AreaRadius = 0;
	uasData->System.AreaCeiling = 0;
	uasData->System.AreaFloor = 0;
	uasData->System.CategoryEU = ODID_CATEGORY_EU_OPEN;
	uasData->System.ClassEU = ODID_CLASS_EU_CLASS_1;
	uasData->System.OperatorAltitudeGeo = 20.5f;
	uasData->System.Timestamp = 28056789;

	uasData->OperatorID.OperatorIdType = ODID_OPERATOR_ID;
	char operatorId[] = "FIN87astrdge12k8";

	strncpy(uasData->OperatorID.OperatorId, operatorId,
		MINIMUM(sizeof(operatorId), sizeof(uasData->OperatorID.OperatorId)));
}

static void fill_example_gps_data(struct ODID_UAS_Data *uasData)
{
	uasData->Location.Status = ODID_STATUS_AIRBORNE;
	uasData->Location.Direction = 361.f;
	uasData->Location.SpeedHorizontal = 0.0f;
	uasData->Location.SpeedVertical = 0.35f;
	uasData->Location.Latitude = 51.4791;
	uasData->Location.Longitude = -0.0013;
	uasData->Location.AltitudeBaro = 100;
	uasData->Location.AltitudeGeo = 110;
	uasData->Location.HeightType = ODID_HEIGHT_REF_OVER_GROUND;
	uasData->Location.Height = 80;
	uasData->Location.HorizAccuracy = createEnumHorizontalAccuracy(5.5f);
	uasData->Location.VertAccuracy = createEnumVerticalAccuracy(9.5f);
	uasData->Location.BaroAccuracy = createEnumVerticalAccuracy(0.5f);
	uasData->Location.SpeedAccuracy = createEnumSpeedAccuracy(0.5f);
	uasData->Location.TSAccuracy = createEnumTimestampAccuracy(0.1f);
	uasData->Location.TimeStamp = 360.52f;
}

static struct ODID_UAS_Data uasData;
static union ODID_Message_encoded encoded;
static uint8_t msg_counters[ODID_MSG_COUNTER_AMOUNT];

static void bt_ready(int err)
{
	char addr_s[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr = {0};
	size_t count = 1;

	if (err) {
		printf("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printf("Bluetooth initialized\n");

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_PARAM_NCONN, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printf("Advertising failed to start (err %d)\n", err);
		return;
	}

	bt_id_get(&addr, &count);
	bt_addr_le_to_str(&addr, addr_s, sizeof(addr_s));

	printf("ODID started, advertising as %s\n", addr_s);
}

static void update_payload(uint8_t turn)
{
	int err = 0;

	switch (turn) {
	case 0: /* BasicID */
		err = encodeBasicIDMessage((ODID_BasicID_encoded *)&encoded, &uasData.BasicID[0]);
		if (err == ODID_SUCCESS) {
			memcpy(&payload[4], &encoded, sizeof(ODID_BasicID_encoded));
			memcpy(&payload[3], &msg_counters[ODID_MSG_COUNTER_BASIC_ID], 1);
			++msg_counters[ODID_MSG_COUNTER_BASIC_ID];
		}
		break;
	case 1: /* Location */
		/* Updating location for checking whether messages are dropped or not. */
		uasData.Location.Latitude = uasData.Location.Latitude - 0.01;
		uasData.Location.Longitude = uasData.Location.Longitude + 0.01;
		err = encodeLocationMessage((ODID_Location_encoded *)&encoded, &uasData.Location);
		if (err == ODID_SUCCESS) {
			memcpy(&payload[4], &encoded, sizeof(ODID_Location_encoded));
			memcpy(&payload[3], &msg_counters[ODID_MSG_COUNTER_LOCATION], 1);
			++msg_counters[ODID_MSG_COUNTER_LOCATION];
		}
		break;
	case 2: /* Auth */
		err = encodeAuthMessage((ODID_Auth_encoded *)&encoded, &uasData.Auth[0]);
		if (err == ODID_SUCCESS) {
			memcpy(&payload[4], &encoded, sizeof(ODID_Auth_encoded));
			memcpy(&payload[3], &msg_counters[ODID_MSG_COUNTER_AUTH], 1);
			++msg_counters[ODID_MSG_COUNTER_AUTH];
		}
		break;
	case 3: /* SelfID */
		err = encodeSelfIDMessage((ODID_SelfID_encoded *)&encoded, &uasData.SelfID);
		if (err == ODID_SUCCESS) {
			memcpy(&payload[4], &encoded, sizeof(ODID_SelfID_encoded));
			memcpy(&payload[3], &msg_counters[ODID_MSG_COUNTER_SELF_ID], 1);
			++msg_counters[ODID_MSG_COUNTER_SELF_ID];
		}
		break;
	case 4: /* System */
		err = encodeSystemMessage((ODID_System_encoded *)&encoded, &uasData.System);
		if (err == ODID_SUCCESS) {
			memcpy(&payload[4], &encoded, sizeof(ODID_System_encoded));
			memcpy(&payload[3], &msg_counters[ODID_MSG_COUNTER_SYSTEM], 1);
			++msg_counters[ODID_MSG_COUNTER_SYSTEM];
		}
		break;
	case 5: /* OperatorID */
		err = encodeOperatorIDMessage((ODID_OperatorID_encoded *)&encoded,
					      &uasData.OperatorID);
		if (err == ODID_SUCCESS) {
			memcpy(&payload[4], &encoded, sizeof(ODID_OperatorID_encoded));
			memcpy(&payload[3], &msg_counters[ODID_MSG_COUNTER_OPERATOR_ID], 1);
			++msg_counters[ODID_MSG_COUNTER_OPERATOR_ID];
		}
		break;
	default:
		break;
	}
}

int main(void)
{
	int err;

	printf("Starting ODID Demo\n");

	/* Initialize UAS data. */
	odid_initUasData(&uasData);
	fill_example_data(&uasData);
	fill_example_gps_data(&uasData);
	memset(&encoded, 0, sizeof(union ODID_Message_encoded));
	for (int i = 0; i < ODID_MSG_COUNTER_AMOUNT; ++i) {
		msg_counters[i] = 0;
	}

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printf("Bluetooth init failed (err %d)\n", err);
		return -1;
	}

	while (true) {
		if (bt_is_ready()) {
			break;
		}

		printf("Bluetooth not ready. Checking again in 100 ms\n");
		k_sleep(K_MSEC(100));
	}

	/* Modify ODID data and update adv data. */
	uint8_t tx_counter = 0;

	while (true) {
		uint8_t turn = tx_counter % 6;

		update_payload(turn);

		static const struct bt_data ad_new[] = {
			{
				.type = BT_DATA_SVC_DATA16,
				.data_len = ARRAY_SIZE(payload),
				.data = payload,
			},
		};

		err = bt_le_adv_update_data(ad_new, ARRAY_SIZE(ad_new), NULL, 0);
		if (err) {
			printf("Bluetooth update adv data failed (err %d)\n", err);
			continue;
		}

		k_sleep(K_MSEC(100));

		++tx_counter;
	}

	return 0;
}
