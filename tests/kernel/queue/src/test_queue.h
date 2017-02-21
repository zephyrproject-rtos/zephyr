#ifndef __TEST_FIFO_H__
#define __TEST_FIFO_H__

#include <ztest.h>
#include <irq_offload.h>

typedef struct qdata {
	sys_snode_t snode;
	uint32_t data;
} qdata_t;
#endif
