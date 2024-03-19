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

#define NVCB_ENTRYHDRS        0b01011000 /* cbor byte string, 1 byte length */
#define NVCB_ENTRYHDRL        0b01011001 /* cbor byte string, 2 byte length */
#define NVCB_BREAKCHAR        0b11111111 /* cbor atom break */
#define NVCB_EEHDRSIZE        4          /* extended entry header size */
#define NVCB_BUFSIZE          64
#define NVCB_INITPASS         0b11110000 /* initial pass value */
#define NVCB_MAXLENSE         0xFF       /* maximum length small entry */
#define NVCB_MAXLENLE         0xFFFF     /* maximum length large entry */
#define ALIGNDOWN(num, align) ((num) & ~((align)-1U))
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
	uint8_t *data8 = (uint8_t *)data;
	int rc = 0;

	if (cfg->prog == NULL) {
		goto end;
	}

	rc = cfg->prog(cfg->ctx, off, data, len);
	if (rc != 0) {
		goto end;
	}

	while (len != 0U) {
		uint8_t buf[MIN(len, NVCB_BUFSIZE)];

		rc = cfg->read(cfg->ctx, off, buf, sizeof(buf));
		if (rc != 0) {
			goto end;
		}

		if (memcmp(data8, buf, sizeof(buf)) != 0) {
			rc = NVCB_ECORRUPT;
			goto end;
		}

		len -= sizeof(buf);
		off += sizeof(buf);
		data8 += sizeof(buf);
	}

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

static int write(const struct nvcb_ent *ent, uint32_t off, const void *data,
		 size_t len)
{
	const struct nvcb_store *store = ent->store;
	const struct nvcb_store_cfg *cfg = store->cfg;
	const size_t psz = cfg->psz;
	const size_t bsz = cfg->bsz;
	uint8_t *pbuf8 = (uint8_t *)cfg->pbuf;
	uint32_t apos = ALIGNDOWN(store->data->cpos, psz);
	uint8_t *data8 = (uint8_t *)data;
	int rc = 0;

	if ((ent->dpos + off) != store->data->cpos) {
		rc = NVCB_EINVAL;
		goto end;
	}

	if (apos != store->data->cpos) { /* add to pbuf, write if needed */
		const uint32_t bpos = store->data->cpos - apos;
		const uint32_t wrlen = MIN(len, psz - bpos);

		memcpy(pbuf8 + bpos, data8, wrlen);
		if ((bpos + wrlen) == psz) {
			const uint32_t moff = ent->blck * bsz + apos;

			rc = mem_prog(store, moff, pbuf8, psz);
			if (rc != 0) {
				goto end;
			}
		}

		store->data->cpos += wrlen;
		data8 += wrlen;
		len -= wrlen;
	}

	if (len >= psz) { /* perform direct write */
		const size_t wrlen = ALIGNDOWN(len, psz);
		const uint32_t moff = ent->blck * bsz + store->data->cpos;

		rc = mem_prog(store, moff, data8, wrlen);
		if (rc != 0) {
			goto end;
		}

		store->data->cpos += wrlen;
		data8 += wrlen;
		len -= wrlen;
	}

	if (len != 0U) { /* add remaining part to pbuf */
		memcpy(pbuf8, data8, len);
		store->data->cpos += len;
	}
end:
	return rc;
}

