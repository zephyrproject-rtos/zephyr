/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2023 Huawei France Technologies SASU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* we piggyback the log level using ivshmem knob */
#define LOG_MODULE_NAME	ivshmem_doorbell_test
#define LOG_LEVEL CONFIG_IVSHMEM_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#ifdef CONFIG_USERSPACE
#include <zephyr/sys/libc-hooks.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/drivers/virtualization/ivshmem.h>

/*
 * result of an interrupt
 *
 * Using the ivshmem API, we will receive a signal event + two integers (the
 * interrupt itself is dealt in the internal ivshmem driver api)
 */
struct ivshmem_irq {
	/* k_poll was signaled or not */
	unsigned int signaled;
	/* vector received */
	int result;
};

struct ivshmem_ctx {
	/* dev, received via DTS */
	const struct device *dev;
	/* virtual address to access shared memory of ivshmem */
	void		*mem;
	/* size of shared memory */
	size_t		 size;
	/* my id for ivshmem protocol */
	uint32_t	 id;
	/* number of MSI vectors allocated */
	uint16_t	 vectors;
};

#ifdef CONFIG_USERSPACE
K_APPMEM_PARTITION_DEFINE(app_a_partition);
struct k_mem_domain app_a_domain;

#define APP_A_DATA	K_APP_DMEM(app_a_partition)
#define APP_A_BSS	K_APP_BMEM(app_a_partition)
#else
#define APP_A_DATA
#define APP_A_BSS
#endif

/* used for interrupt events, see wait_for_int() */
APP_A_BSS static struct ivshmem_irq irq;
APP_A_BSS static struct ivshmem_ctx ctx;

/* signal structure necessary for ivshmem API */
APP_A_BSS static struct k_poll_signal *sig;

/*
 * wait for an interrupt event.
 */
static int wait_for_int(const struct ivshmem_ctx *ctx)
{
	int ret;
	struct k_poll_event events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 sig),
	};

	LOG_DBG("%s: waiting interrupt from client...", __func__);
	ret = k_poll(events, ARRAY_SIZE(events), K_FOREVER);
	if (ret < 0) {
		printf("poll failed\n");
		return ret;
	}

	k_poll_signal_check(sig, &irq.signaled, &irq.result);
	LOG_DBG("%s: sig %p signaled %u result %d", __func__,
		sig, irq.signaled, irq.result);
	/* get ready for next signal */
	k_poll_signal_reset(sig);
	return 0;
}

/*
 * host <-> guest communication loop
 */
static void ivshmem_event_loop(struct ivshmem_ctx *ctx)
{
	int ret;
	char *buf;
	bool truncated;

	buf = malloc(ctx->size);
	if (buf == NULL) {
		printf("Could not allocate message buffer\n");
		return;
	}

	printf("Use write_shared_memory.sh and ivshmem-client to send a message\n");
	while (1) {
		/* host --> guest */
		ret = wait_for_int(ctx);
		if (ret < 0) {
			break;
		}

		/*
		 * QEMU may fail here if the shared memory has been downsized;
		 * the shared memory object must be protected against rogue
		 * users (check README for details)
		 */
		memcpy(buf, ctx->mem, ctx->size);
		truncated = (buf[ctx->size - 1] != '\0');
		buf[ctx->size - 1] = '\0';

		printf("received IRQ and %s message: %s\n",
		       truncated ? "truncated" : "full", buf);
	}
	free(buf);
}

/*
 * setup ivshmem parameters
 *
 * allocates a k_object in sig if running on kernelspace
 */
static int setup_ivshmem(bool in_kernel)
{
	int ret;
	size_t i;

	ctx.dev = DEVICE_DT_GET(DT_NODELABEL(ivshmem0));
	if (ctx.dev == NULL) {
		printf("Could not get ivshmem device\n");
		return -1;
	}

	ctx.size = ivshmem_get_mem(ctx.dev, (uintptr_t *)&ctx.mem);
	if (ctx.size == 0) {
		printf("Size cannot not be 0");
		return -1;
	}
	if (ctx.mem == NULL) {
		printf("Shared memory cannot be null\n");
		return -1;
	}

	ctx.id = ivshmem_get_id(ctx.dev);
	LOG_DBG("id for doorbell: %u", ctx.id);

	ctx.vectors = ivshmem_get_vectors(ctx.dev);
	if (ctx.vectors == 0) {
		printf("ivshmem-doorbell must have vectors\n");
		return -1;
	}

	if (in_kernel) {
		sig = k_malloc(sizeof(*sig));
		if (sig == NULL) {
			printf("could not allocate sig\n");
			return -1;
		}
		k_poll_signal_init(sig);
	}

	for (i = 0; i < ctx.vectors; i++) {
		ret = ivshmem_register_handler(ctx.dev, sig, i);
		if (ret < 0) {
			printf("registering handlers must be supported\n");
			return -1;
		}
	}
	return 0;
}

static void ivshmem_sample_failed(void)
{
	printf("test has failed\n");
	while (1) {
	}
}

#ifndef CONFIG_USERSPACE
static void ivshmem_sample_doorbell(void)
{
	int ret;

	ret = setup_ivshmem(true);
	if (ret < 0)
		return;
	ivshmem_event_loop(&ctx);
	/*
	 * if ivshmem_event_loop() returns, it means the function failed
	 *
	 * for kernelspace, if ivshmem_event_loop() fails, it will lead to
	 * main(), which, in turn, will call ivshmem_sample_failed()
	 */
	k_object_free(sig);
}
#else
static void user_entry(void *a, void *b, void *c)
{
	int ret;

	ret = setup_ivshmem(false);
	if (ret < 0) {
		goto fail;
	}
	ivshmem_event_loop(&ctx);
	/* if ivshmem_event_loop() returns, it means the function failed */
fail:
	k_object_release(sig);
	ivshmem_sample_failed();
}

static void ivshmem_sample_userspace_doorbell(void)
{
	int ret;
	const struct device *dev;
	struct k_mem_partition *parts[] = {
#if Z_LIBC_PARTITION_EXISTS
		&z_libc_partition,
#endif
#if Z_MALLOC_PARTITION_EXISTS
		&z_malloc_partition,
#endif
		&app_a_partition,
	};

	ret = k_mem_domain_init(&app_a_domain, ARRAY_SIZE(parts), parts);
	if (ret != 0) {
		printf("k_mem_domain_init failed %d\n", ret);
		return;
	}

	k_mem_domain_add_thread(&app_a_domain, k_current_get());

	dev = DEVICE_DT_GET(DT_NODELABEL(ivshmem0));
	if (dev == NULL) {
		printf("Could not get ivshmem device\n");
		return;
	}

	sig = k_object_alloc(K_OBJ_POLL_SIGNAL);
	if (sig == NULL) {
		printf("could not allocate sig\n");
		return;
	}
	k_poll_signal_init(sig);

	k_object_access_grant(dev, k_current_get());
	k_object_access_grant(sig, k_current_get());

	k_thread_user_mode_enter(user_entry, NULL, NULL, NULL);
}
#endif /* CONFIG_USERSPACE */

int main(void)
{
#ifdef CONFIG_USERSPACE
	ivshmem_sample_userspace_doorbell();
#else
	ivshmem_sample_doorbell();
#endif
	/* if the code reaches here, it means the setup/loop has failed */
	ivshmem_sample_failed();
	return 0;
}
