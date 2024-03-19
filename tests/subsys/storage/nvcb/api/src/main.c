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
#define ENTRYSIZE	24

uint8_t backend[BLOCKSIZE * BLOCKCOUNT];

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

DEFINE_NVCB(test, (void *)&backend[0], WRITEBLOCKSIZE, BLOCKSIZE, BLOCKCOUNT,
	    read, prog, NULL, sync, init, release, NULL, NULL);

static void *setup(void)
{
	memset(backend, NVCB_FILLCHAR, sizeof(backend));
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

ZTEST_USER(nvcb, test_nvcb_secure_wipe)
{
	struct nvcb_store *store = nvcb_store_get(test);
	int err;

	err = nvcb_secure_wipe(store);
	zassert_true(err == 0,  "nvcb_secure_wipe call failure: %d", err);

	for (size_t i = 0; i < (store->cfg->bcnt * store->cfg->bsz); i++) {
		uint8_t rd;

		err = store->cfg->read(store->cfg->ctx, i, &rd, sizeof(rd));
		zassert_true(err == 0, "cfg->read failure: d", err);
		zassert_true(rd == NVCB_FILLCHAR, "incorrect wipe");
	}
}

ZTEST_USER(nvcb, test_nvcb_write)
{
	struct nvcb_store *store = nvcb_store_get(test);
	uint8_t data[ENTRYSIZE];
	uint32_t pos;
	int err;

	memset(data, 0x00, ENTRYSIZE);
	err = nvcb_mount(store);
	zassert_true(err == 0,  "nvcb_mount call failure: %d", err);

	TC_PRINT("nvcb current block: %d, current position: %d\n",
		 store->data->cblck, store->data->cpos);

	/* Write entry (adds extra 3 byte) */
	pos = store->data->cpos;
	err = nvcb_write(store, data, sizeof(data));
	zassert_true(err == 0,  "nvcb_write call failure: %d", err);
	pos += ENTRYSIZE + NVCB_BHDRSIZE + NVCB_EHDRSIZE;
	err = store->data->cpos - pos;
	zassert_true(err <= WRITEBLOCKSIZE,  "position not in expected range");

	err = nvcb_unmount(store);
	zassert_true(err == 0,  "nvcb_unmount call failure: %d", err);

	err = nvcb_mount(store);
	zassert_true(err == 0,  "nvcb_mount call failure: %d", err);

	TC_PRINT("nvcb current block: %d, current position: %d\n",
		 store->data->cblck, store->data->cpos);

	err = nvcb_unmount(store);
	zassert_true(err == 0,  "nvcb_unmount call failure: %d", err);
}

int read_cb(const struct nvcb_ent *ent, void *cb_arg)
{
	uint32_t *cnt = (uint32_t *)cb_arg;
	char data[ent->dsz];
	int rc;

	rc = nvcb_entry_read(ent, 0U, data, sizeof(data));
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
	uint32_t write_cnt, read_cnt;
	uint8_t data[ENTRYSIZE];
	int err;

	err = nvcb_secure_wipe(store);
	zassert_true(err == 0,  "nvcb_secure_erase call failure: %d", err);

	err = nvcb_mount(store);
	zassert_true(err == 0,  "nvcb_mount call failure: %d", err);
	TC_PRINT("nvcb current block: %d, current position: %d\n",
		 store->data->cblck, store->data->cpos);

	memset(data, 0x00, ENTRYSIZE);
	write_cnt = 0U;
	for (size_t i = 0U; i < 2 * store->cfg->bcnt; i++) {
		while (true) {
			err = nvcb_write(store, data, sizeof(data));
			if (err == NVCB_ENOSPC) {
				break;
			}
			zassert_true(err == 0,  "nvcb_write failure: %d", err);
			write_cnt += ENTRYSIZE;
			if ((i == (2 * store->cfg->bcnt - 1)) &&
			    (store->data->cpos > 256)) {
				break;
			}
		}

		if (i != (2 * store->cfg->bcnt - 1)) {
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
	TC_PRINT("%d byte write %d byte read\n", write_cnt, read_cnt);

	err = nvcb_unmount(store);
	zassert_true(err == 0,  "nvcb_unmount call failure: %d", err);
}
