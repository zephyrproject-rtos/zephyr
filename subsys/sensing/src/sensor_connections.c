
#include <zephyr/sensing/sensing.h>
#include <zephyr/sys/mem_blocks.h>

struct sensing_connection {
	const struct sensing_callback_list *cb_list;
} __packed __aligned(4);

static uint8_t __aligned(4)
	connection_pool[sizeof(struct sensing_connection) * CONFIG_SENSING_MAX_CONNECTIONS];
SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(connection_allocator, sizeof(struct sensing_connection),
				   CONFIG_SENSING_MAX_CONNECTIONS, connection_pool);

int sensing_open_sensor(const struct sensing_sensor_info *info,
			const struct sensing_callback_list *cb_list,
			sensing_sensor_handle_t *handle)
{
	struct sensing_connection *connection;
	int rc;

	__ASSERT_NO_MSG(info != NULL);
	__ASSERT_NO_MSG(cb_list != NULL);
	__ASSERT_NO_MSG(handle != NULL);

	rc = sys_mem_blocks_alloc(&connection_allocator, 1, (void**)&connection);

	if (rc != 0) {
		return rc;
	}

	connection->cb_list = cb_list;
	*handle = connection;
	return 0;
}

int sensing_close_sensor(sensing_sensor_handle_t handle)
{
	return sys_mem_blocks_free(&connection_allocator, 1, &handle);
}

void sensing_reset_connections(void)
{
	uint8_t *ptr = connection_pool;

	while (ptr < connection_pool + sizeof(connection_pool)) {
		sys_mem_blocks_free(&connection_allocator, 1, (void**)&ptr);
		ptr += sizeof(struct sensing_connection);
	}
}
