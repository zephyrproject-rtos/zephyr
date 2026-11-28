/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/ztest.h>

#define N  4
#define SZ 32U
#define CH 0U

#define CH1                    1U
#define EMUL_THREAD_STACK_SIZE 512

#if DT_NODE_EXISTS(DT_NODELABEL(tst_dma0))
static const struct device *const dma = DEVICE_DT_GET(DT_NODELABEL(tst_dma0));
static K_SEM_DEFINE(done, 0, 1);
static K_SEM_DEFINE(done1, 0, 1);
static K_SEM_DEFINE(root_done, 0, 2);
static K_SEM_DEFINE(emul_op_go, 0, 2);
static K_SEM_DEFINE(emul_op_done, 0, 2);
static K_SEM_DEFINE(emul_op_release, 0, 2);
static K_THREAD_STACK_DEFINE(emul_op_stack0, EMUL_THREAD_STACK_SIZE);
static K_THREAD_STACK_DEFINE(emul_op_stack1, EMUL_THREAD_STACK_SIZE);
static struct k_thread emul_op_thread0;
static struct k_thread emul_op_thread1;
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(tst_dma0))
static int cbs;
static int last_status;
static bool root_resubmitted;
static int root_reload_ret;
static int linked_reload_ret;
static __aligned(32) char src[N][SZ];
static __aligned(32) char dst[N][SZ];

struct emul_op_context {
	uint32_t channel;
	uintptr_t source;
	uintptr_t dest;
	bool reload;
	int ret;
};

static struct dma_config base_cfg(struct dma_block_config *blk);

static void on_done(const struct device *d, void *a, uint32_t id, int status)
{
	cbs++;
	last_status = status;
	k_sem_give(a);
}

static void on_root_resubmit(const struct device *d, void *a, uint32_t id, int status)
{
	if (!root_resubmitted) {
		struct dma_block_config block = {
			.source_address = (uintptr_t)src[2],
			.dest_address = (uintptr_t)dst[2],
			.block_size = SZ,
		};
		struct dma_config config = base_cfg(&block);

		root_resubmitted = true;
		config.dma_callback = on_root_resubmit;
		config.user_data = a;
		root_reload_ret = dma_config(d, id, &config);
		if (root_reload_ret == 0) {
			root_reload_ret = dma_start(d, id);
		}
	}
	k_sem_give(a);
}

static void on_link_stop_reload_root(const struct device *d, void *a, uint32_t id, int status)
{
	if (status == DMA_STATUS_COMPLETE) {
		zassert_ok(dma_stop(d, CH));
		linked_reload_ret = dma_reload(d, CH, (uintptr_t)src[3], (uintptr_t)dst[3], SZ);
	}
	k_sem_give(a);
}

static void emul_op_thread(void *p1, void *p2, void *p3)
{
	struct emul_op_context *context = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	k_sem_take(&emul_op_go, K_FOREVER);
	if (context->reload) {
		context->ret =
			dma_reload(dma, context->channel, context->source, context->dest, SZ);
	} else {
		context->ret = dma_start(dma, context->channel);
	}
	k_sem_give(&emul_op_done);
	k_sem_take(&emul_op_release, K_FOREVER);
}

static struct dma_config base_cfg(struct dma_block_config *blk)
{
	struct dma_config c = {
		.channel_direction = MEMORY_TO_MEMORY,
		.source_data_size = 1U,
		.dest_data_size = 1U,
		.source_burst_length = 1U,
		.dest_burst_length = 1U,
		.block_count = 1U,
		.head_block = blk,
		.dma_callback = on_done,
		.user_data = &done,
	};
	return c;
}

/* A channel must deliver a completion for every reload, moving each new block. */
ZTEST(dma_reload, test_reload_completes_each_time)
{
	struct dma_block_config blk = {
		.source_address = (uintptr_t)src[0],
		.dest_address = (uintptr_t)dst[0],
		.block_size = SZ,
	};
	struct dma_config c = base_cfg(&blk);

	cbs = 0;
	k_sem_reset(&done);
	memset(src[0], 'A', SZ);
	zassert_ok(dma_config(dma, CH, &c));
	zassert_ok(dma_start(dma, CH));
	zassert_ok(k_sem_take(&done, K_MSEC(1000)));

	for (int i = 1; i < N; i++) {
		memset(src[i], 'A' + i, SZ);
		k_sem_reset(&done);
		zassert_ok(dma_reload(dma, CH, (uintptr_t)src[i], (uintptr_t)dst[i], SZ),
			   "reload %d failed", i);
		zassert_ok(k_sem_take(&done, K_MSEC(1000)), "no completion for reload %d", i);
		zassert_mem_equal(dst[i], src[i], SZ, "reload %d moved wrong data", i);
	}
	zassert_equal(cbs, N, "expected %d completions, got %d", N, cbs);
}

