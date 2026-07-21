/*
 * Copyright (c) 2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_OBJ_CORE_H_
#define ZEPHYR_INCLUDE_KERNEL_OBJ_CORE_H_

#include <stddef.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/iterable_sections.h>

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
#endif /* CONFIG_OBJ_CORE */

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * Tools may use this list as an entry point to identify all registered
 * object types and the object cores linked to them.
 */
extern sys_slist_t z_obj_type_list;

/** Object core statistics descriptor */
struct k_obj_core_stats_desc {
	size_t  raw_size;   /**< Internal representation stats buffer size */
	size_t  query_size; /**< Stats buffer size used for reporting */

	/** Function pointer to retrieve internal representation of stats */
	int (*raw)(struct k_obj_core *obj_core, void *stats);
	/** Function pointer to retrieve reported statistics */
	int (*query)(struct k_obj_core *obj_core, void *stats);
	/** Function pointer to reset object's statistics */
	int (*reset)(struct k_obj_core *obj_core);
	/** Function pointer to disable object's statistics gathering */
	int (*disable)(struct k_obj_core *obj_core);
	/** Function pointer to enable object's statistics gathering */
	int (*enable)(struct k_obj_core *obj_core);
};

/** Object type structure */
struct k_obj_type {
	sys_snode_t    node;   /**< Node within list of object types */
	sys_slist_t    list;   /**< List of objects of this object type */
	uint32_t       id;     /**< Unique type ID */
	size_t         obj_core_offset;  /**< Offset to obj_core field */
#ifdef CONFIG_OBJ_CORE_STATS
	/** Pointer to object core statistics descriptor */
	struct k_obj_core_stats_desc *stats_desc;
#endif /* CONFIG_OBJ_CORE_STATS */
};

/** Object core structure */
struct k_obj_core {
	sys_snode_t        node;   /**< Object node within object type's list */
	struct k_obj_type *type;   /**< Object type to which object belongs */
#ifdef CONFIG_OBJ_CORE_STATS
	void  *stats;              /**< Pointer to kernel object's stats */
#endif /* CONFIG_OBJ_CORE_STATS */
};

/**
 * @cond INTERNAL_HIDDEN
 */

/**
 * Descriptor used to register an object type with the object core framework at
 * boot. One descriptor is emitted per participating object type via
 * K_OBJ_TYPE_DEFINE(); the kernel walks them all from a single init point
 * instead of each object type providing its own SYS_INIT.
 */
struct k_obj_core_desc {
	struct k_obj_type *type;        /**< Object type storage to initialize */
	const void        *objs_start;  /**< Start of static object section */
	const void        *objs_end;    /**< End of static object section */
	size_t             obj_core_offset; /**< Offset of obj_core in object */
	size_t             obj_size;    /**< Stride between static objects */
	uint32_t           type_id;     /**< Unique type ID */
#ifdef CONFIG_OBJ_CORE_STATS
	struct k_obj_core_stats_desc *stats_desc; /**< Stats descriptor or NULL */
	size_t             stats_offset; /**< Offset of per-object stats buffer */
	size_t             stats_size;   /**< Per-object stats buffer size (0=none) */
#endif /* CONFIG_OBJ_CORE_STATS */
};

#ifdef CONFIG_OBJ_CORE_STATS
#define Z_OBJ_CORE_STATS_DESC(_stats, _soff, _ssz) \
	.stats_desc = (_stats), .stats_offset = (_soff), .stats_size = (_ssz),
#else
#define Z_OBJ_CORE_STATS_DESC(_stats, _soff, _ssz)
#endif /* CONFIG_OBJ_CORE_STATS */

#define Z_K_OBJ_TYPE_DEFINE(_type_var, _struct, _id, _stats, _soff, _ssz)      \
	STRUCT_SECTION_START_EXTERN(_struct);                                  \
	STRUCT_SECTION_END_EXTERN(_struct);                                    \
	static const STRUCT_SECTION_ITERABLE(k_obj_core_desc,                  \
					     _obj_core_desc_##_struct) = {     \
		.type = &(_type_var),                                          \
		.objs_start = STRUCT_SECTION_START(_struct),                   \
		.objs_end = STRUCT_SECTION_END(_struct),                       \
		.obj_core_offset = offsetof(struct _struct, obj_core),         \
		.obj_size = sizeof(struct _struct),                            \
		.type_id = (_id),                                              \
		Z_OBJ_CORE_STATS_DESC(_stats, _soff, _ssz)                     \
	}

