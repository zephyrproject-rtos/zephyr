/*
 * Non volatile circular buffer
 *
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/storage/nvcb.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fs_nvcb, CONFIG_NVCB_LOG_LEVEL);

BUILD_ASSERT(NVCB_BHDRSIZE < NVCB_BUFSIZE, "NVCB_BUFSIZE to small");

#define ALIGNDOWN(num, align) ((num) & ~((align) - 1U))
#define ALIGNUP(num, align)   (ALIGNDOWN(num - 1U, align) + align)

static int init(const struct nvcb_store *store)
{
	const struct nvcb_store_cfg *cfg = store->cfg;
	int rc = 0;

	if (cfg->init == NULL) {
		goto end;
	}

	rc = cfg->init(cfg->ctx);
end:
	return rc;
}

static int release(const struct nvcb_store *store)
{
	const struct nvcb_store_cfg *cfg = store->cfg;
	int rc = 0;

	if (cfg->release == NULL) {
		goto end;
	}

	rc = cfg->release(cfg->ctx);
end:
	return rc;
}

static int mem_sync(const struct nvcb_store *store)
{
	const struct nvcb_store_cfg *cfg = store->cfg;
	int rc = 0;

	if (cfg->sync == NULL) {
		goto end;
	}

	rc = cfg->sync(cfg->ctx);
end:
	return rc;
}

static int mem_read(const struct nvcb_store *store, uint32_t off, void *data,
		    size_t len)
{
	const struct nvcb_store_cfg *cfg = store->cfg;

	return cfg->read(cfg->ctx, off, data, len);
}

static int mem_prog(const struct nvcb_store *store, uint32_t off,
		    const void *data, size_t len)
{
	const struct nvcb_store_cfg *cfg = store->cfg;
	int rc = 0;

	if (cfg->prog == NULL) {
		goto end;
	}

	rc = cfg->prog(cfg->ctx, off, data, len);
end:
	return rc;
}

static int mem_prep(const struct nvcb_store *store, uint32_t off, size_t len)
{
	const struct nvcb_store_cfg *cfg = store->cfg;
	int rc = 0;

	if (cfg->prep == NULL) {
		goto end;
	}

	rc = cfg->prep(cfg->ctx, off, len);
end:
	return rc;
}

static int lock(const struct nvcb_store *store)
{
	const struct nvcb_store_cfg *cfg = store->cfg;
	int rc = 0;

	if (cfg->lock == NULL) {
		goto end;
	}

	rc = cfg->lock(cfg->ctx);
end:
	return rc;
}

static int unlock(const struct nvcb_store *store)
{
	const struct nvcb_store_cfg *cfg = store->cfg;
	int rc = 0;

	if (cfg->unlock == NULL) {
		goto end;
	}

	rc = cfg->unlock(cfg->ctx);
end:
	return rc;
}

static int read(const struct nvcb_ent *ent, uint32_t off, void *data, size_t len)
{
	const struct nvcb_store *store = ent->store;
	const size_t bsz = store->cfg->bsz;
	int rc;

	rc = mem_read(store, bsz * ent->blck + ent->dpos + off, data, len);
	if (rc != 0) {
		goto end;
	}

end:
	return rc;
}

static void next_block(const struct nvcb_store *store, uint32_t *blck)
{
	(*blck)++;
	if ((*blck) == store->cfg->bcnt) {
		*blck = 0U;
	}
}

static void make_header(uint8_t *hdr, size_t dlen)
{
	hdr[0] = NVCB_ECBORHDR;
	hdr[1] = dlen >> 8;
	hdr[2] = dlen;
}

static bool header_match(const uint8_t *hdr)
{
	return hdr[0] == NVCB_ECBORHDR;
}

static int append(const struct nvcb_store *store,
		int (*read)(const void *rdctx, uint32_t off, void *data,
			    size_t len),
		const void *rdctx, size_t len)
{
	struct nvcb_store_data *data = store->data;
	const size_t bsz = store->cfg->bsz;
	const size_t psz = store->cfg->psz;
	const size_t ehsz = (data->cpos == 0U) ? NVCB_BHDRSIZE : 0U;
	const size_t hsz = ehsz + NVCB_EHDRSIZE;
	const size_t end = data->cpos + hsz + len;
	const size_t aend = ALIGNUP(end, psz);
	uint8_t rbuf[MAX(NVCB_BUFSIZE, psz)];
	uint8_t pbuf[MAX(NVCB_BUFSIZE, psz)];
	int rc;

	while (aend < bsz) { /* invalidate anything at next position */
		rc = mem_read(store, data->cblck * bsz + aend, pbuf, psz);
		if (rc != 0) {
			goto end;
		}

		if (!header_match(pbuf)) {
			break;
		}

		pbuf[0] = NVCB_BREAKCHAR;
		rc = mem_prog(store, data->cblck * bsz + aend, pbuf, psz);
		if (rc != 0) {
			goto end;
		}

		rc = mem_sync(store);
		if (rc != 0) {
			goto end;
		}
	}

	while (data->cpos < aend) {
		uint32_t bpos = 0U;
		size_t rdsize;

		/* add header */
		if (data->cpos < (end - len)) {
			if (ehsz != 0U) {
				memcpy(pbuf, NVCB_BCBORHDR, NVCB_BHDRSIZE);
				pbuf[NVCB_BHDRSIZE - 1] = data->pass;
				bpos += NVCB_BHDRSIZE;
			}

			make_header(&pbuf[bpos], len);
			bpos += NVCB_EHDRSIZE;
		}

		/* add data */
		rdsize = MIN(sizeof(pbuf) - bpos, len);
		rc = read(rdctx, end - len, &pbuf[bpos], rdsize);
		bpos += rdsize;
		len -= rdsize;

		/* add fill */
		if (bpos < sizeof(pbuf)) {
			size_t fsize = MIN((sizeof(pbuf) - bpos), (aend - end));

			memset(&pbuf[bpos], NVCB_FILLCHAR, fsize);
			bpos += fsize;
		}

		rc = mem_prog(store, data->cblck * bsz + data->cpos, pbuf, bpos);
		if (rc != 0) {
			break;
		}

		rc = mem_read(store, data->cblck * bsz + data->cpos, rbuf, bpos);
		if (rc != 0) {
			break;
		}

		data->cpos += bpos;
		if (memcmp(pbuf, rbuf, bpos) != 0) {
			rc = NVCB_ECORRUPT;
			break;
		}
	}

	rc = mem_sync(store);