/* dma_get_status() must report a defined struct, not leave the caller's stack. */
ZTEST(dma_reload, test_get_status_conformance)
{
	struct dma_block_config blk = {
		.source_address = (uintptr_t)src[0],
		.dest_address = (uintptr_t)dst[0],
		.block_size = SZ,
	};
	struct dma_config c = base_cfg(&blk);
	struct dma_status st;

	zassert_ok(dma_config(dma, CH, &c));
	zassert_ok(dma_get_status(dma, CH, &st), "get_status must be implemented");
	zassert_equal(st.dir, MEMORY_TO_MEMORY, "dir not reported");
}

ZTEST(dma_reload, test_get_status_after_stop)
{
	struct dma_block_config blk = {
		.source_address = (uintptr_t)src[0],
		.dest_address = (uintptr_t)dst[0],
		.block_size = SZ,
	};
	struct dma_config c = base_cfg(&blk);
	struct dma_status st = {
		.busy = true,
		.dir = PERIPHERAL_TO_MEMORY,
		.pending_length = UINT32_MAX,
		.free = UINT32_MAX,
		.write_position = UINT32_MAX,
		.read_position = UINT32_MAX,
		.total_copied = UINT64_MAX,
	};

	zassert_ok(dma_config(dma, CH, &c));
	zassert_ok(dma_stop(dma, CH));
	zassert_ok(dma_get_status(dma, CH, &st));
	zassert_false(st.busy, "channel is still busy after stop");
	zassert_equal(st.pending_length, 0U, "pending length was not initialized");
	zassert_equal(st.free, 0U, "free space was not initialized");
	zassert_equal(st.dir, MEMORY_TO_MEMORY, "direction was not initialized");
	zassert_equal(st.read_position, 0U, "read position was not initialized");
	zassert_equal(st.write_position, 0U, "write position was not initialized");
	zassert_equal(st.total_copied, 0U, "total copied was not initialized");
}

/* Reloading a queued or active transfer must not replace its work. */
ZTEST(dma_reload, test_emul_rejects_active_reload_without_status_change)
{
	struct dma_block_config blk = {
		.source_address = (uintptr_t)src[0],
		.dest_address = (uintptr_t)dst[0],
		.block_size = SZ,
	};
	struct dma_config c = base_cfg(&blk);
	struct dma_status before;
	struct dma_status after;
	char rejected[SZ];

	cbs = 0;
	zassert_ok(dma_config(dma, CH, &c));
	memset(src[0], 'A', SZ);
	memset(dst[0], 0, SZ);
	memset(dst[1], 'R', SZ);
	memset(rejected, 'R', SZ);
	k_sem_reset(&done);
	zassert_ok(dma_start(dma, CH));
	zassert_ok(dma_get_status(dma, CH, &before));
	zassert_true(before.busy, "transfer did not enter the active state");
	zassert_equal(dma_reload(dma, CH, (uintptr_t)src[1], (uintptr_t)dst[1], SZ), -EBUSY,
		      "active reload was accepted");
	zassert_ok(dma_get_status(dma, CH, &after));
	zassert_equal(after.busy, before.busy, "reload changed busy state");
	zassert_equal(after.dir, before.dir, "reload changed direction");
	zassert_equal(after.pending_length, before.pending_length, "reload changed pending length");
	zassert_equal(after.free, before.free, "reload changed free space");
	zassert_equal(after.read_position, before.read_position, "reload changed read position");
	zassert_equal(after.write_position, before.write_position, "reload changed write position");
	zassert_equal(after.total_copied, before.total_copied, "reload changed total copied");
	zassert_ok(k_sem_take(&done, K_MSEC(1000)), "original transfer did not complete");
	zassert_mem_equal(dst[0], src[0], SZ, "original transfer moved wrong data");
	zassert_mem_equal(dst[1], rejected, SZ, "rejected reload changed its destination");
}

