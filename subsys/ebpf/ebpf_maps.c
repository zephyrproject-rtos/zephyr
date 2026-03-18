/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/iterable_sections.h>
#include <string.h>
#include <stdbool.h>

#include <zephyr/ebpf/ebpf_map.h>

LOG_MODULE_DECLARE(ebpf_vm, CONFIG_EBPF_LOG_LEVEL);

#ifdef CONFIG_EBPF_MAP_PER_CPU_ARRAY
static uint32_t ebpf_current_cpu_id(void)
{
#ifdef CONFIG_SMP
	return arch_curr_cpu()->id;
#else
	return 0;
#endif
}
#endif /* CONFIG_EBPF_MAP_PER_CPU_ARRAY */

/* ==================== Array Map ==================== */
static void *ebpf_array_lookup(struct ebpf_map *map, const void *key)
{
	uint32_t idx = *(const uint32_t *)key;

	if (idx >= map->max_entries) {
		return NULL;
	}

	return (uint8_t *)map->data + (idx * map->value_size);
}

static int ebpf_array_update(struct ebpf_map *map, const void *key,
			     const void *value, uint64_t flags)
{
	uint32_t idx = *(const uint32_t *)key;

	if (idx >= map->max_entries) {
		return -ENOENT;
	}

	memcpy((uint8_t *)map->data + (idx * map->value_size),
	       value, map->value_size);
	return 0;
}

static int ebpf_array_delete(struct ebpf_map *map, const void *key)
{
	return -EINVAL; /* Arrays do not support delete */
}

static const struct ebpf_map_ops array_ops = {
	.lookup_elem = ebpf_array_lookup,
	.update_elem = ebpf_array_update,
	.delete_elem = ebpf_array_delete,
};

/* ==================== Per-CPU Array Map ==================== */
#ifdef CONFIG_EBPF_MAP_PER_CPU_ARRAY
static uint8_t *ebpf_percpu_array_value_ptr(struct ebpf_map *map, uint32_t idx,
					    uint32_t cpu_id)
{
	uint32_t slot = (idx * CONFIG_MP_MAX_NUM_CPUS) + cpu_id;

	return (uint8_t *)map->data + (slot * map->value_size);
}

static void *ebpf_percpu_array_lookup(struct ebpf_map *map, const void *key)
{
	uint32_t idx = *(const uint32_t *)key;
	uint32_t cpu_id = ebpf_current_cpu_id();

	if (idx >= map->max_entries || cpu_id >= CONFIG_MP_MAX_NUM_CPUS) {
		return NULL;
	}

	return ebpf_percpu_array_value_ptr(map, idx, cpu_id);
}

static int ebpf_percpu_array_update(struct ebpf_map *map, const void *key,
				    const void *value, uint64_t flags)
{
	uint32_t idx = *(const uint32_t *)key;
	uint32_t cpu_id = ebpf_current_cpu_id();

	ARG_UNUSED(flags);

	if (idx >= map->max_entries || cpu_id >= CONFIG_MP_MAX_NUM_CPUS) {
		return -ENOENT;
	}

	memcpy(ebpf_percpu_array_value_ptr(map, idx, cpu_id), value, map->value_size);
	return 0;
}

static int ebpf_percpu_array_delete(struct ebpf_map *map, const void *key)
{
	ARG_UNUSED(map);
	ARG_UNUSED(key);
	return -EINVAL;
}

static const struct ebpf_map_ops percpu_array_ops = {
	.lookup_elem = ebpf_percpu_array_lookup,
	.update_elem = ebpf_percpu_array_update,
	.delete_elem = ebpf_percpu_array_delete,
};
#endif /* CONFIG_EBPF_MAP_PER_CPU_ARRAY */

