//
// Created by peress on 27/06/23.
//

#ifndef ZEPHYR_SUBSYS_SENSING_INCLUDE_SENSING_INTERNAL_SENSING_H
#define ZEPHYR_SUBSYS_SENSING_INCLUDE_SENSING_INTERNAL_SENSING_H

#include <zephyr/drivers/sensor.h>
#include <zephyr/sensing/sensor.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/dsp/types.h>
#include <zephyr/sys/mutex.h>

#define __CONNECTION_POOL_COUNT                                                                    \
	(STRUCT_SECTION_END(sensing_connection) - STRUCT_SECTION_START(sensing_connection))

#define __SENSING_POOL_MASK_BUNDLE_COUNT                                                           \
	(DIV_ROUND_UP(DIV_ROUND_UP(__CONNECTION_POOL_COUNT, 8), sizeof(uint32_t)))

extern struct sensing_connection_pool {
	sys_bitarray_t *bitarray;
	struct sys_mutex *lock;
} __sensing_connection_pool;

BUILD_ASSERT(SENSOR_ATTR_COMMON_COUNT <= 32, "Too many sensor attributes");

extern struct rtio sensing_rtio_ctx;

static inline bool __sensing_is_connected(const struct sensing_sensor_info *info,
					  const struct sensing_connection *connection)
{
	int is_set;
	int connection_index = connection - STRUCT_SECTION_START(sensing_connection);
	int rc = sys_bitarray_test_bit(__sensing_connection_pool.bitarray, connection_index,
				       &is_set);

	return rc == 0 && is_set != 0 && (info == NULL || connection->info == info);
}

void __sensing_arbitrate();

#endif // ZEPHYR_SUBSYS_SENSING_INCLUDE_SENSING_INTERNAL_SENSING_H
