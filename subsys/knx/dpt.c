/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/knx/dpt.h>

#include <stdbool.h>
#include <string.h>

#define RESET_PAYLOAD(x)                                                                           \
	for (int pi = 0; pi < (x); ++pi) {                                                         \
		payload[pi] = 0;                                                                   \
	}

void uint8_to_payload(uint8_t *payload, size_t payload_length, int index, uint8_t value,
		      uint8_t mask)
{
	RESET_PAYLOAD(index + 1);
	payload[index] = (payload[index] & ~mask) | (value & mask);
}

void int16_to_payload(uint8_t *payload, size_t payload_length, int index, int16_t value,
		      uint16_t mask)
{
	RESET_PAYLOAD(index + 2);
	payload[index] = (payload[index] & (~mask >> 8)) | ((value >> 8) & (mask >> 8));
	payload[index + 1] = (payload[index + 1] & ~mask) | (value & mask);
}

/* mask = 0xFFFF */
void float16_to_payload(uint8_t *payload, size_t payload_length, int index, double value,
			uint16_t mask)
{
	bool wasNegative = false;

	if (value < 0) {
		wasNegative = true;
		value *= -1;
	}

	value *= 100.0;
	unsigned short exponent = 0;

	if (value >= 2048) {
		exponent = ceil(log2(value) - 11.0);
	}

	short mantissa = roundf(value / (1 << exponent));

	if (wasNegative) {
		mantissa *= -1;
	}

	int16_to_payload(payload, payload_length, index, mantissa, mask);
	/*
	 * Overlay the exponent onto bits[6:3] of the high byte that
	 * int16_to_payload() just wrote, without disturbing the sign bit
	 * (bit 7) or mantissa bits M10-M8 (bits 2:0) already there.
	 * uint8_to_payload() cannot be used here: it unconditionally zeroes
	 * the byte via RESET_PAYLOAD() before applying its mask, which wiped
	 * out the sign and mantissa bits written a line above.
	 */
	uint16_t exp_mask = 0x78 & (mask >> 8);

	payload[index] = (payload[index] & ~exp_mask) | ((exponent << 3) & exp_mask);
}

uint16_t payload_to_uint16(const uint8_t *payload, int index)
{
	return ((((uint16_t)payload[index]) << 8) & 0xFF00) |
	       (((uint16_t)payload[index + 1]) & 0x00FF);
}

double payload_to_float16(const uint8_t *payload, int index)
{
	uint16_t mantissa = payload_to_uint16(payload, index) & 0x87FF;

	if (mantissa & 0x8000) {
		return ((~mantissa & 0x07FF) + 1.0) * -0.01 * (1 << ((payload[index] >> 3) & 0x0F));
	}

	return mantissa * 0.01 * (1 << ((payload[index] >> 3) & 0x0F));
}

/* ============================================================================
 * DPT 1.x - 1-bit Boolean
 * ============================================================================
 */

#ifdef ENABLE_DPT_1
/**
 * Encode a boolean value to DPT 1.x format
 * @param value: Boolean value to encode
 * @param payload: Output buffer (at least 1 byte)
 * @return Number of bytes written (always 1 for DPT 1)
 */
uint8_t dpt1_encode(bool value, uint8_t *payload)
{
	payload[0] = value ? 0x01 : 0x00;
	return 1;
}

/**
 * Decode DPT 1.x format to boolean
 * @param payload: Input buffer (at least 1 byte)
 * @return Decoded boolean value
 */
bool dpt1_decode(const uint8_t *payload)
{
	return (payload[0] & 0x01) != 0;
}
#endif

/* ============================================================================
 * DPT 9.x - 2-byte Float (DPT_Value_Temp, DPT_Value_Volt, etc.)
 * ============================================================================
 */

#ifdef ENABLE_DPT_9
/**
 * Encode a float value to DPT 9.x format (2-byte float)
 * @param value: Float value to encode (-671088.64 to 670760.96)
 * @param payload: Output buffer (at least 2 bytes)
 * @return Number of bytes written (always 2 for DPT 9)
 *
 * Format: MEEE EMMM MMMM MMMM
 *   M = Mantissa (11 bits + sign)
 *   E = Exponent (4 bits)
 *   Value = (0.01 * M) * 2^E
 */
