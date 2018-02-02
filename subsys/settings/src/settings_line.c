/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <string.h>

#include "settings/settings.h"
#include "settings_priv.h"

int settings_line_parse(char *buf, char **namep, char **valp)
{
	char *cp;
	enum {
		FIND_NAME,
		FIND_NAME_END,
		FIND_VAL,
		FIND_VAL_END
		} state = FIND_NAME;

	*valp = NULL;
	for (cp = buf; *cp != '\0'; cp++) {
		switch (state) {
		case FIND_NAME:
			if (!isspace((unsigned char)*cp)) {
				*namep = cp;
				state = FIND_NAME_END;
			}
			break;
		case FIND_NAME_END:
			if (*cp == '=') {
				*cp = '\0';
				state = FIND_VAL;
			} else if (isspace((unsigned char)*cp)) {
				*cp = '\0';
			}
			break;
		case FIND_VAL:
			if (!isspace((unsigned char)*cp)) {
				*valp = cp;
				state = FIND_VAL_END;
			}
			break;
		case FIND_VAL_END:
			if (isspace((unsigned char)*cp)) {
				*cp = '\0';
			}
			break;
		}
	}

	if (state == FIND_VAL_END || state == FIND_VAL) {
		return 0;
	} else {
		return -1;
	}
}

int settings_line_make(char *dst, int dlen, const char *name, const char *value)
{
	int nlen;
	int vlen;
	int off;

	nlen = strlen(name);

	if (value) {
		vlen = strlen(value);
	} else {
		vlen = 0;
	}

	if (nlen + vlen + 2 > dlen) {
		return -1;
	}

	memcpy(dst, name, nlen);
	off = nlen;
	dst[off++] = '=';

	memcpy(dst + off, value, vlen);
	off += vlen;
	dst[off] = '\0';

	return off;
}
