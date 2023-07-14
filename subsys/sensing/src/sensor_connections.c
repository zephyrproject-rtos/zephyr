
#include <zephyr/drivers/sensor.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensor.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/dsp/types.h>
#include <zephyr/logging/log.h>

#include "sensing/internal/sensing.h"

LOG_MODULE_REGISTER(sensing_connect, CONFIG_SENSING_LOG_LEVEL);

#define __HANDLE_TO_CONNECTION(name, handle)                                                       \
	struct sensing_connection *name = handle;                                                  \
	__ASSERT_NO_MSG((uintptr_t)name >= (uintptr_t)STRUCT_SECTION_START(sensing_connection));   \
	__ASSERT_NO_MSG((uintptr_t)name < (uintptr_t)STRUCT_SECTION_END(sensing_connection))

SENSING_DMEM SYS_MUTEX_DEFINE(connection_lock);

SENSING_DMEM static uint32_t bitarray_bundles[4] = {0};
SENSING_DMEM static sys_bitarray_t bitarray = {
	.num_bits = ARRAY_SIZE(bitarray_bundles) * 32,
	.num_bundles = ARRAY_SIZE(bitarray_bundles),
	.bundles = bitarray_bundles,
};

SENSING_DMEM struct sensing_connection_pool __sensing_connection_pool = {
	.bitarray = &bitarray,
	.lock = &connection_lock,
};

STRUCT_SECTION_ITERABLE_ARRAY(sensing_connection, dynamic_connections,
			      CONFIG_SENSING_MAX_DYNAMIC_CONNECTIONS);

#define __lock   sys_mutex_lock(__sensing_connection_pool.lock, K_FOREVER)
#define __unlock sys_mutex_unlock(__sensing_connection_pool.lock)

Z_RTIO_BLOCK_POOL_DEFINE_SCOPED((), sensing_rtio_block_pool, CONFIG_SENSING_RTIO_BLOCK_COUNT,
				CONFIG_SENSING_RTIO_BLOCK_SIZE, 4);

RTIO_DEFINE_WITH_EXT_MEMPOOL(sensing_rtio_ctx, 32, 32, sensing_rtio_block_pool);

int sensing_open_sensor(const struct sensing_sensor_info *info,
			const struct sensing_callback_list *cb_list,
			sensing_sensor_handle_t *handle, void *userdata)
{
	struct sensing_connection *connection;
	size_t offset;
	int rc;

	__ASSERT_NO_MSG(info != NULL);
	__ASSERT_NO_MSG(cb_list != NULL);
	__ASSERT_NO_MSG(handle != NULL);

	__lock;
	bitarray.num_bits = __CONNECTION_POOL_COUNT; // TODO this should only happen once
	rc = sys_bitarray_alloc(__sensing_connection_pool.bitarray, 1, &offset);
	if (rc != 0) {
		__unlock;
		return rc;
	}

	LOG_DBG("Got offset %d/%d", (int)offset, (int)__CONNECTION_POOL_COUNT);
	connection = &STRUCT_SECTION_START(sensing_connection)[offset];

	LOG_DBG("Connection opened @ %p (size=%d) for info @ %p", connection,
		(int)sizeof(struct sensing_connection), info);
	memset(connection, 0, sizeof(struct sensing_connection));
	connection->info = info;
	connection->cb_list = cb_list;
	connection->userdata = userdata;
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
			connection - STRUCT_SECTION_START(sensing_connection));
		rc = sys_bitarray_free(__sensing_connection_pool.bitarray, 1,
				       connection - STRUCT_SECTION_START(sensing_connection));
		if (rc == 0) {
			__sensing_arbitrate();
		} else {
			LOG_WRN("Failed to release connection");
		}
	}
	__unlock;
	return rc;
}

int sensing_set_attributes(sensing_sensor_handle_t handle, enum sensing_sensor_mode mode,
			   struct sensing_sensor_attribute *attributes, size_t count)
{
	__HANDLE_TO_CONNECTION(connection, handle);
	int rc;

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
	connection->mode = mode;
	__sensing_arbitrate();

	switch (mode) {
	case SENSING_SENSOR_MODE_ONE_SHOT:
		LOG_DBG("Starting one-shot read");
		rc = sensor_read(connection->info->iodev, &sensing_rtio_ctx,
				 (void *)connection->info);
		break;
	case SENSING_SENSOR_MODE_DONE:
		rc = 0;
		break;
	default:
		rc = -EINVAL;
		break;
	}
	__unlock;
	return rc;
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
	memset(STRUCT_SECTION_START(sensing_connection), 0,
	       (uintptr_t)STRUCT_SECTION_END(sensing_connection) -
		       (uintptr_t)STRUCT_SECTION_START(sensing_connection));
	__sensing_arbitrate();
	__unlock;
}
