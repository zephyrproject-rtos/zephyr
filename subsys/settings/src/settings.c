/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <errno.h>

#include "base64.h"

#include "settings/settings.h"
#include "settings_priv.h"
#include <zephyr/types.h>

/* mbedtls-base64 lib encodes data to null-terminated string */
#define BASE64_ENCODE_SIZE(in_size) ((((((in_size) - 1) / 3) * 4) + 4) + 1)

sys_slist_t settings_handlers;

static u8_t settings_cmd_inited;

void settings_store_init(void);
static void s64_to_dec(char *ptr, int buf_len, s64_t value, int base);
static s64_t dec_to_s64(char *p_str, char **e_ptr);

void settings_init(void)
{
	if (!settings_cmd_inited) {
		sys_slist_init(&settings_handlers);
		settings_store_init();

		settings_cmd_inited = 1;
	}
}

int settings_register(struct settings_handler *handler)
{
	sys_slist_prepend(&settings_handlers, &handler->node);
	return 0;
}

/*
 * Find settings_handler based on name.
 */
struct settings_handler *settings_handler_lookup(char *name)
{
	struct settings_handler *ch;

	SYS_SLIST_FOR_EACH_CONTAINER(&settings_handlers, ch, node) {
		if (!strcmp(name, ch->name)) {
			return ch;
		}
	}
	return NULL;
}

/*
 * Separate string into argv array.
 */
int settings_parse_name(char *name, int *name_argc, char *name_argv[])
{
	int i = 0;

	while (name) {
		name_argv[i++] = name;

		while (1) {
			if (*name == '\0') {
				name = NULL;
				break;
			}

			if (*name == *SETTINGS_NAME_SEPARATOR) {
				*name = '\0';
				name++;
				break;
			}
			name++;
		}
	}

	*name_argc = i;

	return 0;
}

static struct settings_handler *settings_parse_and_lookup(char *name,
							  int *name_argc,
							  char *name_argv[])
{
	int rc;

	rc = settings_parse_name(name, name_argc, name_argv);
	if (rc) {
		return NULL;
	}
	return settings_handler_lookup(name_argv[0]);
}

int settings_value_from_str(char *val_str, enum settings_type type, void *vp,
			    int maxlen)
{
	s32_t val;
	s64_t val64;
	char *eptr;

	if (!val_str) {
		goto err;
	}
	switch (type) {
	case SETTINGS_INT8:
	case SETTINGS_INT16:
	case SETTINGS_INT32:
	case SETTINGS_BOOL:
		val = strtol(val_str, &eptr, 0);
		if (*eptr != '\0') {
			goto err;
		}
		if (type == SETTINGS_BOOL) {
			if (val < 0 || val > 1) {
				goto err;
			}
			*(bool *)vp = val;
		} else if (type == SETTINGS_INT8) {
			if (val < INT8_MIN || val > UINT8_MAX) {
				goto err;
			}
			*(int8_t *)vp = val;
		} else if (type == SETTINGS_INT16) {
			if (val < INT16_MIN || val > UINT16_MAX) {
				goto err;
			}
			*(int16_t *)vp = val;
		} else if (type == SETTINGS_INT32) {
			*(s32_t *)vp = val;
		}
		break;
	case SETTINGS_INT64:
		val64 = dec_to_s64(val_str, &eptr);
		if (*eptr != '\0') {
			goto err;
		}
		*(s64_t *)vp = val64;
		break;
	case SETTINGS_STRING:
		val = strlen(val_str);
		if (val + 1 > maxlen) {
			goto err;
		}
		strcpy(vp, val_str);
		break;
	default:
		goto err;
	}
	return 0;
err:
	return -EINVAL;
}

int settings_bytes_from_str(char *val_str, void *vp, int *len)
{
	int err;
	int rc;

	err = base64_decode(vp, *len, &rc, val_str, strlen(val_str));

	if (err) {
		return -1;
	}

	*len = rc;
	return 0;
}