/* ==================== Hash Map ==================== */
#ifdef CONFIG_EBPF_MAP_HASH
static uint32_t ebpf_fnv1a_hash(const void *key, uint32_t key_size)
{
	const uint8_t *data = (const uint8_t *)key;
	uint32_t hash = 0x811c9dc5U;

	for (uint32_t i = 0; i < key_size; i++) {
		hash ^= data[i];
		hash *= 0x01000193U;
	}

	return hash;
}

static inline uint32_t ebpf_hash_entry_size(const struct ebpf_map *map)
{
	return sizeof(struct ebpf_hash_entry) + map->key_size + map->value_size;
}

static inline struct ebpf_hash_entry *ebpf_hash_entry_at(const struct ebpf_map *map,
						 uint32_t slot)
{
	return (struct ebpf_hash_entry *)((uint8_t *)map->data +
					  slot * ebpf_hash_entry_size(map));
}

static inline void *ebpf_hash_entry_key(const struct ebpf_map *map,
					struct ebpf_hash_entry *entry)
{
	return (uint8_t *)entry + sizeof(struct ebpf_hash_entry);
}

static inline void *ebpf_hash_entry_value(const struct ebpf_map *map,
					  struct ebpf_hash_entry *entry)
{
	return (uint8_t *)entry + sizeof(struct ebpf_hash_entry) + map->key_size;
}

static void *ebpf_hash_lookup(struct ebpf_map *map, const void *key)
{
	uint32_t hash = ebpf_fnv1a_hash(key, map->key_size);
	uint32_t slot = hash % map->max_entries;

	for (uint32_t i = 0; i < map->max_entries; i++) {
		uint32_t idx = (slot + i) % map->max_entries;
		struct ebpf_hash_entry *entry = ebpf_hash_entry_at(map, idx);

		if (entry->occupied == 0) {
			return NULL; /* Empty slot = not found */
		}
		if (entry->occupied == 1 &&
		    memcmp(ebpf_hash_entry_key(map, entry), key, map->key_size) == 0) {
			return ebpf_hash_entry_value(map, entry);
		}
		/* occupied == 2 (tombstone) = skip and continue */
	}

	return NULL;
}

static int ebpf_hash_update(struct ebpf_map *map, const void *key,
			    const void *value, uint64_t flags)
{
	uint32_t hash = ebpf_fnv1a_hash(key, map->key_size);
	uint32_t slot = hash % map->max_entries;
	int32_t first_tombstone = -1;

	for (uint32_t i = 0; i < map->max_entries; i++) {
		uint32_t idx = (slot + i) % map->max_entries;
		struct ebpf_hash_entry *entry = ebpf_hash_entry_at(map, idx);

		if (entry->occupied == 0) {
			/* Use tombstone if we found one, otherwise use this empty slot */
			if (first_tombstone >= 0) {
				idx = (uint32_t)first_tombstone;
				entry = ebpf_hash_entry_at(map, idx);
			}
			entry->occupied = 1;
			memcpy(ebpf_hash_entry_key(map, entry), key, map->key_size);
			memcpy(ebpf_hash_entry_value(map, entry), value, map->value_size);
			map->count++;
			return 0;
		}
		if (entry->occupied == 2 && first_tombstone < 0) {
			first_tombstone = (int32_t)idx;
		}
		if (entry->occupied == 1 &&
		    memcmp(ebpf_hash_entry_key(map, entry), key, map->key_size) == 0) {
			/* Key exists: overwrite value */
			memcpy(ebpf_hash_entry_value(map, entry), value, map->value_size);
			return 0;
		}
	}

	/* Table full, try tombstone */
	if (first_tombstone >= 0) {
		struct ebpf_hash_entry *entry = ebpf_hash_entry_at(map,
							      (uint32_t)first_tombstone);
		entry->occupied = 1;
		memcpy(ebpf_hash_entry_key(map, entry), key, map->key_size);
		memcpy(ebpf_hash_entry_value(map, entry), value, map->value_size);
		map->count++;
		return 0;
	}

	return -ENOMEM;
}

