/*
 * Copyright (c) 2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __KERNEL_OBJ_CORE_H__
#define __KERNEL_OBJ_CORE_H__

#include <zephyr/sys/slist.h>

/**
 * @defgroup obj_core_apis Object Core APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Convert kernel object pointer into its object core pointer
 */
#define K_OBJ_CORE(kobj)  (&((kobj)->obj_core))

/**
 * @brief Generate new object type IDs based on a 4 letter string
 */
#define K_OBJ_TYPE_ID_GEN(s)     ((s[0] << 24) | (s[1] << 16) | (s[2] << 8) | (s[3]))

/* Known kernel object types */

/** Condition variable object type */
#define K_OBJ_TYPE_CONDVAR_ID    K_OBJ_TYPE_ID_GEN("COND")
/** CPU object type */
#define K_OBJ_TYPE_CPU_ID        K_OBJ_TYPE_ID_GEN("CPU_")
/** Event object type */
#define K_OBJ_TYPE_EVENT_ID      K_OBJ_TYPE_ID_GEN("EVNT")
/** FIFO object type */
#define K_OBJ_TYPE_FIFO_ID       K_OBJ_TYPE_ID_GEN("FIFO")
/** Kernel object type */
#define K_OBJ_TYPE_KERNEL_ID     K_OBJ_TYPE_ID_GEN("KRNL")
/** LIFO object type */
#define K_OBJ_TYPE_LIFO_ID       K_OBJ_TYPE_ID_GEN("LIFO")
/** Memory block object type */
#define K_OBJ_TYPE_MEM_BLOCK_ID  K_OBJ_TYPE_ID_GEN("MBLK")
/** Mailbox object type */
#define K_OBJ_TYPE_MBOX_ID       K_OBJ_TYPE_ID_GEN("MBOX")
/** Memory slab object type */
#define K_OBJ_TYPE_MEM_SLAB_ID   K_OBJ_TYPE_ID_GEN("SLAB")
/** Message queue object type */
#define K_OBJ_TYPE_MSGQ_ID       K_OBJ_TYPE_ID_GEN("MSGQ")
/** Mutex object type */
#define K_OBJ_TYPE_MUTEX_ID      K_OBJ_TYPE_ID_GEN("MUTX")
/** Pipe object type */
#define K_OBJ_TYPE_PIPE_ID       K_OBJ_TYPE_ID_GEN("PIPE")
/** Semaphore object type */
#define K_OBJ_TYPE_SEM_ID        K_OBJ_TYPE_ID_GEN("SEM4")
/** Stack object type */
#define K_OBJ_TYPE_STACK_ID      K_OBJ_TYPE_ID_GEN("STCK")
/** Thread object type */
#define K_OBJ_TYPE_THREAD_ID     K_OBJ_TYPE_ID_GEN("THRD")
/** Timer object type */
#define K_OBJ_TYPE_TIMER_ID      K_OBJ_TYPE_ID_GEN("TIMR")

struct k_obj_type;
struct k_obj_core;

/**
 * @cond INTERNAL_HIDDEN
 */

#ifdef CONFIG_OBJ_CORE
#define K_OBJ_CORE_INIT(_objp, _obj_type)   \
	extern struct k_obj_type _obj_type; \
	k_obj_core_init(_objp, &_obj_type)

#define K_OBJ_CORE_LINK(objp) k_obj_core_link(objp)
#else
#define K_OBJ_CORE_INIT(objp, type)   do { } while (0)
#define K_OBJ_CORE_LINK(objp)         do { } while (0)
#endif

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * Tools may use this list as an entry point to identify all registered
 * object types and the object cores linked to them.
 */
extern sys_slist_t z_obj_type_list;

/** Object type structure */
struct k_obj_type {
	sys_snode_t    node;   /**< Node within list of object types */
	sys_slist_t    list;   /**< List of objects of this object type */
	uint32_t       id;     /**< Unique type ID */
	size_t         obj_core_offset;  /**< Offset to obj_core field */
};