end:
	if (rc != 0) {
		data->cpos = bsz;
	}

	return rc;
}

static int advance(const struct nvcb_store *store)
{
	const size_t bsz = store->cfg->bsz;
	struct nvcb_store_data *data = store->data;

	data->cpos = 0U;
	next_block(store, &data->cblck);
	if (data->cblck == 0U) {
		data->pass = 1 - data->pass;
	}

	return mem_prep(store, data->cblck * bsz, bsz);
}

static int read_properties(struct nvcb_ent *ent)
{
	const size_t bsz = ent->store->cfg->bsz;
	const size_t rdsz = bsz - ent->dpos;
	uint8_t hdr[NVCB_EHDRSIZE];
	const size_t hsz = MIN(sizeof(hdr), rdsz);
	int rc = NVCB_ENOENT;

	if (hsz < 3U) {
		goto end;
	}

	if (read(ent, 0U, hdr, hsz) != 0) {
		goto end;
	}

	if (hdr[0] == NVCB_ECBORHDR) {
		ent->dpos += 3U;
		ent->dsz = (hdr[1] << 8) + hdr[2];
		rc = 0;
	}

end:
	return rc;
}

static bool valid_block(struct nvcb_ent *ent, uint8_t *pass)
{
	uint8_t bhdr[NVCB_BHDRSIZE];
	bool rv = false;

	if (read(ent, 0U, bhdr, sizeof(bhdr)) != 0) {
		goto end;
	}

	if (memcmp(bhdr, NVCB_BCBORHDR, NVCB_BHDRSIZE - 1) != 0) {
		goto end;
	}

	*pass = bhdr[NVCB_BHDRSIZE - 1];
	rv = true;
end:
	return rv;
}

static int get_next_in_block(struct nvcb_ent *ent)
{
	const struct nvcb_store *store = ent->store;
	uint8_t pass;

	if (ent->dpos != 0U) {
		ent->dpos = ALIGNUP(ent->dpos + ent->dsz, store->cfg->psz);
		goto end;
	}

	if ((!valid_block(ent, &pass)) || (pass > 1U)) {
		ent->dpos = store->cfg->bsz;
		goto end;
	}

	ent->dpos = NVCB_BHDRSIZE;
end:
	return read_properties(ent);
}