static int ebpf_hash_delete(struct ebpf_map *map, const void *key)
{
	uint32_t hash = ebpf_fnv1a_hash(key, map->key_size);
	uint32_t slot = hash % map->max_entries;

	for (uint32_t i = 0; i < map->max_entries; i++) {
		uint32_t idx = (slot + i) % map->max_entries;
		struct ebpf_hash_entry *entry = ebpf_hash_entry_at(map, idx);

		if (entry->occupied == 0) {
			return -ENOENT;
		}
		if (entry->occupied == 1 &&
		    memcmp(ebpf_hash_entry_key(map, entry), key, map->key_size) == 0) {
			entry->occupied = 2; /* Mark as tombstone */
			map->count--;
			return 0;
		}
	}

	return -ENOENT;
}

static const struct ebpf_map_ops hash_ops = {
	.lookup_elem = ebpf_hash_lookup,
	.update_elem = ebpf_hash_update,
	.delete_elem = ebpf_hash_delete,
};
#endif /* CONFIG_EBPF_MAP_HASH */

/* ==================== Ring Buffer Map ==================== */
#ifdef CONFIG_EBPF_MAP_RINGBUF
#include <zephyr/sys/ring_buffer.h>

/**
 * Write data into the ring buffer; caller must hold map->lock.
 */
static int ebpf_ringbuf_write_locked(struct ebpf_map *map, const void *data, uint32_t size)
{
	struct ebpf_ringbuf_data *rb_data = (struct ebpf_ringbuf_data *)map->data;
	struct ring_buf *rb = &rb_data->rb;
	uint32_t header = size;
	uint32_t total = sizeof(header) + size;

	if (ring_buf_space_get(rb) < total) {
		return -ENOMEM;
	}

	ring_buf_put(rb, (uint8_t *)&header, sizeof(header));
	ring_buf_put(rb, data, size);

	return 0;
}

int ebpf_ringbuf_output(struct ebpf_map *map, const void *data,
			uint32_t size, uint64_t flags)
{
	ARG_UNUSED(flags);

	if (map->type != EBPF_MAP_TYPE_RINGBUF) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&map->lock);
	int ret = ebpf_ringbuf_write_locked(map, data, size);

	k_spin_unlock(&map->lock, key);

	return ret;
}

int ebpf_ringbuf_read(struct ebpf_map *map, void *data,
		      uint32_t capacity, uint32_t *size_out)
{
	struct ebpf_ringbuf_data *rb_data;
	struct ring_buf *rb;
	uint32_t payload_size;
	uint32_t got;
	k_spinlock_key_t key;

	if (map == NULL || map->type != EBPF_MAP_TYPE_RINGBUF ||
	    map->ops == NULL || size_out == NULL) {
		return -EINVAL;
	}

	rb_data = (struct ebpf_ringbuf_data *)map->data;
	rb = &rb_data->rb;

	key = k_spin_lock(&map->lock);
	if (ring_buf_size_get(rb) < sizeof(payload_size)) {
		k_spin_unlock(&map->lock, key);
		return -EAGAIN;
	}

	got = ring_buf_get(rb, (uint8_t *)&payload_size, sizeof(payload_size));
	if (got != sizeof(payload_size)) {
		k_spin_unlock(&map->lock, key);
		return -EIO;
	}

	if (payload_size > ring_buf_size_get(rb)) {
		ring_buf_reset(rb);
		k_spin_unlock(&map->lock, key);
		return -EIO;
	}

	if (payload_size > capacity) {
		ring_buf_get(rb, NULL, payload_size);
		k_spin_unlock(&map->lock, key);
		return -EMSGSIZE;
	}

	got = ring_buf_get(rb, data, payload_size);
	k_spin_unlock(&map->lock, key);

	if (got != payload_size) {
		return -EIO;
	}

	*size_out = payload_size;
	return 0;
}

static void *ebpf_ringbuf_lookup(struct ebpf_map *map, const void *key)
{
	ARG_UNUSED(key);
	return NULL; /* Ring buffers don't support lookup */
}

