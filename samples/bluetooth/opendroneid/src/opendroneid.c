/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2019 Intel Corporation
 * Copyright 2024 NXP
 *
 * Referred from - https://github.com/opendroneid/opendroneid-core-c
 */

#include "opendroneid.h"
#include <math.h>
#include <stdio.h>
#define ENABLE_DEBUG 1

const float SPEED_DIV[2] = {0.25f, 0.75f};
const float VSPEED_DIV = 0.5f;
const int32_t LATLON_MULT = 10000000;
const float ALT_DIV = 0.5f;
const int ALT_ADDER = 1000;

static char *safe_dec_copyfill(char *dstStr, const char *srcStr, int dstSize);
static int intRangeMax(int64_t inValue, int startRange, int endRange);
static int intInRange(int inValue, int startRange, int endRange);

/*
 * Initialize basic ID data fields to their default values
 *
 * @param data (non encoded/packed) structure
 */
void odid_initBasicIDData(ODID_BasicID_data *data)
{
	if (!data) {
		return;
	}
	memset(data, 0, sizeof(ODID_BasicID_data));
}

/*
 * Initialize location data fields to their default values
 *
 * @param data (non encoded/packed) structure
 */
void odid_initLocationData(ODID_Location_data *data)
{
	if (!data) {
		return;
	}
	memset(data, 0, sizeof(ODID_Location_data));
	data->Direction = INV_DIR;
	data->SpeedHorizontal = INV_SPEED_H;
	data->SpeedVertical = INV_SPEED_V;
	data->AltitudeBaro = INV_ALT;
	data->AltitudeGeo = INV_ALT;
	data->Height = INV_ALT;
}

/*
 * Initialize authorization data fields to their default values
 *
 * @param data (non encoded/packed) structure
 */
void odid_initAuthData(ODID_Auth_data *data)
{
	if (!data) {
		return;
	}
	memset(data, 0, sizeof(ODID_Auth_data));
}

/*
 * Initialize self ID data fields to their default values
 *
 * @param data (non encoded/packed) structure
 */
void odid_initSelfIDData(ODID_SelfID_data *data)
{
	if (!data) {
		return;
	}
	memset(data, 0, sizeof(ODID_SelfID_data));
}

/*
 * Initialize system data fields to their default values
 *
 * @param data (non encoded/packed) structure
 */

void odid_initSystemData(ODID_System_data *data)
{
	if (!data) {
		return;
	}
	memset(data, 0, sizeof(ODID_System_data));
	data->AreaCount = 1;
	data->AreaCeiling = INV_ALT;
	data->AreaFloor = INV_ALT;
	data->OperatorAltitudeGeo = INV_ALT;
}

/*
 * Initialize operator ID data fields to their default values
 *
 * @param data (non encoded/packed) structure
 */

void odid_initOperatorIDData(ODID_OperatorID_data *data)
{
	if (!data) {
		return;
	}
	memset(data, 0, sizeof(ODID_OperatorID_data));
}

/*
 * Initialize message pack data fields to their default values
 *
 * @param data (non encoded/packed) structure
 */

void odid_initMessagePackData(ODID_MessagePack_data *data)
{
	if (!data) {
		return;
	}
	memset(data, 0, sizeof(ODID_MessagePack_data));
	data->SingleMessageSize = ODID_MESSAGE_SIZE;
}

/*
 * Initialize UAS data fields to their default values
 *
 * @param data (non encoded/packed) structure
 */

void odid_initUasData(ODID_UAS_Data *data)
{
	if (!data) {
		return;
	}
	for (int i = 0; i < ODID_BASIC_ID_MAX_MESSAGES; i++) {
		data->BasicIDValid[i] = 0;
		odid_initBasicIDData(&data->BasicID[i]);
	}
	data->LocationValid = 0;
	odid_initLocationData(&data->Location);
	for (int i = 0; i < ODID_AUTH_MAX_PAGES; i++) {
		data->AuthValid[i] = 0;
		odid_initAuthData(&data->Auth[i]);
	}
	data->SelfIDValid = 0;
	odid_initSelfIDData(&data->SelfID);
	data->SystemValid = 0;
	odid_initSystemData(&data->System);
	data->OperatorIDValid = 0;
	odid_initOperatorIDData(&data->OperatorID);
}

/*
 * Encode direction as defined by Open Drone ID
 *
 * The encoding method uses 8 bits for the direction in degrees and
 * one extra bit for indicating the East/West direction.
 *
 * @param Direcction in degrees. 0 <= x < 360. Route course based on true North
 * @param EWDirection Bit flag indicating whether the direction is towards
 *		      East (0 - 179 degrees) or West (180 - 359)
 * @return Encoded Direction in a single byte
 */
static uint8_t encodeDirection(float Direction, uint8_t *EWDirection)
{
	unsigned int direction_int = (unsigned int)roundf(Direction);

	if (direction_int == 360) {
		direction_int = 0;
	}
	if (direction_int < 180) {
		*EWDirection = 0;
	} else {
		*EWDirection = 1;
		direction_int -= 180;
	}
	return (uint8_t)intRangeMax(direction_int, 0, UINT8_MAX);
}

/*
 * Encode speed into units defined by Open Drone ID
 *
 * The quantization algorithm allows for speed to be stored in units of 0.25 m/s
 * on the low end of the scale and 0.75 m/s on the high end of the scale.
 * This allows for more precise speeds to be represented in a single Uint8 byte
 * rather than using a large float value.
 *
 * @param Speed_data Speed (and decimal) in m/s
 * @param mult a (write only) value that sets the multiplier flag
 * @return Encoded Speed in a single byte or max speed if over max encoded speed.
 */
static uint8_t encodeSpeedHorizontal(float Speed_data, uint8_t *mult)
{
	if (Speed_data <= UINT8_MAX * SPEED_DIV[0]) {
		*mult = 0;
		return (uint8_t)(Speed_data / SPEED_DIV[0]);
	}

	*mult = 1;
	int big_value = (int)((Speed_data - (UINT8_MAX * SPEED_DIV[0])) / SPEED_DIV[1]);

	return (uint8_t)intRangeMax(big_value, 0, UINT8_MAX);
}

/*
 * Encode Vertical Speed into a signed Integer ODID format
 *
 * @param SpeedVertical_data vertical speed (in m/s)
 * @return Encoded vertical speed
 */
static int8_t encodeSpeedVertical(float SpeedVertical_data)
{
	int encValue = (int)(SpeedVertical_data / VSPEED_DIV);

	return (int8_t)intRangeMax(encValue, INT8_MIN, INT8_MAX);
}

/*
 * Encode Latitude or Longitude value into a signed Integer ODID format
 *
 * This encodes a 64bit double into a 32 bit integer yet still maintains
 * 10^7 of a degree of accuracy (about 1cm)
 *
 * @param LatLon_data Either Lat or Lon double float value
 * @return Encoded Lat or Lon
 */
static int32_t encodeLatLon(double LatLon_data)
{
	return (int32_t)intRangeMax((int64_t)(LatLon_data * LATLON_MULT), -180 * LATLON_MULT,
				    180 * LATLON_MULT);
}

/*
 * Encode Altitude value into an int16 ODID format
 *
 * This encodes a 32bit floating point altitude into an uint16 compressed
 * scale that starts at -1000m.
 *
 * @param Alt_data Altitude to encode (in meters)
 * @return Encoded Altitude
 */
static uint16_t encodeAltitude(float Alt_data)
{
	return (uint16_t)intRangeMax((int)((Alt_data + (float)ALT_ADDER) / ALT_DIV), 0, UINT16_MAX);
}

