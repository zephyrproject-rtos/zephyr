/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "page_map.h"

#define MAX_PAGE_TABLE_NUM 40

#define PAGE_SIZE 4096

#define NL0PG (PAGE_SIZE / sizeof(pd_entry_t))
#define NL1PG (PAGE_SIZE / sizeof(pd_entry_t))
#define NL2PG (PAGE_SIZE / sizeof(pd_entry_t))
#define NL3PG (PAGE_SIZE / sizeof(pt_entry_t))

#define NUL0E L0_ENTRIES
#define NUL1E (NUL0E * NL1PG)
#define NUL2E (NUL1E * NL2PG)

static uint64_t page_tables[MAX_PAGE_TABLE_NUM][Ln_ENTRIES] __aligned(Ln_ENTRIES *
								      sizeof(uint64_t));
static uint16_t page_tables_use_map[MAX_PAGE_TABLE_NUM];
static struct k_spinlock page_tables_lock;

#define PT_USE_MAP_ALLOC   BIT(15)
#define PT_USE_MAP_COUNT_M GENMASK(7, 0)

uint64_t* page_table_alloc_empty()
{
	int i;
	uint64_t *page_table = NULL;
	k_spinlock_key_t key;

	key = k_spin_lock(&page_tables_lock);
	for (i = 0; i < ARRAY_SIZE(page_tables_use_map); i++) {
		if ((page_tables_use_map[i] & PT_USE_MAP_ALLOC) == 0) {
			page_tables_use_map[i] |= PT_USE_MAP_ALLOC;
			page_table = page_tables[i];
			break;
		}
	}
	k_spin_unlock(&page_tables_lock, key);

	memset((void *)page_table, 0, sizeof(uint64_t) * Ln_ENTRIES);
	return page_table;
}