static int ebpf_ringbuf_update(struct ebpf_map *map, const void *key,
			       const void *value, uint64_t flags)
{
	ARG_UNUSED(key);
	ARG_UNUSED(flags);
	/* Called from ebpf_map_update_elem which already holds map->lock. */
	return ebpf_ringbuf_write_locked(map, value, map->value_size);
}

static int ebpf_ringbuf_delete(struct ebpf_map *map, const void *key)
{
	ARG_UNUSED(key);
	return -EINVAL; /* Ring buffers don't support delete */
}

static const struct ebpf_map_ops ringbuf_ops = {
	.lookup_elem = ebpf_ringbuf_lookup,
	.update_elem = ebpf_ringbuf_update,
	.delete_elem = ebpf_ringbuf_delete,
};
#endif /* CONFIG_EBPF_MAP_RINGBUF */

/* ==================== Generic Map API ==================== */
void *ebpf_map_lookup_elem(struct ebpf_map *map, const void *key)
{
	if (map == NULL || map->ops == NULL || key == NULL) {
		return NULL;
	}

	k_spinlock_key_t k = k_spin_lock(&map->lock);
	void *value = map->ops->lookup_elem(map, key);

	k_spin_unlock(&map->lock, k);

	return value;
}

int ebpf_map_update_elem(struct ebpf_map *map, const void *key,
			 const void *value, uint64_t flags)
{
	if (map == NULL || map->ops == NULL || key == NULL || value == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t k = k_spin_lock(&map->lock);
	int ret = map->ops->update_elem(map, key, value, flags);

	k_spin_unlock(&map->lock, k);

	return ret;
}

int ebpf_map_delete_elem(struct ebpf_map *map, const void *key)
{
	if (map == NULL || map->ops == NULL || key == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t k = k_spin_lock(&map->lock);
	int ret = map->ops->delete_elem(map, key);

	k_spin_unlock(&map->lock, k);

	return ret;
}

struct ebpf_map *ebpf_map_find(const char *name)
{
	STRUCT_SECTION_FOREACH(ebpf_map, map) {
		if (strcmp(map->name, name) == 0) {
			return map;
		}
	}

	return NULL;
}

struct ebpf_map *ebpf_map_from_id(uint32_t id)
{
	STRUCT_SECTION_FOREACH(ebpf_map, map) {
		if (map->id == id) {
			return map;
		}
	}

	return NULL;
}

/* ==================== Map Initialization ==================== */
int ebpf_maps_init(void)
{
	uint32_t next_id = 1;

	STRUCT_SECTION_FOREACH(ebpf_map, map) {
		map->id = next_id++;

		switch (map->type) {
		case EBPF_MAP_TYPE_ARRAY:
			map->ops = &array_ops;
			memset(map->data, 0, map->value_size * map->max_entries);
			break;
#ifdef CONFIG_EBPF_MAP_PER_CPU_ARRAY
		case EBPF_MAP_TYPE_PER_CPU_ARRAY:
			map->ops = &percpu_array_ops;
			memset(map->data, 0, map->data_size);
			break;
#endif
#ifdef CONFIG_EBPF_MAP_HASH
		case EBPF_MAP_TYPE_HASH:
			map->ops = &hash_ops;
			memset(map->data, 0, ebpf_hash_entry_size(map) * map->max_entries);
			break;
#endif
#ifdef CONFIG_EBPF_MAP_RINGBUF
		case EBPF_MAP_TYPE_RINGBUF:
			map->ops = &ringbuf_ops;
			memset(map->data, 0, map->data_size);
			ring_buf_init(&((struct ebpf_ringbuf_data *)map->data)->rb,
				      map->max_entries,
				      ((struct ebpf_ringbuf_data *)map->data)->buffer);
			break;
#endif
		default:
			LOG_ERR("Unknown map type %d for '%s'", map->type, map->name);
			break;
		}
	}

	return 0;
}
