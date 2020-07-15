
struct lll_scan {
	struct ull_cp_conn *conn;
	u8_t  init_addr[BDADDR_SIZE];
	u8_t  adv_addr[BDADDR_SIZE];
	u8_t adv_addr_type:1;
	u16_t conn_timeout;
	u8_t conn_ticks_slot;
	u8_t connect_expire;
	u8_t supervision_expire;
	u8_t supervision_reload;
	u8_t procedure_expire;
	u8_t procedure_reload;
};
