/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <zephyr/storage/storage_area/storage_area_ram.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(storage_area_ram, CONFIG_STORAGE_AREA_LOG_LEVEL);

static int sa_ram_read(const struct storage_area *area, size_t start,
		       const struct storage_area_chunk *ch, size_t cnt)
{
	const struct storage_area_ram *ram =
		CONTAINER_OF(area, struct storage_area_ram, area);
	const uint8_t *rstart = (uint8_t *)ram->start;

	for (size_t i = 0U; i < cnt; i++) {

		memcpy(ch[i].data, rstart + start, ch[i].len);
		start += ch[i].len;
	}

	return 0;
}

static int sa_ram_prog(const struct storage_area *area, size_t start,
		       const struct storage_area_chunk *ch, size_t cnt)
{
	const struct storage_area_ram *ram =
		CONTAINER_OF(area, struct storage_area_ram, area);
	const size_t align = area->write_size;
	uint8_t *rstart = (uint8_t *)ram->start;
	uint8_t buf[align];
	size_t bpos = 0U;
	int rc = 0;

	for (size_t i = 0U; i < cnt; i++) {
		uint8_t *data8 = (uint8_t *)ch[i].data;
		size_t blen = ch[i].len;

		if (bpos != 0U) {
			size_t cplen = MIN(blen, align - bpos);

			memcpy(buf + bpos, data8, cplen);
			bpos += cplen;
			blen -= cplen;
			data8 += cplen;

			if (bpos == align) {
				memcpy(rstart + start, buf, align);
				start += align;
				bpos = 0U;
			}
		}

		if (blen >= align) {
			size_t wrlen = blen & ~(align - 1);

			memcpy(rstart + start, data8, wrlen);
			blen -= wrlen;
			data8 += wrlen;
			start += wrlen;
		}

		if (blen > 0U) {
			memcpy(buf, data8, blen);
			bpos = blen;
		}
	}

	return rc;
}

static int sa_ram_erase(const struct storage_area *area, size_t start,
			size_t len)
{
	const struct storage_area_ram *ram =
		CONTAINER_OF(area, struct storage_area_ram, area);

	start *= area->erase_size;

	for (size_t i = 0; i < len; i++) {
		uint8_t *rstart = (uint8_t *)ram->start;
		(void)memset(rstart + start, STORAGE_AREA_ERASEVALUE(area),
			     area->erase_size);
		start += area->erase_size;
	}

	return 0;
}

static int sa_ram_ioctl(const struct storage_area *area,
			enum storage_area_ioctl_cmd cmd, void *data)
{
	const struct storage_area_ram *ram =
		CONTAINER_OF(area, struct storage_area_ram, area);

	int rc = -ENOTSUP;

	switch (cmd) {
	case SA_IOCTL_XIPADDRESS:
		if (data == NULL) {
			LOG_DBG("No return data supplied");
			rc = -EINVAL;
			break;
		}

		uintptr_t *xip_address = (uintptr_t *)data;
		*xip_address = ram->start;
		rc = 0;
		break;

	default:
		break;
	}

	return rc;
}

const struct storage_area_api storage_area_ram_api = {
	.read = sa_ram_read,
	.prog = sa_ram_prog,
	.erase = sa_ram_erase,
	.ioctl = sa_ram_ioctl,
};