/*
 * Encode timestamp data in ODID format
 *
 * This encodes a fractional seconds value into a 2 byte int16
 * on a scale of tenths of seconds since after the hour.
 *
 * @param Seconds_data Seconds (to at least 1 decimal place) since the hour
 * @return Encoded timestamp (Tenths of seconds since the hour)
 */
static uint16_t encodeTimeStamp(float Seconds_data)
{
	if (Seconds_data == INV_TIMESTAMP) {
		return INV_TIMESTAMP;
	} else {
		return (uint16_t)intRangeMax((int64_t)roundf(Seconds_data * 10), 0,
					     MAX_TIMESTAMP * 10);
	}
}

/*
 * Encode area radius data in ODID format
 *
 * This encodes the area radius in meters into a 1 byte value
 *
 * @param Radius The radius of the drone area/swarm
 * @return Encoded area radius
 */
static uint8_t encodeAreaRadius(uint16_t Radius)
{
	return (uint8_t)intRangeMax(Radius / 10, 0, 255);
}

/*
 * Encode Basic ID message (packed, ready for broadcast)
 *
 * @param outEncoded Output (encoded/packed) structure
 * @param inData     Input data (non encoded/packed) structure
 * @return           ODID_SUCCESS or ODID_FAIL;
 */
int encodeBasicIDMessage(ODID_BasicID_encoded *outEncoded, ODID_BasicID_data *inData)
{
	if (!outEncoded || !inData || !intInRange(inData->IDType, 0, 15) ||
	    !intInRange(inData->UAType, 0, 15)) {
		return ODID_FAIL;
	}

	outEncoded->MessageType = ODID_MESSAGETYPE_BASIC_ID;
	outEncoded->ProtoVersion = ODID_PROTOCOL_VERSION;
	outEncoded->IDType = inData->IDType;
	outEncoded->UAType = inData->UAType;
	switch (inData->IDType) {
	case ODID_IDTYPE_SERIAL_NUMBER:
	case ODID_IDTYPE_CAA_REGISTRATION_ID:
		memset(outEncoded->UASID, 0, sizeof(outEncoded->UASID));
		strncpy(outEncoded->UASID, inData->UASID, sizeof(outEncoded->UASID));
		break;
	default:
		memcpy(outEncoded->UASID, inData->UASID, sizeof(outEncoded->UASID));
		break;
	}
	memset(outEncoded->Reserved, 0, sizeof(outEncoded->Reserved));
	return ODID_SUCCESS;
}

/*
 * Encode Location message (packed, ready for broadcast)
 *
 * @param outEncoded Output (encoded/packed) structure
 * @param inData     Input data (non encoded/packed) structure
 * @return           ODID_SUCCESS or ODID_FAIL;
 */
int encodeLocationMessage(ODID_Location_encoded *outEncoded, ODID_Location_data *inData)
{
	uint8_t bitflag;

	if (!outEncoded || !inData || !intInRange(inData->Status, 0, 15) ||
	    !intInRange(inData->HeightType, 0, 1) || !intInRange(inData->HorizAccuracy, 0, 15) ||
	    !intInRange(inData->VertAccuracy, 0, 15) || !intInRange(inData->BaroAccuracy, 0, 15) ||
	    !intInRange(inData->SpeedAccuracy, 0, 15) || !intInRange(inData->TSAccuracy, 0, 15)) {
		return ODID_FAIL;
	}

	if (inData->Direction < MIN_DIR || inData->Direction > INV_DIR ||
	    (inData->Direction > MAX_DIR && inData->Direction < INV_DIR)) {
		return ODID_FAIL;
	}

	if (inData->SpeedHorizontal < MIN_SPEED_H || inData->SpeedHorizontal > INV_SPEED_H ||
	    (inData->SpeedHorizontal > MAX_SPEED_H && inData->SpeedHorizontal < INV_SPEED_H)) {
		return ODID_FAIL;
	}

	if (inData->SpeedVertical < MIN_SPEED_V || inData->SpeedVertical > INV_SPEED_V ||
	    (inData->SpeedVertical > MAX_SPEED_V && inData->SpeedVertical < INV_SPEED_V)) {
		return ODID_FAIL;
	}

	if (inData->Latitude < MIN_LAT || inData->Latitude > MAX_LAT ||
	    inData->Longitude < MIN_LON || inData->Longitude > MAX_LON) {
		return ODID_FAIL;
	}

	if (inData->AltitudeBaro < MIN_ALT || inData->AltitudeBaro > MAX_ALT ||
	    inData->AltitudeGeo < MIN_ALT || inData->AltitudeGeo > MAX_ALT ||
	    inData->Height < MIN_ALT || inData->Height > MAX_ALT) {
		return ODID_FAIL;
	}

	if (inData->TimeStamp < 0 ||
	    (inData->TimeStamp > MAX_TIMESTAMP && inData->TimeStamp != INV_TIMESTAMP)) {
		return ODID_FAIL;
	}

	outEncoded->MessageType = ODID_MESSAGETYPE_LOCATION;
	outEncoded->ProtoVersion = ODID_PROTOCOL_VERSION;
	outEncoded->Status = inData->Status;
	outEncoded->Reserved = 0;
	outEncoded->Direction = encodeDirection(inData->Direction, &bitflag);
	outEncoded->EWDirection = bitflag;
	outEncoded->SpeedHorizontal = encodeSpeedHorizontal(inData->SpeedHorizontal, &bitflag);
	outEncoded->SpeedMult = bitflag;
	outEncoded->SpeedVertical = encodeSpeedVertical(inData->SpeedVertical);
	outEncoded->Latitude = encodeLatLon(inData->Latitude);
	outEncoded->Longitude = encodeLatLon(inData->Longitude);
	outEncoded->AltitudeBaro = encodeAltitude(inData->AltitudeBaro);
	outEncoded->AltitudeGeo = encodeAltitude(inData->AltitudeGeo);
	outEncoded->HeightType = inData->HeightType;
	outEncoded->Height = encodeAltitude(inData->Height);
	outEncoded->HorizAccuracy = inData->HorizAccuracy;
	outEncoded->VertAccuracy = inData->VertAccuracy;
	outEncoded->BaroAccuracy = inData->BaroAccuracy;
	outEncoded->SpeedAccuracy = inData->SpeedAccuracy;
	outEncoded->TSAccuracy = inData->TSAccuracy;
	outEncoded->Reserved2 = 0;
	outEncoded->TimeStamp = encodeTimeStamp(inData->TimeStamp);
	outEncoded->Reserved3 = 0;
	return ODID_SUCCESS;
}

/*
 * Encode Auth message (packed, ready for broadcast)
 *
 * @param outEncoded Output (encoded/packed) structure
 * @param inData     Input data (non encoded/packed) structure
 * @return           ODID_SUCCESS or ODID_FAIL;
 */