uint8_t dpt9_encode(float value, uint8_t *payload)
{
	float16_to_payload(payload, 2, 0, (double)value, 0xFFFF);
	return 2;
}

/**
 * Decode DPT 9.x format to float
 * @param payload: Input buffer (at least 2 bytes)
 * @return Decoded float value
 */
float dpt9_decode(const uint8_t *payload)
{
	return (float)payload_to_float16(payload, 0);
}
#endif

/* ============================================================================
 * DPT 2.x - 1-bit Controlled (2 bits: control + value)
 * ============================================================================
 */

#ifdef ENABLE_DPT_2
/**
 * Encode DPT 2.x format (1-bit controlled)
 * @param data: Control and value bits
 * @param payload: Output buffer (at least 1 byte)
 * @return Number of bytes written (always 1 for DPT 2)
 *
 * Format: xxxx xxCV
 *   C = Control bit
 *   V = Value bit
 */
uint8_t dpt2_encode(dpt2_t data, uint8_t *payload)
{
	payload[0] = ((data.control ? 1 : 0) << 1) | (data.value ? 1 : 0);
	return 1;
}

/**
 * Decode DPT 2.x format
 * @param payload: Input buffer (at least 1 byte)
 * @return Decoded control and value
 */
dpt2_t dpt2_decode(const uint8_t *payload)
{
	dpt2_t result;

	result.control = (payload[0] & 0x02) != 0;
	result.value = (payload[0] & 0x01) != 0;
	return result;
}
#endif

/* ============================================================================
 * DPT 3.x - 3-bit Controlled (4 bits: control + 3-bit step code)
 * ============================================================================
 */

#ifdef ENABLE_DPT_3
/**
 * Encode DPT 3.x format (3-bit controlled for dimming/blinds)
 * @param data: Control bit and step code (0-7)
 * @param payload: Output buffer (at least 1 byte)
 * @return Number of bytes written (always 1 for DPT 3)
 *
 * Format: xxxx CSSS
 *   C = Control bit (0=decrease, 1=increase)
 *   SSS = Step code (0=break, 1-7=step intervals)
 */
uint8_t dpt3_encode(dpt3_t data, uint8_t *payload)
{
	payload[0] = ((data.control ? 1 : 0) << 3) | (data.step_code & 0x07);
	return 1;
}

/**
 * Decode DPT 3.x format
 * @param payload: Input buffer (at least 1 byte)
 * @return Decoded control and step code
 */
dpt3_t dpt3_decode(const uint8_t *payload)
{
	dpt3_t result;

	result.control = (payload[0] & 0x08) != 0;
	result.step_code = payload[0] & 0x07;
	return result;
}
#endif

/* ============================================================================
 * DPT 4.x - Character (8-bit ASCII/ISO-8859-1)
 * ============================================================================
 */

#ifdef ENABLE_DPT_4
/**
 * Encode a character to DPT 4.x format
 * @param value: ASCII/ISO-8859-1 character
 * @param payload: Output buffer (at least 1 byte)
 * @return Number of bytes written (always 1 for DPT 4)
 */
uint8_t dpt4_encode(char value, uint8_t *payload)
{
	payload[0] = (uint8_t)value;
	return 1;
}

/**
 * Decode DPT 4.x format to character
 * @param payload: Input buffer (at least 1 byte)
 * @return Decoded character
 */
char dpt4_decode(const uint8_t *payload)
{
	return (char)payload[0];
}
#endif

/* ============================================================================
 * DPT 5.x - 8-bit Unsigned Value (0-255, Scaling, Angle, Percent)
 * ============================================================================
 */

#ifdef ENABLE_DPT_5
/**
 * Encode an 8-bit unsigned value to DPT 5.x format
 * @param value: 0-255
 * @param payload: Output buffer (at least 1 byte)
 * @return Number of bytes written (always 1 for DPT 5)
 */
uint8_t dpt5_encode(uint8_t value, uint8_t *payload)
{
	payload[0] = value;
	return 1;
}

/**
 * Decode DPT 5.x format to 8-bit unsigned value
 * @param payload: Input buffer (at least 1 byte)
 * @return Decoded value (0-255)
 */
