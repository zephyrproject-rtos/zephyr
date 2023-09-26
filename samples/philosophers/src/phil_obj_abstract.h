/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Object type abstraction.
 *
 * Each object type that can be used as a "fork" provides:
 *
 * - a definition for fork_t (a reference to a fork) and fork_obj_t (an
 *   instance of the fork object)
 * - a 'fork_init' function that initializes the object
 * - a 'take' function that simulates taking the fork (eg. k_sem_take)
 * - a 'drop' function that simulates dropping the fork (eg. k_mutex_unlock)
 * - a 'fork_type_str' string defining the object type
 *
 * When using dynamic objects, the instances of the fork objects are placed
 * automatically in the fork_objs[] array . References to these are in turn
 * placed automatically in the forks[] array, the array of references to the
 * forks.
 *
 * When using static objects, references to each object must be put by hand in
 * the forks[] array.
 */

#ifndef phil_obj_abstract__h
#define phil_obj_abstract__h

#define MAGIC 0xa5a5ee11

#if (FORKS == FIFOS) || (FORKS == LIFOS)
	struct packet {
		void *next;
		int data;
	} orig_packet[NUM_PHIL];
#endif

#if FORKS == SEMAPHORES
	#define fork_t struct k_sem *
	#if STATIC_OBJS
		K_SEM_DEFINE(fork0, 1, 1);
		K_SEM_DEFINE(fork1, 1, 1);
		K_SEM_DEFINE(fork2, 1, 1);
		K_SEM_DEFINE(fork3, 1, 1);
		K_SEM_DEFINE(fork4, 1, 1);
		K_SEM_DEFINE(fork5, 1, 1);
	#else
		#define fork_obj_t struct k_sem
		#define fork_init(x) k_sem_init(x, 1, 1)
	#endif
	#define take(x) k_sem_take(x, K_FOREVER)
	#define drop(x) k_sem_give(x)
	#define fork_type_str "semaphores"
#elif FORKS == MUTEXES
	#define fork_t struct k_mutex *
	#if STATIC_OBJS
		K_MUTEX_DEFINE(fork0);
		K_MUTEX_DEFINE(fork1);
		K_MUTEX_DEFINE(fork2);
		K_MUTEX_DEFINE(fork3);
		K_MUTEX_DEFINE(fork4);
		K_MUTEX_DEFINE(fork5);
	#else
		#define fork_obj_t struct k_mutex
		#define fork_init(x) k_mutex_init(x)
	#endif
	#define take(x) k_mutex_lock(x, K_FOREVER)
	#define drop(x) k_mutex_unlock(x)
	#define fork_type_str "mutexes"
#elif FORKS == STACKS
	#define fork_t struct k_stack *
	#if STATIC_OBJS
		#error "not implemented yet."
	#else
		typedef struct {
			struct k_stack stack;
			uint32_t stack_mem[1];
		} fork_obj_t;
		#define fork_init(x) do { \
			k_stack_init(x, (stack_data_t *)((x) + 1), 1); \
			k_stack_push(x, MAGIC); \
		} while ((0))
	#endif
	#define take(x) do { \
		stack_data_t data; k_stack_pop(x, &data, K_FOREVER); \
		__ASSERT(data == MAGIC, "data was %lx\n", data); \
	} while ((0))
	#define drop(x) k_stack_push(x, MAGIC)
	#define fork_type_str "stacks"
#elif FORKS == FIFOS
	#define fork_t struct k_fifo *
	#if STATIC_OBJS
		#error "not implemented yet."
	#else
		typedef struct {
			struct k_fifo fifo;
			struct packet data;
		} fork_obj_t;
		#define fork_init(x) do { \
			k_fifo_init(x); \
			((fork_obj_t *)(x))->data.data = MAGIC; \
			k_fifo_put(x, &(((fork_obj_t *)(x))->data)); \
		} while ((0))
	#endif
	#define take(x) do { \
		struct packet *data; \
		data = k_fifo_get(x, K_FOREVER); \
		__ASSERT(data->data == MAGIC, ""); \
	} while ((0))
	#define drop(x) k_fifo_put(x, &(((fork_obj_t *)(x))->data))
	#define fork_type_str "fifos"
#elif FORKS == LIFOS
	#define fork_t struct k_lifo *
	#if STATIC_OBJS
		#error "not implemented yet."
	#else
		typedef struct {
			struct k_lifo lifo;
			struct packet data;
		} fork_obj_t;
		#define fork_init(x) do { \
			k_lifo_init(x); \
			((fork_obj_t *)(x))->data.data = MAGIC; \
			k_lifo_put(x, &(((fork_obj_t *)(x))->data)); \
		} while ((0))
	#endif
	#define take(x) do { \
		struct packet *data; \
		data = k_lifo_get(x, K_FOREVER); \
		__ASSERT(data->data == MAGIC, ""); \
	} while ((0))
	#define drop(x) k_lifo_put(x, &(((fork_obj_t *)(x))->data))
	#define fork_type_str "lifos"
#else
	#error unknown fork type
#endif

#if STATIC_OBJS
	#define obj_init_type "static"
#else
	#define obj_init_type "dynamic"
	fork_obj_t fork_objs[NUM_PHIL];
#endif

static fork_t forks[NUM_PHIL] = {
#if STATIC_OBJS
	&fork0, &fork1, &fork2,
	&fork3, &fork4, &fork5,
#else
	(fork_t)&fork_objs[0], (fork_t)&fork_objs[1], (fork_t)&fork_objs[2],
	(fork_t)&fork_objs[3], (fork_t)&fork_objs[4], (fork_t)&fork_objs[5],
#endif
};

static K_THREAD_STACK_ARRAY_DEFINE(stacks, NUM_PHIL, STACK_SIZE);
static struct k_thread threads[NUM_PHIL];

#endif /* phil_obj_abstract__h */