ZTEST(dma_reload, test_emul_starts_two_channels_without_work_coalescing)
{
	struct dma_block_config blk0 = {
		.source_address = (uintptr_t)src[0],
		.dest_address = (uintptr_t)dst[0],
		.block_size = SZ,
	};
	struct dma_block_config blk1 = {
		.source_address = (uintptr_t)src[1],
		.dest_address = (uintptr_t)dst[1],
		.block_size = SZ,
	};
	struct dma_config c0 = base_cfg(&blk0);
	struct dma_config c1 = base_cfg(&blk1);
	struct emul_op_context op0 = {.channel = CH};
	struct emul_op_context op1 = {.channel = CH1};
	struct dma_status status;

	c1.user_data = &done1;
	cbs = 0;
	k_sem_reset(&done);
	k_sem_reset(&done1);
	memset(src[0], '0', SZ);
	memset(src[1], '1', SZ);
	memset(dst[0], 0, SZ);
	memset(dst[1], 0, SZ);
	zassert_ok(dma_config(dma, CH, &c0));
	zassert_ok(dma_config(dma, CH1, &c1));
	k_sem_reset(&emul_op_go);
	k_sem_reset(&emul_op_done);
	k_sem_reset(&emul_op_release);
	k_thread_create(&emul_op_thread0, emul_op_stack0, K_THREAD_STACK_SIZEOF(emul_op_stack0),
			emul_op_thread, &op0, NULL, NULL, K_PRIO_COOP(0), 0, K_FOREVER);
	k_thread_create(&emul_op_thread1, emul_op_stack1, K_THREAD_STACK_SIZEOF(emul_op_stack1),
			emul_op_thread, &op1, NULL, NULL, K_PRIO_COOP(0), 0, K_FOREVER);
	k_thread_start(&emul_op_thread0);
	k_thread_start(&emul_op_thread1);
	k_sem_give(&emul_op_go);
	k_sem_give(&emul_op_go);
	zassert_ok(k_sem_take(&emul_op_done, K_MSEC(1000)), "channel 0 did not start");
	zassert_ok(k_sem_take(&emul_op_done, K_MSEC(1000)), "channel 1 did not start");
	zassert_ok(op0.ret, "channel 0 start failed");
	zassert_ok(op1.ret, "channel 1 start failed");
	k_sem_give(&emul_op_release);
	k_sem_give(&emul_op_release);
	zassert_ok(k_sem_take(&done, K_MSEC(1000)), "channel 0 did not complete");
	zassert_ok(k_sem_take(&done1, K_MSEC(1000)), "channel 1 did not complete");
	zassert_mem_equal(dst[0], src[0], SZ, "channel 0 moved wrong data");
	zassert_mem_equal(dst[1], src[1], SZ, "channel 1 moved wrong data");
	zassert_ok(dma_get_status(dma, CH, &status));
	zassert_false(status.busy, "channel 0 remained active");
	zassert_ok(dma_get_status(dma, CH1, &status));
	zassert_false(status.busy, "channel 1 remained active");
}

ZTEST(dma_reload, test_emul_serializes_simultaneous_reloads)
{
	struct dma_block_config blk = {
		.source_address = (uintptr_t)src[0],
		.dest_address = (uintptr_t)dst[0],
		.block_size = SZ,
	};
	struct dma_config c = base_cfg(&blk);
	struct emul_op_context op0 = {
		.channel = CH,
		.source = (uintptr_t)src[1],
		.dest = (uintptr_t)dst[1],
		.reload = true,
	};
	struct emul_op_context op1 = {
		.channel = CH,
		.source = (uintptr_t)src[2],
		.dest = (uintptr_t)dst[2],
		.reload = true,
	};

	cbs = 0;
	k_sem_reset(&done);
	k_sem_reset(&emul_op_go);
	k_sem_reset(&emul_op_done);
	k_sem_reset(&emul_op_release);
	memset(src[1], '1', SZ);
	memset(src[2], '2', SZ);
	memset(dst[1], 0, SZ);
	memset(dst[2], 0, SZ);
	zassert_ok(dma_config(dma, CH, &c));
	k_thread_create(&emul_op_thread0, emul_op_stack0, K_THREAD_STACK_SIZEOF(emul_op_stack0),
			emul_op_thread, &op0, NULL, NULL, K_PRIO_COOP(0), 0, K_FOREVER);
	k_thread_create(&emul_op_thread1, emul_op_stack1, K_THREAD_STACK_SIZEOF(emul_op_stack1),
			emul_op_thread, &op1, NULL, NULL, K_PRIO_COOP(0), 0, K_FOREVER);
	k_thread_start(&emul_op_thread0);
	k_thread_start(&emul_op_thread1);
	k_sem_give(&emul_op_go);
	k_sem_give(&emul_op_go);
	zassert_ok(k_sem_take(&emul_op_done, K_MSEC(1000)), "first reload did not return");
	zassert_ok(k_sem_take(&emul_op_done, K_MSEC(1000)), "second reload did not return");
	zassert_true((op0.ret == 0 && op1.ret == -EBUSY) || (op0.ret == -EBUSY && op1.ret == 0),
		     "reloads were not serialized");
	k_sem_give(&emul_op_release);
	k_sem_give(&emul_op_release);
	zassert_ok(k_sem_take(&done, K_MSEC(1000)), "accepted reload did not complete");
	if (op0.ret == 0) {
		zassert_mem_equal(dst[1], src[1], SZ, "first reload moved wrong data");
	} else {
		zassert_mem_equal(dst[2], src[2], SZ, "second reload moved wrong data");
	}
}