uint8_t dpt5_decode(const uint8_t *payload)
{
	return payload[0];
}

/**
 * Encode percentage (0.0-100.0%) to DPT 5.x format
 * @param percent: 0.0 to 100.0
 * @param payload: Output buffer (at least 1 byte)
 * @return Number of bytes written (always 1 for DPT 5)
 */
uint8_t dpt5_encode_percent(float percent, uint8_t *payload)
{
	if (percent < 0.0f) {
		percent = 0.0f;
	}
	if (percent > 100.0f) {
		percent = 100.0f;
	}
	payload[0] = (uint8_t)((percent * 255.0f) / 100.0f + 0.5f);
	return 1;
}

/**
 * Decode DPT 5.x format to percentage
 * @param payload: Input buffer (at least 1 byte)
 * @return Percentage (0.0-100.0%)
 */
float dpt5_decode_percent(const uint8_t *payload)
{
	return ((float)payload[0] * 100.0f) / 255.0f;
}
#endif

/* ============================================================================
 * DPT 6.x - 8-bit Signed Value (-128 to 127, Percent)
 * ============================================================================
 */

#ifdef ENABLE_DPT_6
/**
 * Encode an 8-bit signed value to DPT 6.x format
 * @param value: -128 to 127
 * @param payload: Output buffer (at least 1 byte)
 * @return Number of bytes written (always 1 for DPT 6)
 */
uint8_t dpt6_encode(int8_t value, uint8_t *payload)
{
	payload[0] = (uint8_t)value;
	return 1;
}

/**
 * Decode DPT 6.x format to 8-bit signed value
 * @param payload: Input buffer (at least 1 byte)
 * @return Decoded value (-128 to 127)
 */
int8_t dpt6_decode(const uint8_t *payload)
{
	return (int8_t)payload[0];
}

/**
 * Encode percentage (-100.0% to 100.0%) to DPT 6.x format
 * @param percent: -100.0 to 100.0
 * @param payload: Output buffer (at least 1 byte)
 * @return Number of bytes written (always 1 for DPT 6)
 */
uint8_t dpt6_encode_percent(float percent, uint8_t *payload)
{
	/*
	 * DPT 6.001 (Percent_V8) is a direct 1-count-per-1% mapping over its
	 * native -128..127 range (KNX spec 3/7/2 table: resolution "1 %"),
	 * NOT a -100..100% value rescaled onto the full -128..127 byte range.
	 */
	if (percent < -100.0f) {
		percent = -100.0f;
	}
	if (percent > 100.0f) {
		percent = 100.0f;
	}
	int8_t value = (int8_t)roundf(percent);

	payload[0] = (uint8_t)value;
	return 1;
}

/**
 * Decode DPT 6.x format to percentage
 * @param payload: Input buffer (at least 1 byte)
 * @return Percentage (-128.0% to 127.0%)
 */
float dpt6_decode_percent(const uint8_t *payload)
{
	return (float)(int8_t)payload[0];
}
#endif

/* ============================================================================
 * DPT 7.x - 16-bit Unsigned Value (0-65535, Counters, Time Periods)
 * ============================================================================
 */

#ifdef ENABLE_DPT_7
/**
 * Encode a 16-bit unsigned value to DPT 7.x format
 * @param value: 0-65535
 * @param payload: Output buffer (at least 2 bytes)
 * @return Number of bytes written (always 2 for DPT 7)
 */
uint8_t dpt7_encode(uint16_t value, uint8_t *payload)
{
	payload[0] = (value >> 8) & 0xFF;
	payload[1] = value & 0xFF;
	return 2;
}

/**
 * Decode DPT 7.x format to 16-bit unsigned value
 * @param payload: Input buffer (at least 2 bytes)
 * @return Decoded value (0-65535)
 */
uint16_t dpt7_decode(const uint8_t *payload)
{
	return (((uint16_t)payload[0]) << 8) | payload[1];
}
#endif

/* ============================================================================
 * DPT 8.x - 16-bit Signed Value (-32768 to 32767, Delta Values)
 * ============================================================================
 */

