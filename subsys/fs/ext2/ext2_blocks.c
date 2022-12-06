/*
 * Copyright (c) 2022 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "ext2_struct.h"
#include "ext2_blocks.h"

LOG_MODULE_DECLARE(ext2);

/* Overview of the page cache:
 *   Pages are used to cache memory to make less writes to the flash. Number of
 *   available pages is determined according to block size and minimal write size.
 *   Maximum number of pages can is specified with CONFIG_EXT2_MAX_PAGES and size
 *   of buffer for pages memory is defined with CONFIG_EXT2_PAGES_BUF_SIZE. Pages
 *   are written to the storage device only when sync operation is performed or
 *   some page must be freed to hold a new region of memory.
 *
 *   USED pages -- the pages that hold the fetched memory. Each page is
 *                 described by a page number, flags and used field.
 *   page FLAGS -- DIRTY flag indicates if something was written to that page
 *                 and it must be written to the storage device
 *   USED field -- Indicates which blocks of page memory are used by some block
 *                 structures. Page can't be reused if some of its blocks are
 *                 used.
 *
 * Allocating new pages:
 *   Number of currently used pages is stored in `num_pages` variable. When page
 *   is allocated it is stored as last entry in the `pages` array. If there is
 *   no free entry in that array, then new page is fetched in place of some
 *   unused page (if such page doesn't exist then error is returned).
 */

#define PAGE_FLAGS_DIRTY BIT(0)
#define PAGE_FLAGS_FREE BIT(1)
#define PAGE_FLAGS_MASK 0x3

#define PAGE_USED_BLOCK(i) BIT(i)
#define PAGE_USED_BLOCK_MASK 0xf

struct page {
	void *memory;   /* memory of page */
	uint32_t num;   /* number of fetched page */
	uint8_t  used;  /* bitmap of used blocks */
	uint8_t  flags; /* flags */
};

static int32_t num_pages;
static bool initialized;
static uint32_t glob_block_size, glob_page_size, glob_max_pages;
static struct page pages[CONFIG_EXT2_MAX_PAGES];

struct k_mem_slab page_memory;
char __aligned(8) __page_memory_buffer[CONFIG_EXT2_PAGES_BUF_SIZE];

K_MEM_SLAB_DEFINE_STATIC(block_struct, sizeof(struct block), CONFIG_EXT2_MAX_BLOCKS, 4);

static void *slab_alloc(struct k_mem_slab *slab);
static void slab_free(struct k_mem_slab *slab, void *blk);

/* Find page of given block. */
static struct page *find_block_page(uint32_t blk_num);

/* Find unused page that may be used to store new page */
static struct page *find_unused_page(void);

/* Allocate new page if it is possible. */
static struct page *get_new_page(struct ext2_data *fs);

/* Find existing page or get new from backing store */
static struct page *get_page(struct ext2_data *fs, uint32_t blk_num);

/* Free page */
static void free_page(struct page *pg);

/* Get memory of block within given page */
static void *page_block_memory(struct page *pg, uint32_t blk_num);

/* Set/unset used flag on page */
static void page_set_used(struct page *pg, uint32_t blk_num);
static void page_unset_used(struct page *pg, uint32_t blk_num);

struct block *get_block(struct ext2_data *fs, uint32_t num)
{
	struct page *pg;
	struct block *blk = slab_alloc(&block_struct);

	if (blk == NULL) {
		return NULL;
	}

	pg = get_page(fs, num);
	if (pg == NULL) {
		slab_free(&block_struct, blk);
		return NULL;
	}

	LOG_DBG("block: %d", num);

	page_set_used(pg, num);

	blk->memory = page_block_memory(pg, num);
	blk->num = num;
	blk->backend = BLOCK_BACKEND_FLASH;
	return blk;
}

void drop_block(struct block *blk)
{
	if (blk == NULL) {
		return;
	}
	if (blk->backend & BLOCK_BACKEND_FLASH) {
		struct page *pg = find_block_page(blk->num);

		__ASSERT(pg != NULL, "Block corrupted. Cannot find block page.");
		page_unset_used(pg, blk->num);
	}
	slab_free(&block_struct, blk);
}

void block_set_dirty(struct block *blk)
{
	struct page *pg = find_block_page(blk->num);

	__ASSERT(pg != NULL, "Block corrupted. Cannot find block page.");
	pg->flags |= PAGE_FLAGS_DIRTY;
}

int sync_blocks(struct ext2_data *fs)
{
	int ret, synced = 0;
	struct page *pg;

	for (int i = 0; i < num_pages; ++i) {
		pg = &pages[i];

		if (pg->flags & PAGE_FLAGS_DIRTY) {
			ret = fs->write_page(fs, pg->memory, pg->num);
			if (ret < 0) {
				LOG_ERR("Page %d write error (%d)", pg->num, ret);
				return ret;
			}
			synced++;
			pg->flags &= ~PAGE_FLAGS_DIRTY;
		}
	}
	return synced;
}

