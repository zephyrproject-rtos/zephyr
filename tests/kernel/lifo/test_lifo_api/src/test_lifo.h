#ifndef __TEST_LIFO_H__
#define __TEST_LIFO_H__

#include <ztest.h>
#include <irq_offload.h>

typedef struct ldata {
	sys_snode_t snode;
	uint32_t data;
} ldata_t;
#endif