#ifdef ENABLE_DPT_8
/**
 * Encode a 16-bit signed value to DPT 8.x format
 * @param value: -32768 to 32767
 * @param payload: Output buffer (at least 2 bytes)
 * @return Number of bytes written (always 2 for DPT 8)
 */
uint8_t dpt8_encode(int16_t value, uint8_t *payload)
{
	payload[0] = (value >> 8) & 0xFF;
	payload[1] = value & 0xFF;
	return 2;
}

/**
 * Decode DPT 8.x format to 16-bit signed value
 * @param payload: Input buffer (at least 2 bytes)
 * @return Decoded value (-32768 to 32767)
 */
int16_t dpt8_decode(const uint8_t *payload)
{
	return (int16_t)((((uint16_t)payload[0]) << 8) | payload[1]);
}
#endif

/* ============================================================================
 * DPT 10.x - Time of Day (3 bytes: day, hour, minute, second)
 * ============================================================================
 */

#ifdef ENABLE_DPT_10
/**
 * Encode time of day to DPT 10.x format
 * @param time: Day (0-7), hour (0-23), minute (0-59), second (0-59)
 * @param payload: Output buffer (at least 3 bytes)
 * @return Number of bytes written (always 3 for DPT 10)
 *
 * Format: DDDHHHHHMMMMMMSSSSSS
 *   DDD = Day (0=no day, 1=Mon, ..., 7=Sun)
 *   HHHHH = Hour (0-23)
 *   MMMMMM = Minute (0-59)
 *   SSSSSS = Second (0-59)
 */
uint8_t dpt10_encode(dpt10_t time, uint8_t *payload)
{
	payload[0] = ((time.day & 0x07) << 5) | (time.hour & 0x1F);
	payload[1] = time.minute & 0x3F;
	payload[2] = time.second & 0x3F;
	return 3;
}

/**
 * Decode DPT 10.x format to time of day
 * @param payload: Input buffer (at least 3 bytes)
 * @return Decoded time
 */
dpt10_t dpt10_decode(const uint8_t *payload)
{
	dpt10_t ret;

	ret.day = (payload[0] >> 5) & 0x07;
	ret.hour = payload[0] & 0x1F;
	ret.minute = payload[1] & 0x3F;
	ret.second = payload[2] & 0x3F;
	return ret;
}
#endif

/* ============================================================================
 * DPT 11.x - Date (3 bytes: day, month, year)
 * ============================================================================
 */

#ifdef ENABLE_DPT_11
/**
 * Encode date to DPT 11.x format
 * @param date: Day (1-31), month (1-12), year (0-99 for 2000-2099)
 * @param payload: Output buffer (at least 3 bytes)
 * @return Number of bytes written (always 3 for DPT 11)
 */
uint8_t dpt11_encode(dpt11_t date, uint8_t *payload)
{
	payload[0] = date.day & 0x1F;
	payload[1] = date.month & 0x0F;
	payload[2] = date.year & 0x7F;
	return 3;
}

/**
 * Decode DPT 11.x format to date
 * @param payload: Input buffer (at least 3 bytes)
 * @return Decoded date
 */
dpt11_t dpt11_decode(const uint8_t *payload)
{
	dpt11_t date;

	date.day = payload[0] & 0x1F;
	date.month = payload[1] & 0x0F;
	date.year = payload[2] & 0x7F;
	return date;
}
#endif

/* ============================================================================
 * DPT 12.x - 32-bit Unsigned Value (0-4294967295, Long Counters)
 * ============================================================================
 */

#ifdef ENABLE_DPT_12
/**
 * Encode a 32-bit unsigned value to DPT 12.x format
 * @param value: 0-4294967295
 * @param payload: Output buffer (at least 4 bytes)
 * @return Number of bytes written (always 4 for DPT 12)
 */
uint8_t dpt12_encode(uint32_t value, uint8_t *payload)
{
	payload[0] = (value >> 24) & 0xFF;
	payload[1] = (value >> 16) & 0xFF;
	payload[2] = (value >> 8) & 0xFF;
	payload[3] = value & 0xFF;
	return 4;
}

