/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <atomic.h>
#include <kernel.h>
#include <random.h>
#include <string.h>
#include <tinycrypt/aes.h>

#define STIR_THREAD_STACK_SIZE 768

struct random_pool {
	struct k_sem sem;
	u32_t pot[4];

#if defined(CONFIG_RANDOM_POOL_SOURCE_CYCLE_COUNTER)
	atomic_val_t rand32_counter;
#define RAND32_INC 1000000013
#endif

#if defined(CONFIG_RANDOM_SOURCE_HARDWARE)
	struct device *hardware_rng;
#endif

	struct k_thread stir_thread;
	K_THREAD_STACK_MEMBER(stir_thread_stack, STIR_THREAD_STACK_SIZE);
};

static inline void explicit_bzero(void *ptr, size_t size)
{
	memset(ptr, 0, size);
	__asm__ __volatile__ ("" : : "g"(ptr) : "memory");
}

static inline u32_t rol32(u32_t ch, u32_t n)
{
	n &= 31;
	return ch << n | ch >> (32 - n);
}

static void stir_bytes(struct random_pool *ctx, void *ptr, size_t size)
{
	/* Assumes pot_sem is locked. */
	static u32_t rotate_count;
	u8_t *p = ptr;
	size_t i;

	for (i = 0; i < size; i++) {
		u32_t v = rol32(*p++, rotate_count++);

		v ^= ctx->pot[0];
		v ^= ctx->pot[1];
		v ^= ctx->pot[2];
		v ^= ctx->pot[3];

		ctx->pot[rotate_count & 3] = v;
	}
}

#if defined(CONFIG_RANDOM_POOL_SOURCE_DEVICES)
static void init_devices_source(struct random_pool *ctx)
{
	struct device *device_list;
	int devices;
	int i;

	/* While not random, this ensures that there will be different bits
	 * for each different device configuration.
	 */

	device_list_get(&device_list, &devices);
	for (i = 0; i < devices; i++) {
		stir_bytes(ctx, device_list[i], sizeof(struct device));
	}
}
#else
#define init_devices_source(...) do { } while (0)
#endif

#if defined(CONFIG_RANDOM_SOURCE_HARDWARE)
static inline void stir_hardware_rng(struct random_pool *ctx)
{
	u32_t v;

	random_get_entropy(ctx->hardware_rng, (u8_t *)&v, sizeof(v));
	stir_bytes(ctx, &v, sizeof(v));

	explicit_bzero(&v, sizeof(v));
}

static void init_hardware_source(struct random_pool *ctx)
{
	ctx->hardware_rng = device_get_binding(CONFIG_RANDOM_HARDWARE_NAME);
	if (!ctx->hardware_rng) {
		k_panic();
	}

	stir_hardware_rng(ctx);
}
#else
#define init_hardware_source(...) do { } while (0)
#endif

static inline void stir_u32(struct random_pool *ctx, u32_t v)
{
	stir_bytes(ctx, &v, sizeof(v));
	explicit_bzero(&v, sizeof(v));
}

static void pot_stirrer(void *arg1, void *arg2, void *arg3)
{
	struct device *device = arg1;
	struct random_pool *ctx = device->driver_data;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		k_sem_take(&ctx->sem, K_FOREVER);

		/* TODO: Maybe try to keep the mutex locked for the least
		 * amount possible, stirring the pot with stuff from one
		 * source each time.
		 */

#if defined(CONFIG_RANDOM_SOURCE_HARDWARE)
		stir_hardware_rng(ctx);
#endif

#if defined(CONFIG_RANDOM_SOURCE_TIMESTAMP)
		stir_u32(ctx, _do_read_cpu_timestamp32());
#endif

#if defined(CONFIG_RANDOM_SOURCE_TIMER)
		stir_u32(ctx, k_cycle_get_32() +
			 atomic_add(&ctx->rand32_counter, RAND32_INC));
#endif

		k_sem_give(&ctx->sem);

		/* TODO: One second might be too much: maybe increase this
		 * value?  Make it configurable?
		 */
		k_sleep(K_SECONDS(1));
	}
}

static u32_t increment_counter(void)
{
	static atomic_val_t aes_counter;

	/* TODO: store counter in flash? */

	return atomic_add(&aes_counter, 1);
}

static u32_t random_pool_get_u32(struct device *device)
{
	struct random_pool *ctx = device->driver_data;
	struct tc_aes_key_sched_struct sched;
	union {
		u32_t dw[4];
		u8_t b[16];
	} key = {
		.dw[3] = increment_counter()
	}, cyphertext;
	u32_t ret;

	tc_aes128_set_encrypt_key(&sched, key.b);

	k_sem_take(&ctx->sem, K_FOREVER);

#if defined(CONFIG_RANDOM_SOURCE_HARDWARE)
	/* Don't wait for the pot_stirrer to obtain a random number
	 * from the hardware: stir some of those in now.
	 */
	stir_hardware_rng(ctx);
#endif

	tc_aes_encrypt(cyphertext.b, (u8_t *)ctx->pot, &sched);
	stir_bytes(ctx, cyphertext.b, sizeof(cyphertext.b));

	k_sem_give(&ctx->sem);

	ret = cyphertext.dw[0];
	ret ^= cyphertext.dw[1];
	ret ^= cyphertext.dw[2];
	ret ^= cyphertext.dw[3];

	explicit_bzero(&sched, sizeof(sched));
	explicit_bzero(&key, sizeof(key));
	explicit_bzero(&cyphertext, sizeof(cyphertext));

	return ret;
}

static int random_pool_get_entropy(struct device *device, u8_t *buf, u16_t len)
{
	u32_t v;
	u16_t pos = 0;

	while (len) {
		u16_t rest = (len >= sizeof(v)) ? sizeof(v) : len;

		v = random_pool_get_u32(device);
		memcpy(buf + pos, &v, rest);

		pos += sizeof(u32_t);
		len -= rest;
	}

	explicit_bzero(&v, sizeof(v));

	return 0;
}

static int random_pool_init(struct device *device)
{
	struct random_pool *ctx = device->driver_data;

	init_devices_source(ctx);
	init_hardware_source(ctx);

	/* TODO: restore counter from flash? */

	k_sem_init(&ctx->sem, 0, UINT_MAX);
	k_sem_give(&ctx->sem);

	/* TODO: Maybe not even create the thread if no sources have been
	 * chosen?
	 */
	k_thread_create(&ctx->stir_thread, ctx->stir_thread_stack,
			STIR_THREAD_STACK_SIZE,
			pot_stirrer, device, NULL, NULL,
			K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

	return 0;
}

static struct random_pool random_pool_ctx;

static struct random_driver_api random_pool_api_funcs = {
	.get_entropy = random_pool_get_entropy
};

DEVICE_AND_API_INIT(random_pool, CONFIG_RANDOM_NAME,
		    random_pool_init, &random_pool_ctx, NULL,
		    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &random_pool_api_funcs);

u32_t sys_rand32_get(void)
{
	return random_pool_get_u32(DEVICE_GET(random_pool));
}