int encodeAuthMessage(ODID_Auth_encoded *outEncoded, ODID_Auth_data *inData)
{
	if (!outEncoded || !inData || !intInRange(inData->AuthType, 0, 15)) {
		return ODID_FAIL;
	}

	if (inData->DataPage >= ODID_AUTH_MAX_PAGES) {
		return ODID_FAIL;
	}

	if (inData->DataPage == 0) {
		if (inData->LastPageIndex >= ODID_AUTH_MAX_PAGES) {
			return ODID_FAIL;
		}

#if (MAX_AUTH_LENGTH < UINT8_MAX)
		if (inData->Length > MAX_AUTH_LENGTH) {
			return ODID_FAIL;
		}
#endif

		int len = ODID_AUTH_PAGE_ZERO_DATA_SIZE +
			  inData->LastPageIndex * ODID_AUTH_PAGE_NONZERO_DATA_SIZE;
		if (len < inData->Length) {
			return ODID_FAIL;
		}
	}

	outEncoded->page_zero.MessageType = ODID_MESSAGETYPE_AUTH;
	outEncoded->page_zero.ProtoVersion = ODID_PROTOCOL_VERSION;
	outEncoded->page_zero.AuthType = inData->AuthType;
	outEncoded->page_zero.DataPage = inData->DataPage;

	if (inData->DataPage == 0) {
		outEncoded->page_zero.LastPageIndex = inData->LastPageIndex;
		outEncoded->page_zero.Length = inData->Length;
		outEncoded->page_zero.Timestamp = inData->Timestamp;
		memcpy(outEncoded->page_zero.AuthData, inData->AuthData,
		       sizeof(outEncoded->page_zero.AuthData));
	} else {
		memcpy(outEncoded->page_non_zero.AuthData, inData->AuthData,
		       sizeof(outEncoded->page_non_zero.AuthData));
	}
	return ODID_SUCCESS;
}

/*
 * Encode Self ID message (packed, ready for broadcast)
 *
 * @param outEncoded Output (encoded/packed) structure
 * @param inData     Input data (non encoded/packed) structure
 * @return           ODID_SUCCESS or ODID_FAIL;
 */
int encodeSelfIDMessage(ODID_SelfID_encoded *outEncoded, ODID_SelfID_data *inData)
{
	if (!outEncoded || !inData || !intInRange(inData->DescType, 0, 255)) {
		return ODID_FAIL;
	}

	outEncoded->MessageType = ODID_MESSAGETYPE_SELF_ID;
	outEncoded->ProtoVersion = ODID_PROTOCOL_VERSION;
	outEncoded->DescType = inData->DescType;
	strncpy(outEncoded->Desc, inData->Desc, sizeof(outEncoded->Desc));
	return ODID_SUCCESS;
}

/*
 * Encode System message (packed, ready for broadcast)
 *
 * @param outEncoded Output (encoded/packed) structure
 * @param inData     Input data (non encoded/packed) structure
 * @return           ODID_SUCCESS or ODID_FAIL;
 */
int encodeSystemMessage(ODID_System_encoded *outEncoded, ODID_System_data *inData)
{
	if (!outEncoded || !inData || !intInRange(inData->OperatorLocationType, 0, 3) ||
	    !intInRange(inData->ClassificationType, 0, 7) ||
	    !intInRange(inData->CategoryEU, 0, 15) || !intInRange(inData->ClassEU, 0, 15)) {
		return ODID_FAIL;
	}

	if (inData->OperatorLatitude < MIN_LAT || inData->OperatorLatitude > MAX_LAT ||
	    inData->OperatorLongitude < MIN_LON || inData->OperatorLongitude > MAX_LON) {
		return ODID_FAIL;
	}

	if (inData->AreaRadius > MAX_AREA_RADIUS) {
		return ODID_FAIL;
	}

	if (inData->AreaCeiling < MIN_ALT || inData->AreaCeiling > MAX_ALT ||
	    inData->AreaFloor < MIN_ALT || inData->AreaFloor > MAX_ALT ||
	    inData->OperatorAltitudeGeo < MIN_ALT || inData->OperatorAltitudeGeo > MAX_ALT) {
		return ODID_FAIL;
	}

	outEncoded->MessageType = ODID_MESSAGETYPE_SYSTEM;
	outEncoded->ProtoVersion = ODID_PROTOCOL_VERSION;
	outEncoded->Reserved = 0;
	outEncoded->OperatorLocationType = inData->OperatorLocationType;
	outEncoded->ClassificationType = inData->ClassificationType;
	outEncoded->OperatorLatitude = encodeLatLon(inData->OperatorLatitude);
	outEncoded->OperatorLongitude = encodeLatLon(inData->OperatorLongitude);
	outEncoded->AreaCount = inData->AreaCount;
	outEncoded->AreaRadius = encodeAreaRadius(inData->AreaRadius);
	outEncoded->AreaCeiling = encodeAltitude(inData->AreaCeiling);
	outEncoded->AreaFloor = encodeAltitude(inData->AreaFloor);
	outEncoded->CategoryEU = inData->CategoryEU;
	outEncoded->ClassEU = inData->ClassEU;
	outEncoded->OperatorAltitudeGeo = encodeAltitude(inData->OperatorAltitudeGeo);
	outEncoded->Timestamp = inData->Timestamp;
	outEncoded->Reserved2 = 0;
	return ODID_SUCCESS;
}

/*
 * Encode Operator ID message (packed, ready for broadcast)
 *
 * @param outEncoded Output (encoded/packed) structure
 * @param inData     Input data (non encoded/packed) structure
 * @return           ODID_SUCCESS or ODID_FAIL;
 */
int encodeOperatorIDMessage(ODID_OperatorID_encoded *outEncoded, ODID_OperatorID_data *inData)
{
	if (!outEncoded || !inData || !intInRange(inData->OperatorIdType, 0, 255)) {
		return ODID_FAIL;
	}

	outEncoded->MessageType = ODID_MESSAGETYPE_OPERATOR_ID;
	outEncoded->ProtoVersion = ODID_PROTOCOL_VERSION;
	outEncoded->OperatorIdType = inData->OperatorIdType;
	strncpy(outEncoded->OperatorId, inData->OperatorId, sizeof(outEncoded->OperatorId));
	memset(outEncoded->Reserved, 0, sizeof(outEncoded->Reserved));
	return ODID_SUCCESS;
}

/*
 * Check whether the data fields of a pack structure are valid
 *
 * @param msgs   Pointer to the buffer containing the messages
 * @param amount The amount of messages in the pack
 * @return       ODID_SUCCESS or ODID_FAIL;
 */
static int checkPackContent(ODID_Message_encoded *msgs, int amount)
{
	if (amount <= 0 || amount > ODID_PACK_MAX_MESSAGES) {
		return ODID_FAIL;
	}

	int numMessages[6] = {0}; /* Counters for relevant parts of ODID_messagetype_t */

	for (int i = 0; i < amount; i++) {
		uint8_t MessageType = decodeMessageType(msgs[i].rawData[0]);

		/* Check for illegal content. This also avoids recursive calls between */
		/* decodeOpenDroneID() and decodeMessagePack()/checkPackContent() */
		if (MessageType <= ODID_MESSAGETYPE_OPERATOR_ID) {
			numMessages[MessageType]++;
		} else {
			return ODID_FAIL;
		}
	}

	/* Allow max one of each message except Basic ID and Authorization. */
	if (numMessages[ODID_MESSAGETYPE_BASIC_ID] > ODID_BASIC_ID_MAX_MESSAGES ||
	    numMessages[ODID_MESSAGETYPE_LOCATION] > 1 ||
	    numMessages[ODID_MESSAGETYPE_AUTH] > ODID_AUTH_MAX_PAGES ||
	    numMessages[ODID_MESSAGETYPE_SELF_ID] > 1 || numMessages[ODID_MESSAGETYPE_SYSTEM] > 1 ||
	    numMessages[ODID_MESSAGETYPE_OPERATOR_ID] > 1) {
		return ODID_FAIL;
	}

	return ODID_SUCCESS;
}

/*
 * Encode message pack. I.e. a collection of multiple encoded messages
 *
 * @param outEncoded Output (encoded/packed) structure
 * @param inData     Input data (non encoded/packed) structure
 * @return           ODID_SUCCESS or ODID_FAIL;
 */