/**
 * Decode DPT 12.x format to 32-bit unsigned value
 * @param payload: Input buffer (at least 4 bytes)
 * @return Decoded value (0-4294967295)
 */
uint32_t dpt12_decode(const uint8_t *payload)
{
	return (((uint32_t)payload[0]) << 24) | (((uint32_t)payload[1]) << 16) |
	       (((uint32_t)payload[2]) << 8) | ((uint32_t)payload[3]);
}
#endif

/* ============================================================================
 * DPT 13.x - 32-bit Signed Value (±2147483647, Flow, Power, Energy)
 * ============================================================================
 */

#ifdef ENABLE_DPT_13
/**
 * Encode a 32-bit signed value to DPT 13.x format
 * @param value: -2147483648 to 2147483647
 * @param payload: Output buffer (at least 4 bytes)
 * @return Number of bytes written (always 4 for DPT 13)
 */
uint8_t dpt13_encode(int32_t value, uint8_t *payload)
{
	payload[0] = (value >> 24) & 0xFF;
	payload[1] = (value >> 16) & 0xFF;
	payload[2] = (value >> 8) & 0xFF;
	payload[3] = value & 0xFF;
	return 4;
}

/**
 * Decode DPT 13.x format to 32-bit signed value
 * @param payload: Input buffer (at least 4 bytes)
 * @return Decoded value (-2147483648 to 2147483647)
 */
int32_t dpt13_decode(const uint8_t *payload)
{
	return (int32_t)((((uint32_t)payload[0]) << 24) | (((uint32_t)payload[1]) << 16) |
			 (((uint32_t)payload[2]) << 8) | ((uint32_t)payload[3]));
}
#endif

/* ============================================================================
 * DPT 14.x - 4-byte Float (IEEE 754 single precision)
 * ============================================================================
 */

#ifdef ENABLE_DPT_14
/**
 * Encode a float to DPT 14.x format (IEEE 754 single precision)
 * @param value: IEEE 754 float
 * @param payload: Output buffer (at least 4 bytes)
 * @return Number of bytes written (always 4 for DPT 14)
 */
uint8_t dpt14_encode(float value, uint8_t *payload)
{
	union {
		float f;
		uint32_t u;
	} converter;
	converter.f = value;

	payload[0] = (converter.u >> 24) & 0xFF;
	payload[1] = (converter.u >> 16) & 0xFF;
	payload[2] = (converter.u >> 8) & 0xFF;
	payload[3] = converter.u & 0xFF;
	return 4;
}

/**
 * Decode DPT 14.x format to IEEE 754 float
 * @param payload: Input buffer (at least 4 bytes)
 * @return Decoded IEEE 754 float
 */
float dpt14_decode(const uint8_t *payload)
{
	union {
		float f;
		uint32_t u;
	} converter;

	converter.u = (((uint32_t)payload[0]) << 24) | (((uint32_t)payload[1]) << 16) |
		      (((uint32_t)payload[2]) << 8) | ((uint32_t)payload[3]);
	return converter.f;
}
#endif

/* ============================================================================
 * DPT 15.x - Entrance Access (4 bytes: access code + flags)
 * ============================================================================
 */

#ifdef ENABLE_DPT_15
/**
 * Encode entrance access to DPT 15.x format
 * @param data: Access code (4 digits) and flags
 * @param payload: Output buffer (at least 4 bytes)
 * @return Number of bytes written (always 4 for DPT 15)
 */
uint8_t dpt15_encode(dpt15_t data, uint8_t *payload)
{
	payload[0] = data.access_code[0];
	payload[1] = data.access_code[1];
	payload[2] = data.access_code[2];
	payload[3] = ((data.error ? 1 : 0) << 7) | ((data.permission ? 1 : 0) << 6) |
		     ((data.read_direction ? 1 : 0) << 5) | ((data.encrypted ? 1 : 0) << 4) |
		     (data.access_code[3] & 0x0F);
	return 4;
}

/**
 * Decode DPT 15.x format to entrance access
 * @param payload: Input buffer (at least 4 bytes)
 * @return Decoded access data
 */
