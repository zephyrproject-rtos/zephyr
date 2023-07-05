//
// Created by peress on 27/06/23.
//

#ifndef ZEPHYR_SUBSYS_SENSING_INCLUDE_SENSING_INTERNAL_SENSING_H
#define ZEPHYR_SUBSYS_SENSING_INCLUDE_SENSING_INTERNAL_SENSING_H

#include <zephyr/drivers/sensor.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/dsp/types.h>
#include <zephyr/sys/mutex.h>

#define __SENSING_POOL_MASK_BUNDLE_COUNT                                                           \
	(DIV_ROUND_UP(DIV_ROUND_UP(CONFIG_SENSING_MAX_CONNECTIONS, 8), sizeof(uint32_t)))

struct sensing_connection {
	const struct sensing_sensor_info *info;
	const struct sensing_callback_list *cb_list;
	enum sensing_sensor_mode mode;
	q31_t attributes[SENSOR_ATTR_COMMON_COUNT];
	uint32_t attribute_mask;
} __packed __aligned(4);

extern struct sensing_connection_pool {
	struct sensing_connection pool[CONFIG_SENSING_MAX_CONNECTIONS];
	sys_bitarray_t *bitarray;
	struct sys_mutex *lock;
} __sensing_connection_pool;

BUILD_ASSERT(SENSOR_ATTR_COMMON_COUNT <= 32, "Too many sensor attributes");

extern struct rtio sensing_rtio_ctx;

static inline bool __sensing_is_connected(const struct sensing_sensor_info *info,
					  const struct sensing_connection *connection)
{
	int is_set;
	int connection_index = connection - __sensing_connection_pool.pool;
	int rc = sys_bitarray_test_bit(__sensing_connection_pool.bitarray, connection_index,
				       &is_set);

	return rc == 0 && is_set != 0 && (info == NULL || connection->info == info);
}

void __sensing_arbitrate();

#endif // ZEPHYR_SUBSYS_SENSING_INCLUDE_SENSING_INTERNAL_SENSING_H