int encodeMessagePack(ODID_MessagePack_encoded *outEncoded, ODID_MessagePack_data *inData)
{
	if (!outEncoded || !inData || inData->SingleMessageSize != ODID_MESSAGE_SIZE) {
		return ODID_FAIL;
	}

	if (checkPackContent(inData->Messages, inData->MsgPackSize) != ODID_SUCCESS) {
		return ODID_FAIL;
	}

	outEncoded->MessageType = ODID_MESSAGETYPE_PACKED;
	outEncoded->ProtoVersion = ODID_PROTOCOL_VERSION;

	outEncoded->SingleMessageSize = inData->SingleMessageSize;
	outEncoded->MsgPackSize = inData->MsgPackSize;

	for (int i = 0; i < inData->MsgPackSize; i++) {
		memcpy(&outEncoded->Messages[i], &inData->Messages[i], ODID_MESSAGE_SIZE);
	}

	return ODID_SUCCESS;
}

/*
 * Dencode direction from Open Drone ID packed message
 *
 * @param Direction_enc encoded direction
 * @param EWDirection East/West direction flag
 * @return direction in degrees (0 - 359)
 */
static float decodeDirection(uint8_t Direction_enc, uint8_t EWDirection)
{
	if (EWDirection) {
		return (float)Direction_enc + 180;
	} else {
		return (float)Direction_enc;
	}
}

/*
 * Dencode speed from Open Drone ID packed message
 *
 * @param Speed_enc encoded speed
 * @param mult multiplier flag
 * @return decoded speed in m/s
 */
static float decodeSpeedHorizontal(uint8_t Speed_enc, uint8_t mult)
{
	if (mult) {
		return ((float)Speed_enc * SPEED_DIV[1]) + (UINT8_MAX * SPEED_DIV[0]);
	} else {
		return (float)Speed_enc * SPEED_DIV[0];
	}
}

/*
 * Decode Vertical Speed from Open Drone ID Packed Message
 *
 * @param SpeedVertical_enc Encoded Vertical Speed
 * @return decoded Vertical Speed in m/s
 */
static float decodeSpeedVertical(int8_t SpeedVertical_enc)
{
	return (float)SpeedVertical_enc * VSPEED_DIV;
}

/*
 * Decode Latitude or Longitude value into a signed Integer ODID format
 *
 * @param LatLon_enc Either Lat or Lon ecoded int value
 * @return decoded (double) Lat or Lon
 */
static double decodeLatLon(int32_t LatLon_enc)
{
	return (double)LatLon_enc / LATLON_MULT;
}

/*
 * Decode Altitude from ODID packed format
 *
 * @param Alt_enc Encoded Altitude to decode
 * @return decoded Altitude (in meters)
 */
static float decodeAltitude(uint16_t Alt_enc)
{
	return (float)Alt_enc * ALT_DIV - (float)ALT_ADDER;
}

/*
 * Decode timestamp data from ODID packed format
 *
 * @param Seconds_enc Encoded Timestamp
 * @return Decoded timestamp (seconds since the hour)
 */
static float decodeTimeStamp(uint16_t Seconds_enc)
{
	if (Seconds_enc == INV_TIMESTAMP) {
		return INV_TIMESTAMP;
	} else {
		return (float)Seconds_enc / 10;
	}
}

/*
 * Decode area radius data from ODID format
 *
 * This decodes a 1 byte value to the area radius in meters
 *
 * @param Radius_enc Encoded area radius
 * @return The radius of the drone area/swarm in meters
 */
static uint16_t decodeAreaRadius(uint8_t Radius_enc)
{
	return (uint16_t)((int)Radius_enc * 10);
}

/*
 * Get the ID type of the basic ID message
 *
 * @param inEncoded  Input message (encoded/packed) structure
 * @param idType     Output: The ID type of this basic ID message
 * @return           ODID_SUCCESS or ODID_FAIL;
 */
int getBasicIDType(ODID_BasicID_encoded *inEncoded, enum ODID_idtype *idType)
{
	if (!inEncoded || !idType || inEncoded->MessageType != ODID_MESSAGETYPE_BASIC_ID) {
		return ODID_FAIL;
	}

	*idType = (enum ODID_idtype)inEncoded->IDType;
	return ODID_SUCCESS;
}

/*
 * Decode Basic ID data from packed message
 *
 * @param outData   Output: decoded message
 * @param inEncoded Input message (encoded/packed) structure
 * @return          ODID_SUCCESS or ODID_FAIL;
 */
int decodeBasicIDMessage(ODID_BasicID_data *outData, ODID_BasicID_encoded *inEncoded)
{
	if (!outData || !inEncoded || inEncoded->MessageType != ODID_MESSAGETYPE_BASIC_ID ||
	    !intInRange(inEncoded->IDType, 0, 15) || !intInRange(inEncoded->UAType, 0, 15)) {
		return ODID_FAIL;
	}

	outData->IDType = (ODID_idtype_t)inEncoded->IDType;
	outData->UAType = (ODID_uatype_t)inEncoded->UAType;
	switch (inEncoded->IDType) {
	case ODID_IDTYPE_SERIAL_NUMBER:
	case ODID_IDTYPE_CAA_REGISTRATION_ID:
		safe_dec_copyfill(outData->UASID, inEncoded->UASID, sizeof(outData->UASID));
		break;
	default:
		memcpy(outData->UASID, inEncoded->UASID, sizeof(outData->UASID));
		break;
	}
	return ODID_SUCCESS;
}

/*
 * Decode Location data from packed message
 *
 * @param outData   Output: decoded message
 * @param inEncoded Input message (encoded/packed) structure
 * @return          ODID_SUCCESS or ODID_FAIL;
 */
int decodeLocationMessage(ODID_Location_data *outData, ODID_Location_encoded *inEncoded)
{
	if (!outData || !inEncoded || inEncoded->MessageType != ODID_MESSAGETYPE_LOCATION ||
	    !intInRange(inEncoded->Status, 0, 15)) {
		return ODID_FAIL;
	}

	outData->Status = (ODID_status_t)inEncoded->Status;
	outData->Direction = decodeDirection(inEncoded->Direction, inEncoded->EWDirection);
	outData->SpeedHorizontal =
		decodeSpeedHorizontal(inEncoded->SpeedHorizontal, inEncoded->SpeedMult);
	outData->SpeedVertical = decodeSpeedVertical(inEncoded->SpeedVertical);
	outData->Latitude = decodeLatLon(inEncoded->Latitude);
	outData->Longitude = decodeLatLon(inEncoded->Longitude);
	outData->AltitudeBaro = decodeAltitude(inEncoded->AltitudeBaro);
	outData->AltitudeGeo = decodeAltitude(inEncoded->AltitudeGeo);
	outData->HeightType = (ODID_Height_reference_t)inEncoded->HeightType;
	outData->Height = decodeAltitude(inEncoded->Height);
	outData->HorizAccuracy = (ODID_Horizontal_accuracy_t)inEncoded->HorizAccuracy;
	outData->VertAccuracy = (ODID_Vertical_accuracy_t)inEncoded->VertAccuracy;
	outData->BaroAccuracy = (ODID_Vertical_accuracy_t)inEncoded->BaroAccuracy;
	outData->SpeedAccuracy = (ODID_Speed_accuracy_t)inEncoded->SpeedAccuracy;
	outData->TSAccuracy = (ODID_Timestamp_accuracy_t)inEncoded->TSAccuracy;
	outData->TimeStamp = decodeTimeStamp(inEncoded->TimeStamp);
	return ODID_SUCCESS;
}

