/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/storage/storage_area/storage_area_ram.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(storage_area_ram, CONFIG_STORAGE_AREA_LOG_LEVEL);

static int sa_ram_readv(const struct storage_area *area, sa_off_t offset,
			const struct storage_area_iovec *iovec, size_t iovcnt)
{
	const struct storage_area_ram *ram =
		CONTAINER_OF(area, struct storage_area_ram, area);
	uint8_t *rd = (uint8_t *)(ram->start + (uintptr_t)offset);

	for (size_t i = 0U; i < iovcnt; i++) {
		memcpy(iovec[i].data, rd, iovec[i].len);
		rd += iovec[i].len;
	}

	return 0;
}

static int sa_ram_writev(const struct storage_area *area, sa_off_t offset,
			 const struct storage_area_iovec *iovec, size_t iovcnt)
{
	const struct storage_area_ram *ram =
		CONTAINER_OF(area, struct storage_area_ram, area);
	const size_t align = area->write_size;
	uint8_t *wr = (uint8_t *)(ram->start + (uintptr_t)offset);
	uint8_t buf[align];
	size_t bpos = 0U;
	int rc = 0;

	for (size_t i = 0U; i < iovcnt; i++) {
		uint8_t *data8 = (uint8_t *)iovec[i].data;
		size_t blen = iovec[i].len;

		if (bpos != 0U) {
			size_t cplen = MIN(blen, align - bpos);

			memcpy(buf + bpos, data8, cplen);
			bpos += cplen;
			blen -= cplen;
			data8 += cplen;

			if (bpos == align) {
				memcpy(wr, buf, align);
				wr += align;
				bpos = 0U;
			}
		}

		if (blen >= align) {
			size_t wrlen = blen & ~(align - 1);

			memcpy(wr, data8, wrlen);
			blen -= wrlen;
			data8 += wrlen;
			wr += wrlen;
		}

		if (blen > 0U) {
			memcpy(buf, data8, blen);
			bpos = blen;
		}
	}

	return rc;
}

static int sa_ram_erase(const struct storage_area *area, size_t sblk,
			size_t bcnt)
{
	const struct storage_area_ram *ram =
		CONTAINER_OF(area, struct storage_area_ram, area);
	uint8_t *wr = (uint8_t *)(ram->start + sblk * area->erase_size);

	(void)memset(wr, STORAGE_AREA_ERASEVALUE(area), bcnt * area->erase_size);
	return 0;
}

static int sa_ram_ioctl(const struct storage_area *area,
			enum storage_area_ioctl_cmd cmd, void *data)
{
	const struct storage_area_ram *ram =
		CONTAINER_OF(area, struct storage_area_ram, area);

	int rc = -ENOTSUP;

	switch (cmd) {
	case STORAGE_AREA_IOCTL_XIPADDRESS:
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

const struct storage_area_api storage_area_ram_rw_api = {
	.readv = sa_ram_readv,
	.writev = sa_ram_writev,
	.erase = sa_ram_erase,
	.ioctl = sa_ram_ioctl,
};

const struct storage_area_api storage_area_ram_ro_api = {
	.readv = sa_ram_readv,
	.ioctl = sa_ram_ioctl,
};