/** Object core structure */
struct k_obj_core {
	sys_snode_t        node;   /**< Object node within object type's list */
	struct k_obj_type *type;   /**< Object type to which object belongs */
};

/**
 * @brief Initialize a specific object type
 *
 * Initializes a specific object type and links it into the object core
 * framework.
 *
 * @param type Pointer to the object type to initialize
 * @param id A means to identify the object type
 * @param off Offset of object core within the structure
 *
 * @retval Pointer to initialized object type
 */
struct k_obj_type *z_obj_type_init(struct k_obj_type *type,
				   uint32_t id, size_t off);

/**
 * @brief Find a specific object type by ID
 *
 * Given an object type ID, this function searches for the object type that
 * is associated with the specified type ID @a type_id.
 *
 * @param type_id  Type ID associated with object type
 *
 * @retval NULL if object type not found
 * @retval Pointer to object type if found
 */
struct k_obj_type *k_obj_type_find(uint32_t type_id);

/**
 * @brief Walk the object type's list of object cores
 *
 * This function takes a global spinlock and walks the object type's list
 * of object cores and invokes the callback function on each element while
 * holding that lock. Although this will ensure that the list is not modified,
 * one can expect a significant penalty in terms of performance and latency.
 *
 * The callback function shall either return non-zero to stop further walking,
 * or it shall return 0 to continue walking.
 *
 * @param type  Pointer to the object type
 * @param func  Callback to invoke on each object core of the object type
 * @param data  Custom data passed to the callback
 *
 * @retval non-zero if walk is terminated by the callback; otherwise 0
 */
int k_obj_type_walk_locked(struct k_obj_type *type,
			   int (*func)(struct k_obj_core *, void *),
				  void *data);

/**
 * @brief Walk the object type's list of object cores
 *
 * This function is similar to k_obj_type_walk_locked() except that it walks
 * the list without obtaining the global spinlock. No synchronization is
 * provided here. Mutation of the list of objects while this function is in
 * progress must be prevented at the application layer, otherwise
 * undefined/unreliable behavior, corruption and/or crashes may result.
 *
 * The callback function shall either return non-zero to stop further walking,
 * or it shall return 0 to continue walking.
 *
 * @param type  Pointer to the object type
 * @param func  Callback to invoke on each object core of the object type
 * @param data  Custom data passed to the callback
 *
 * @retval non-zero if walk is terminated by the callback; otherwise 0
 */
int k_obj_type_walk_unlocked(struct k_obj_type *type,
			     int (*func)(struct k_obj_core *, void *),
			     void *data);

/**
 * @brief Initialize the core of the kernel object
 *
 * Initializing the kernel object core associates it with the specified
 * kernel object type.
 *
 * @param obj_core Pointer to the kernel object to initialize
 * @param type Pointer to the kernel object type
 */
void k_obj_core_init(struct k_obj_core *obj_core, struct k_obj_type *type);

/**
 * @brief Link the kernel object to the kernel object type list
 *
 * A kernel object can be optionally linked into the kernel object type's
 * list of objects. A kernel object must have been initialized before it
 * can be linked. Linked kernel objects can be traversed and have information
 * extracted from them by system tools.
 *
 * @param obj_core Pointer to the kernel object
 */
void k_obj_core_link(struct k_obj_core *obj_core);

/**
 * @brief Automatically link the kernel object after initializing it
 *
 * A useful wrapper to both initialize the core of the kernel object and
 * automatically link it into the kernel object type's list of objects.
 *
 * @param obj_core Pointer to the kernel object to initialize
 * @param type Pointer to the kernel object type
 */
void k_obj_core_init_and_link(struct k_obj_core *obj_core,
			      struct k_obj_type *type);

/**
 * @brief Unlink the kernel object from the kernel object type list
 *
 * Kernel objects can be unlinked from their respective kernel object type
 * lists. If on a list, it must be done at the end of the kernel object's life
 * cycle.
 *
 * @param obj_core Pointer to the kernel object
 */
void k_obj_core_unlink(struct k_obj_core *obj_core);

/** @} */
#endif /* __KERNEL_OBJ_CORE_H__ */
