/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/storage/nvcb.h>

#define BLOCKSIZE 512
#define BLOCKCOUNT 4
#define WRITEBLOCKSIZE	4
#define SMALLENTRYSIZE	17
#define LARGEENTRYSIZE	256

uint8_t backend[BLOCKSIZE * BLOCKCOUNT];
uint8_t pbuf[WRITEBLOCKSIZE];

int read(const void *ctx, uint32_t off, void *data, size_t len)
{
	uint8_t *backend = (uint8_t *)ctx;

	memcpy(data, backend + off, len);
	return 0;
}

int prog(const void *ctx, uint32_t off, const void *data, size_t len)
{
	uint8_t *backend = (uint8_t *)ctx;

	memcpy(backend + off, data, len);
	return 0;
}

int prep(const void *ctx, uint32_t off, size_t len)
{
	uint8_t *backend = (uint8_t *)ctx;

	memset(backend + off, 0U, len);
	return 0;
}

int sync(const void *ctx)
{
	return 0;
}

int init(const void *ctx)
{
	return 0;
}

int release(const void *ctx)
{
	return 0;
}

DEFINE_NVCB(test, (void *)&backend[0], (void *)&pbuf[0], WRITEBLOCKSIZE,
	    BLOCKSIZE, BLOCKCOUNT, read, prog, prep, sync, init, release, NULL,
	    NULL);

static void *setup(void)
{
	memset(backend, 0, sizeof(backend));
	return NULL;
}

ZTEST_SUITE(nvcb, NULL, setup, NULL, NULL, NULL);

ZTEST_USER(nvcb, test_nvcb_mount_unmount)
{
	struct nvcb_store *store = nvcb_store_get(test);
	int err;

	err = nvcb_mount(store);
	zassert_true(err == 0,  "nvcb_mount call failure: %d", err);

	TC_PRINT("nvcb current block: %d, current position: %d\n",
		 store->data->cblck, store->data->cpos);

	err = nvcb_unmount(store);
	zassert_true(err == 0,  "nvcb_unmount call failure: %d", err);
}

ZTEST_USER(nvcb, test_nvcb_erase)
{
	struct nvcb_store *store = nvcb_store_get(test);
	int err;

	err = nvcb_erase(store);
	zassert_true(err == 0,  "nvcb_erase call failure: %d", err);

	for (size_t i = 0; i < (store->cfg->bcnt * store->cfg->bsz); i++) {
		uint8_t rd;

		err = store->cfg->read(store->cfg->ctx, i, &rd, sizeof(rd));
		zassert_true(err == 0, "cfg->read failure: d", err);
		zassert_true(rd == NVCB_FILLCHAR, "incorrect erase");
	}
}

ZTEST_USER(nvcb, test_nvcb_append)
{
	struct nvcb_store *store = nvcb_store_get(test);
	struct nvcb_ent ent;
	uint32_t pos;
	int err;

	err = nvcb_mount(store);
	zassert_true(err == 0,  "nvcb_mount call failure: %d", err);

	/* Add small entry (adds extra 3 byte) */
	pos = store->data->cpos;
	err = nvcb_append(store, SMALLENTRYSIZE, &ent);
	zassert_true(err == 0,  "nvcb_append call failure: %d", err);
	err = nvcb_append_done(&ent);
	zassert_true(err == 0,  "nvcb_append call failure: %d", err);
	err = store->data->cpos - SMALLENTRYSIZE - 3U - pos;
	zassert_true(err == 0,  "wrong position after append");

	/* Add large entry (adds extra 4 byte) */
	pos = store->data->cpos;
	err = nvcb_append(store, LARGEENTRYSIZE, &ent);
	zassert_true(err == 0,  "nvcb_append call failure: %d", err);
	err = nvcb_append_done(&ent);
	zassert_true(err == 0,  "nvcb_append call failure: %d", err);
	err = store->data->cpos - LARGEENTRYSIZE - 4U - pos;
	zassert_true(err == 0,  "wrong position after append");
	err = nvcb_append(store, LARGEENTRYSIZE, &ent);

	err = nvcb_unmount(store);
	zassert_true(err == 0,  "nvcb_unmount call failure: %d", err);
}

int read_cb(const struct nvcb_ent *ent, void *cb_arg)
{
	uint32_t *cnt = (uint32_t *)cb_arg;
	char data[ent->dsz];
	int rc;

	rc = nvcb_read(ent, 0U, data, sizeof(data));
	if (rc != 0) {
		goto end;
	}

	(*cnt) += sizeof(data);
end:
	return rc;
}

ZTEST_USER(nvcb, test_nvcb_walk)
{
	struct nvcb_store *store = nvcb_store_get(test);
	struct nvcb_ent ent;
	uint32_t write_cnt, read_cnt;
	int err;

	err = nvcb_erase(store);
	zassert_true(err == 0,  "nvcb_erase call failure: %d", err);

	err = nvcb_mount(store);
	zassert_true(err == 0,  "nvcb_mount call failure: %d", err);

	write_cnt = 0U;
	for (size_t i = 0U; i < store->cfg->bcnt; i++) {
		while (true) {
			err = nvcb_append(store, SMALLENTRYSIZE, &ent);
			if (err == NVCB_ENOSPC) {
				break;
			}
			zassert_true(err == 0,  "nvcb_append failure: %d", err);
			err = nvcb_append_done(&ent);
			zassert_true(err == 0,  "nvcb_append_done failure: %d",
				     err);
			write_cnt += SMALLENTRYSIZE;
			if ((i == (store->cfg->bcnt - 1)) &&
			    ((ent.dpos + ent.dsz) > 256)) {
				break;
			}
		}

		if (i != (store->cfg->bcnt - 1)) {
			err = nvcb_advance(store);
			zassert_true(err == 0, "nvcb_advance failure: %d", err);
		}
	}

	err = nvcb_unmount(store);
	zassert_true(err == 0,  "nvcb_unmount call failure: %d", err);

	err = nvcb_mount(store);
	zassert_true(err == 0,  "nvcb_mount call failure: %d", err);

	read_cnt = 0U;
	err = nvcb_walk_forward(store, read_cb, (void *)&read_cnt);
	zassert_true(err == 0,  "nvcb_walk_forward call failure: %d", err);
	zassert_true(read_cnt == write_cnt, "nvcb_walk_forward wrong result");

	read_cnt = 0U;
	err = nvcb_walk_reverse(store, read_cb, (void *)&read_cnt);
	zassert_true(err == 0,  "nvcb_walk_forward call failure: %d", err);
	zassert_true(read_cnt == write_cnt, "nvcb_walk_forward wrong result");

	err = nvcb_unmount(store);
	zassert_true(err == 0,  "nvcb_unmount call failure: %d", err);
}
