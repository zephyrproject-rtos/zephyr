/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	gcc/atomic.h
 * @brief	GCC specific atomic primitives for libmetal.
 */

#ifndef __METAL_GCC_ATOMIC__H__
#define __METAL_GCC_ATOMIC__H__

#ifdef __cplusplus
extern "C" {
#endif

typedef int atomic_flag;
typedef char atomic_char;
typedef unsigned char atomic_uchar;
typedef short atomic_short;
typedef unsigned short atomic_ushort;
typedef int atomic_int;
typedef unsigned int atomic_uint;
typedef long atomic_long;
typedef unsigned long atomic_ulong;
typedef long long atomic_llong;
typedef unsigned long long atomic_ullong;

#define ATOMIC_FLAG_INIT	0
#define ATOMIC_VAR_INIT(VAL)	(VAL)

typedef enum {
	memory_order_relaxed,
	memory_order_consume,
	memory_order_acquire,
	memory_order_release,
	memory_order_acq_rel,
	memory_order_seq_cst,
} memory_order;

#define atomic_flag_test_and_set(FLAG)					\
	__sync_lock_test_and_set((FLAG), 1)
#define atomic_flag_test_and_set_explicit(FLAG, MO)			\
	atomic_flag_test_and_set(FLAG)
#define atomic_flag_clear(FLAG)						\
	__sync_lock_release((FLAG))
#define atomic_flag_clear_explicit(FLAG, MO)				\
	atomic_flag_clear(FLAG)
#define atomic_init(OBJ, VAL)						\
	do { *(OBJ) = (VAL); } while (0)
#define atomic_is_lock_free(OBJ)					\
	(sizeof(*(OBJ)) <= sizeof(long))
#define atomic_store(OBJ, VAL)						\
	do { *(OBJ) = (VAL); __sync_synchronize(); } while (0)
#define atomic_store_explicit(OBJ, VAL, MO)				\
	atomic_store((OBJ), (VAL))
#define atomic_load(OBJ)						\
	({ __sync_synchronize(); *(OBJ); })
#define atomic_load_explicit(OBJ, MO)					\
	atomic_load(OBJ)
#define atomic_exchange(OBJ, DES)					\
	({								\
		typeof(OBJ) obj = (OBJ);				\
		typeof(*obj) des = (DES);				\
		typeof(*obj) expval;					\
		typeof(*obj) oldval = atomic_load(obj);			\
		do {							\
			expval = oldval;				\
			oldval = __sync_val_compare_and_swap(		\
				obj, expval, des);			\
		} while (oldval != expval);				\
		oldval;							\
	})
#define atomic_exchange_explicit(OBJ, DES, MO)				\
	atomic_exchange((OBJ), (DES))
#define atomic_compare_exchange_strong(OBJ, EXP, DES)			\
	({								\
		typeof(OBJ) obj = (OBJ);				\
		typeof(EXP) exp = (EXP);				\
		typeof(*obj) expval = *exp;				\
		typeof(*obj) oldval = __sync_val_compare_and_swap(	\
			obj, expval, (DES));				\
		*exp = oldval;						\
		oldval == expval;					\
	})
#define atomic_compare_exchange_strong_explicit(OBJ, EXP, DES, MO)	\
	atomic_compare_exchange_strong((OBJ), (EXP), (DES))
#define atomic_compare_exchange_weak(OBJ, EXP, DES)			\
	atomic_compare_exchange_strong((OBJ), (EXP), (DES))
#define atomic_compare_exchange_weak_explicit(OBJ, EXP, DES, MO)	\
	atomic_compare_exchange_weak((OBJ), (EXP), (DES))
#define atomic_fetch_add(OBJ, VAL)					\
	__sync_fetch_and_add((OBJ), (VAL))
#define atomic_fetch_add_explicit(OBJ, VAL, MO)				\
	atomic_fetch_add((OBJ), (VAL))
#define atomic_fetch_sub(OBJ, VAL)					\
	__sync_fetch_and_sub((OBJ), (VAL))
#define atomic_fetch_sub_explicit(OBJ, VAL, MO)				\
	atomic_fetch_sub((OBJ), (VAL))
#define atomic_fetch_or(OBJ, VAL)					\
	__sync_fetch_and_or((OBJ), (VAL))
#define atomic_fetch_or_explicit(OBJ, VAL, MO)				\
	atomic_fetch_or((OBJ), (VAL))
#define atomic_fetch_xor(OBJ, VAL)					\
	__sync_fetch_and_xor((OBJ), (VAL))
#define atomic_fetch_xor_explicit(OBJ, VAL, MO)				\
	atomic_fetch_xor((OBJ), (VAL))
#define atomic_fetch_and(OBJ, VAL)					\
	__sync_fetch_and_and((OBJ), (VAL))
#define atomic_fetch_and_explicit(OBJ, VAL, MO)				\
	atomic_fetch_and((OBJ), (VAL))
#define atomic_thread_fence(MO)						\
	__sync_synchronize()
#define atomic_signal_fence(MO)						\
	__sync_synchronize()

#ifdef __cplusplus
}
#endif

#endif /* __METAL_GCC_ATOMIC__H__ */