/**
 * @brief Register an object type with the object core framework
 *
 * Emits a descriptor that the kernel uses at boot to initialize @a _type_var
 * and to initialize and link every statically defined object of @a _struct.
 * This replaces the per-type SYS_INIT boilerplate that would otherwise iterate
 * the object's static section by hand.
 *
 * @param _type_var Object type storage (struct k_obj_type) to initialize
 * @param _struct   Object struct type (e.g. k_sem) with an obj_core member
 * @param _id       Unique type ID (e.g. K_OBJ_TYPE_SEM_ID)
 * @param _stats    Pointer to a k_obj_core_stats_desc, or NULL
 */
#define K_OBJ_TYPE_DEFINE(_type_var, _struct, _id, _stats)                     \
	Z_K_OBJ_TYPE_DEFINE(_type_var, _struct, _id, _stats, 0, 0)

/**
 * @brief Register an object type that also gathers per-object statistics
 *
 * Like K_OBJ_TYPE_DEFINE(), but additionally registers each statically defined
 * object's embedded statistics buffer with the object core framework at boot.
 *
 * @param _type_var Object type storage (struct k_obj_type) to initialize
 * @param _struct   Object struct type with obj_core and stats members
 * @param _id       Unique type ID
 * @param _stats    Pointer to a k_obj_core_stats_desc
 * @param _member   Name of the per-object stats buffer member within @a _struct
 */
#define K_OBJ_TYPE_DEFINE_STATS(_type_var, _struct, _id, _stats, _member)      \
	Z_K_OBJ_TYPE_DEFINE(_type_var, _struct, _id, _stats,                   \
			    offsetof(struct _struct, _member),                 \
			    sizeof(((struct _struct *)0)->_member))

/**
 * @brief Register an object type without linking any static objects
 *
 * Like K_OBJ_TYPE_DEFINE(), but for object types that have no statically
 * defined instances to walk and link at boot (e.g. threads, which link their
 * own object core as they are created). Only the object type (and its stats
 * descriptor, if any) is initialized.
 *
 * @param _type_var Object type storage (struct k_obj_type) to initialize
 * @param _struct   Object struct type with an obj_core member
 * @param _id       Unique type ID
 * @param _stats    Pointer to a k_obj_core_stats_desc, or NULL
 */
