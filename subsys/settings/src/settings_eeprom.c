/*
 * Copyright (c) 2021 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <drivers/eeprom.h>
#include <sys/crc.h>

#include "settings/settings.h"
#include "settings/settings_eeprom.h"
#include "settings_priv.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(settings, CONFIG_SETTINGS_LOG_LEVEL);

BUILD_ASSERT(SETTINGS_MAX_VAL_LEN <= 256,
	     "Settings max value length to large for eeprom settings storage");
BUILD_ASSERT((SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN) <= 256,
	     "Settings max name length to large for eeprom settings storage");

#define MAX_REC_LEN sizeof(uint16_t) + sizeof(uint8_t) + SETTINGS_MAX_NAME_LEN \
		    + SETTINGS_EXTRA_LEN + SETTINGS_MAX_VAL_LEN \
		    + sizeof(uint16_t)
#define MIN_REC_LEN sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint16_t)

struct settings_eeprom_read_fn_arg {
	const struct device *eeprom;
	off_t address; /* address to read from */
	size_t len;    /* maximum length to read */
};

static int settings_eeprom_load(struct settings_store *cs,
				const struct settings_load_arg *arg);
static int settings_eeprom_save(struct settings_store *cs, const char *name,
				const char *value, size_t val_len);

static struct settings_store_itf settings_eeprom_itf = {
	.csi_load = settings_eeprom_load,
	.csi_save = settings_eeprom_save,
};

static ssize_t settings_eeprom_read_fn(void *back_end, void *data, size_t len)
{
	struct settings_eeprom_read_fn_arg *rd_fn_arg;
	int rc;

	rd_fn_arg = (struct settings_eeprom_read_fn_arg *)back_end;

	if (!device_is_ready(rd_fn_arg->eeprom)) {
		return -EIO;
	}

	if (rd_fn_arg->len < len) {
		len = rd_fn_arg->len;
	}

	rc = eeprom_read(rd_fn_arg->eeprom, rd_fn_arg->address, data, len);
	if (rc) {
		return (ssize_t)rc;
	}
	rd_fn_arg->address += len;
	rd_fn_arg->len -= len;

	return (ssize_t)len;
}

int settings_eeprom_src(struct settings_eeprom *cf)
{
	cf->cf_store.cs_itf = &settings_eeprom_itf;
	settings_src_register(&cf->cf_store);

	return 0;
}

int settings_eeprom_dst(struct settings_eeprom *cf)
{
	cf->cf_store.cs_itf = &settings_eeprom_itf;
	settings_dst_register(&cf->cf_store);

	return 0;
}

/*
 * Step to the end of the next valid record, update record info and record id.
 * The record info and record id are "optional" arguments, NULL is allowed.
 * return value: size of the valid record if there was a record,
 *               0 if there was no record,
 *               Error code on EEPROM read error
 */
static int settings_eeprom_recstep(struct settings_eeprom *cf, off_t *addr,
				   struct settings_eeprom_rec_info *info,
				   uint8_t *id)
{
	off_t dataend, datastart, dataidend;
	uint16_t reclen, crc16sto, crc16cal;
	uint8_t recidmaxidx, buf[8];
	int rc = 0;

	while (*addr < cf->end) {
		/* read the record length */
		rc = eeprom_read(cf->eeprom, *addr, &reclen, sizeof(reclen));
		if (rc) {
			break;
		}
		/* invalid record length: end of fs */
		if ((reclen <= MIN_REC_LEN) || (reclen > MAX_REC_LEN)) {
			break;
		}

		dataend = *addr + reclen - sizeof(crc16sto);
		*addr += sizeof(reclen);

		/* read the id length info */
		rc = eeprom_read(cf->eeprom, *addr, &recidmaxidx,
				 sizeof(recidmaxidx));
		if (rc) {
			break;
		}

		*addr += sizeof(recidmaxidx);

		datastart = *addr;
		dataidend = datastart + recidmaxidx + 1;

		/* calculate the crc16 over the record id and data, copy the
		 * id if needed
		 */
		crc16cal = 0xffff;
		while ((dataend - *addr) > 0) {
			size_t rdlen = MIN((dataend - *addr), sizeof(buf));

			rc = eeprom_read(cf->eeprom, *addr, buf, rdlen);
			if (rc) {
				break;
			}
			/* copy the id if required */
			if ((id) && ((dataidend - *addr) > 0)) {
				size_t cplen = MIN((dataidend - *addr), rdlen);

				memcpy(id + (*addr - datastart), buf, cplen);
			}
			/* calculate the crc16 */
			crc16cal = crc16_ccitt(crc16cal, buf, rdlen);
			*addr += rdlen;
		}

		/* read the stored crc16 */
		rc = eeprom_read(cf->eeprom, *addr, &crc16sto,
				 sizeof(crc16sto));
		if (rc) {
			break;
		}
		*addr += sizeof(crc16sto);

		if (crc16sto == crc16cal) {
			/* update the info if needed */
			if (info) {
				info->idlen = dataidend - datastart;
				info->dataoffset = dataidend;
				info->datalen = dataend - dataidend;
			}
			/* set the return value to the record length */
			rc = (int)reclen;
			break;
		}
	}
	return rc;
}

