struct bt_conn_win_test{
	uint32_t pkt_conn_delay_us; 		/* 30us quantum */
	uint8_t tr_win_offset; 				/* Units 1.25ms */
	uint8_t win_size; 					/* Units 1.25ms */
	uint8_t skip_pkt;					/* Skip the X packet after the CONN_IND */
	uint32_t conn_interval_us;			/* Connection interval */
};

extern struct bt_conn_win_test conn_win_test;
