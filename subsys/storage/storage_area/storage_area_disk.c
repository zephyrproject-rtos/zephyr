/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>
#include <zephyr/storage/storage_area/storage_area_disk.h>
#include <zephyr/storage/disk_access.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(storage_area_disk, CONFIG_STORAGE_AREA_LOG_LEVEL);

static int sa_disk_valid(const struct storage_area_disk *disk)
{
	int rc = disk_access_init(disk->name);

	if (rc != 0) {
		return rc;
	}

	if (IS_ENABLED(CONFIG_STORAGE_AREA_VERIFY)) {
		size_t scount;
		size_t ssize;

		rc = disk_access_ioctl(disk->name, DISK_IOCTL_GET_SECTOR_COUNT,
				       &scount);
		if (rc != 0) {
			LOG_DBG("Unable to get disk sector count");
			return -EINVAL;
		}

		rc = disk_access_ioctl(disk->name, DISK_IOCTL_GET_SECTOR_SIZE,
				       &ssize);
		if (rc != 0) {
			LOG_DBG("Unable to get disk sector size");
			return -EINVAL;
		}

		if (disk->ssize != ssize) {
			LOG_DBG("Bad disk sector size");
			return -EINVAL;
		}

		const struct storage_area *area = &disk->area;
		const size_t asz = area->erase_blocks * area->erase_size;
		const size_t esz = scount * ssize;

		if (esz < ((disk->start * ssize) + asz)) {
			LOG_DBG("Bad area size");
			return -EINVAL;
		}

		if ((area->write_size & (ssize - 1)) != 0U) {
			LOG_DBG("Bad area write size");
			return -EINVAL;
		}

		if ((area->erase_size & (ssize - 1)) != 0U) {
			LOG_DBG("Bad area erase size");
			return -EINVAL;
		}
	}

	return 0;
}

static int sa_disk_read(const struct storage_area *area, size_t start,
			const struct storage_area_chunk *ch, size_t cnt)
{
	const struct storage_area_disk *disk =
		CONTAINER_OF(area, struct storage_area_disk, area);
	size_t starts = start / disk->ssize;
	size_t bpos = start % disk->ssize;
	uint8_t buf[disk->ssize];
	int rc = sa_disk_valid(disk);

	if (rc != 0) {
		goto end;
	}

	starts += disk->start;
	rc = disk_access_read(disk->name, buf, starts, 1U);
	if (rc != 0) {
		goto end;
	}

	for (size_t i = 0U; i < cnt; i++) {
		uint8_t *data8 = (uint8_t *)ch[i].data;
		size_t blen = ch[i].len;

		while (blen != 0U) {
			size_t cplen = MIN(blen, disk->ssize - bpos);

			memcpy(data8, buf + bpos, cplen);
			bpos += cplen;
			blen -= cplen;
			data8 += cplen;

			if (bpos == sizeof(buf)) {
				starts++;
				rc = disk_access_read(disk->name, buf, starts,
						      1U);
				if (rc != 0) {
					break;
				}

				bpos = 0U;
			}
		}
	}

	if (rc != 0) {
		LOG_DBG("read failed at %x", starts * disk->ssize);
	}
end:
	return rc;
}

static int sa_disk_prog(const struct storage_area *area, size_t start,
			const struct storage_area_chunk *ch, size_t cnt)
{
	const struct storage_area_disk *disk =
		CONTAINER_OF(area, struct storage_area_disk, area);
	const size_t align = area->write_size;
	const size_t spws = align / disk->ssize;
	uint8_t buf[align];
	size_t bpos = 0U;
	size_t starts = start / disk->ssize;
	int rc = sa_disk_valid(disk);

	if (rc != 0) {
		goto end;
	}

	starts += disk->start;
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
				rc = disk_access_write(disk->name, buf, starts,
						       spws);
				if (rc != 0) {
					break;
				}

				starts += spws;
				bpos = 0U;
			}
		}

		if (blen >= align) {
			size_t wrlen = blen & ~(align - 1);
			size_t wrs = wrlen / disk->ssize;

			rc = disk_access_write(disk->name, data8, starts, wrs);
			if (rc != 0) {
				break;
			}

			blen -= wrlen;
			data8 += wrlen;
			starts += wrs;
		}

		if (blen > 0U) {
			memcpy(buf, data8, blen);
			bpos = blen;
		}
	}

	if (rc != 0) {
		LOG_DBG("prog failed at %x", starts * disk->ssize);
	}
end:
	return rc;
}

static int sa_disk_erase(const struct storage_area *area, size_t start,
			 size_t len)
{
	const struct storage_area_disk *disk =
		CONTAINER_OF(area, struct storage_area_disk, area);
	const size_t spws = area->erase_size / disk->ssize;
	size_t starts = start * spws;
	uint8_t buf[area->erase_size];
	int rc = sa_disk_valid(disk);

	if (rc != 0) {
		goto end;
	}

	memset(buf, STORAGE_AREA_ERASEVALUE(area), sizeof(buf));
	starts += disk->start;

	for (size_t i = 0; i < len; i++) {
		rc = disk_access_write(disk->name, buf, starts, spws);
		if (rc != 0) {
			break;
		}

		starts += spws;
	}

	if (rc != 0) {
		LOG_DBG("prog failed at %x", starts * disk->ssize);
	}
end:
	return rc;
}

static int sa_disk_ioctl(const struct storage_area *area,
			 enum storage_area_ioctl_cmd cmd, void *data)
{
	const struct storage_area_disk *disk =
		CONTAINER_OF(area, struct storage_area_disk, area);
	int rc = sa_disk_valid(disk);

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

const struct storage_area_api storage_area_disk_api = {
	.read = sa_disk_read,
	.prog = sa_disk_prog,
	.erase = sa_disk_erase,
	.ioctl = sa_disk_ioctl,
};