static int settings_eeprom_load(struct settings_store *cs,
			     const struct settings_load_arg *arg)
{
	struct settings_eeprom *cf = (struct settings_eeprom *)cs;
	struct settings_eeprom_read_fn_arg read_fn_arg;
	struct settings_eeprom_rec_info info;
	char id[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
	off_t addr = cf->start + sizeof(struct settings_eeprom_hdr);
	int rc = 0;

	if (!device_is_ready(cf->eeprom)) {
		return -EIO;
	}

	while (1) {

		rc = settings_eeprom_recstep(cf, &addr, &info, (uint8_t *)id);
		if (rc <= 0) {
			break;
		}

		id[info.idlen] = '\0';

		read_fn_arg.eeprom = cf->eeprom;
		read_fn_arg.address = info.dataoffset;
		read_fn_arg.len = info.datalen;

		rc = settings_call_set_handler(id, info.datalen,
					       settings_eeprom_read_fn,
					       &read_fn_arg, (void *)arg);
		if (rc) {
			break;
		}
	}

	return rc;
}

/* invalidate the record that ends at addr */
static int settings_eeprom_invalidate(struct settings_eeprom *cf, off_t addr)
{
	uint16_t crc16inv;
	int rc;

	addr -= sizeof(crc16inv);
	rc = eeprom_read(cf->eeprom, addr, &crc16inv, sizeof(crc16inv));
	if (rc) {
		return rc;
	}
	crc16inv = ~crc16inv;
	return eeprom_write(cf->eeprom, addr, &crc16inv, sizeof(crc16inv));
}

/* Compress data in eeprom (remove deleted items) */
static int settings_eeprom_compress(struct settings_eeprom *cf)
{
	off_t addr = cf->start + sizeof(struct settings_eeprom_hdr);
	off_t wraddr = cf->start + sizeof(struct settings_eeprom_hdr);
	uint16_t reclen;
	uint8_t buf[4];
	int rc = 0;

	if (!device_is_ready(cf->eeprom)) {
		return -EIO;
	}

	while (1) {
		rc = settings_eeprom_recstep(cf, &addr, NULL, NULL);
		if (rc <= 0) {
			break;
		}
		reclen = (uint16_t)rc;
		addr -= reclen;

		while (reclen) {
			size_t cplen = MIN(reclen, sizeof(buf));

			rc = eeprom_read(cf->eeprom, addr, buf, cplen);
			if (rc) {
				break;
			}
			addr += cplen;

			rc = eeprom_write(cf->eeprom, wraddr, buf, cplen);
			if (rc) {
				break;
			}
			wraddr += cplen;
			reclen -= cplen;
		}

		/* invalidate the record at addr unless it is at wraddr */
		if (addr > wraddr) {
			rc = settings_eeprom_invalidate(cf, addr);
			if (rc) {
				break;
			}
		}
	}

	cf->end = wraddr;

	return rc;
}
static int settings_eeprom_save(struct settings_store *cs, const char *name,
				const char *value, size_t val_len)
{
	struct settings_eeprom *cf = (struct settings_eeprom *)cs;
	struct settings_eeprom_rec_info info;
	char id[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
	off_t addr = cf->start + sizeof(struct settings_eeprom_hdr);
	off_t matchend = 0;
	size_t namelen, matchdatalen = 0U;
	uint16_t reclen, crc16;
	uint8_t namemaxidx;
	int rc;

	if (!name) {
		return -EINVAL;
	}

	if (!device_is_ready(cf->eeprom)) {
		return -EIO;
	}

	namelen = strlen(name);

	if (namelen > (SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN)) {
		return -EINVAL;
	}

	/* In case of value = NULL, adapt the length */
	if (value == NULL) {
		val_len = 0;
	}

	/* Find out if the name is already used and update wraddr to the
	 * next write address
	 */
	while (1) {
		rc = settings_eeprom_recstep(cf, &addr, &info, (uint8_t *)id);
		if (rc <= 0) {
			break;
		}
		id[info.idlen] = '\0';
		if (!strcmp(id, name)) {
			matchend = addr;
			matchdatalen = info.datalen;
		}
	}

	/* Invalidate the entry for a delete or length change */
	if ((matchdatalen) && (val_len != matchdatalen)) {
		rc = settings_eeprom_invalidate(cf, matchend);
		if (rc || (!val_len)) {
			return rc;
		}
	}

	/* Calculate the crc16 */
	crc16 = crc16_ccitt(0xffff, name, namelen);
	crc16 = crc16_ccitt(crc16, value, val_len);

	/* Overwrite the old record if the length does not change */
	if ((matchdatalen) && (val_len == matchdatalen)) {
		addr = matchend - (val_len + sizeof(crc16));
		rc = eeprom_write(cf->eeprom, addr, value, val_len);
		if (rc) {
			return rc;
		}
		addr += val_len;
		return eeprom_write(cf->eeprom, addr, &crc16, sizeof(crc16));
	}

	/* Something new to write */
	reclen = MIN_REC_LEN + namelen + val_len;

	/* Not enough space for record: compress */
	if ((cf->end + reclen) > (cf->start + cf->size)) {
		rc = settings_eeprom_compress(cf);
		if (rc) {
			return rc;
		}
		/* check again if there is sufficient space now */
		if ((cf->end + reclen) > (cf->start + cf->size)) {
			return -ENOSPC;
		}
	}

	/* Write the record */
	rc = eeprom_write(cf->eeprom, cf->end, &reclen, sizeof(reclen));
	if (rc) {
		return rc;
	}
	cf->end += sizeof(reclen);

	namemaxidx = (uint8_t)(namelen - 1);

	rc = eeprom_write(cf->eeprom, cf->end, &namemaxidx, sizeof(namemaxidx));
	if (rc) {
		return rc;
	}
	cf->end += sizeof(namemaxidx);

	rc = eeprom_write(cf->eeprom, cf->end, name, namelen);
	if (rc) {
		return rc;
	}
	cf->end += namelen;

	rc = eeprom_write(cf->eeprom, cf->end, value, val_len);
	if (rc) {
		return rc;
	}
	cf->end += val_len;

	rc = eeprom_write(cf->eeprom, cf->end, &crc16, sizeof(crc16));
	if (rc) {
		return rc;
	}
	cf->end += sizeof(crc16);

	return 0;
}

/* Initialize the eeprom backend. */
int settings_eeprom_backend_init(struct settings_eeprom *cf)
{
	struct settings_eeprom_hdr rdhdr, hdr, ehdr;
	const size_t hdrsize = sizeof(struct settings_eeprom_hdr);
	off_t addr = cf->start;
	off_t end = cf->start + hdrsize;
	int rc;

	if (!device_is_ready(cf->eeprom)) {
		return -EIO;
	}

	if ((addr + cf->size) > eeprom_get_size(cf->eeprom)) {
		LOG_DBG("Area to big to fit EEPROM");
		return -EINVAL;
	}
	if (cf->size <= hdrsize) {
		LOG_DBG("EEPROM to small for settings");
		return -EIO;
	}
	rc = eeprom_read(cf->eeprom, addr, &rdhdr, hdrsize);
	if (rc) {
		return rc;
	}

	memset(&ehdr, 0xff, sizeof(ehdr));
	hdr.ver = EEPROM_SETTINGS_VERSION;
	hdr.magic = EEPROM_SETTINGS_MAGIC;

	if (memcmp(&hdr, &rdhdr, hdrsize) && memcmp(&ehdr, &rdhdr, hdrsize)) {
		LOG_DBG("Not a settings EEPROM");
		return -EIO;
	}

	if (memcmp(&hdr, &rdhdr, hdrsize)) {
		/* Add the info to the start of the eeprom area */
		rc = eeprom_write(cf->eeprom, addr, &hdr, hdrsize);
		if (rc) {
			return rc;
		}
	}

	addr += hdrsize;
	/* Find the end of the in use area */
	cf->end = cf->start + cf->size;
	while (1) {
		rc = settings_eeprom_recstep(cf, &addr, NULL, NULL);
		if (rc <= 0) {
			break;
		}
		end = addr;
	}
	cf->end = end;

	LOG_DBG("Initialized");
	return rc;
}

int settings_backend_init(void)
{
	static struct settings_eeprom default_settings_eeprom = {
		.eeprom = DEVICE_DT_GET(DT_ALIAS(eeprom_0)),
		.start = 0,
	};
	int rc;

	default_settings_eeprom.size =
		eeprom_get_size(default_settings_eeprom.eeprom);

	rc = settings_eeprom_backend_init(&default_settings_eeprom);
	if (rc) {
		return rc;
	}

	rc = settings_eeprom_src(&default_settings_eeprom);

	if (rc) {
		return rc;
	}

	rc = settings_eeprom_dst(&default_settings_eeprom);

	return rc;
}