/*
 * Get the page number of the authorization message
 *
 * @param inEncoded  Input message (encoded/packed) structure
 * @param pageNum    Output: The page number of this authorization message
 * @return           ODID_SUCCESS or ODID_FAIL;
 */
int getAuthPageNum(ODID_Auth_encoded *inEncoded, int *pageNum)
{
	if (!inEncoded || !pageNum || inEncoded->page_zero.MessageType != ODID_MESSAGETYPE_AUTH ||
	    !intInRange(inEncoded->page_zero.AuthType, 0, 15) ||
	    !intInRange(inEncoded->page_zero.DataPage, 0, ODID_AUTH_MAX_PAGES - 1)) {
		return ODID_FAIL;
	}

	*pageNum = inEncoded->page_zero.DataPage;
	return ODID_SUCCESS;
}

/*
 * Decode Auth data from packed message
 *
 * @param outData   Output: decoded message
 * @param inEncoded Input message (encoded/packed) structure
 * @return          ODID_SUCCESS or ODID_FAIL;
 */
int decodeAuthMessage(ODID_Auth_data *outData, ODID_Auth_encoded *inEncoded)
{
	if (!outData || !inEncoded || inEncoded->page_zero.MessageType != ODID_MESSAGETYPE_AUTH ||
	    !intInRange(inEncoded->page_zero.AuthType, 0, 15) ||
	    !intInRange(inEncoded->page_zero.DataPage, 0, ODID_AUTH_MAX_PAGES - 1)) {
		return ODID_FAIL;
	}

	if (inEncoded->page_zero.DataPage == 0) {
		if (inEncoded->page_zero.LastPageIndex >= ODID_AUTH_MAX_PAGES) {
			return ODID_FAIL;
		}

#if (MAX_AUTH_LENGTH < UINT8_MAX)
		if (inEncoded->page_zero.Length > MAX_AUTH_LENGTH) {
			return ODID_FAIL;
		}
#endif

		int len = ODID_AUTH_PAGE_ZERO_DATA_SIZE +
			  inEncoded->page_zero.LastPageIndex * ODID_AUTH_PAGE_NONZERO_DATA_SIZE;

		if (len < inEncoded->page_zero.Length) {
			return ODID_FAIL;
		}
	}

	outData->AuthType = (ODID_authtype_t)inEncoded->page_zero.AuthType;
	outData->DataPage = inEncoded->page_zero.DataPage;
	if (inEncoded->page_zero.DataPage == 0) {
		outData->LastPageIndex = inEncoded->page_zero.LastPageIndex;
		outData->Length = inEncoded->page_zero.Length;
		outData->Timestamp = inEncoded->page_zero.Timestamp;
		memset(outData->AuthData, 0, sizeof(outData->AuthData));
		memcpy(outData->AuthData, inEncoded->page_zero.AuthData,
		       ODID_AUTH_PAGE_ZERO_DATA_SIZE);
	} else {
		memset(outData->AuthData, 0, sizeof(outData->AuthData));
		memcpy(outData->AuthData, inEncoded->page_non_zero.AuthData,
		       ODID_AUTH_PAGE_NONZERO_DATA_SIZE);
	}

	return ODID_SUCCESS;
}

/*
 * Decode Self ID data from packed message
 *
 * @param outData   Output: decoded message
 * @param inEncoded Input message (encoded/packed) structure
 * @return          ODID_SUCCESS or ODID_FAIL;
 */
int decodeSelfIDMessage(ODID_SelfID_data *outData, ODID_SelfID_encoded *inEncoded)
{
	if (!outData || !inEncoded || inEncoded->MessageType != ODID_MESSAGETYPE_SELF_ID) {
		return ODID_FAIL;
	}

	outData->DescType = (ODID_desctype_t)inEncoded->DescType;
	safe_dec_copyfill(outData->Desc, inEncoded->Desc, sizeof(outData->Desc));
	return ODID_SUCCESS;
}

/*
 * Decode System data from packed message
 *
 * @param outData   Output: decoded message
 * @param inEncoded Input message (encoded/packed) structure
 * @return          ODID_SUCCESS or ODID_FAIL;
 */
int decodeSystemMessage(ODID_System_data *outData, ODID_System_encoded *inEncoded)
{
	if (!outData || !inEncoded || inEncoded->MessageType != ODID_MESSAGETYPE_SYSTEM) {
		return ODID_FAIL;
	}

	outData->OperatorLocationType =
		(ODID_operator_location_type_t)inEncoded->OperatorLocationType;
	outData->ClassificationType = (ODID_classification_type_t)inEncoded->ClassificationType;
	outData->OperatorLatitude = decodeLatLon(inEncoded->OperatorLatitude);
	outData->OperatorLongitude = decodeLatLon(inEncoded->OperatorLongitude);
	outData->AreaCount = inEncoded->AreaCount;
	outData->AreaRadius = decodeAreaRadius(inEncoded->AreaRadius);
	outData->AreaCeiling = decodeAltitude(inEncoded->AreaCeiling);
	outData->AreaFloor = decodeAltitude(inEncoded->AreaFloor);
	outData->CategoryEU = (ODID_category_EU_t)inEncoded->CategoryEU;
	outData->ClassEU = (ODID_class_EU_t)inEncoded->ClassEU;
	outData->OperatorAltitudeGeo = decodeAltitude(inEncoded->OperatorAltitudeGeo);
	outData->Timestamp = inEncoded->Timestamp;
	return ODID_SUCCESS;
}

/*
 * Decode Operator ID data from packed message
 *
 * @param outData   Output: decoded message
 * @param inEncoded Input message (encoded/packed) structure
 * @return          ODID_SUCCESS or ODID_FAIL;
 */
int decodeOperatorIDMessage(ODID_OperatorID_data *outData, ODID_OperatorID_encoded *inEncoded)
{
	if (!outData || !inEncoded || inEncoded->MessageType != ODID_MESSAGETYPE_OPERATOR_ID) {
		return ODID_FAIL;
	}

	outData->OperatorIdType = (ODID_operatorIdType_t)inEncoded->OperatorIdType;
	safe_dec_copyfill(outData->OperatorId, inEncoded->OperatorId, sizeof(outData->OperatorId));
	return ODID_SUCCESS;
}

/*
 * Decode Message Pack from packed message
 *
 * The various Valid flags in uasData are set true whenever a message has been
 * decoded and the corresponding data structure has been filled. The caller must
 * clear these flags before calling decodeMessagePack().
 *
 * @param uasData Output: Structure containing buffers for all message data
 * @param pack    Pointer to an encoded packed message
 * @return        ODID_SUCCESS or ODID_FAIL;
 */
int decodeMessagePack(ODID_UAS_Data *uasData, ODID_MessagePack_encoded *pack)
{
	if (!uasData || !pack || pack->MessageType != ODID_MESSAGETYPE_PACKED) {
		return ODID_FAIL;
	}

	if (pack->SingleMessageSize != ODID_MESSAGE_SIZE) {
		return ODID_FAIL;
	}

	if (checkPackContent(pack->Messages, pack->MsgPackSize) != ODID_SUCCESS) {
		return ODID_FAIL;
	}

	for (int i = 0; i < pack->MsgPackSize; i++) {
		decodeOpenDroneID(uasData, pack->Messages[i].rawData);
	}

	return ODID_SUCCESS;
}

/*
 * Decodes the message type of a packed Open Drone ID message
 *
 * @param byte   The first byte of the message
 * @return       The message type: ODID_messagetype_t
 */