dpt15_t dpt15_decode(const uint8_t *payload)
{
	dpt15_t data;

	data.access_code[0] = payload[0];
	data.access_code[1] = payload[1];
	data.access_code[2] = payload[2];
	data.access_code[3] = payload[3] & 0x0F;
	data.error = (payload[3] & 0x80) != 0;
	data.permission = (payload[3] & 0x40) != 0;
	data.read_direction = (payload[3] & 0x20) != 0;
	data.encrypted = (payload[3] & 0x10) != 0;
	return data;
}
#endif

/* ============================================================================
 * DPT 16.x - Character String (14 ASCII/ISO-8859-1 characters)
 * ============================================================================
 */

#ifdef ENABLE_DPT_16
/**
 * Encode a string to DPT 16.x format
 * @param str: String to encode (null-terminated, max 14 chars)
 * @param payload: Output buffer (exactly 14 bytes)
 * @return Number of bytes written (always 14 for DPT 16)
 */
uint8_t dpt16_encode(const char *str, uint8_t *payload)
{
	size_t len = strlen(str);

	if (len > 14) {
		len = 14;
	}

	memcpy(payload, str, len);
	/* Pad with null bytes */
	for (size_t i = len; i < 14; i++) {
		payload[i] = 0x00;
	}
	return 14;
}

/**
 * Decode DPT 16.x format to string
 * @param payload: Input buffer (14 bytes)
 * @param str: Output string buffer
 * @param max_len: Maximum length of output buffer
 */
void dpt16_decode(const uint8_t *payload, char *str, size_t max_len)
{
	size_t len = 0;

	while (len < 14 && len < (max_len - 1) && payload[len] != 0x00) {
		str[len] = payload[len];
		len++;
	}
	str[len] = '\0';
}
#endif

/* ============================================================================
 * DPT 17.x - Scene Number (1 byte: 0-63 scene number)
 * ============================================================================
 */

#ifdef ENABLE_DPT_17
/**
 * Encode scene number to DPT 17.x format
 * @param scene: Scene number (0-63)
 * @param payload: Output buffer (at least 1 byte)
 * @return Number of bytes written (always 1 for DPT 17)
 */
uint8_t dpt17_encode(uint8_t scene, uint8_t *payload)
{
	payload[0] = scene & 0x3F;
	return 1;
}

/**
 * Decode DPT 17.x format to scene number
 * @param payload: Input buffer (at least 1 byte)
 * @return Decoded scene number (0-63)
 */
uint8_t dpt17_decode(const uint8_t *payload)
{
	return payload[0] & 0x3F;
}
#endif

/* ============================================================================
 * DPT 18.x - Scene Control (1 byte: scene number + learn bit)
 * ============================================================================
 */

#ifdef ENABLE_DPT_18
/**
 * Encode scene control to DPT 18.x format
 * @param data: Scene number (0-63) and learn bit
 * @param payload: Output buffer (at least 1 byte)
 * @return Number of bytes written (always 1 for DPT 18)
 *
 * Format: LXSSSSSS
 *   L = Learn bit (0=activate, 1=learn)
 *   SSSSSS = Scene number (0-63)
 */
uint8_t dpt18_encode(dpt18_t data, uint8_t *payload)
{
	payload[0] = ((data.learn ? 1 : 0) << 7) | (data.scene & 0x3F);
	return 1;
}

/**
 * Decode DPT 18.x format to scene control
 * @param payload: Input buffer (at least 1 byte)
 * @return Decoded scene control
 */
dpt18_t dpt18_decode(const uint8_t *payload)
{
	dpt18_t data;

	data.learn = (payload[0] & 0x80) != 0;
	data.scene = payload[0] & 0x3F;
	return data;
}
#endif

/* ============================================================================
 * DPT 19.x - Date/Time (8 bytes: full date+time with flags)
 * ============================================================================
 */

#ifdef ENABLE_DPT_19
/**
 * Encode date/time to DPT 19.x format
 * @param datetime: Full date and time with flags
 * @param payload: Output buffer (at least 8 bytes)
 * @return Number of bytes written (always 8 for DPT 19)
 */