static int sync(const struct nvcb_ent *ent)
{
	const uint32_t end = ent->dpos + ent->dsz;
	const uint32_t cpos = ent->store->data->cpos;
	const size_t wsz = ALIGNUP(end, ent->store->cfg->psz) - cpos;
	uint8_t buf[wsz];
	int rc;

	for (size_t i = 0; i < wsz; i++) {
		buf[i] = NVCB_FILLCHAR;
	}

	rc = write(ent, cpos - ent->dpos, buf, wsz);
	if (rc != 0) {
		goto end;
	}

	rc = mem_sync(ent->store);
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

static void previous_block(const struct nvcb_store *store, uint32_t *blck)
{
	if ((*blck) == 0) {
		(*blck) = store->cfg->bcnt;
	}

	(*blck)--;
}

static size_t make_header(uint8_t *hdr, size_t dlen, uint8_t pass)
{
	size_t rv;

	if (dlen > NVCB_MAXLENSE) {
		hdr[0] = NVCB_ENTRYHDRL;
		hdr[1] = dlen >> 8;
		hdr[2] = dlen;
		hdr[3] = pass;
		rv = 4U;
		goto end;
	}

	hdr[0] = NVCB_ENTRYHDRS;
	hdr[1] = dlen;
	hdr[2] = pass;
	rv = 3U;
end:
	return rv;
}

static int header_get_prop(const uint8_t *hdr, size_t *hsz, size_t *dsz)
{
	int rc = 0;

	if (hdr[0] == NVCB_ENTRYHDRS) {
		(*hsz) = 3U;
		(*dsz) = hdr[1];
		goto end;
	}

	if (hdr[0] == NVCB_ENTRYHDRL) {
		(*hsz) = 4U;
		(*dsz) = (hdr[1] << 8) + hdr[2];
		goto end;
	}

	rc = NVCB_ENOENT;

end:
	return rc;
}

static bool header_pass_match(const uint8_t *hdr, uint8_t pass)
{
	bool rv;

	if ((hdr[0] == NVCB_ENTRYHDRS) && (hdr[2] == pass)) {
		rv = true;
		goto end;
	}

	if ((hdr[0] == NVCB_ENTRYHDRL) && (hdr[3] == pass)) {
		rv = true;
		goto end;
	}

	rv = false;
end:
	return rv;
}

static int invalidate(const struct nvcb_ent *ent)
{
	const size_t psz = ent->store->cfg->psz;
	const size_t bsz = ent->store->cfg->bsz;
	const uint8_t pass = ent->store->data->pass;
	uint32_t next = ALIGNUP(ent->dpos + ent->dsz, psz);
	uint8_t buf[MAX(psz, NVCB_EEHDRSIZE)];
	int rc;

	if ((next + sizeof(buf)) > bsz) {
		rc = 0;
		goto end;
	}

	rc = mem_read(ent->store, ent->blck * bsz + next, buf, sizeof(buf));
	if (rc != 0) {
		goto end;
	}

	if (!header_pass_match(buf, pass)) {
		goto end;
	}

	for (int i = 0; i < psz; i++) {
		buf[i] = NVCB_BREAKCHAR;
	}

	rc = mem_prog(ent->store, ent->blck * bsz + next, buf, sizeof(buf));
	if (rc != 0) {
		goto end;
	}

	rc = mem_sync(ent->store);
end:
	return rc;
}

static int append(struct nvcb_ent *ent, size_t dlen)
{
	const struct nvcb_store *store = ent->store;
	const struct nvcb_store_data *data = store->data;
	const size_t psz = store->cfg->psz;
	const size_t bsz = store->cfg->bsz;
	const uint8_t pass = data->pass;
	uint8_t hdr[NVCB_EEHDRSIZE];
	size_t hsz;
	int rc;

	hsz = make_header(hdr, dlen, pass);
	ent->blck = data->cblck;
	ent->dpos = data->cpos + hsz;
	ent->dsz = dlen;
	if (ALIGNUP(ent->dpos + ent->dsz, psz) > bsz) {
		rc = NVCB_ENOSPC;
		goto end;
	}

	rc = invalidate(ent);
	if (rc != 0) {
		goto end;
	}

	rc = lock(store);
	if (rc != 0) {
		goto end;
	}

	ent->dpos -= hsz;
	rc = write(ent, 0U, hdr, hsz);
	if (rc != 0) {
		(void)unlock(store);
		goto end;
	}

	ent->dpos += hsz;
end:
	return rc;
}

static int append_done(const struct nvcb_ent *ent)
{
	const struct nvcb_store *store = ent->store;
	int rc;

	rc = sync(ent);
	unlock(store);
	return rc;
}

static int advance(const struct nvcb_store *store)
{
	const size_t bsz = store->cfg->bsz;
	struct nvcb_store_data *data = store->data;
	int rc = 0;

	rc = lock(store);
	if (rc != 0) {
		goto end;
	}

	data->cpos = 0U;
	next_block(store, &data->cblck);
	if (data->cblck == 0U) {
		data->pass = ~data->pass;
	}

	rc = mem_prep(store, data->cblck * bsz, bsz);
end:
	unlock(store);
	return rc;
}

static int read_properties(struct nvcb_ent *ent)
{
	const size_t bsz = ent->store->cfg->bsz;
	uint8_t hdr[NVCB_EEHDRSIZE];
	size_t hsz, dsz;
	int rc;

	if ((bsz - ent->dpos) < sizeof(hdr)) {
		rc = NVCB_ENOENT;
		goto end;
	}

	rc = read(ent, 0U, hdr, sizeof(hdr));
	if (rc != 0) {
		goto end;
	}

	rc = header_get_prop(hdr, &hsz, &dsz);
	if (rc != 0) {
		goto end;
	}

	ent->dpos += hsz;
	ent->dsz = dsz;
end:
	return rc;
}

static int walk_in_block(struct nvcb_ent *ent,
			 int (*cb)(const struct nvcb_ent *ent, void *cb_arg),
			 void *cb_arg)
{
	const struct nvcb_store *store = ent->store;
	const size_t bsz = store->cfg->bsz;
	const size_t psz = store->cfg->psz;
	int rc = 0;

	while (ent->dpos < bsz) {
		if ((ent->blck == store->data->cblck) &&
		    (ent->dpos == store->data->cpos)) {
			break;
		}

		rc = read_properties(ent);
		if (rc != 0) {
			break;
		}

		rc = cb(ent, cb_arg);
		if (rc != 0) {
			break;
		}

		ent->dpos = ALIGNUP(ent->dpos + ent->dsz, psz);
	}

	if (rc < 0) {
		rc = NVCB_SKIP;
	}

	return rc;
}

static void walk_forward(const struct nvcb_store *store, uint32_t bcnt,
			 int (*cb)(const struct nvcb_ent *ent, void *cb_arg),
			 void *cb_arg)
{
	struct nvcb_ent ent;

	ent.store = store;
	ent.blck = store->data->cblck;
	for (int i = 0; i < bcnt; i++) {
		ent.dpos = 0U;
		next_block(store, &ent.blck);
		if (walk_in_block(&ent, cb, cb_arg) == NVCB_DONE) {
			break;
		}
	}
}

static void walk_reverse(const struct nvcb_store *store, uint32_t bcnt,
			int (*cb)(const struct nvcb_ent *ent, void *cb_arg),
			void *cb_arg)
{
	struct nvcb_ent ent;

	ent.store = store;
	ent.blck = store->data->cblck;
	for (int i = 0; i < bcnt; i++) {
		ent.dpos = 0U;
		if (walk_in_block(&ent, cb, cb_arg) == NVCB_DONE) {
			break;
		}
		previous_block(store, &ent.blck);
	}
}

static int init_cblck_cb(const struct nvcb_ent *ent, void *cb_arg)
{
	const uint32_t boff = ent->store->cfg->bsz * ent->blck;
	struct nvcb_store_data *data = ent->store->data;
	bool *found = (bool *)cb_arg;
	uint8_t rdpass;
	int rc;

	rc = mem_read(ent->store, boff + ent->dpos - 1U, &rdpass, 1);
	if (rc != 0) {
		goto end;
	}

	if (!(*found)) {
		(*found) = true;
		if (rdpass != data->pass) {
			data->pass = rdpass;
		}
	}

	if (rdpass != data->pass) {
		rc = NVCB_DONE;
		goto end;
	}

	data->cblck = ent->blck;
	rc = NVCB_SKIP;
end:
	return rc;
}

static int init_cpos_cb(const struct nvcb_ent *ent, void *cb_arg)
{
	const uint32_t boff = ent->store->cfg->bsz * ent->blck;
	const size_t psz = ent->store->cfg->psz;
	struct nvcb_store_data *data = ent->store->data;
	uint32_t *cpos = (uint32_t *)cb_arg;
	uint8_t rdpass;
	int rc;

	rc = mem_read(ent->store, boff + ent->dpos - 1U, &rdpass, 1);
	if (rc != 0) {
		goto end;
	}

	if (rdpass != data->pass) {
		rc = NVCB_DONE;
		goto end;
	}

	(*cpos) = ALIGNUP(ent->dpos + ent->dsz, psz);
end:
	return rc;
}

static int init_nvcb_store_data(const struct nvcb_store *store)
{
	const uint32_t bcnt = store->cfg->bcnt;
	struct nvcb_store_data *data = store->data;
	bool found;
	uint32_t cpos = 0U;
	int rc;

	rc = lock(store);
	if (rc != 0) {
		goto end;
	}
	data->cblck = bcnt - 1;
	data->cpos = store->cfg->bsz;
	walk_forward(store, bcnt, init_cblck_cb, (void *)&found);
	if (!found) {
		data->cblck = 0U;
		data->cpos = 0U;
		goto end;
	}

	walk_reverse(store, 1, init_cpos_cb, (void *)&cpos);
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

int nvcb_append(const struct nvcb_store *store, size_t len, struct nvcb_ent *ent)
{
	int rc = NVCB_EINVAL;

	if ((!store_ready(store)) || (ent == NULL)) {
		goto end;
	}

	if (len > NVCB_MAXLENLE) {
		goto end;
	}

	if ((len > NVCB_MAXLENSE) && (store->cfg->bsz < (len + 4U))) {
		goto end;
	}

	if (store->cfg->bsz < (len + 3U)) {
		goto end;
	}

	ent->store = store;
	rc = append(ent, len);
end:
	return rc;
}

int nvcb_append_done(const struct nvcb_ent *ent)
{
	int rc = NVCB_EINVAL;

	if ((ent == NULL) || (!store_ready(ent->store))) {
		goto end;
	}

	rc = append_done(ent);
end:
	return rc;
}

int nvcb_advance(const struct nvcb_store *store)
{
	int rc = NVCB_EINVAL;

	if (!store_ready(store)) {
		goto end;
	}

	advance(store);
	rc = 0;
end:
	return rc;
}

int nvcb_read(const struct nvcb_ent *ent, uint32_t off, void *data, size_t len)
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

int nvcb_write(const struct nvcb_ent *ent, uint32_t off, const void *data,
	       size_t len)
{
	int rc = NVCB_EINVAL;

	if ((ent == NULL) || (!store_ready(ent->store)) ||
	    ((off + len) > ent->dsz)) {
		goto end;
	}

	rc = write(ent, off, data, len);
end:
	return rc;
}

int nvcb_walk_forward(const struct nvcb_store *store,
		      int (*cb)(const struct nvcb_ent *ent, void *cb_arg),
		      void *cb_arg)
{
	int rc = NVCB_EINVAL;

	if ((!store_ready(store)) || (cb == NULL)) {
		goto end;
	}

	const uint32_t bcnt = store->cfg->bcnt;

	walk_forward(store, bcnt, cb, cb_arg);
	rc = 0;
end:
	return rc;
}

int nvcb_walk_reverse(const struct nvcb_store *store,
		      int (*cb)(const struct nvcb_ent *ent, void *cb_arg),
		      void *cb_arg)
{
	int rc = NVCB_EINVAL;

	if ((!store_ready(store)) || (cb == NULL)) {
		goto end;
	}

	const uint32_t bcnt = store->cfg->bcnt;

	walk_reverse(store, bcnt, cb, cb_arg);
	rc = 0;
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

	/* program size power of 2 (zero is allowed) */
	if ((cfg->psz & (cfg->psz - 1)) != 0U) {
		goto end;
	}

	/* block size larger than program size */
	if (cfg->bsz < cfg->psz) {
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

	data->pass = NVCB_INITPASS;
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

int nvcb_erase(const struct nvcb_store *store)
{
	uint8_t buf[store->cfg->psz];
	uint32_t off = 0;
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