ODID_messagetype_t decodeMessageType(uint8_t byte)
{
	switch (byte >> 4) {
	case ODID_MESSAGETYPE_BASIC_ID:
		return ODID_MESSAGETYPE_BASIC_ID;
	case ODID_MESSAGETYPE_LOCATION:
		return ODID_MESSAGETYPE_LOCATION;
	case ODID_MESSAGETYPE_AUTH:
		return ODID_MESSAGETYPE_AUTH;
	case ODID_MESSAGETYPE_SELF_ID:
		return ODID_MESSAGETYPE_SELF_ID;
	case ODID_MESSAGETYPE_SYSTEM:
		return ODID_MESSAGETYPE_SYSTEM;
	case ODID_MESSAGETYPE_OPERATOR_ID:
		return ODID_MESSAGETYPE_OPERATOR_ID;
	case ODID_MESSAGETYPE_PACKED:
		return ODID_MESSAGETYPE_PACKED;
	default:
		return ODID_MESSAGETYPE_INVALID;
	}
}

/*
 * Parse encoded Open Drone ID data to identify the message type. Then decode
 * from Open Drone ID packed format into the appropriate Open Drone ID structure
 *
 * This function assumes that msgData points to a buffer containing all
 * ODID_MESSAGE_SIZE bytes of an Open Drone ID message.
 *
 * The various Valid flags in uasData are set true whenever a message has been
 * decoded and the corresponding data structure has been filled. The caller must
 * clear these flags before calling decodeOpenDroneID().
 *
 * @param uasData    Structure containing buffers for all message data
 * @param msgData    Pointer to a buffer containing a full encoded Open Drone ID
 *                   message
 * @return           The message type: ODID_messagetype_t
 */
ODID_messagetype_t decodeOpenDroneID(ODID_UAS_Data *uasData, uint8_t *msgData)
{
	if (!uasData || !msgData) {
		return ODID_MESSAGETYPE_INVALID;
	}

	switch (decodeMessageType(msgData[0])) {
	case ODID_MESSAGETYPE_BASIC_ID: {
		ODID_BasicID_encoded *basicId = (ODID_BasicID_encoded *)msgData;
		enum ODID_idtype idType;

		if (getBasicIDType(basicId, &idType) == ODID_SUCCESS) {
			/* Find a free slot to store the current message in or overwrite old data
			 * of the same type.
			 */
			for (int i = 0; i < ODID_BASIC_ID_MAX_MESSAGES; i++) {
				enum ODID_idtype storedType = uasData->BasicID[i].IDType;

				if (storedType == ODID_IDTYPE_NONE || storedType == idType) {
					if (decodeBasicIDMessage(&uasData->BasicID[i], basicId) ==
					    ODID_SUCCESS) {
						uasData->BasicIDValid[i] = 1;
						return ODID_MESSAGETYPE_BASIC_ID;
					}
				}
			}
		}
		break;
	}
	case ODID_MESSAGETYPE_LOCATION: {
		ODID_Location_encoded *location = (ODID_Location_encoded *)msgData;

		if (decodeLocationMessage(&uasData->Location, location) == ODID_SUCCESS) {
			uasData->LocationValid = 1;
			return ODID_MESSAGETYPE_LOCATION;
		}
		break;
	}
	case ODID_MESSAGETYPE_AUTH: {
		ODID_Auth_encoded *auth = (ODID_Auth_encoded *)msgData;
		int pageNum;

		if (getAuthPageNum(auth, &pageNum) == ODID_SUCCESS) {
			ODID_Auth_data *authData = &uasData->Auth[pageNum];

			if (decodeAuthMessage(authData, auth) == ODID_SUCCESS) {
				uasData->AuthValid[pageNum] = 1;
				return ODID_MESSAGETYPE_AUTH;
			}
		}
		break;
	}
	case ODID_MESSAGETYPE_SELF_ID: {
		ODID_SelfID_encoded *selfId = (ODID_SelfID_encoded *)msgData;

		if (decodeSelfIDMessage(&uasData->SelfID, selfId) == ODID_SUCCESS) {
			uasData->SelfIDValid = 1;
			return ODID_MESSAGETYPE_SELF_ID;
		}
		break;
	}
	case ODID_MESSAGETYPE_SYSTEM: {
		ODID_System_encoded *odid_system = (ODID_System_encoded *)msgData;

		if (decodeSystemMessage(&uasData->System, odid_system) == ODID_SUCCESS) {
			uasData->SystemValid = 1;
			return ODID_MESSAGETYPE_SYSTEM;
		}
		break;
	}
	case ODID_MESSAGETYPE_OPERATOR_ID: {
		ODID_OperatorID_encoded *operatorId = (ODID_OperatorID_encoded *)msgData;

		if (decodeOperatorIDMessage(&uasData->OperatorID, operatorId) == ODID_SUCCESS) {
			uasData->OperatorIDValid = 1;
			return ODID_MESSAGETYPE_OPERATOR_ID;
		}
		break;
	}
	case ODID_MESSAGETYPE_PACKED: {
		ODID_MessagePack_encoded *pack = (ODID_MessagePack_encoded *)msgData;

		if (decodeMessagePack(uasData, pack) == ODID_SUCCESS) {
			return ODID_MESSAGETYPE_PACKED;
		}
		break;
	}
	default:
		break;
	}

	return ODID_MESSAGETYPE_INVALID;
}

/*
 * Safely fill then copy string to destination (when decoding)
 *
 * This prevents overrun and guarantees copy behavior (fully null padded)
 * This function was specially made because the encoded data may not be null
 * terminated (if full size).
 * Therefore, the destination must use the last byte for a null (and is +1 in size)
 *
 * @param dstStr Destination string
 * @param srcStr Source string
 * @param dstSize Destination size
 */
static char *safe_dec_copyfill(char *dstStr, const char *srcStr, int dstSize)
{
	memset(dstStr, 0, dstSize);           /* fills destination with nulls */
	strncpy(dstStr, srcStr, dstSize - 1); /* copy only up to dst size-1 (no overruns) */
	return dstStr;
}

/*
 * Safely range check a value and return the minimum or max within the range if exceeded
 *
 * @param inValue Value to range-check
 * @param startRange Start of range to compare
 * @param endRange End of range to compare
 * @return same value if it fits, otherwise, min or max of range as appropriate.
 */
static int intRangeMax(int64_t inValue, int startRange, int endRange)
{
	if (inValue < startRange) {
		return startRange;
	} else if (inValue > endRange) {
		return endRange;
	} else {
		return (int)inValue;
	}
}

/*
 * Determine if an Int is in range
 *
 * @param inValue Value to range-check
 * @param startRange Start of range to compare
 * @param endRange End of range to compare
 * @return 1 = yes, 0 = no
 */
static int intInRange(int inValue, int startRange, int endRange)
{
	if (inValue < startRange || inValue > endRange) {
		return 0;
	} else {
		return 1;
	}
}

/*
 * This converts a horizontal accuracy float value to the corresponding enum
 *
 * @param Accuracy The horizontal accuracy in meters
 * @return Enum value representing the accuracy
 */