uint8_t dpt19_encode(dpt19_t datetime, uint8_t *payload)
{
	payload[0] = datetime.year;
	payload[1] = datetime.month & 0x0F;
	payload[2] = datetime.day & 0x1F;
	payload[3] = ((datetime.day_of_week & 0x07) << 5) | (datetime.hour & 0x1F);
	payload[4] = datetime.minute & 0x3F;
	payload[5] = datetime.second & 0x3F;
	payload[6] = ((datetime.fault ? 1 : 0) << 7) | ((datetime.working_day ? 1 : 0) << 6) |
		     ((datetime.no_working_day ? 1 : 0) << 5) | ((datetime.no_year ? 1 : 0) << 4) |
		     ((datetime.no_date ? 1 : 0) << 3) | ((datetime.no_day_of_week ? 1 : 0) << 2) |
		     ((datetime.no_time ? 1 : 0) << 1) | (datetime.summer_time ? 1 : 0);
	payload[7] = (datetime.quality ? 1 : 0);
	return 8;
}

/**
 * Decode DPT 19.x format to date/time
 * @param payload: Input buffer (at least 8 bytes)
 * @return Decoded date/time
 */
dpt19_t dpt19_decode(const uint8_t *payload)
{
	dpt19_t datetime;

	datetime.year = payload[0];
	datetime.month = payload[1] & 0x0F;
	datetime.day = payload[2] & 0x1F;
	datetime.day_of_week = (payload[3] >> 5) & 0x07;
	datetime.hour = payload[3] & 0x1F;
	datetime.minute = payload[4] & 0x3F;
	datetime.second = payload[5] & 0x3F;
	datetime.fault = (payload[6] & 0x80) != 0;
	datetime.working_day = (payload[6] & 0x40) != 0;
	datetime.no_working_day = (payload[6] & 0x20) != 0;
	datetime.no_year = (payload[6] & 0x10) != 0;
	datetime.no_date = (payload[6] & 0x08) != 0;
	datetime.no_day_of_week = (payload[6] & 0x04) != 0;
	datetime.no_time = (payload[6] & 0x02) != 0;
	datetime.summer_time = (payload[6] & 0x01) != 0;
	datetime.quality = (payload[7] & 0x01) != 0;
	return datetime;
}
#endif

/* ============================================================================
 * DPT 20.x - 8-bit Enumeration (HVAC Mode, Building Mode, etc.)
 * ============================================================================
 */

#ifdef ENABLE_DPT_20
/**
 * Encode enumeration to DPT 20.x format
 * @param value: Enumeration value (0-255)
 * @param payload: Output buffer (at least 1 byte)
 * @return Number of bytes written (always 1 for DPT 20)
 */
uint8_t dpt20_encode(uint8_t value, uint8_t *payload)
{
	payload[0] = value;
	return 1;
}

/**
 * Decode DPT 20.x format to enumeration
 * @param payload: Input buffer (at least 1 byte)
 * @return Decoded enumeration value
 */
uint8_t dpt20_decode(const uint8_t *payload)
{
	return payload[0];
}
#endif

/* ============================================================================
 * DPT 21.x - 8-bit Bitmap (General Status, Device Control)
 * ============================================================================
 */

#ifdef ENABLE_DPT_21
/**
 * Encode bitmap to DPT 21.x format
 * @param value: 8-bit bitmap
 * @param payload: Output buffer (at least 1 byte)
 * @return Number of bytes written (always 1 for DPT 21)
 */
uint8_t dpt21_encode(uint8_t value, uint8_t *payload)
{
	payload[0] = value;
	return 1;
}

/**
 * Decode DPT 21.x format to bitmap
 * @param payload: Input buffer (at least 1 byte)
 * @return Decoded 8-bit bitmap
 */
uint8_t dpt21_decode(const uint8_t *payload)
{
	return payload[0];
}
#endif

/* ============================================================================
 * DPT 22.x - 16-bit Bitmap (DHW Controller Status, etc.)
 * ============================================================================
 */

#ifdef ENABLE_DPT_22
/**
 * Encode bitmap to DPT 22.x format
 * @param value: 16-bit bitmap
 * @param payload: Output buffer (at least 2 bytes)
 * @return Number of bytes written (always 2 for DPT 22)
 */
uint8_t dpt22_encode(uint16_t value, uint8_t *payload)
{
	payload[0] = (value >> 8) & 0xFF;
	payload[1] = value & 0xFF;
	return 2;
}

