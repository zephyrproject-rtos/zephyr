#include <zephyr/sys/atomic_builtin.h>

struct bt_conn;

void bt_testlib_conn_unref(struct bt_conn **conn)
{
	__ASSERT_NO_MSG(conn);
	struct bt_conn *tmp = atomic_ptr_set((void **)conn, NULL);
	__ASSERT_NO_MSG(tmp);
	bt_conn_unref(tmp);
}
