/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spinel.h"

#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spinel);

static bool spinel_validate_utf8(const uint8_t *string)
{
	uint8_t byte;
	uint8_t cont_bytes = 0;

	while ((byte = *string++) != 0) {
		if ((byte & 0x80) == 0) {
			continue;
		}

		if ((byte & 0x40) == 0) {
			return false;
		} else if ((byte & 0x20) == 0) {
			cont_bytes = 1;
		} else if ((byte & 0x10) == 0) {
			cont_bytes = 2;
		} else if ((byte & 0x08) == 0) {
			cont_bytes = 3;
		} else {
			return false;
		}

		while (cont_bytes-- != 0) {
			byte = *string++;

			if ((byte & 0xc0) != 0x80) {
				return false;
			}
		}
	}

	return true;
}

static const char *spinel_next_packed_datatype(const char *fmt)
{
	int depth = 0;

	do {
		switch (*++fmt) {
		case '(':
			depth++;
			break;

		case ')':
			depth--;

			if (!depth) {
				fmt++;
			}
			break;
		}
	} while ((depth > 0) && *fmt != 0);

	return fmt;
}

static uint8_t spinel_packed_uint_size(uint32_t value)
{
	uint8_t ret;

	if (value < (1 << 7)) {
		ret = 1;
	} else if (value < (1 << 14)) {
		ret = 2;
	} else if (value < (1 << 21)) {
		ret = 3;
	} else if (value < (1 << 28)) {
		ret = 4;
	} else {
		ret = 5;
	}

	return ret;
}

static uint8_t spinel_packed_uint_encode(uint8_t *bytes, uint8_t len, unsigned int value)
{
	uint8_t encoded_size = spinel_packed_uint_size(value);

	if (len >= encoded_size) {
		for (uint8_t i = 0; i != encoded_size - 1; ++i) {
			*bytes++ = (value & 0x7F) | 0x80;
			value = (value >> 7);
		}

		*bytes++ = (value & 0x7F);
	}

	return encoded_size;
}

static int spinel_packed_uint_decode(const uint8_t *bytes, uint16_t len, unsigned int *value_ptr)
{
	int ret = 0;
	unsigned int value = 0;
	unsigned int i = 0;

	do {
		if ((len < sizeof(uint8_t)) || (i >= sizeof(unsigned int) * 8)) {
			return -EINVAL;
		}

		value |= (unsigned int)(bytes[0] & 0x7F) << i;
		i += 7;
		ret += sizeof(uint8_t);
		bytes += sizeof(uint8_t);
		len -= sizeof(uint8_t);
	} while ((bytes[-1] & 0x80) == 0x80);

	if (value_ptr) {
		*value_ptr = value;
	}

	return ret;
}

