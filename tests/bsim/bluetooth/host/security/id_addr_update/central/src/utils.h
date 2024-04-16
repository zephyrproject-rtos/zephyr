#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

void test_tick(bs_time_t HW_device_time);
void test_init(void);

void bs_bt_utils_setup(void);

void clear_conn(struct bt_conn *conn);
void wait_connected(struct bt_conn **conn);
void wait_disconnected(void);
void disconnect(struct bt_conn *conn);
void scan_connect_to_first_result(void);

void set_security(struct bt_conn *conn, bt_security_t sec);
void wait_pairing_completed(void);

void bas_subscribe(struct bt_conn *conn);
void wait_bas_notification(void);