ODID_Horizontal_accuracy_t createEnumHorizontalAccuracy(float Accuracy)
{
	if (Accuracy >= 18520) {
		return ODID_HOR_ACC_UNKNOWN;
	} else if (Accuracy >= 7408) {
		return ODID_HOR_ACC_10NM;
	} else if (Accuracy >= 3704) {
		return ODID_HOR_ACC_4NM;
	} else if (Accuracy >= 1852) {
		return ODID_HOR_ACC_2NM;
	} else if (Accuracy >= 926) {
		return ODID_HOR_ACC_1NM;
	} else if (Accuracy >= 555.6f) {
		return ODID_HOR_ACC_0_5NM;
	} else if (Accuracy >= 185.2f) {
		return ODID_HOR_ACC_0_3NM;
	} else if (Accuracy >= 92.6f) {
		return ODID_HOR_ACC_0_1NM;
	} else if (Accuracy >= 30) {
		return ODID_HOR_ACC_0_05NM;
	} else if (Accuracy >= 10) {
		return ODID_HOR_ACC_30_METER;
	} else if (Accuracy >= 3) {
		return ODID_HOR_ACC_10_METER;
	} else if (Accuracy >= 1) {
		return ODID_HOR_ACC_3_METER;
	} else if (Accuracy > 0) {
		return ODID_HOR_ACC_1_METER;
	} else {
		return ODID_HOR_ACC_UNKNOWN;
	}
}

/*
 * This converts a vertical accuracy float value to the corresponding enum
 *
 * @param Accuracy The vertical accuracy in meters
 * @return Enum value representing the accuracy
 */
ODID_Vertical_accuracy_t createEnumVerticalAccuracy(float Accuracy)
{
	if (Accuracy >= 150) {
		return ODID_VER_ACC_UNKNOWN;
	} else if (Accuracy >= 45) {
		return ODID_VER_ACC_150_METER;
	} else if (Accuracy >= 25) {
		return ODID_VER_ACC_45_METER;
	} else if (Accuracy >= 10) {
		return ODID_VER_ACC_25_METER;
	} else if (Accuracy >= 3) {
		return ODID_VER_ACC_10_METER;
	} else if (Accuracy >= 1) {
		return ODID_VER_ACC_3_METER;
	} else if (Accuracy > 0) {
		return ODID_VER_ACC_1_METER;
	} else {
		return ODID_VER_ACC_UNKNOWN;
	}
}

/*
 * This converts a speed accuracy float value to the corresponding enum
 *
 * @param Accuracy The speed accuracy in m/s
 * @return Enum value representing the accuracy
 */
ODID_Speed_accuracy_t createEnumSpeedAccuracy(float Accuracy)
{
	if (Accuracy >= 10) {
		return ODID_SPEED_ACC_UNKNOWN;
	} else if (Accuracy >= 3) {
		return ODID_SPEED_ACC_10_METERS_PER_SECOND;
	} else if (Accuracy >= 1) {
		return ODID_SPEED_ACC_3_METERS_PER_SECOND;
	} else if (Accuracy >= 0.3f) {
		return ODID_SPEED_ACC_1_METERS_PER_SECOND;
	} else if (Accuracy > 0) {
		return ODID_SPEED_ACC_0_3_METERS_PER_SECOND;
	} else {
		return ODID_SPEED_ACC_UNKNOWN;
	}
}

/*
 * This converts a timestamp accuracy float value to the corresponding enum
 *
 * @param Accuracy The timestamp accuracy in seconds
 * @return Enum value representing the accuracy
 */
ODID_Timestamp_accuracy_t createEnumTimestampAccuracy(float Accuracy)
{
	if (Accuracy > 1.5f) {
		return ODID_TIME_ACC_UNKNOWN;
	} else if (Accuracy > 1.4f) {
		return ODID_TIME_ACC_1_5_SECOND;
	} else if (Accuracy > 1.3f) {
		return ODID_TIME_ACC_1_4_SECOND;
	} else if (Accuracy > 1.2f) {
		return ODID_TIME_ACC_1_3_SECOND;
	} else if (Accuracy > 1.1f) {
		return ODID_TIME_ACC_1_2_SECOND;
	} else if (Accuracy > 1.0f) {
		return ODID_TIME_ACC_1_1_SECOND;
	} else if (Accuracy > 0.9f) {
		return ODID_TIME_ACC_1_0_SECOND;
	} else if (Accuracy > 0.8f) {
		return ODID_TIME_ACC_0_9_SECOND;
	} else if (Accuracy > 0.7f) {
		return ODID_TIME_ACC_0_8_SECOND;
	} else if (Accuracy > 0.6f) {
		return ODID_TIME_ACC_0_7_SECOND;
	} else if (Accuracy > 0.5f) {
		return ODID_TIME_ACC_0_6_SECOND;
	} else if (Accuracy > 0.4f) {
		return ODID_TIME_ACC_0_5_SECOND;
	} else if (Accuracy > 0.3f) {
		return ODID_TIME_ACC_0_4_SECOND;
	} else if (Accuracy > 0.2f) {
		return ODID_TIME_ACC_0_3_SECOND;
	} else if (Accuracy > 0.1f) {
		return ODID_TIME_ACC_0_2_SECOND;
	} else if (Accuracy > 0.0f) {
		return ODID_TIME_ACC_0_1_SECOND;
	} else {
		return ODID_TIME_ACC_UNKNOWN;
	}
}

/*
 * This decodes a horizontal accuracy enum to the corresponding float value
 *
 * @param Accuracy Enum value representing the accuracy
 * @return The maximum horizontal accuracy in meters
 */
float decodeHorizontalAccuracy(ODID_Horizontal_accuracy_t Accuracy)
{
	switch (Accuracy) {
	case ODID_HOR_ACC_UNKNOWN:
		return 18520;
	case ODID_HOR_ACC_10NM:
		return 18520;
	case ODID_HOR_ACC_4NM:
		return 7808;
	case ODID_HOR_ACC_2NM:
		return 3704;
	case ODID_HOR_ACC_1NM:
		return 1852;
	case ODID_HOR_ACC_0_5NM:
		return 926;
	case ODID_HOR_ACC_0_3NM:
		return 555.6f;
	case ODID_HOR_ACC_0_1NM:
		return 185.2f;
	case ODID_HOR_ACC_0_05NM:
		return 92.6f;
	case ODID_HOR_ACC_30_METER:
		return 30;
	case ODID_HOR_ACC_10_METER:
		return 10;
	case ODID_HOR_ACC_3_METER:
		return 3;
	case ODID_HOR_ACC_1_METER:
		return 1;
	default:
		return 18520;
	}
}

/*
 * This decodes a vertical accuracy enum to the corresponding float value
 *
 * @param Accuracy Enum value representing the accuracy
 * @return The maximum vertical accuracy in meters
 */
float decodeVerticalAccuracy(ODID_Vertical_accuracy_t Accuracy)
{
	switch (Accuracy) {
	case ODID_VER_ACC_UNKNOWN:
		return 150;
	case ODID_VER_ACC_150_METER:
		return 150;
	case ODID_VER_ACC_45_METER:
		return 45;
	case ODID_VER_ACC_25_METER:
		return 25;
	case ODID_VER_ACC_10_METER:
		return 10;
	case ODID_VER_ACC_3_METER:
		return 3;
	case ODID_VER_ACC_1_METER:
		return 1;
	default:
		return 150;
	}
}

/*
 * This decodes a speed accuracy enum to the corresponding float value
 *
 * @param Accuracy Enum value representing the accuracy
 * @return The maximum speed accuracy in m/s
 */
float decodeSpeedAccuracy(ODID_Speed_accuracy_t Accuracy)
{
	switch (Accuracy) {
	case ODID_SPEED_ACC_UNKNOWN:
		return 10;
	case ODID_SPEED_ACC_10_METERS_PER_SECOND:
		return 10;
	case ODID_SPEED_ACC_3_METERS_PER_SECOND:
		return 3;
	case ODID_SPEED_ACC_1_METERS_PER_SECOND:
		return 1;
	case ODID_SPEED_ACC_0_3_METERS_PER_SECOND:
		return 0.3f;
	default:
		return 10;
	}
}