static int get_next(struct nvcb_ent *ent, uint32_t *bcnt)
{
	const struct nvcb_store *store = ent->store;
	int rc = NVCB_ENOENT;

	while ((*bcnt) != 0U) {
		if ((ent->blck == store->data->cblck) &&
		    (ent->dpos == store->data->cpos)) {
			break;
		}

		rc = get_next_in_block(ent);
		if (rc == 0) {
			break;
		}

		(*bcnt) -= 1U;
		ent->dpos = 0U;
		next_block(store, &ent->blck);
	}

	return rc;
}

static void walk_forward(struct nvcb_ent *ent,
			 int (*cb)(const struct nvcb_ent *ent, void *cb_arg),
			 void *cb_arg)
{
	uint32_t bcnt = ent->store->cfg->bcnt;

	while (get_next(ent, &bcnt) != NVCB_ENOENT) {
		int rc = cb(ent, cb_arg);

		if (rc == NVCB_DONE) {
			break;
		}

		if (rc == NVCB_SKIP) {
			ent->dpos = 0U;
			next_block(ent->store, &ent->blck);
			bcnt--;
		}
	}
}

static int init_cpos_cb(const struct nvcb_ent *ent, void *cb_arg)
{
	uint32_t *cpos = (uint32_t *)cb_arg;
	uint32_t psz = ent->store->cfg->psz;

	if (ent->blck != ent->store->data->cblck) {
		return NVCB_DONE;
	}

	(*cpos) = ALIGNUP(ent->dpos + ent->dsz, psz);
	return 0;
}

static int init_nvcb_store_data(const struct nvcb_store *store)
{
	const uint32_t bcnt = store->cfg->bcnt;
	struct nvcb_store_data *data = store->data;
	struct nvcb_ent ent = {
		.store = store,
		.dpos = 0U,
	};
	bool found = false;
	uint8_t pass;
	uint32_t cpos;
	int rc;

	rc = lock(store);
	if (rc != 0) {
		goto end;
	}

	for (int i = 0; i < bcnt; i++) {
		ent.blck = i;
		if ((!valid_block(&ent, &pass)) || (pass > 1)) {
			continue;
		}

		if (!found) {
			data->pass = pass;
		}

		if (pass != data->pass) {
			break;
		}

		data->cblck = i;
		found = true;
	}

	if (!found) {
		data->cblck = 0U;
		data->cpos = 0U;
		goto end;
	}

	data->cpos = store->cfg->bsz;
	ent.dpos = 0U;
	ent.blck = data->cblck;

	walk_forward(&ent, init_cpos_cb, (void *)&cpos);
	data->cpos = cpos;
end:
	unlock(store);
	return rc;
}

static bool store_valid(const struct nvcb_store *store)
{
	bool rv = false;

	if (store == NULL) {
		goto end;
	}

	if ((store->cfg == NULL) || (store->data == NULL)) {
		goto end;
	}

	rv = true;
end:
	return rv;
}

static bool store_ready(const struct nvcb_store *store)
{
	bool rv = false;

	if (!store_valid(store)) {
		goto end;
	}

	if (!store->data->ready) {
		goto end;
	}

	rv = true;
end:
	return rv;
}

int nvcb_advance(const struct nvcb_store *store)
{
	int rc = NVCB_EINVAL;

	if (!store_ready(store)) {
		goto end;
	}

	rc = lock(store);
	if (rc != 0) {
		goto end;
	}

	advance(store);
	(void)unlock(store);
end:
	return rc;
}

int nvcb_append(const struct nvcb_store *store,
		int (*read)(const void *rdctx, uint32_t off, void *data,
			    size_t len),
		const void *rdctx, size_t len)
{
	int rc = NVCB_EINVAL;

	if (!store_ready(store)) {
		goto end;
	}

	if ((len > NVCB_EMAXLEN) ||
	    (store->cfg->bsz < (len + NVCB_BHDRSIZE + NVCB_EHDRSIZE))) {
		goto end;
	}

	rc = lock(store);
	if (rc != 0) {
		goto end;
	}

	if ((store->data->cpos + len + NVCB_EHDRSIZE) > store->cfg->bsz) {
		rc = NVCB_ENOSPC;
		goto end_unlock;
	}

	rc = append(store, read, rdctx, len);
end_unlock:
	(void)unlock(store);
end:
	return rc;
}