ZTEST(dma_reload, test_emul_rejects_reload_until_stopped_work_quiesces)
{
	struct dma_block_config blk = {
		.source_address = (uintptr_t)src[0],
		.dest_address = (uintptr_t)dst[0],
		.block_size = SZ,
	};
	struct dma_config c = base_cfg(&blk);

	cbs = 0;
	k_sem_reset(&done);
	memset(src[0], 'A', SZ);
	zassert_ok(dma_config(dma, CH, &c));
	zassert_ok(dma_start(dma, CH));
	zassert_ok(dma_stop(dma, CH));
	zassert_equal(dma_reload(dma, CH, (uintptr_t)src[1], (uintptr_t)dst[1], SZ), -EBUSY,
		      "reload replaced stopped work before it quiesced");
	zassert_ok(k_sem_take(&done, K_MSEC(1000)), "stopped work did not finish");
	zassert_equal(last_status, -ECANCELED, "stopped work completed instead of canceling");
	k_sem_reset(&done);
	memset(src[1], 'B', SZ);
	zassert_ok(dma_reload(dma, CH, (uintptr_t)src[1], (uintptr_t)dst[1], SZ));
	zassert_ok(k_sem_take(&done, K_MSEC(1000)), "reload after quiesce did not complete");
	zassert_mem_equal(dst[1], src[1], SZ, "reload after quiesce moved wrong data");
}

ZTEST(dma_reload, test_emul_rejects_linked_reload_until_root_work_quiesces)
{
	struct dma_block_config root_block = {
		.source_address = (uintptr_t)src[0],
		.dest_address = (uintptr_t)dst[0],
		.block_size = SZ,
	};
	struct dma_block_config linked_block = {
		.source_address = (uintptr_t)src[1],
		.dest_address = (uintptr_t)dst[1],
		.block_size = SZ,
	};
	struct dma_config root_config = base_cfg(&root_block);
	struct dma_config linked_config = base_cfg(&linked_block);

	root_config.source_chaining_en = true;
	root_config.linked_channel = CH1;
	linked_config.user_data = &done1;
	k_sem_reset(&done);
	k_sem_reset(&done1);
	memset(src[0], '0', SZ);
	memset(src[1], '1', SZ);
	zassert_ok(dma_config(dma, CH1, &linked_config));
	zassert_ok(dma_config(dma, CH, &root_config));
	zassert_ok(dma_start(dma, CH));
	zassert_ok(dma_stop(dma, CH1));
	zassert_equal(dma_reload(dma, CH1, (uintptr_t)src[2], (uintptr_t)dst[2], SZ), -EBUSY,
		      "reload replaced linked work before root work quiesced");
	zassert_ok(k_sem_take(&done, K_MSEC(1000)), "root transfer did not complete");
	zassert_ok(k_sem_take(&done1, K_MSEC(1000)), "linked cancellation did not complete");
	k_sem_reset(&done1);
	zassert_ok(dma_reload(dma, CH1, (uintptr_t)src[2], (uintptr_t)dst[2], SZ));
	zassert_ok(k_sem_take(&done1, K_MSEC(1000)), "linked reload did not complete");
}

