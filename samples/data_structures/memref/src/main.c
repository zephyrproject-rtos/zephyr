/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/zassert.h>
#include <zephyr/sys/memref.h>
#include <zephyr/sys/printk.h>

ZASSERT_MODULE(DEFAULT);

struct message {
	int32_t id;
};

K_HEAP_DEFINE(memref_pool, 1024);

static void *heap_alloc(void *ctx, size_t size)
{
	return k_heap_alloc((struct k_heap *)ctx, size, K_NO_WAIT);
}

static void heap_free(void *ctx, void *mem)
{
	k_heap_free((struct k_heap *)ctx, mem);
}

MEMREF_BACKEND_DEFINE(pool_backend, heap_alloc, heap_free, &memref_pool);

struct consumer {
	struct k_msgq q;
	struct k_thread thread;
	void (*consume)(void *self, struct message *msg);
	uint8_t sleep_time;
	uint8_t msgq_buffer[10 * sizeof(struct message *)];
};

static void msg_cleanup(void *mem)
{
	struct message *msg = mem;

	printk("cleanup msg=%d (released by %p)\n", msg->id, k_current_get());
}

static void msg_consume_even(void *self, struct message *msg)
{
	struct consumer *c = (struct consumer *)self;

	if (!(msg->id % 2)) {
		printk("[even] consuming id=%d\n", msg->id);
		memref_ref(msg);
		if (k_msgq_put(&c->q, &msg, K_NO_WAIT)) {
			printk("msgq full, dropping msg=%d\n", msg->id);
			memref_unref(msg);
		}
	}
}

static void msg_consume_odd(void *self, struct message *msg)
{
	struct consumer *c = (struct consumer *)self;

	if (msg->id % 2) {
		printk("[odd] takes ownership of message %p\n", msg);
		memref_ref(msg);
		if (k_msgq_put(&c->q, &msg, K_NO_WAIT)) {
			printk("msgq full, dropping msg=%d\n", msg->id);
			memref_unref(msg);
		}
	}

}

static void msg_consume_every_third(void *self, struct message *msg)
{
	struct consumer *c = (struct consumer *)self;

	if (msg->id % 3 == 0) {
		printk("[every third] takes ownership of message: %d\n", msg->id);
		memref_ref(msg);
		if (k_msgq_put(&c->q, &msg, K_NO_WAIT)) {
			printk("msgq full, dropping msg=%d\n", msg->id);
			memref_unref(msg);
		}
	}
}

static void consumer_entry(void *self, void *arg2, void *arg3)
{
	struct message *msg;
	struct consumer *c = (struct consumer *)self;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	const char *consumer_name = c->sleep_time == 100 ? "even" : c->sleep_time == 200 ? "odd" : "every third";
	for (;;) {
		k_msgq_get(&c->q, &msg, K_FOREVER);
		printk("consumer[%s] processing message %d\n", consumer_name, msg->id);
		k_msleep(c->sleep_time);
		printk("consumer[%s] done processing message %d\n", consumer_name, msg->id);
		memref_unref(msg);
	}
}

static struct consumer consumers[] = {
	{
		.consume = msg_consume_even,
		.sleep_time = 100,
	},
	{
		.consume = msg_consume_odd,
		.sleep_time = 200,
	},
	{
		.consume = msg_consume_every_third,
		.sleep_time = 250,
	},
};
K_THREAD_STACK_ARRAY_DEFINE(consumer_stacks, ARRAY_SIZE(consumers), 1024);

int main(void)
{
	struct message *msg;
	for (int i = 0; i < ARRAY_SIZE(consumers); i++) {
		k_msgq_init(&consumers[i].q, consumers[i].msgq_buffer, sizeof(struct message *), 10);
		k_thread_create(&consumers[i].thread, consumer_stacks[i], K_THREAD_STACK_SIZEOF(consumer_stacks[i]),
				consumer_entry, &consumers[i], NULL, NULL,
				K_PRIO_PREEMPT(ARRAY_SIZE(consumers) - i), 0, K_NO_WAIT);
	}

	for (int i = 0; i < 15; i++) {
		msg = memref_alloc(&pool_backend, sizeof(struct message), msg_cleanup);
		ZASSERT(msg != NULL, "Failed to allocate message");
		msg->id = i;
		printk("[Main thread] producing id=%d\n", msg->id);
		for (int j = 0; j < ARRAY_SIZE(consumers); j++) {
			consumers[j].consume(&consumers[j], msg);
		}
		memref_unref(msg);
	}
}