#define K_OBJ_TYPE_DEFINE_TYPE_ONLY(_type_var, _struct, _id, _stats)           \
	static const STRUCT_SECTION_ITERABLE(k_obj_core_desc,                  \
					     _obj_core_desc_##_struct) = {     \
		.type = &(_type_var),                                          \
		.objs_start = NULL,                                            \
		.objs_end = NULL,                                              \
		.obj_core_offset = offsetof(struct _struct, obj_core),         \
		.obj_size = sizeof(struct _struct),                            \
		.type_id = (_id),                                              \
		Z_OBJ_CORE_STATS_DESC(_stats, 0, 0)                            \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

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
 * @return Pointer to initialized object type
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
 * @return Pointer to object type if found
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

/**
 * @defgroup obj_core_stats_apis Object Core Statistics APIs
 * @ingroup kernel_apis
 * @{
 */

#ifdef CONFIG_OBJ_CORE_STATS
/**
 * @brief Initialize the object type's stats descriptor
 *
 * This routine initializes the object type's stats descriptor.
 *
 * @param type Pointer to the object type
 * @param stats_desc Pointer to the object core statistics descriptor
 */
static inline void k_obj_type_stats_init(struct k_obj_type *type,
					 struct k_obj_core_stats_desc *stats_desc)
{
	type->stats_desc = stats_desc;
}

/**
 * @brief Initialize the object core for statistics
 *
 * This routine initializes the object core to operate within the object core
 * statistics framework.
 *
 * @param obj_core Pointer to the object core
 * @param stats Pointer to the object's raw statistics
 */
static inline void k_obj_core_stats_init(struct k_obj_core *obj_core,
					 void *stats)
{
	obj_core->stats = stats;
}
#endif /* CONFIG_OBJ_CORE_STATS */

/**
 * @brief Register kernel object for gathering statistics
 *
 * Before a kernel object can gather statistics, it must be registered to do
 * so. Registering will also automatically enable the kernel object to gather
 * its statistics.
 *
 * @param obj_core Pointer to kernel object core
 * @param stats Pointer to raw kernel statistics
 * @param stats_len Size of raw kernel statistics buffer
 *
 * @retval 0 on success
 * @retval -errno on failure
 */
int k_obj_core_stats_register(struct k_obj_core *obj_core, void *stats,
			      size_t stats_len);

/**
 * @brief Deregister kernel object from gathering statistics
 *
 * Deregistering a kernel object core from gathering statistics prevents it
 * from gathering any more statistics. It is expected to be invoked at the end
 * of a kernel object's life cycle.
 *
 * @param obj_core Pointer to kernel object core
 *
 * @retval 0 on success
 * @retval -errno on failure
 */
int k_obj_core_stats_deregister(struct k_obj_core *obj_core);

/**
 * @brief Retrieve the raw statistics associated with the kernel object
 *
 * This function copies the raw statistics associated with the kernel object
 * core specified by @a obj_core into the buffer @a stats. Note that the size
 * of the buffer (@a stats_len) must match the size specified by the kernel
 * object type's statistics descriptor.
 *
 * @param obj_core Pointer to kernel object core
 * @param stats Pointer to memory buffer into which to copy raw stats
 * @param stats_len Length of the memory buffer
 *
 * @retval 0 on success
 * @retval -errno on failure
 */
int k_obj_core_stats_raw(struct k_obj_core *obj_core, void *stats,
			 size_t stats_len);

/**
 * @brief Retrieve the statistics associated with the kernel object
 *
 * This function copies the statistics associated with the kernel object core
 * specified by @a obj_core into the buffer @a stats. Unlike the raw statistics
 * this may report calculated values such as averages.  Note that the size of
 * the buffer (@a stats_len) must match the size specified by the kernel object
 * type's statistics descriptor.
 *
 * @param obj_core Pointer to kernel object core
 * @param stats Pointer to memory buffer into which to copy the queried stats
 * @param stats_len Length of the memory buffer
 *
 * @retval 0 on success
 * @retval -errno on failure
 */
int k_obj_core_stats_query(struct k_obj_core *obj_core, void *stats,
			   size_t stats_len);

/**
 * @brief Reset the stats associated with the kernel object
 *
 * This function resets the statistics associated with the kernel object core
 * specified by @a obj_core.
 *
 * @param obj_core Pointer to kernel object core
 *
 * @retval 0 on success
 * @retval -errno on failure
 */
int k_obj_core_stats_reset(struct k_obj_core *obj_core);

/**
 * @brief Stop gathering the stats associated with the kernel object
 *
 * This function temporarily stops the gathering of statistics associated with
 * the kernel object core specified by @a obj_core. The gathering of statistics
 * can be resumed by invoking :c:func :`k_obj_core_stats_enable`.
 *
 * @param obj_core Pointer to kernel object core
 *
 * @retval 0 on success
 * @retval -errno on failure
 */
int k_obj_core_stats_disable(struct k_obj_core *obj_core);

/**
 * @brief Reset the stats associated with the kernel object
 *
 * This function resumes the gathering of statistics associated with the kernel
 * object core specified by @a obj_core.
 *
 * @param obj_core Pointer to kernel object core
 *
 * @retval 0 on success
 * @retval -errno on failure
 */
int k_obj_core_stats_enable(struct k_obj_core *obj_core);

/** @} */
#endif /* ZEPHYR_INCLUDE_KERNEL_OBJ_CORE_H_ */