int init_blocks_cache(uint32_t block_size, uint32_t write_size)
{
	int ret = 0;

	if (block_size > write_size) {
		return -ENOTSUP;
	}

	glob_max_pages = MIN(CONFIG_EXT2_PAGES_BUF_SIZE / write_size, CONFIG_EXT2_MAX_PAGES);
	ret = k_mem_slab_init(&page_memory, __page_memory_buffer, write_size, glob_max_pages);

	if (ret < 0) {
		return ret;
	}

	glob_block_size = block_size;
	glob_page_size = write_size;
	initialized = true;
	LOG_INF("Initialized blocks cache with %dB blocks and %dB pages.", block_size, write_size);
	return 0;
}

void close_blocks_cache(void)
{
	if (!initialized) {
		return;
	}

	struct page *pg;

	for (int i = 0; i < num_pages; ++i) {
		pg = &pages[i];
		__ASSERT((pg->used & PAGE_USED_BLOCK_MASK) == 0, "Page is still in use!");
		slab_free(&page_memory, pg->memory);
	}
	memset(pages, 0, sizeof(struct page) * CONFIG_EXT2_MAX_PAGES);
	num_pages = 0;
	initialized = false;
}

static void *slab_alloc(struct k_mem_slab *slab)
{
	int ret = 0;
	void *mem;

	ret = k_mem_slab_alloc(slab, &mem, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("slab alloc error: %d", ret);
		return NULL;
	}
	return mem;
}

static void slab_free(struct k_mem_slab *slab, void *mem)
{
	if (mem != NULL) {
		k_mem_slab_free(slab, &mem);
	}
}

static struct page *find_block_page(uint32_t blk_num)
{
	uint32_t page_num = blk_num / (glob_page_size / glob_block_size);

	for (int i = 0; i < num_pages; ++i) {
		if (pages[i].num == page_num) {
			return &pages[i];
		}
	}
	return NULL;
}

static struct page *find_unused_page(void)
{
	/* Look for page that is free */
	for (int i = 0; i < num_pages; ++i) {
		if (pages[i].flags & PAGE_FLAGS_FREE) {
			return &pages[i];
		}
	}

	/* If free page not found look for unused page */
	for (int i = 0; i < num_pages; ++i) {
		if ((pages[i].used & PAGE_USED_BLOCK_MASK) == 0) {
			return &pages[i];
		}
	}
	return NULL;
}

static struct page *get_new_page(struct ext2_data *fs)
{
	struct page *pg;

	if (num_pages >= glob_max_pages) {
		pg = find_unused_page();
		if (pg == NULL) {
			return NULL;
		}
		/* The found page will be overwritten.
		 * If it is dirty we have to sync all changes before returning it.
		 */
		if (pg->flags & PAGE_FLAGS_DIRTY) {
			sync_blocks(fs);
		}
	} else {
		/* Allocate new page structure */
		pg = &pages[num_pages];
		pg->memory = slab_alloc(&page_memory);
		if (pg->memory == NULL) {
			return NULL;
		}
		num_pages++;
	}
	return pg;
}

static struct page *get_page(struct ext2_data *fs, uint32_t blk_num)
{
	int ret;
	struct page *pg = find_block_page(blk_num);
	uint32_t page_num = blk_num / (glob_page_size / glob_block_size);

	if (pg != NULL) {
		return pg;
	}

	pg = get_new_page(fs);
	if (pg == NULL) {
		return NULL;
	}

	ret = fs->read_page(fs, pg->memory, page_num);
	if (ret < 0) {
		LOG_ERR("Page %d read error (%d)", page_num, ret);
		/* Free the page because it may have invalid contents */
		free_page(pg);
		return NULL;
	}

	pg->num = page_num;
	pg->used = 0;
	pg->flags = 0;
	return pg;
}

static void free_page(struct page *pg)
{
	__ASSERT((pg->used & PAGE_USED_BLOCK_MASK) == 0, "Page is in use");
	pg->flags |= PAGE_FLAGS_FREE;
	pg->num = 0;
}

static void *page_block_memory(struct page *pg, uint32_t blk_num)
{
	uint32_t blk_in_page = blk_num % (glob_page_size / glob_block_size);

	return &((uint8_t *)pg->memory)[blk_in_page * glob_block_size];
}

static void page_set_used(struct page *pg, uint32_t blk_num)
{
	uint32_t blk_in_page = blk_num % (glob_page_size / glob_block_size);

	__ASSERT((pg->used & PAGE_USED_BLOCK(blk_in_page)) == 0, "Block is already used");
	pg->used |= PAGE_USED_BLOCK(blk_in_page);
}

static void page_unset_used(struct page *pg, uint32_t blk_num)
{
	uint32_t blk_in_page = blk_num % (glob_page_size / glob_block_size);

	__ASSERT((pg->used & PAGE_USED_BLOCK(blk_in_page)) != 0, "Block is already unused");
	pg->used &= ~PAGE_USED_BLOCK(blk_in_page);
}
