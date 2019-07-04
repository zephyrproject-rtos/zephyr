/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sys/memobj.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(memobj);

struct memobj_hdr {
	u32_t chunk_size:MEMOBJ_MAX_CHUNK_SIZE_BITS;
	u32_t size	:MEMOBJ_MAX_SIZE_BITS;
};

union memobj_cont;

struct memobj_mid {
	union memobj_cont *cont;
	u8_t data[0];
};

struct memobj_last {
	struct k_mem_slab *slab;
	u8_t data[0];
};

union memobj_cont {
	struct memobj_mid mid;
	struct memobj_last last;
};

struct memobj {
	union memobj_cont *cont;
	struct memobj_hdr hdr;
	u8_t data[0];
};

BUILD_ASSERT_MSG(offsetof(struct memobj, data) == MEMOBJ1_OVERHEAD,
		  "Unexpected overhead size.");

int memobj_alloc(struct k_mem_slab *slab, memobj_t **memobj,
		u32_t size, s32_t timeout)
{
	int err;
	union memobj_cont *chunk;
	struct memobj *obj;
	u32_t chunk_size;
	s32_t remain = size;
	u32_t objsize;

	if (size > MEMOBJ_MAX_SIZE ||
		slab->block_size > MEMOBJ_MAX_CHUNK_SIZE) {
		return -EINVAL;
	}

	/* Allocate initial chunk which contains header */
	err = k_mem_slab_alloc(slab, memobj, timeout);
	if (err != 0) {
		return err;
	}

	obj = (struct memobj *)*memobj;
	chunk_size = slab->block_size - sizeof(void *);
	objsize = chunk_size - sizeof(struct memobj_hdr);
	/* gradually increase size to allow freeing memobj and any time if
	 * allocation fails.
	 */
	obj->hdr.size = objsize;
	obj->hdr.chunk_size = chunk_size;

	remain -= objsize;
	chunk = (union memobj_cont *)*memobj;
	chunk->last.slab = slab;

	while (remain > 0) {
		/* Ensure that last chunk header points to mem_slab to allow
		 * freeing.
		 */
		chunk->last.slab = slab;
		err = k_mem_slab_alloc(slab, (void **)&chunk->mid.cont,
				       timeout);
		if (err != 0) {
			memobj_free(*memobj);
			return err;
		}

		obj->hdr.size += chunk_size;
		remain -= chunk_size;
		chunk = chunk->mid.cont;
	}
	chunk->last.slab = slab;

	obj->hdr.size = size;

	return 0;
}

memobj_t *memobjectize(u8_t *data, u32_t len)
{
	struct memobj *memobj = (memobj_t *)data;

	memobj->hdr.chunk_size = len + sizeof(struct memobj_hdr);
	memobj->hdr.size = len + sizeof(struct memobj_hdr);
	memobj->cont = NULL;

	return memobj;
}

static struct k_mem_slab *get_mem_slab(memobj_t *memobj)
{
	struct memobj *obj = (struct memobj *)memobj;
	s32_t size = obj->hdr.size + sizeof(struct memobj_hdr);
	u32_t chunk_size = obj->hdr.chunk_size;
	union memobj_cont *cont = (union memobj_cont *)memobj;
	struct k_mem_slab *slab;

	do {
		size -= chunk_size;
		slab = cont->last.slab;
		cont = cont->mid.cont;
	} while (size > 0);

	return slab;
}

static void memobj_free_slab(memobj_t *memobj, struct k_mem_slab *slab)
{
	struct memobj *obj = (struct memobj *)memobj;
	s32_t size = obj->hdr.size + sizeof(struct memobj_hdr);
	u32_t chunk_size = obj->hdr.chunk_size;
	union memobj_cont *cont = (union memobj_cont *)memobj;
	union memobj_cont *next;

	do {
		size -= chunk_size;
		next = cont->mid.cont;
		k_mem_slab_free(slab, (void **)&cont);
		cont = next;
	} while (size > 0);
}

void memobj_free(memobj_t *memobj)
{
	struct k_mem_slab *slab = get_mem_slab(memobj);

	memobj_free_slab(memobj, slab);
}

static void cpy_op(u8_t *p1, u8_t *p2, u32_t len, bool to_p1)
{
	memcpy(to_p1 ? p1 : p2, to_p1 ? p2 : p1, len);
}

static u32_t memobj_op(memobj_t *memobj, u8_t *data, u32_t len,
		       u32_t offset, bool read)
{
	struct memobj *obj = (struct memobj *)memobj;
	u32_t size = obj->hdr.size;
	u32_t chunk_size = obj->hdr.chunk_size;
	u32_t rem_len;
	u32_t first_chunk_len;
	union memobj_cont *chunk = (union memobj_cont *)memobj;

	if (offset > size) {
		return 0;
	}

	len = (len + offset) > size ? (size - offset) : len;
	rem_len = len;

	offset += sizeof(struct memobj_hdr);

	/* Traverse to first chunk. */
	while (offset >= chunk_size) {
		chunk = chunk->mid.cont;
		offset -= chunk_size;
	}

	first_chunk_len = MIN(chunk_size - offset, rem_len);
	cpy_op(data, &chunk->mid.data[offset], first_chunk_len, read);
	rem_len -= first_chunk_len;
	data += first_chunk_len;
	chunk = chunk->mid.cont;
	while (rem_len >= chunk_size) {
		cpy_op(data, chunk->mid.data, chunk_size, read);
		chunk = chunk->mid.cont;
		data += chunk_size;
		rem_len -= chunk_size;
	}

	cpy_op(data, chunk->mid.data, rem_len, read);

	return len;
}

u32_t memobj_write(memobj_t *memobj, u8_t *data, u32_t len, u32_t offset)
{
	return memobj_op(memobj, data, len, offset, false);
}

u32_t memobj_read(memobj_t *memobj, u8_t *data, u32_t len, u32_t offset)
{
	return memobj_op(memobj, data, len, offset, true);
}