static int memread(const void *ctx, uint32_t off, void *data, size_t len)
{
	const uint8_t *memoff = (const uint8_t *)ctx;

	memcpy(data, memoff + off, len);
	return 0;
}

int nvcb_write(const struct nvcb_store *store, const void *data, size_t len)
{
	const uint8_t *memoff = (const uint8_t *)data;

	return nvcb_append(store, memread, (void *)memoff, len);
}

int nvcb_walk_forward(const struct nvcb_store *store,
		      int (*cb)(const struct nvcb_ent *ent, void *cb_arg),
		      void *cb_arg)
{
	int rc = NVCB_EINVAL;

	if ((!store_ready(store)) || (cb == NULL)) {
		goto end;
	}

	struct nvcb_ent ent = {
		.store = store,
		.dpos = 0U,
		.blck = store->data->cblck,
	};

	next_block(store, &ent.blck);
	walk_forward(&ent, cb, cb_arg);
	rc = 0;
end:
	return rc;
}

int nvcb_get_start(struct nvcb_ent *ent, const struct nvcb_store *store,
		   uint32_t skip)
{
	int rc = NVCB_EINVAL;

	if (!store_ready(store)) {
		goto end;
	}

	ent->store = store;
	ent->dpos = 0U;
	ent->blck = store->data->cblck;

	for (uint32_t i = 0U; i <= skip; i++) {
		next_block(store, &ent->blck);
	}

	rc = 0;
end:
	return rc;
}

int nvcb_get_next(struct nvcb_ent *ent, uint32_t *blimit)
{
	int rc = NVCB_EINVAL;

	if ((ent == NULL) || (!store_ready(ent->store))) {
		goto end;
	}

	if (blimit != NULL) {
		rc = get_next(ent, blimit);
		goto end;
	}

	uint32_t bcnt = ent->store->cfg->bcnt;

	rc = get_next(ent, &bcnt);
end:
	return rc;
}

int nvcb_entry_read(const struct nvcb_ent *ent, uint32_t off, void *data,
		    size_t len)
{
	int rc = NVCB_EINVAL;

	if ((ent == NULL) || (!store_ready(ent->store)) ||
	    ((off + len) > ent->dsz)) {
		goto end;
	}

	rc = read(ent, off, data, len);
end:
	return rc;
}

int nvcb_mount(const struct nvcb_store *store)
{
	int rc = NVCB_EINVAL;

	/* basic config checks */
	if (!store_valid(store)) {
		goto end;
	}

	const struct nvcb_store_cfg *cfg = store->cfg;
	struct nvcb_store_data *data = store->data;

	/* read routines check */
	if (cfg->read == NULL) {
		goto end;
	}

	if (data->ready) {
		rc = NVCB_EAGAIN;
		goto end;
	}

	rc = init(store);
	if (rc != 0) {
		goto end;
	}

	data->pass = 0U;
	rc = init_nvcb_store_data(store);
	if (rc != 0) {
		goto end;
	}

	data->ready = true;
end:
	return rc;
}

int nvcb_unmount(const struct nvcb_store *store)
{
	int rc = NVCB_EINVAL;

	if (!store_valid(store)) {
		goto end;
	}

	rc = lock(store);
	if (rc != 0) {
		goto end;
	}

	store->data->ready = false;
	rc = release(store);
	(void)unlock(store);
end:
	return rc;
}

int nvcb_secure_wipe(const struct nvcb_store *store)
{
	uint8_t buf[store->cfg->psz];
	uint32_t off = 0U;
	int rc = NVCB_EINVAL;

	if (!store_valid(store)) {
		goto end;
	}

	if (store->cfg->prog == NULL) {
		goto end;
	}

	if (store->data->ready) {
		goto end;
	}

	rc = init(store);
	if (rc != 0) {
		goto end;
	}

	rc = lock(store);
	if (rc != 0) {
		goto end;
	}

	for (int i = 0; i < sizeof(buf); i++) {
		buf[i] = NVCB_FILLCHAR;
	}

	while (off < (store->cfg->bsz * store->cfg->bcnt)) {
		if ((off % store->cfg->bsz) == 0U) {
			rc = mem_prep(store, off, store->cfg->bsz);
			if (rc != 0) {
				break;
			}
		}

		rc = mem_prog(store, off, buf, sizeof(buf));
		if (rc != 0) {
			break;
		}
		off += sizeof(buf);
	}

end:
	(void)unlock(store);
	(void)release(store);
	return rc;
}
