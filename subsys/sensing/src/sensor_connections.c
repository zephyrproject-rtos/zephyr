
#include <zephyr/drivers/sensor.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/dsp/types.h>
#include <zephyr/logging/log.h>

#include "sensing/internal/sensing.h"

LOG_MODULE_REGISTER(sensing_connect, CONFIG_SENSING_LOG_LEVEL);

#define __HANDLE_TO_CONNECTION(name, handle)                                                       \
	struct sensing_connection *name = handle;                                                  \
	__ASSERT_NO_MSG((uintptr_t)name >= (uintptr_t)__sensing_connection_pool.pool);             \
	__ASSERT_NO_MSG((uintptr_t)name < (uintptr_t)__sensing_connection_pool.pool +              \
						  sizeof(__sensing_connection_pool.pool));

SENSING_DMEM SYS_MUTEX_DEFINE(connection_lock);

SENSING_DMEM static uint32_t bitarray_bundles[__SENSING_POOL_MASK_BUNDLE_COUNT] = {0};
SENSING_DMEM static sys_bitarray_t bitarray = {
	.num_bits = CONFIG_SENSING_MAX_CONNECTIONS,
	.num_bundles = __SENSING_POOL_MASK_BUNDLE_COUNT,
	.bundles = bitarray_bundles,
};

SENSING_DMEM struct sensing_connection_pool __sensing_connection_pool = {
	.bitarray = &bitarray,
	.lock = &connection_lock,
};

#define __lock   sys_mutex_lock(__sensing_connection_pool.lock, K_FOREVER)
#define __unlock sys_mutex_unlock(__sensing_connection_pool.lock)

int sensing_open_sensor(const struct sensing_sensor_info *info,
			const struct sensing_callback_list *cb_list,
			sensing_sensor_handle_t *handle)
{
	struct sensing_connection *connection;
	size_t offset;
	int rc;

	__ASSERT_NO_MSG(info != NULL);
	__ASSERT_NO_MSG(cb_list != NULL);
	__ASSERT_NO_MSG(handle != NULL);

	__lock;
	rc = sys_bitarray_alloc(__sensing_connection_pool.bitarray, 1, &offset);
	if (rc != 0) {
		__unlock;
		return rc;
	}

	connection = &__sensing_connection_pool.pool[offset];

	LOG_DBG("Connection opened @ %p (size=%d) for info @ %p", connection,
		(int)sizeof(struct sensing_connection), info);
	memset(connection, 0, sizeof(struct sensing_connection));
	connection->info = info;
	connection->cb_list = cb_list;
	*handle = connection;
	__unlock;
	return 0;
}

int sensing_close_sensor(sensing_sensor_handle_t handle)
{
	__HANDLE_TO_CONNECTION(connection, handle);
	int rc = -EINVAL;

	__lock;
	if (__sensing_is_connected(NULL, connection)) {
		LOG_DBG("Releasing connection at %p/%d", handle,
			connection - __sensing_connection_pool.pool);
		rc = sys_bitarray_free(__sensing_connection_pool.bitarray, 1,
				       connection - __sensing_connection_pool.pool);
		if (rc == 0) {
			__sensing_arbitrate();
		} else {
			LOG_WRN("Failed to release connection");
		}
	}
	__unlock;
	return rc;
}

int sensing_set_attributes(sensing_sensor_handle_t handle,
			   struct sensing_sensor_attribute *attributes, size_t count)
{
	__HANDLE_TO_CONNECTION(connection, handle);

	__lock;
	for (size_t i = 0; i < count; ++i) {
		const int8_t shift = 16 - attributes[i].shift;
		const q31_t value = shift >= 0 ? (attributes[i].value >> shift)
					       : (attributes[i].value << -shift);

		__ASSERT_NO_MSG(attributes[i].attribute < 32);
		connection->attributes[attributes[i].attribute] = value;
		connection->attribute_mask |= BIT(attributes[i].attribute);
		LOG_DBG("Updated attribute (%d) to 0x%08x->0x%08x", attributes[i].attribute,
			attributes[i].value, value);
	}
	__sensing_arbitrate();
	__unlock;
	return 0;
}

const struct sensing_sensor_info *sensing_get_sensor_info(sensing_sensor_handle_t handle)
{
	__HANDLE_TO_CONNECTION(connection, handle);

	return connection->info;
}

void sensing_reset_connections(void)
{
	__lock;
	sys_bitarray_clear_region(__sensing_connection_pool.bitarray,
				  __sensing_connection_pool.bitarray->num_bits, 0);
	memset(__sensing_connection_pool.pool, 0, sizeof(__sensing_connection_pool.pool));
	__sensing_arbitrate();
	__unlock;
}
