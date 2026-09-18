/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief KNX Datapoint Type (DPT) encode/decode API.
 *
 * Enabled by CONFIG_KNX_DPT=y.  All DPT families (DPT 1-20, DPT 232) are
 * built in; there is no per-family Kconfig knob to exclude one.
 */

#ifndef ZEPHYR_INCLUDE_KNX_DPT_H_
#define ZEPHYR_INCLUDE_KNX_DPT_H_

#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @defgroup knx_dpt KNX Datapoint Types (DPT)
 * @ingroup knx_core
 * @{
 */

/* Enable all DPT families when CONFIG_KNX_DPT=y */
#ifdef CONFIG_KNX_DPT
#define KNX_ENABLE_DPT_1
#define KNX_ENABLE_DPT_2
#define KNX_ENABLE_DPT_3
#define KNX_ENABLE_DPT_4
#define KNX_ENABLE_DPT_5
#define KNX_ENABLE_DPT_6
#define KNX_ENABLE_DPT_7
#define KNX_ENABLE_DPT_8
#define KNX_ENABLE_DPT_9
#define KNX_ENABLE_DPT_10
#define KNX_ENABLE_DPT_11
#define KNX_ENABLE_DPT_12
#define KNX_ENABLE_DPT_13
#define KNX_ENABLE_DPT_14
#define KNX_ENABLE_DPT_15
#define KNX_ENABLE_DPT_16
#define KNX_ENABLE_DPT_17
#define KNX_ENABLE_DPT_18
#define KNX_ENABLE_DPT_19
#define KNX_ENABLE_DPT_20
#define KNX_ENABLE_DPT_232
#endif

/* ============================================================
 * DPT descriptor
 * ============================================================
 */

/** @brief DPT main/sub group identifier (KNX spec 3/7/2). */
struct knx_dpt {
	uint8_t main_group; /**< Main number, e.g. 9 for DPT 9.xxx */
	uint8_t sub_group;  /**< Sub number, e.g. 1 for KNX_DPT_VALUE_TEMP */
	uint8_t index;      /**< Reserved, always 0 */
};

/** @brief Build a struct knx_dpt literal from a main/sub group pair. */
#define KNX_DPT(main, sub) {.main_group = (main), .sub_group = (sub), .index = 0}

/* ============================================================
 * Low-level payload helpers
 * ============================================================
 */

/**
 * @brief Store one masked byte into a DPT payload buffer.
 *
 * @param payload        Destination buffer.
 * @param payload_length Size of @p payload, in bytes.
 * @param index          Byte offset within @p payload to write.
 * @param value          Value to store (masked with @p mask before writing).
 * @param mask           Bitmask applied to @p value.
 */
void uint8_to_payload(uint8_t *payload, size_t payload_length, int index, uint8_t value,
		      uint8_t mask);

/**
 * @brief Store a masked 16-bit signed value into a DPT payload buffer, big-endian.
 *
 * @param payload        Destination buffer.
 * @param payload_length Size of @p payload, in bytes.
 * @param index          Byte offset of the first (high) byte within @p payload.
 * @param value          Value to store (masked with @p mask before writing).
 * @param mask           Bitmask applied to @p value.
 */
void int16_to_payload(uint8_t *payload, size_t payload_length, int index, int16_t value,
		      uint16_t mask);

/**
 * @brief Store a masked KNX 2-byte float (DPT 9) into a payload buffer.
 *
 * @param payload        Destination buffer.
 * @param payload_length Size of @p payload, in bytes.
 * @param index          Byte offset of the first (high) byte within @p payload.
 * @param value          Value to encode.
 * @param mask           Bitmask applied to the encoded 16-bit field.
 */
void float16_to_payload(uint8_t *payload, size_t payload_length, int index, double value,
			uint16_t mask);

/**
 * @brief Read a big-endian 16-bit value out of a DPT payload buffer.
 *
 * @param payload Source buffer.
 * @param index   Byte offset of the first (high) byte within @p payload.
 * @return The decoded 16-bit value.
 */
uint16_t payload_to_uint16(const uint8_t *payload, int index);

/**
 * @brief Decode a KNX 2-byte float (DPT 9) out of a payload buffer.
 *
 * @param payload Source buffer.
 * @param index   Byte offset of the first (high) byte within @p payload.
 * @return The decoded value.
 */
double payload_to_float16(const uint8_t *payload, int index);

/* ============================================================
 * DPT family encode/decode functions
 * ============================================================
 */

#ifdef KNX_ENABLE_DPT_1
uint8_t dpt1_encode(bool value, uint8_t *payload);
bool dpt1_decode(const uint8_t *payload);

/* Common DPT 1.x names */
#define KNX_DPT_SWITCH      KNX_DPT(1, 1)
#define KNX_DPT_BOOL        KNX_DPT(1, 2)
#define KNX_DPT_ENABLE      KNX_DPT(1, 3)
#define KNX_DPT_ALARM       KNX_DPT(1, 5)
#define KNX_DPT_BINARY_VALUE KNX_DPT(1, 6)
#define KNX_DPT_STEP        KNX_DPT(1, 7)
#define KNX_DPT_UP_DOWN      KNX_DPT(1, 8)
#define KNX_DPT_OPEN_CLOSE   KNX_DPT(1, 9)
#define KNX_DPT_START       KNX_DPT(1, 10)
#define KNX_DPT_STATE       KNX_DPT(1, 11)
#define KNX_DPT_INVERT      KNX_DPT(1, 12)
#define KNX_DPT_RESET       KNX_DPT(1, 15)
#define KNX_DPT_ACK         KNX_DPT(1, 16)
#define KNX_DPT_TRIGGER     KNX_DPT(1, 17)
#define KNX_DPT_OCCUPANCY   KNX_DPT(1, 18)

/* Group Object storage type — matches dpt1_encode()/dpt1_decode() (bool). */
typedef bool knx_dpt_enable_t;
#endif

#ifdef KNX_ENABLE_DPT_2
typedef struct {
	bool control;
	bool value;
} dpt2_t;
uint8_t dpt2_encode(dpt2_t data, uint8_t *payload);
dpt2_t dpt2_decode(const uint8_t *payload);
#endif

#ifdef KNX_ENABLE_DPT_3
typedef struct {
	bool control;
	uint8_t step_code;
} dpt3_t;
uint8_t dpt3_encode(dpt3_t data, uint8_t *payload);
dpt3_t dpt3_decode(const uint8_t *payload);
#define KNX_DPT_CONTROL_DIMMING KNX_DPT(3, 7)
#endif

#ifdef KNX_ENABLE_DPT_4
uint8_t dpt4_encode(char value, uint8_t *payload);
char dpt4_decode(const uint8_t *payload);
#endif

#ifdef KNX_ENABLE_DPT_5
uint8_t dpt5_encode(uint8_t value, uint8_t *payload);
uint8_t dpt5_decode(const uint8_t *payload);
uint8_t dpt5_encode_percent(float percent, uint8_t *payload);
#define KNX_DPT_SCALING KNX_DPT(5, 1)
#define KNX_DPT_ANGLE   KNX_DPT(5, 3)
#define KNX_DPT_PERCENT KNX_DPT(5, 4)
#endif

#ifdef KNX_ENABLE_DPT_6
uint8_t dpt6_encode(int8_t value, uint8_t *payload);
int8_t dpt6_decode(const uint8_t *payload);
uint8_t dpt6_encode_percent(float percent, uint8_t *payload);
float dpt6_decode_percent(const uint8_t *payload);
/* dpt6_encode/decode implement 6.010 (plain signed byte, no scaling).
 * dpt6_encode_percent/decode_percent implement 6.001 (direct 1-count-per-1%,
 * NOT rescaled to +-127 — that was this macro's old, wrong KNX_DPT(6, 1) name).
 */
#define KNX_DPT_PERCENT_V8    KNX_DPT(6, 1)
#define KNX_DPT_VALUE_1_COUNT KNX_DPT(6, 10)
#endif

#ifdef KNX_ENABLE_DPT_7
uint8_t dpt7_encode(uint16_t value, uint8_t *payload);
uint16_t dpt7_decode(const uint8_t *payload);
#define KNX_DPT_VALUE_2_UCOUNT KNX_DPT(7, 1)
#define KNX_DPT_TIME_PERIOD_MS   KNX_DPT(7, 4)
#endif

#ifdef KNX_ENABLE_DPT_8
uint8_t dpt8_encode(int16_t value, uint8_t *payload);
int16_t dpt8_decode(const uint8_t *payload);
#define KNX_DPT_VALUE_2_COUNT KNX_DPT(8, 1)
#endif

#ifdef KNX_ENABLE_DPT_9
uint8_t dpt9_encode(float value, uint8_t *payload);
float dpt9_decode(const uint8_t *payload);
#define KNX_DPT_VALUE_TEMP     KNX_DPT(9, 1)
#define KNX_DPT_VALUE_LUX      KNX_DPT(9, 4)
#define KNX_DPT_VALUE_HUMIDITY KNX_DPT(9, 7)
#define KNX_DPT_VALUE_VOLT     KNX_DPT(9, 20)

/* Group Object storage type — matches dpt9_encode()/dpt9_decode() (float). */
typedef float knx_dpt_value_temp_t;
typedef float knx_dpt_value_volt_t;
#endif

#ifdef KNX_ENABLE_DPT_10
typedef struct {
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} dpt10_t;
uint8_t dpt10_encode(dpt10_t value, uint8_t *payload);
dpt10_t dpt10_decode(const uint8_t *payload);
#define KNX_DPT_TIME_OF_DAY KNX_DPT(10, 1)
#endif

#ifdef KNX_ENABLE_DPT_11
typedef struct {
	uint8_t day;
	uint8_t month;
	uint8_t year;
} dpt11_t;
uint8_t dpt11_encode(dpt11_t value, uint8_t *payload);
dpt11_t dpt11_decode(const uint8_t *payload);
#define KNX_DPT_DATE KNX_DPT(11, 1)
#endif

#ifdef KNX_ENABLE_DPT_12
uint8_t dpt12_encode(uint32_t value, uint8_t *payload);
uint32_t dpt12_decode(const uint8_t *payload);
#define KNX_DPT_VALUE_4_UCOUNT KNX_DPT(12, 1)
#endif

#ifdef KNX_ENABLE_DPT_13
uint8_t dpt13_encode(int32_t value, uint8_t *payload);
int32_t dpt13_decode(const uint8_t *payload);
#define KNX_DPT_VALUE_4_COUNT KNX_DPT(13, 1)
#define KNX_DPT_FLOW_RATE_M3H  KNX_DPT(13, 10)
#define KNX_DPT_ACTIVE_ENERGY  KNX_DPT(13, 13)
#endif

#ifdef KNX_ENABLE_DPT_14
uint8_t dpt14_encode(float value, uint8_t *payload);
float dpt14_decode(const uint8_t *payload);
#define KNX_DPT_VALUE_4_FLOAT KNX_DPT(14, 1)
#define KNX_DPT_ACCELERATION  KNX_DPT(14, 0)
#endif

#ifdef KNX_ENABLE_DPT_15
typedef struct {
	uint8_t access_code[4];
	bool error;
	bool permission;
	bool read_direction;
	bool encrypted;
} dpt15_t;
uint8_t dpt15_encode(dpt15_t data, uint8_t *payload);
dpt15_t dpt15_decode(const uint8_t *payload);
#define KNX_DPT_ACCESS_DATA KNX_DPT(15, 0)
#endif

#ifdef KNX_ENABLE_DPT_16
uint8_t dpt16_encode(const char *value, uint8_t *payload);
void dpt16_decode(const uint8_t *payload, char *value, size_t max_len);
#define KNX_DPT_STRING_ASCII KNX_DPT(16, 0)
#define KNX_DPT_STRING_8859  KNX_DPT(16, 1)
#endif

#ifdef KNX_ENABLE_DPT_17
uint8_t dpt17_encode(uint8_t scene_number, uint8_t *payload);
uint8_t dpt17_decode(const uint8_t *payload);
#define KNX_DPT_SCENE_NUMBER KNX_DPT(17, 1)
#endif

#ifdef KNX_ENABLE_DPT_18
typedef struct {
	uint8_t scene;
	bool learn;
} dpt18_t;
uint8_t dpt18_encode(dpt18_t data, uint8_t *payload);
dpt18_t dpt18_decode(const uint8_t *payload);
#define KNX_DPT_SCENE_CONTROL KNX_DPT(18, 1)
#endif

#ifdef KNX_ENABLE_DPT_19
typedef struct {
	uint16_t year;       /* 1900-2155 */
	uint8_t month;       /* 1-12 */
	uint8_t day;         /* 1-31 */
	uint8_t day_of_week; /* 0=none, 1=Mon, …, 7=Sun */
	uint8_t hour;        /* 0-23 */
	uint8_t minute;      /* 0-59 */
	uint8_t second;      /* 0-59 */
	bool fault;
	bool working_day;
	bool no_working_day;
	bool no_year;
	bool no_date;
	bool no_day_of_week;
	bool no_time;
	bool summer_time;
	bool quality;
} dpt19_t;
uint8_t dpt19_encode(dpt19_t datetime, uint8_t *payload);
dpt19_t dpt19_decode(const uint8_t *payload);
#define KNX_DPT_DATE_TIME KNX_DPT(19, 1)
#endif

#ifdef KNX_ENABLE_DPT_20
uint8_t dpt20_encode(uint8_t value, uint8_t *payload);
uint8_t dpt20_decode(const uint8_t *payload);
#define KNX_DPT_HVAC_MODE     KNX_DPT(20, 102)
#define KNX_DPT_BUILDING_MODE KNX_DPT(20, 101)
#endif

#ifdef KNX_ENABLE_DPT_232
typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} dpt232_t;
uint8_t dpt232_encode(dpt232_t data, uint8_t *payload);
dpt232_t dpt232_decode(const uint8_t *payload);
#define KNX_DPT_COLOUR_RGB KNX_DPT(232, 600)
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_KNX_DPT_H_ */