/*
 * This decodes a timestamp accuracy enum to the corresponding float value
 *
 * @param Accuracy Enum value representing the accuracy
 * @return The maximum timestamp accuracy in seconds
 */
float decodeTimestampAccuracy(ODID_Timestamp_accuracy_t Accuracy)
{
	switch (Accuracy) {
	case ODID_TIME_ACC_UNKNOWN:
		return 0.0f;
	case ODID_TIME_ACC_0_1_SECOND:
		return 0.1f;
	case ODID_TIME_ACC_0_2_SECOND:
		return 0.2f;
	case ODID_TIME_ACC_0_3_SECOND:
		return 0.3f;
	case ODID_TIME_ACC_0_4_SECOND:
		return 0.4f;
	case ODID_TIME_ACC_0_5_SECOND:
		return 0.5f;
	case ODID_TIME_ACC_0_6_SECOND:
		return 0.6f;
	case ODID_TIME_ACC_0_7_SECOND:
		return 0.7f;
	case ODID_TIME_ACC_0_8_SECOND:
		return 0.8f;
	case ODID_TIME_ACC_0_9_SECOND:
		return 0.9f;
	case ODID_TIME_ACC_1_0_SECOND:
		return 1.0f;
	case ODID_TIME_ACC_1_1_SECOND:
		return 1.1f;
	case ODID_TIME_ACC_1_2_SECOND:
		return 1.2f;
	case ODID_TIME_ACC_1_3_SECOND:
		return 1.3f;
	case ODID_TIME_ACC_1_4_SECOND:
		return 1.4f;
	case ODID_TIME_ACC_1_5_SECOND:
		return 1.5f;
	default:
		return 0.0f;
	}
}

#ifndef ODID_DISABLE_PRINTF

/*
 * Print array of bytes as a hex string
 *
 * @param byteArray Array of bytes to be printed
 * @param asize Size of array of bytes to be printed
 */

void printByteArray(uint8_t *byteArray, uint16_t asize, int spaced)
{
	if (ENABLE_DEBUG) {
		int x;

		for (x = 0; x < asize; x++) {
			printf("%02x", (unsigned int)byteArray[x]);
			if (spaced) {
				printf(" ");
			}
		}
		printf("\n");
	}
}

/*
 * Print formatted BasicID Data
 *
 * @param BasicID structure to be printed
 */
void printBasicID_data(ODID_BasicID_data *BasicID)
{
	/* Ensure the ID is null-terminated */
	char buf[ODID_ID_SIZE + 1] = {0};

	memcpy(buf, BasicID->UASID, ODID_ID_SIZE);

	static const char ODID_BasicID_data_format[] = "UAType: %d\nIDType: %d\nUASID: %s\n";

	printf(ODID_BasicID_data_format, BasicID->UAType, BasicID->IDType, buf);
}

/*
 * Print formatted Location Data
 *
 * @param Location structure to be printed
 */
void printLocation_data(ODID_Location_data *Location)
{
	const char ODID_Location_data_format[] =
		"Status: %d\nDirection: %.1f\nSpeedHori: %.2f\nSpeedVert: "
		"%.2f\nLat/Lon: %.7f, %.7f\nAlt: Baro, Geo, Height above %s: %.2f, "
		"%.2f, %.2f\nHoriz, Vert, Baro, Speed, TS Accuracy: %.1f, %.1f, %.1f, "
		"%.1f, %.1f\nTimeStamp: %.2f\n";
	printf(ODID_Location_data_format, Location->Status, (double)Location->Direction,
	       (double)Location->SpeedHorizontal, (double)Location->SpeedVertical,
	       Location->Latitude, Location->Longitude, Location->HeightType ? "Ground" : "TakeOff",
	       (double)Location->AltitudeBaro, (double)Location->AltitudeGeo,
	       (double)Location->Height, (double)decodeHorizontalAccuracy(Location->HorizAccuracy),
	       (double)decodeVerticalAccuracy(Location->VertAccuracy),
	       (double)decodeVerticalAccuracy(Location->BaroAccuracy),
	       (double)decodeSpeedAccuracy(Location->SpeedAccuracy),
	       (double)decodeTimestampAccuracy(Location->TSAccuracy), (double)Location->TimeStamp);
}

/*
 * Print formatted Auth Data
 *
 * @param Auth structure to be printed
 */
void printAuth_data(ODID_Auth_data *Auth)
{
	if (Auth->DataPage == 0) {
		static const char ODID_Auth_data_format[] = "AuthType: %d\nDataPage: %d\n"
							    "LastPageIndex: %d\nLength: %d\n"
							    "Timestamp: %u\nAuthData: ";

		printf(ODID_Auth_data_format, Auth->AuthType, Auth->DataPage, Auth->LastPageIndex,
		       Auth->Length, Auth->Timestamp);
		for (int i = 0; i < ODID_AUTH_PAGE_ZERO_DATA_SIZE; i++) {
			printf("0x%02X ", Auth->AuthData[i]);
		}
	} else {
		static const char ODID_Auth_data_format[] =
			"AuthType: %d\nDataPage: %d\nAuthData: ";

		printf(ODID_Auth_data_format, Auth->AuthType, Auth->DataPage);
		for (int i = 0; i < ODID_AUTH_PAGE_NONZERO_DATA_SIZE; i++) {
			printf("0x%02X ", Auth->AuthData[i]);
		}
	}
	printf("\n");
}

/*
 * Print formatted SelfID Data
 *
 * @param SelfID structure to be printed
 */
void printSelfID_data(ODID_SelfID_data *SelfID)
{
	/* Ensure the description is null-terminated */
	char buf[ODID_STR_SIZE + 1] = {0};

	memcpy(buf, SelfID->Desc, ODID_STR_SIZE);

	static const char ODID_SelfID_data_format[] = "DescType: %d\nDesc: %s\n";

	printf(ODID_SelfID_data_format, SelfID->DescType, buf);
}

/*
 * Print formatted System Data
 *
 * @param System_data structure to be printed
 */
void printSystem_data(ODID_System_data *System_data)
{
	static const char ODID_System_data_format[] =
		"Operator Location Type: %d\n"
		"Classification Type: %d\nLat/Lon: %.7f, %.7f\n"
		"Area Count, Radius, Ceiling, Floor: %d, %d, %.2f, %.2f\n"
		"Category EU: %d, Class EU: %d, Altitude: %.2f, Timestamp: %u\n";
	printf(ODID_System_data_format, System_data->OperatorLocationType,
	       System_data->ClassificationType, System_data->OperatorLatitude,
	       System_data->OperatorLongitude, System_data->AreaCount, System_data->AreaRadius,
	       (double)System_data->AreaCeiling, (double)System_data->AreaFloor,
	       System_data->CategoryEU, System_data->ClassEU,
	       (double)System_data->OperatorAltitudeGeo, System_data->Timestamp);
}

/*
 * Print formatted OperatorID Data
 *
 * @param OperatorID structure to be printed
 */
void printOperatorID_data(ODID_OperatorID_data *operatorID)
{
	/* Ensure the ID is null-terminated */
	char buf[ODID_ID_SIZE + 1] = {0};

	memcpy(buf, operatorID->OperatorId, ODID_ID_SIZE);

	static const char ODID_OperatorID_data_format[] = "OperatorIdType: %d\nOperatorId: %s\n";

	printf(ODID_OperatorID_data_format, operatorID->OperatorIdType, buf);
}

#endif /* ODID_DISABLE_PRINTF */
