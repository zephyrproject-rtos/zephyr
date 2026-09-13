/*
 * Copyright (c) 2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_OBJ_CORE_H_
#define ZEPHYR_INCLUDE_KERNEL_OBJ_CORE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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
 * List of every object type: those defined with K_OBJ_TYPE_DEFINE() are added
 * at boot, those initialized with z_obj_type_init() as they are initialized.
 * Tools may use it as an entry point to identify all object types and the
 * object cores registered with them.
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

/**
 * @brief Contiguous range of permanent objects of one type
 *
 * Objects in the range are enumerated by walking it, so they are never
 * registered at run time. The range holds either the objects themselves or,
 * when @a indirect is set, pointers to them.
 */
struct k_obj_range {
	const void *start;   /**< First element */
	const void *end;     /**< One past the last element */
	size_t      stride;  /**< Element size */
	bool        indirect; /**< Elements are pointers to the objects */
};

/** Object type structure */
struct k_obj_type {
	sys_snode_t    node;   /**< Node within list of object types */
	uint32_t       id;     /**< Unique type ID */
	size_t         obj_core_offset;  /**< Offset to obj_core field */
	/** Permanent objects of this type, walked in place */
	struct k_obj_range statics;
	/** Registrations refused because the registry was full */
	uint32_t       dropped;
	/** Registrations refused because the object lives in stack storage */
	uint32_t       skipped;
#ifdef CONFIG_OBJ_CORE_STATS
	/** Pointer to object core statistics descriptor */
	struct k_obj_core_stats_desc *stats_desc;
	/** Offset of the stats buffer within each permanent object, if any */
	size_t         stats_offset;
	/** Size of the stats buffer within each permanent object (0 = none) */
	size_t         stats_size;
#endif /* CONFIG_OBJ_CORE_STATS */
};

/**
 * Object core structure
 *
 * The @a type pointer doubles as the registry's validity tag: an object whose
 * storage has been reused no longer carries the type it was registered with.
 */
struct k_obj_core {
	struct k_obj_type *type;   /**< Object type to which object belongs */
#ifdef CONFIG_OBJ_CORE_STATS
	void  *stats;              /**< Pointer to kernel object's stats */
#endif /* CONFIG_OBJ_CORE_STATS */
};

/**
 * @cond INTERNAL_HIDDEN
 */

#ifdef CONFIG_OBJ_CORE_STATS
#define Z_OBJ_CORE_STATS_INIT(_stats, _soff, _ssz) \
	.stats_desc = (_stats), .stats_offset = (_soff), .stats_size = (_ssz),
#else
#define Z_OBJ_CORE_STATS_INIT(_stats, _soff, _ssz)
#endif /* CONFIG_OBJ_CORE_STATS */

#define Z_K_OBJ_TYPE_DEFINE(_type_var, _start, _end, _stride, _off, _id,       \
			    _stats, _soff, _ssz)                               \
	STRUCT_SECTION_ITERABLE(k_obj_type, _type_var) = {                     \
		.id = (_id),                                                   \
		.obj_core_offset = (_off),                                     \
		.statics = {                                                   \
			.start = (_start),                                     \
			.end = (_end),                                         \
			.stride = (_stride),                                   \
			.indirect = false,                                     \
		},                                                             \
		Z_OBJ_CORE_STATS_INIT(_stats, _soff, _ssz)                     \
	}

#define Z_K_OBJ_TYPE_DEFINE_STRUCT(_type_var, _struct, _id, _stats, _soff, _ssz) \
	STRUCT_SECTION_START_EXTERN(_struct);                                  \
	STRUCT_SECTION_END_EXTERN(_struct);                                    \
	Z_K_OBJ_TYPE_DEFINE(_type_var, STRUCT_SECTION_START(_struct),          \
			    STRUCT_SECTION_END(_struct), sizeof(struct _struct), \
			    offsetof(struct _struct, obj_core), _id, _stats,    \
			    _soff, _ssz)

/**
 * @brief Define an object type
 *
 * Defines the object type @a _type_var, initialized at build time, whose
 * permanent objects are the statically defined instances of @a _struct.
 * The kernel initializes the object cores of those instances at boot.
 *
 * @param _type_var Name of the object type (struct k_obj_type) to define
 * @param _struct   Object struct type (e.g. k_sem) with an obj_core member
 * @param _id       Unique type ID (e.g. K_OBJ_TYPE_SEM_ID)
 * @param _stats    Pointer to a k_obj_core_stats_desc, or NULL
 */
#define K_OBJ_TYPE_DEFINE(_type_var, _struct, _id, _stats)                     \
	Z_K_OBJ_TYPE_DEFINE_STRUCT(_type_var, _struct, _id, _stats, 0, 0)