char *settings_str_from_value(enum settings_type type, void *vp, char *buf,
			      int buf_len)
{
	s32_t val;

	if (type == SETTINGS_STRING) {
		return vp;
	}
	switch (type) {
	case SETTINGS_INT8:
	case SETTINGS_INT16:
	case SETTINGS_INT32:
	case SETTINGS_BOOL:
		if (type == SETTINGS_BOOL) {
			val = *(bool *)vp;
		} else if (type == SETTINGS_INT8) {
			val = *(int8_t *)vp;
		} else if (type == SETTINGS_INT16) {
			val = *(int16_t *)vp;
		} else {
			val = *(s32_t *)vp;
		}
		snprintf(buf, buf_len, "%ld", (long)val);
		return buf;
	case SETTINGS_INT64:
		s64_to_dec(buf, buf_len, *(s64_t *)vp, 10);
		return buf;
	default:
		return NULL;
	}
}

static void u64_to_dec(char *ptr, int buf_len, u64_t value, int base)
{
	u64_t t = 0, res = 0;
	u64_t tmp = value;
	int count = 0;

	if (ptr == NULL) {
		return;
	}

	if (tmp == 0) {
		count++;
	}

	while (tmp > 0) {
		tmp = tmp/base;
		count++;
	}

	ptr += count;

	*ptr = '\0';

	do {
		res = value - base * (t = value / base);
		if (res < 10) {
			*--ptr = '0' + res;
		} else if ((res >= 10) && (res < 16)) {
			*--ptr = 'A' - 10 + res;
		}
		value = t;
	} while (value != 0);
}

static void s64_to_dec(char *ptr, int buf_len, s64_t value, int base)
{
	u64_t val64;

	if (ptr == NULL || buf_len < 1) {
		return;
	}

	if (value < 0) {
		*ptr = '-';
		ptr++;
		buf_len--;
		val64 = value * (-1);
	} else {
		val64 = value;
	}

	u64_to_dec(ptr, buf_len, val64, base);
}

static s64_t dec_to_s64(char *p_str, char **e_ptr)
{
	u64_t val = 0, prev_val = 0;
	bool neg = false;
	int digit;

	if (*p_str == '-') {
		neg = true;
		p_str++;
	} else if (*p_str == '+') {
		p_str++;
	}

	while (1) {
		if (*p_str >= '0' && *p_str <= '9') {
			digit = *p_str - '0';
		} else {
			break;
		}

		val *= 10;
		val += digit;

		/* this is only a fuse */
		if (val < prev_val) {
			break;
		}

		prev_val = val;
		p_str++;
	}

	if (e_ptr != 0)
		*e_ptr = p_str;

	if (neg) {
		return -val;
	} else {
		return val;
	}
}

char *settings_str_from_bytes(void *vp, int vp_len, char *buf, int buf_len)
{
	if (BASE64_ENCODE_SIZE(vp_len) > buf_len) {
		return NULL;
	}

	size_t enc_len;

	base64_encode(buf, buf_len, &enc_len, vp, vp_len);

	return buf;
}

int settings_set_value(char *name, char *val_str)
{
	int name_argc;
	char *name_argv[SETTINGS_MAX_DIR_DEPTH];
	struct settings_handler *ch;

	ch = settings_parse_and_lookup(name, &name_argc, name_argv);
	if (!ch) {
		return -EINVAL;
	}

	return ch->h_set(name_argc - 1, &name_argv[1], val_str);
}

/*
 * Get value in printable string form. If value is not string, the value
 * will be filled in *buf.
 * Return value will be pointer to beginning of that buffer,
 * except for string it will pointer to beginning of string.
 */
char *settings_get_value(char *name, char *buf, int buf_len)
{
	int name_argc;
	char *name_argv[SETTINGS_MAX_DIR_DEPTH];
	struct settings_handler *ch;

	ch = settings_parse_and_lookup(name, &name_argc, name_argv);
	if (!ch) {
		return NULL;
	}

	if (!ch->h_get) {
		return NULL;
	}
	return ch->h_get(name_argc - 1, &name_argv[1], buf, buf_len);
}

int settings_commit(char *name)
{
	int name_argc;
	char *name_argv[SETTINGS_MAX_DIR_DEPTH];
	struct settings_handler *ch;
	int rc;
	int rc2;

	if (name) {
		ch = settings_parse_and_lookup(name, &name_argc, name_argv);
		if (!ch) {
			return -EINVAL;
		}
		if (ch->h_commit) {
			return ch->h_commit();
		} else {
			return 0;
		}
	} else {
		rc = 0;
		SYS_SLIST_FOR_EACH_CONTAINER(&settings_handlers, ch, node) {
			if (ch->h_commit) {
				rc2 = ch->h_commit();
				if (!rc) {
					rc = rc2;
				}
			}
		}
		return rc;
	}
}