int spinel_datatype_vpack(spinel_tx_cb tx_cb, const void *ctx, const char *fmt, va_list *args)
{
	int ret = 0;

	for (; *fmt != 0; fmt = spinel_next_packed_datatype(fmt)) {
		if (*fmt == ')') {
			/* Don't go past the end of a struct. */
			break;
		}

		switch ((uint8_t)*fmt) {
		case SPINEL_DATATYPE_BOOL_C: {
			uint8_t arg = (uint8_t)(va_arg(*args, int) != false);

			if (tx_cb) {
				tx_cb(arg, ctx);
			}

			ret += sizeof(uint8_t);
			break;
		}

		case SPINEL_DATATYPE_INT8_C:
		case SPINEL_DATATYPE_UINT8_C: {
			uint8_t arg = (uint8_t)va_arg(*args, int);

			if (tx_cb) {
				tx_cb(arg, ctx);
			}

			ret += sizeof(uint8_t);
			break;
		}

		case SPINEL_DATATYPE_INT16_C:
		case SPINEL_DATATYPE_UINT16_C: {
			uint16_t arg = (uint16_t)va_arg(*args, int);

			if (tx_cb) {
				tx_cb((uint8_t)(arg & 0xFF), ctx);
				tx_cb((uint8_t)((arg >> 8) & 0xFF), ctx);
			}

			ret += sizeof(uint16_t);
			break;
		}

		case SPINEL_DATATYPE_INT32_C:
		case SPINEL_DATATYPE_UINT32_C: {
			uint32_t arg = (uint32_t)va_arg(*args, int);

			if (tx_cb) {
				tx_cb((uint8_t)(arg & 0xFF), ctx);
				tx_cb((uint8_t)((arg >> 8) & 0xFF), ctx);
				tx_cb((uint8_t)((arg >> 16) & 0xFF), ctx);
				tx_cb((uint8_t)((arg >> 24) & 0xFF), ctx);
			}

			ret += sizeof(uint32_t);
			break;
		}

		case SPINEL_DATATYPE_INT64_C:
		case SPINEL_DATATYPE_UINT64_C: {
			uint64_t arg = va_arg(*args, uint64_t);

			if (tx_cb) {
				tx_cb((uint8_t)(arg & 0xFF), ctx);
				tx_cb((uint8_t)((arg >> 8) & 0xFF), ctx);
				tx_cb((uint8_t)((arg >> 16) & 0xFF), ctx);
				tx_cb((uint8_t)((arg >> 24) & 0xFF), ctx);
				tx_cb((uint8_t)((arg >> 32) & 0xFF), ctx);
				tx_cb((uint8_t)((arg >> 40) & 0xFF), ctx);
				tx_cb((uint8_t)((arg >> 48) & 0xFF), ctx);
				tx_cb((uint8_t)((arg >> 56) & 0xFF), ctx);
			}

			ret += sizeof(uint64_t);
			break;
		}

		case SPINEL_DATATYPE_IPv6ADDR_C: {
			struct spinel_ipv6addr *arg = va_arg(*args, struct spinel_ipv6addr *);

			if (tx_cb) {
				for (uint8_t i = 0; i < sizeof(struct spinel_ipv6addr); i++) {
					tx_cb(arg->bytes[i], ctx);
				}
			}

			ret += sizeof(struct spinel_ipv6addr);
			break;
		}

		case SPINEL_DATATYPE_EUI48_C: {
			struct spinel_eui48 *arg = va_arg(*args, struct spinel_eui48 *);

			if (tx_cb) {
				for (uint8_t i = 0; i < sizeof(struct spinel_eui48); i++) {
					tx_cb(arg->bytes[i], ctx);
				}
			}

			ret += sizeof(struct spinel_eui48);
			break;
		}

		case SPINEL_DATATYPE_EUI64_C: {
			struct spinel_eui64 *arg = va_arg(*args, struct spinel_eui64 *);

			if (tx_cb) {
				for (uint8_t i = 0; i < sizeof(struct spinel_eui64); i++) {
					tx_cb(arg->bytes[i], ctx);
				}
			}

			ret += sizeof(struct spinel_eui64);
			break;
		}

		case SPINEL_DATATYPE_UINT_PACKED_C: {
			unsigned int arg = va_arg(*args, unsigned int);

			if (arg > SPINEL_MAX_UINT_PACKED) {
				return -EINVAL;
			}

			uint8_t bytes[5];
			uint8_t encoded_size = spinel_packed_uint_encode(bytes, sizeof(bytes), arg);

			if (tx_cb) {
				for (uint8_t i = 0; i < encoded_size; i++) {
					tx_cb(bytes[i], ctx);
				}
			}

			ret += encoded_size;
			break;
		}

		case SPINEL_DATATYPE_UTF8_C: {
			char *string_arg = va_arg(*args, char *);

			if (!string_arg) {
				string_arg = "";
			}

			size_t len = strlen(string_arg) + 1;

			if (tx_cb) {
				for (size_t i = 0; i < len; i++) {
					tx_cb(string_arg[i], ctx);
				}
			}

			ret += len;
			break;
		}

		case SPINEL_DATATYPE_DATA_WLEN_C:
		case SPINEL_DATATYPE_DATA_C: {
			uint8_t *data_arg = va_arg(*args, uint8_t *);
			uint16_t data_size_arg = (uint16_t)va_arg(*args, uint32_t);

			if (!data_arg || !data_size_arg) {
				return -EINVAL;
			}

			char nextformat = *spinel_next_packed_datatype(fmt);

			if ((fmt[0] == SPINEL_DATATYPE_DATA_WLEN_C) ||
			    ((nextformat != 0) && (nextformat != ')'))) {

				int size_len = spinel_datatype_pack(
					tx_cb, ctx, SPINEL_DATATYPE_UINT16_S, data_size_arg);

				if (size_len < 0) {
					return size_len;
				}

				ret += size_len;
			}

			if (tx_cb) {
				for (uint16_t i = 0; i < data_size_arg; i++) {
					tx_cb(data_arg[i], ctx);
				}
			}

			ret += data_size_arg;
			break;
		}

		case 'T':
		case SPINEL_DATATYPE_STRUCT_C: {
			int size_len = 0;
			int struct_len = 0;
			char nextformat = *spinel_next_packed_datatype(fmt);

			if (fmt[1] != '(') {
				return -EINVAL;
			}

			do {
				va_list sub_args;

				va_copy(sub_args, *args);
				struct_len = spinel_datatype_vpack(NULL, NULL, fmt + 2, &sub_args);
				va_end(sub_args);

				if (struct_len < 0) {
					return struct_len;
				}
			} while (0);

			if ((fmt[0] == SPINEL_DATATYPE_STRUCT_C) ||
			    ((nextformat != 0) && (nextformat != ')'))) {

				size_len = spinel_datatype_pack(
					tx_cb, ctx, SPINEL_DATATYPE_UINT16_S, struct_len);

				if (size_len < 0) {
					return size_len;
				}
			}

			struct_len = spinel_datatype_vpack(tx_cb, ctx, fmt + 2, args);
			if (struct_len < 0) {
				return struct_len;
			}

			ret += struct_len + size_len;
			break;
		}

		case SPINEL_DATATYPE_VOID_C:
			break;

		default:
			LOG_ERR("Unsupported spinel type: %u", (char)*fmt);
			ret -= EINVAL;
		}
	}

	return ret;
}