/**
 * @brief Define an object type that also gathers per-object statistics
 *
 * Like K_OBJ_TYPE_DEFINE(), but additionally registers each statically
 * defined object's embedded statistics buffer at boot.
 *
 * @param _type_var Name of the object type (struct k_obj_type) to define
 * @param _struct   Object struct type with obj_core and stats members
 * @param _id       Unique type ID
 * @param _stats    Pointer to a k_obj_core_stats_desc
 * @param _member   Name of the per-object stats buffer member within @a _struct
 */
#define K_OBJ_TYPE_DEFINE_STATS(_type_var, _struct, _id, _stats, _member)      \
	Z_K_OBJ_TYPE_DEFINE_STRUCT(_type_var, _struct, _id, _stats,            \
				   offsetof(struct _struct, _member),          \
				   sizeof(((struct _struct *)0)->_member))

/**
 * @brief Define an object type without permanent objects
 *
 * Like K_OBJ_TYPE_DEFINE(), but for object types whose objects are all
 * registered at run time (e.g. threads, which register their own object core
 * as they are created).
 *
 * @param _type_var Name of the object type (struct k_obj_type) to define
 * @param _struct   Object struct type with an obj_core member
 * @param _id       Unique type ID
 * @param _stats    Pointer to a k_obj_core_stats_desc, or NULL
 */
#define K_OBJ_TYPE_DEFINE_TYPE_ONLY(_type_var, _struct, _id, _stats)           \
	Z_K_OBJ_TYPE_DEFINE(_type_var, NULL, NULL, sizeof(struct _struct),     \
			    offsetof(struct _struct, obj_core), _id, _stats, 0, 0)

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Initialize an object type at run time
 *
 * Initializes an object type that is not defined with K_OBJ_TYPE_DEFINE()
 * and links it into the object core framework.
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
 * @brief Register the permanent objects of an object type
 *
 * Objects in [@a start, @a end) are enumerated by walking the range and are
 * not registered individually. The storage must outlive the object type.
 *
 * @param type Pointer to the object type
 * @param start First element of the range
 * @param end One past the last element of the range
 * @param stride Element size
 * @param indirect True if the elements are pointers to the objects
 */
void z_obj_type_init_range(struct k_obj_type *type, const void *start,
			   const void *end, size_t stride, bool indirect);

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
 * @brief Walk the object cores of an object type
 *
 * This function takes a global spinlock and invokes the callback on every
 * object core of the object type while holding that lock: first the permanent
 * objects of the type, then the objects registered at run time. A registered
 * object whose storage has been reused is removed from the registry instead
 * of being reported. Although the lock ensures that the registry is not
 * modified, one can expect a significant penalty in terms of performance and
 * latency.
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
			   int (*func)(struct k_obj_core *obj_core, void *data),
				  void *data);

/**
 * @brief Walk the object cores of an object type
 *
 * This function is similar to k_obj_type_walk_locked() except that it walks
 * the registry without obtaining the global spinlock. No synchronization is
 * provided here: objects registered or unregistered during the walk may or
 * may not be reported, and a registered object whose storage has been reused
 * is skipped rather than removed.
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
			     int (*func)(struct k_obj_core *obj_core, void *data),
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
 * @brief Register the kernel object with its object type
 *
 * A kernel object can be optionally registered so that it is reported by
 * k_obj_type_walk_locked() and k_obj_type_walk_unlocked(). It must have been
 * initialized with k_obj_core_init() first. Registering an object that is
 * already registered, or that belongs to the permanent range of its type, has
 * no effect. An object located in the current thread's stack or in the
 * interrupt stack is not registered and the type's @a skipped count is
 * incremented. The registry holds no reference inside the object, so an
 * object may be discarded without unregistering it; the stale entry is
 * dropped when its storage is reused or when the registry is full. When the
 * registry is full the object is not registered and the type's @a dropped
 * count is incremented.
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
 * @brief Unregister the kernel object from its object type
 *
 * Unregistering is optional and removes the object from the walks
 * immediately, instead of when its storage is reused. Unregistering an object
 * that is not registered has no effect.
 *
 * @param obj_core Pointer to the kernel object
 */
void k_obj_core_unlink(struct k_obj_core *obj_core);

/**
 * @brief Unregister every kernel object located in a memory range
 *
 * Intended for memory allocators: removes the registry entries of all objects
 * whose object core lies in [@a addr, @a addr + @a len) as the memory is
 * released.
 *
 * @param addr Start of the released memory
 * @param len Size of the released memory in bytes
 */
void k_obj_core_evict_range(const void *addr, size_t len);

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
 * @brief Resume gathering the stats associated with the kernel object
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