/**
 * Decode DPT 22.x format to bitmap
 * @param payload: Input buffer (at least 2 bytes)
 * @return Decoded 16-bit bitmap
 */
uint16_t dpt22_decode(const uint8_t *payload)
{
	return (((uint16_t)payload[0]) << 8) | payload[1];
}
#endif

/* ============================================================================
 * DPT 28.x - Variable Length String (UTF-8, null-terminated)
 * ============================================================================
 */

#ifdef ENABLE_DPT_28
/**
 * Encode variable length string to DPT 28.x format
 * @param str: UTF-8 string (null-terminated)
 * @param payload: Output buffer
 * @param max_len: Maximum length of payload
 * @return Number of bytes written
 */
uint8_t dpt28_encode(const char *str, uint8_t *payload, size_t max_len)
{
	size_t len = strlen(str);

	if (len >= max_len) {
		len = max_len - 1;
	}

	memcpy(payload, str, len);
	payload[len] = '\0';
	return (uint8_t)(len + 1);
}

/**
 * Decode DPT 28.x format to string
 * @param payload: Input buffer
 * @param payload_len: Length of payload
 * @param str: Output string buffer
 * @param max_len: Maximum length of output buffer
 */
void dpt28_decode(const uint8_t *payload, uint8_t payload_len, char *str, size_t max_len)
{
	size_t len = 0;

	while (len < payload_len && len < (max_len - 1) && payload[len] != 0x00) {
		str[len] = payload[len];
		len++;
	}
	str[len] = '\0';
}
#endif

/* ============================================================================
 * DPT 29.x - 64-bit Signed Value (Energy, Apparent Energy)
 * ============================================================================
 */

#ifdef ENABLE_DPT_29
/**
 * Encode a 64-bit signed value to DPT 29.x format
 * @param value: 64-bit signed value
 * @param payload: Output buffer (at least 8 bytes)
 * @return Number of bytes written (always 8 for DPT 29)
 */
uint8_t dpt29_encode(int64_t value, uint8_t *payload)
{
	payload[0] = (value >> 56) & 0xFF;
	payload[1] = (value >> 48) & 0xFF;
	payload[2] = (value >> 40) & 0xFF;
	payload[3] = (value >> 32) & 0xFF;
	payload[4] = (value >> 24) & 0xFF;
	payload[5] = (value >> 16) & 0xFF;
	payload[6] = (value >> 8) & 0xFF;
	payload[7] = value & 0xFF;
	return 8;
}

/**
 * Decode DPT 29.x format to 64-bit signed value
 * @param payload: Input buffer (at least 8 bytes)
 * @return Decoded 64-bit signed value
 */
int64_t dpt29_decode(const uint8_t *payload)
{
	return (int64_t)((((uint64_t)payload[0]) << 56) | (((uint64_t)payload[1]) << 48) |
			 (((uint64_t)payload[2]) << 40) | (((uint64_t)payload[3]) << 32) |
			 (((uint64_t)payload[4]) << 24) | (((uint64_t)payload[5]) << 16) |
			 (((uint64_t)payload[6]) << 8) | ((uint64_t)payload[7]));
}
#endif

/* ============================================================================
 * DPT 232.x - RGB Color (3 bytes: R, G, B)
 * ============================================================================
 */

#ifdef ENABLE_DPT_232
/**
 * Encode RGB color to DPT 232.x format
 * @param color: RGB color (0-255 per channel)
 * @param payload: Output buffer (at least 3 bytes)
 * @return Number of bytes written (always 3 for DPT 232)
 */
uint8_t dpt232_encode(dpt232_t color, uint8_t *payload)
{
	payload[0] = color.red;
	payload[1] = color.green;
	payload[2] = color.blue;
	return 3;
}

/**
 * Decode DPT 232.x format to RGB color
 * @param payload: Input buffer (at least 3 bytes)
 * @return Decoded RGB color
 */
dpt232_t dpt232_decode(const uint8_t *payload)
{
	dpt232_t color;

	color.red = payload[0];
	color.green = payload[1];
	color.blue = payload[2];
	return color;
}
#endif