int spinel_datatype_pack(spinel_tx_cb tx_cb, const void *ctx, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);

	int ret = spinel_datatype_vpack(tx_cb, ctx, fmt, &args);

	va_end(args);
	return ret;
}

int spinel_datatype_vunpack(const uint8_t *data_in, uint16_t data_len, bool copy_buff,
			    const char *fmt, va_list *args)
{
	int ret = 0;

	for (; *fmt != 0; fmt = spinel_next_packed_datatype(fmt)) {
		if (*fmt == ')') {
			/* Don't go past the end of a struct. */
			break;
		}

		switch ((uint8_t)*fmt) {
		case SPINEL_DATATYPE_BOOL_C: {
			bool *arg_ptr = va_arg(*args, bool *);

			if (data_len < sizeof(uint8_t)) {
				return -EOVERFLOW;
			}

			if (arg_ptr) {
				*arg_ptr = data_in[0] != 0;
			}

			ret += sizeof(uint8_t);
			data_in += sizeof(uint8_t);
			data_len -= sizeof(uint8_t);
			break;
		}

		case SPINEL_DATATYPE_INT8_C:
		case SPINEL_DATATYPE_UINT8_C: {
			uint8_t *arg_ptr = va_arg(*args, uint8_t *);

			if (data_len < sizeof(uint8_t)) {
				return -EOVERFLOW;
			}

			if (arg_ptr) {
				*arg_ptr = data_in[0];
			}

			ret += sizeof(uint8_t);
			data_in += sizeof(uint8_t);
			data_len -= sizeof(uint8_t);
			break;
		}

		case SPINEL_DATATYPE_INT16_C:
		case SPINEL_DATATYPE_UINT16_C: {
			uint16_t *arg_ptr = va_arg(*args, uint16_t *);

			if (data_len < sizeof(uint16_t)) {
				return -EOVERFLOW;
			}

			if (arg_ptr) {
				*arg_ptr = (uint16_t)((data_in[1] << 8) | data_in[0]);
			}

			ret += sizeof(uint16_t);
			data_in += sizeof(uint16_t);
			data_len -= sizeof(uint16_t);
			break;
		}

		case SPINEL_DATATYPE_INT32_C:
		case SPINEL_DATATYPE_UINT32_C: {
			uint32_t *arg_ptr = va_arg(*args, uint32_t *);

			if (data_len < sizeof(uint32_t)) {
				return -EOVERFLOW;
			}

			if (arg_ptr) {
				*arg_ptr = (uint32_t)((data_in[3] << 24) | (data_in[2] << 16) |
						      (data_in[1] << 8) | data_in[0]);
			}

			ret += sizeof(uint32_t);
			data_in += sizeof(uint32_t);
			data_len -= sizeof(uint32_t);
			break;
		}

		case SPINEL_DATATYPE_INT64_C:
		case SPINEL_DATATYPE_UINT64_C: {
			uint64_t *arg_ptr = va_arg(*args, uint64_t *);

			if (data_len < sizeof(uint64_t)) {
				return -EOVERFLOW;
			}

			if (arg_ptr) {
				uint32_t l32 = (uint32_t)((data_in[3] << 24) | (data_in[2] << 16) |
							  (data_in[1] << 8) | data_in[0]);
				uint32_t h32 = (uint32_t)((data_in[7] << 24) | (data_in[6] << 16) |
							  (data_in[5] << 8) | data_in[4]);

				*arg_ptr = ((uint64_t)l32) | (((uint64_t)h32) << 32);
			}

			ret += sizeof(uint64_t);
			data_in += sizeof(uint64_t);
			data_len -= sizeof(uint64_t);
			break;
		}

		case SPINEL_DATATYPE_IPv6ADDR_C: {
			if (data_len < sizeof(struct spinel_ipv6addr)) {
				return -EOVERFLOW;
			}

			if (copy_buff) {
				struct spinel_ipv6addr *arg =
					va_arg(*args, struct spinel_ipv6addr *);

				if (arg) {
					memcpy(arg, data_in, sizeof(struct spinel_ipv6addr));
				}
			} else {
				const struct spinel_ipv6addr **arg_ptr =
					va_arg(*args, const struct spinel_ipv6addr **);

				if (arg_ptr) {
					*arg_ptr = (const struct spinel_ipv6addr *)data_in;
				}
			}

			ret += sizeof(struct spinel_ipv6addr);
			data_in += sizeof(struct spinel_ipv6addr);
			data_len -= sizeof(struct spinel_ipv6addr);
			break;
		}

		case SPINEL_DATATYPE_EUI64_C: {
			if (data_len < sizeof(struct spinel_eui64)) {
				return -EOVERFLOW;
			}

			if (copy_buff) {
				struct spinel_eui64 *arg = va_arg(*args, struct spinel_eui64 *);

				if (arg) {
					memcpy(arg, data_in, sizeof(struct spinel_eui64));
				}
			} else {
				const struct spinel_eui64 **arg_ptr =
					va_arg(*args, const struct spinel_eui64 **);

				if (arg_ptr) {
					*arg_ptr = (const struct spinel_eui64 *)data_in;
				}
			}

			ret += sizeof(struct spinel_eui64);
			data_in += sizeof(struct spinel_eui64);
			data_len -= sizeof(struct spinel_eui64);
			break;
		}

		case SPINEL_DATATYPE_EUI48_C: {
			if (data_len < sizeof(struct spinel_eui48)) {
				return -EOVERFLOW;
			}

			if (copy_buff) {
				struct spinel_eui48 *arg = va_arg(*args, struct spinel_eui48 *);

				if (arg) {
					memcpy(arg, data_in, sizeof(struct spinel_eui48));
				}
			} else {
				const struct spinel_eui48 **arg_ptr =
					va_arg(*args, const struct spinel_eui48 **);

				if (arg_ptr) {
					*arg_ptr = (const struct spinel_eui48 *)data_in;
				}
			}

			ret += sizeof(struct spinel_eui48);
			data_in += sizeof(struct spinel_eui48);
			data_len -= sizeof(struct spinel_eui48);
			break;
		}

		case SPINEL_DATATYPE_UINT_PACKED_C: {
			unsigned int *arg_ptr = va_arg(*args, unsigned int *);
			int decoded_size = spinel_packed_uint_decode(data_in, data_len, arg_ptr);

			if (arg_ptr && *arg_ptr > SPINEL_MAX_UINT_PACKED) {
				return -ERANGE;
			}

			if (decoded_size <= 0 || data_len < decoded_size) {
				return -EINVAL;
			}

			ret += decoded_size;
			data_in += (uint16_t)decoded_size;
			data_len -= (uint16_t)decoded_size;
			break;
		}

		case SPINEL_DATATYPE_UTF8_C: {
			size_t len = strnlen((const char *)data_in, data_len) + 1;

			if (data_len < len) {
				return -EOVERFLOW;
			}

			if (!spinel_validate_utf8(data_in)) {
				return -EINVAL;
			}

			if (copy_buff) {
				char *arg = va_arg(*args, char *);
				size_t len_arg = va_arg(*args, size_t);

				if (arg) {
					if (len_arg < len) {
						return -EOVERFLOW;
					}

					memcpy(arg, data_in, len);
				}
			} else {
				const char **arg_ptr = va_arg(*args, const char **);

				if (arg_ptr) {
					*arg_ptr = (const char *)data_in;
				}
			}

			ret += len;
			data_in += (uint16_t)len;
			data_len -= (uint16_t)len;
			break;
		}

		case SPINEL_DATATYPE_DATA_C:
		case SPINEL_DATATYPE_DATA_WLEN_C: {
			int pui_len = 0;
			uint16_t block_len = 0;
			const uint8_t *block_ptr = data_in;
			void *arg_ptr = va_arg(*args, void *);
			uint16_t *block_len_ptr = va_arg(*args, uint16_t *);
			char nextformat = *spinel_next_packed_datatype(fmt);

			if ((fmt[0] == SPINEL_DATATYPE_DATA_WLEN_C) ||
			    ((nextformat != 0) && (nextformat != ')'))) {

				pui_len = spinel_datatype_unpack(data_in, data_len, copy_buff,
								 SPINEL_DATATYPE_UINT16_S,
								 &block_len);

				if (pui_len < 0) {
					return pui_len;
				}

				block_ptr += (uint16_t)pui_len;
			} else {
				block_len = data_len;
				pui_len = 0;
			}

			if (data_len < block_len + pui_len) {
				return -EOVERFLOW;
			}

			if (copy_buff) {
				if (arg_ptr && block_len_ptr) {
					if (*block_len_ptr < block_len) {
						return -EOVERFLOW;
					}

					memcpy(arg_ptr, block_ptr, block_len);
				}
			} else {
				const uint8_t **block_ptr_ptr = (const uint8_t **)arg_ptr;

				if (block_ptr_ptr) {
					*block_ptr_ptr = block_ptr;
				}
			}

			if (block_len_ptr) {
				*block_len_ptr = block_len;
			}

			block_len += (uint16_t)pui_len;
			ret += block_len;
			data_in += block_len;
			data_len -= block_len;
			break;
		}

		case 'T':
		case SPINEL_DATATYPE_STRUCT_C: {
			int pui_len = 0;
			uint16_t block_len = 0;
			const uint8_t *block_ptr = data_in;
			int actual_len = 0;
			char nextformat = *spinel_next_packed_datatype(fmt);

			if ((fmt[0] == SPINEL_DATATYPE_STRUCT_C) ||
			    ((nextformat != 0) && (nextformat != ')'))) {

				pui_len = spinel_datatype_unpack(data_in, data_len, copy_buff,
								 SPINEL_DATATYPE_UINT16_S,
								 &block_len);

				if (pui_len < 0) {
					return pui_len;
				}

				block_ptr += (uint16_t)pui_len;
			} else {
				block_len = data_len;
				pui_len = 0;
			}

			if (data_len < block_len + pui_len) {
				return -EOVERFLOW;
			}

			actual_len = spinel_datatype_vunpack(block_ptr, block_len, copy_buff,
							     fmt + 2, args);

			if (actual_len < 0) {
				return actual_len;
			}

			if (pui_len) {
				block_len += (uint16_t)pui_len;
			} else {
				block_len = (uint16_t)actual_len;
			}

			ret += block_len;
			data_in += block_len;
			data_len -= block_len;
			break;
		}

		case SPINEL_DATATYPE_VOID_C:
			break;

		default:
			LOG_ERR("Unsupported spinel type: %u", (char)*fmt);
			ret -= EINVAL;
		}
	}

	return ret;
}

int spinel_datatype_unpack(const uint8_t *data_in, uint16_t data_len, bool copy_buff,
			   const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);

	int ret = spinel_datatype_vunpack(data_in, data_len, copy_buff, fmt, &args);

	va_end(args);
	return ret;
}