ZTEST(dma_reload, test_emul_preserves_callback_root_resubmission_reservation)
{
	struct dma_block_config root_block = {
		.source_address = (uintptr_t)src[0],
		.dest_address = (uintptr_t)dst[0],
		.block_size = SZ,
	};
	struct dma_block_config linked_block = {
		.source_address = (uintptr_t)src[1],
		.dest_address = (uintptr_t)dst[1],
		.block_size = SZ,
	};
	struct dma_config root_config = base_cfg(&root_block);
	struct dma_config linked_config = base_cfg(&linked_block);

	root_config.source_chaining_en = true;
	root_config.linked_channel = CH1;
	root_config.dma_callback = on_root_resubmit;
	root_config.user_data = &root_done;
	linked_config.dma_callback = on_link_stop_reload_root;
	linked_config.user_data = &done1;
	root_resubmitted = false;
	root_reload_ret = -EIO;
	linked_reload_ret = -EIO;
	k_sem_reset(&root_done);
	k_sem_reset(&done1);
	memset(src[0], '0', SZ);
	memset(src[1], '1', SZ);
	memset(src[2], '2', SZ);
	zassert_ok(dma_config(dma, CH1, &linked_config));
	zassert_ok(dma_config(dma, CH, &root_config));
	zassert_ok(dma_start(dma, CH));
	zassert_ok(k_sem_take(&root_done, K_MSEC(1000)), "root completion did not resubmit");
	zassert_ok(k_sem_take(&done1, K_MSEC(1000)), "linked completion did not run");
	zassert_true(root_resubmitted, "root callback did not resubmit");
	zassert_ok(root_reload_ret, "root callback reload failed");
	zassert_equal(linked_reload_ret, -EBUSY, "old root work lost the new reservation");
	zassert_ok(k_sem_take(&root_done, K_MSEC(1000)), "resubmitted root did not finish");
}

ZTEST(dma_reload, test_emul_rejects_null_config_and_invalid_status_arguments)
{
	struct dma_block_config blk = {
		.source_address = (uintptr_t)src[0],
		.dest_address = (uintptr_t)dst[0],
		.block_size = SZ,
	};
	struct dma_config c = base_cfg(&blk);
	struct dma_status before;
	struct dma_status after;

	zassert_ok(dma_config(dma, CH, &c));
	zassert_ok(dma_get_status(dma, CH, &before));
	zassert_equal(dma_config(dma, CH, NULL), -EINVAL, "null config was accepted");
	zassert_equal(dma_config(dma, UINT32_MAX, &c), -EINVAL,
		      "invalid config channel was accepted");
	zassert_equal(dma_start(dma, UINT32_MAX), -EINVAL, "invalid start channel was accepted");
	zassert_equal(dma_stop(dma, UINT32_MAX), -EINVAL, "invalid stop channel was accepted");
	zassert_equal(dma_reload(dma, UINT32_MAX, 0U, 0U, SZ), -EINVAL,
		      "invalid reload channel was accepted");
	zassert_equal(dma_get_status(dma, UINT32_MAX, &after), -EINVAL,
		      "invalid status channel was accepted");
	zassert_equal(dma_get_status(dma, CH, NULL), -EINVAL, "null status was accepted");
	zassert_ok(dma_get_status(dma, CH, &after));
	zassert_equal(after.busy, before.busy, "rejection changed busy state");
	zassert_equal(after.dir, before.dir, "rejection changed direction");
	zassert_equal(after.pending_length, before.pending_length,
		      "rejection changed pending length");
	zassert_equal(after.free, before.free, "rejection changed free space");
	zassert_equal(after.read_position, before.read_position, "rejection changed read position");
	zassert_equal(after.write_position, before.write_position,
		      "rejection changed write position");
	zassert_equal(after.total_copied, before.total_copied, "rejection changed total copied");
}
#endif

static void *setup(void)
{
#if DT_NODE_EXISTS(DT_NODELABEL(tst_dma0))
	zassert_true(device_is_ready(dma), "DMA controller not ready");
#endif
	return NULL;
}

/* Stop after each test so the next one can (re)configure a fresh channel.
 * This runs only once a channel has been configured, so it never stops an
 * unconfigured channel.
 */
static void after(void *fixture)
{
	ARG_UNUSED(fixture);
#if DT_NODE_EXISTS(DT_NODELABEL(tst_dma0))
	(void)dma_stop(dma, CH);
#endif
}

ZTEST_SUITE(dma_reload, NULL, setup, NULL, after, NULL);
