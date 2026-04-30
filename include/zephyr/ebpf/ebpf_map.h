/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief eBPF map descriptor and API.
 * @ingroup ebpf
 */

#ifndef ZEPHYR_INCLUDE_EBPF_MAP_H_
#define ZEPHYR_INCLUDE_EBPF_MAP_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Supported eBPF map types. */
enum ebpf_map_type {
	EBPF_MAP_TYPE_ARRAY         = 0, /**< Fixed-size array indexed by integer key. */
	EBPF_MAP_TYPE_HASH          = 1, /**< Hash table keyed by arbitrary byte sequences. */
	EBPF_MAP_TYPE_RINGBUF       = 2, /**< Ring buffer used for variable-size event records. */
	EBPF_MAP_TYPE_PER_CPU_ARRAY = 3, /**< Per-CPU array with one value instance per CPU. */
	EBPF_MAP_TYPE_MAX                /**< Number of supported map types. */
};

struct ebpf_map;

/** @cond INTERNAL_HIDDEN */

/** Map operations vtable (different implementations per map type) */
struct ebpf_map_ops {
	void *(*lookup_elem)(struct ebpf_map *map, const void *key);
	int   (*update_elem)(struct ebpf_map *map, const void *key,
			     const void *value, uint64_t flags);
	int   (*delete_elem)(struct ebpf_map *map, const void *key);
};

/** Hash entry header for hash map implementation */
struct ebpf_hash_entry {
	uint8_t occupied; /* 0=empty, 1=occupied, 2=tombstone */
	/* followed by key_size bytes, then value_size bytes */
};

struct ebpf_ringbuf_data {
	struct ring_buf rb;
	uint8_t buffer[];
};

/** @endcond */

/**
 * @brief eBPF map descriptor.
 *
 * Placed in RAM iterable section so the data storage is mutable.
 */
struct ebpf_map {
	/** Human-readable name */
	const char *name;

	/** Map type */
	enum ebpf_map_type type;

	/** Key size in bytes */
	uint32_t key_size;

	/** Value size in bytes */
	uint32_t value_size;

	/** Maximum number of entries */
	uint32_t max_entries;

	/** @cond INTERNAL_HIDDEN */

	/** Runtime-assigned small integer handle */
	uint32_t id;

	/** Pointer to operations vtable (set during init based on type) */
	const struct ebpf_map_ops *ops;

	/** Pointer to backing data storage (statically allocated) */
	void *data;

	/** Size in bytes of the allocated backing storage */
	uint32_t data_size;

	/** Current number of entries (for hash maps) */
	uint32_t count;

	/** Spinlock for concurrent access */
	struct k_spinlock lock;

	/** @endcond */
};

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Compute required data size for a map.
 */
#define _EBPF_MAP_DATA_SIZE(_type, _ks, _vs, _max)				\
	((_type) == EBPF_MAP_TYPE_ARRAY ? ((_vs) * (_max)) :			\
	 (_type) == EBPF_MAP_TYPE_HASH ?					\
		(((_ks) + (_vs) + sizeof(struct ebpf_hash_entry)) * (_max)) :	\
	 (_type) == EBPF_MAP_TYPE_RINGBUF ?					\
		(sizeof(struct ebpf_ringbuf_data) + (_max)) :			\
	 (_type) == EBPF_MAP_TYPE_PER_CPU_ARRAY ?				\
		((_vs) * (_max) * CONFIG_MP_MAX_NUM_CPUS) :			\
	 ((_vs) * (_max)))

/** @endcond */

/**
 * @brief Define an eBPF map at compile time.
 *
 * This allocates static storage for the map data.
 *
 * @param _name        C identifier for this map
 * @param _type        Map type (enum ebpf_map_type)
 * @param _key_size    Key size in bytes
 * @param _value_size  Value size in bytes
 * @param _max_entries Maximum number of entries
 */
#define EBPF_MAP_DEFINE(_name, _type, _key_size, _value_size, _max_entries)		\
	static uint8_t _ebpf_map_data_##_name						\
		[_EBPF_MAP_DATA_SIZE(_type, _key_size, _value_size, _max_entries)];	\
											\
	STRUCT_SECTION_ITERABLE(ebpf_map, _name) = {					\
		.name        = STRINGIFY(_name),					\
		.id          = 0,							\
		.type        = (_type),							\
		.key_size    = (_key_size),						\
		.value_size  = (_value_size),						\
		.max_entries = (_max_entries),						\
		.ops         = NULL,							\
		.data        = _ebpf_map_data_##_name,					\
		.data_size   = sizeof(_ebpf_map_data_##_name),				\
		.count       = 0,							\
	}

/**
 * @brief Look up a value in a map.
 *
 * @param map Map to look up in
 * @param key Pointer to the key
 * @return Pointer to the value, or NULL if not found
 */
void *ebpf_map_lookup_elem(struct ebpf_map *map, const void *key);

/**
 * @brief Update a value in a map.
 *
 * @param map   Map to update
 * @param key   Pointer to the key
 * @param value Pointer to the new value
 * @param flags Update flags (0 = create or update)
 * @return 0 on success, negative errno on failure
 */
int ebpf_map_update_elem(struct ebpf_map *map, const void *key,
			 const void *value, uint64_t flags);

/**
 * @brief Delete an element from a map.
 *
 * @param map Map to delete from
 * @param key Pointer to the key
 * @return 0 on success, negative errno on failure
 */
int ebpf_map_delete_elem(struct ebpf_map *map, const void *key);

/**
 * @brief Write data to a ring buffer map.
 *
 * @param map  Ring buffer map
 * @param data Pointer to data to write
 * @param size Size of data in bytes
 * @param flags Flags (reserved)
 * @return 0 on success, -ENOMEM if buffer full
 */
int ebpf_ringbuf_output(struct ebpf_map *map, const void *data,
			uint32_t size, uint64_t flags);

/**
 * @brief Read one record from a ring buffer map.
 *
 * @param map Ring buffer map
 * @param data Destination buffer, or NULL to discard the record payload
 * @param capacity Capacity of @p data in bytes
 * @param size_out On success, receives the payload size
 * @return 0 on success, -EAGAIN if empty, -EMSGSIZE if buffer too small,
 *         or other negative errno on failure
 */
int ebpf_ringbuf_read(struct ebpf_map *map, void *data,
		      uint32_t capacity, uint32_t *size_out);

/**
 * @brief Find a map by name.
 *
 * @param name Name string
 * @return Pointer to map, or NULL if not found
 */
struct ebpf_map *ebpf_map_find(const char *name);

/**
 * @brief Find a map by runtime-assigned handle.
 *
 * @param id Map handle
 * @return Pointer to map, or NULL if not found
 */
struct ebpf_map *ebpf_map_from_id(uint32_t id);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_EBPF_MAP_H_ */
