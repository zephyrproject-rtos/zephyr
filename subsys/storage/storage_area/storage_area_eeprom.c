/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/storage/storage_area/storage_area_eeprom.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(storage_area_eeprom, CONFIG_STORAGE_AREA_LOG_LEVEL);

static int sa_eeprom_valid(const struct storage_area_eeprom *eeprom)
{
	if (!device_is_ready(eeprom->dev)) {
		LOG_DBG("Device is not ready");
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_STORAGE_AREA_VERIFY)) {
		const struct storage_area *area = &eeprom->area;
		const size_t asz = area->erase_blocks * area->erase_size;
		const size_t esz = eeprom_get_size(eeprom->dev);

		if (esz < (eeprom->doffset + asz)) {
			LOG_DBG("Bad area size");
			return -EINVAL;
		}
	}

	return 0;
}

static int sa_eeprom_readv(const struct storage_area *area, sa_off_t offset,
			   const struct storage_area_iovec *iovec, size_t iovcnt)
{
	const struct storage_area_eeprom *eeprom =
		CONTAINER_OF(area, struct storage_area_eeprom, area);
	int rc = sa_eeprom_valid(eeprom);

	if (rc != 0) {
		goto end;
	}

	off_t rdoff = eeprom->doffset + (off_t)offset;

	for (size_t i = 0U; i < iovcnt; i++) {
		rc = eeprom_read(eeprom->dev, rdoff, iovec[i].data,
				 iovec[i].len);
		if (rc != 0) {
			break;
		}

		rdoff += iovec[i].len;
	}

	if (rc != 0) {
		LOG_DBG("read failed at %lx", rdoff - eeprom->doffset);
	}
end:
	return rc;
}

static int sa_eeprom_writev(const struct storage_area *area, sa_off_t offset,
			    const struct storage_area_iovec *iovec,
			    size_t iovcnt)
{
	const struct storage_area_eeprom *eeprom =
		CONTAINER_OF(area, struct storage_area_eeprom, area);
	const size_t align = area->write_size;
	uint8_t buf[align];
	size_t bpos = 0U;
	int rc = sa_eeprom_valid(eeprom);

	if (rc != 0) {
		goto end;
	}

	off_t wroff = eeprom->doffset + (off_t)offset;

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
				rc = eeprom_write(eeprom->dev, wroff, buf,
						  align);
				if (rc != 0) {
					break;
				}

				wroff += align;
				bpos = 0U;
			}
		}

		if (blen >= align) {
			size_t wrlen = blen & ~(align - 1);

			rc = eeprom_write(eeprom->dev, wroff, data8, wrlen);
			if (rc != 0) {
				break;
			}

			blen -= wrlen;
			data8 += wrlen;
			wroff += wrlen;
		}

		if (blen > 0U) {
			memcpy(buf, data8, blen);
			bpos = blen;
		}
	}

	if (rc != 0) {
		LOG_DBG("write failed at %lx", wroff - eeprom->doffset);
	}
end:
	return rc;
}

static int sa_eeprom_erase(const struct storage_area *area, size_t sblk,
			   size_t bcnt)
{
	const struct storage_area_eeprom *eeprom =
		CONTAINER_OF(area, struct storage_area_eeprom, area);
	off_t eoff = eeprom->doffset + sblk * area->erase_size;
	uint8_t buf[area->erase_size];
	int rc = sa_eeprom_valid(eeprom);

	if (rc != 0) {
		goto end;
	}

	memset(buf, STORAGE_AREA_ERASEVALUE(area), sizeof(buf));
	for (size_t i = 0; i < bcnt; i++) {
		rc = eeprom_write(eeprom->dev, eoff, buf, sizeof(buf));
		if (rc != 0) {
			break;
		}

		eoff += area->erase_size;
	}

	if (rc != 0) {
		LOG_DBG("write failed at %lx", eoff - eeprom->doffset);
	}
end:
	return rc;
}

static int sa_eeprom_ioctl(const struct storage_area *area,
			   enum storage_area_ioctl_cmd cmd, void *data)
{
	const struct storage_area_eeprom *eeprom =
		CONTAINER_OF(area, struct storage_area_eeprom, area);
	int rc = sa_eeprom_valid(eeprom);

	if (rc != 0) {
		goto end;
	}

	rc = -ENOTSUP;
	switch (cmd) {
	default:
		break;
	}
end:
	return rc;
}

const struct storage_area_api storage_area_eeprom_rw_api = {
	.readv = sa_eeprom_readv,
	.writev = sa_eeprom_writev,
	.erase = sa_eeprom_erase,
	.ioctl = sa_eeprom_ioctl,
};

const struct storage_area_api storage_area_eeprom_ro_api = {
	.readv = sa_eeprom_readv,
	.ioctl = sa_eeprom_ioctl,
};
