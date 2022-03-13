/*
 * Copyright (c) 2021 Carlo Caione, <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sys/sys_heap.h>
#include <sys/multi_heap.h>
#include <linker/linker-defs.h>

#include <multi_heap/shared_multi_heap.h>

#define DT_DRV_COMPAT shared_multi_heap

#define NUM_REGIONS DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)

static struct sys_multi_heap shared_multi_heap;
static struct sys_heap heap_pool[SMH_REG_ATTR_NUM][NUM_REGIONS];

static smh_init_reg_fn_t smh_init_reg;

#define FOREACH_REG(n)								\
	{ .addr = (uintptr_t) LINKER_DT_RESERVED_MEM_GET_PTR(DT_DRV_INST(n)),	\
	  .size = LINKER_DT_RESERVED_MEM_GET_SIZE(DT_DRV_INST(n)),		\
	  .attr = DT_ENUM_IDX(DT_DRV_INST(n), capability),			\
	},

static struct shared_multi_heap_region dt_region[NUM_REGIONS] = {
	DT_INST_FOREACH_STATUS_OKAY(FOREACH_REG)
};

static void *smh_choice(struct sys_multi_heap *mheap, void *cfg, size_t align, size_t size)
{
	enum smh_reg_attr attr;
	struct sys_heap *h;
	void *block;

	attr = (enum smh_reg_attr) cfg;

	if (attr >= SMH_REG_ATTR_NUM || size == 0) {
		return NULL;
	}

	for (size_t reg = 0; reg < NUM_REGIONS; reg++) {
		h = &heap_pool[attr][reg];

		if (h->heap == NULL) {
			return NULL;
		}

		block = sys_heap_aligned_alloc(h, align, size);
		if (block != NULL) {
			break;
		}
	}

	return block;
}

static void smh_init_with_attr(enum smh_reg_attr attr)
{
	unsigned int slot = 0;
	uint8_t *mapped;
	size_t size;

	for (size_t reg = 0; reg < NUM_REGIONS; reg++) {
		if (dt_region[reg].attr == attr) {

			if (smh_init_reg != NULL) {
				smh_init_reg(&dt_region[reg], &mapped, &size);
			} else {
				mapped = (uint8_t *) dt_region[reg].addr;
				size = dt_region[reg].size;
			}

			sys_heap_init(&heap_pool[attr][slot], mapped, size);
			sys_multi_heap_add_heap(&shared_multi_heap,
				&heap_pool[attr][slot], &dt_region[reg]);

			slot++;
		}
	}
}

void shared_multi_heap_free(void *block)
{
	sys_multi_heap_free(&shared_multi_heap, block);
}

void *shared_multi_heap_alloc(enum smh_reg_attr attr, size_t bytes)
{
	return sys_multi_heap_alloc(&shared_multi_heap, (void *) attr, bytes);
}

int shared_multi_heap_pool_init(smh_init_reg_fn_t smh_init_reg_fn)
{
	smh_init_reg = smh_init_reg_fn;

	sys_multi_heap_init(&shared_multi_heap, smh_choice);

	for (size_t attr = 0; attr < SMH_REG_ATTR_NUM; attr++) {
		smh_init_with_attr(attr);
	}

	return 0;
}

static int shared_multi_heap_init(const struct device *dev)
{
	__ASSERT_NO_MSG(NUM_REGIONS <= MAX_MULTI_HEAPS);

	/* Nothing to do here. */

	return 0;
}
SYS_INIT(shared_multi_heap_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
