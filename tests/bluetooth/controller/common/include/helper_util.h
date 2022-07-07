/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void test_print_conn(struct ll_conn *conn);
uint16_t test_ctx_buffers_cnt(void);
void test_set_role(struct ll_conn *conn, uint8_t role);
void test_setup(struct ll_conn *conn);
void test_setup_idx(struct ll_conn *conn, uint8_t idx);

void event_prepare(struct ll_conn *conn);
void event_tx_ack(struct ll_conn *conn, struct node_tx *tx);
void event_done(struct ll_conn *conn);
uint16_t event_counter(struct ll_conn *conn);

#define lt_tx(_opcode, _conn, _param) lt_tx_real(__FILE__, __LINE__, _opcode, _conn, _param)
#define lt_tx_no_encode(_pdu, _conn, _param)                                                       \
	lt_tx_real_no_encode(__FILE__, __LINE__, _pdu, _conn, _param)
#define lt_rx(_opcode, _conn, _tx_ref, _param)                                                     \
	lt_rx_real(__FILE__, __LINE__, _opcode, _conn, _tx_ref, _param)
#define lt_rx_q_is_empty(_conn) lt_rx_q_is_empty_real(__FILE__, __LINE__, _conn)

#define ut_rx_pdu(_opcode, _ntf_ref, _param)                                                       \
	ut_rx_pdu_real(__FILE__, __LINE__, _opcode, _ntf_ref, _param)
#define ut_rx_node(_opcode, _ntf_ref, _param)                                                      \
	ut_rx_node_real(__FILE__, __LINE__, _opcode, _ntf_ref, _param)
#define ut_rx_q_is_empty() ut_rx_q_is_empty_real(__FILE__, __LINE__)

void lt_tx_real(const char *file, uint32_t line, enum helper_pdu_opcode opcode,
		struct ll_conn *conn, void *param);
void lt_tx_real_no_encode(const char *file, uint32_t line, struct pdu_data *pdu,
			  struct ll_conn *conn, void *param);
void lt_rx_real(const char *file, uint32_t line, enum helper_pdu_opcode opcode,
		struct ll_conn *conn, struct node_tx **tx_ref, void *param);
void lt_rx_q_is_empty_real(const char *file, uint32_t line, struct ll_conn *conn);
void ut_rx_pdu_real(const char *file, uint32_t line, enum helper_pdu_opcode opcode,
		    struct node_rx_pdu **ntf_ref, void *param);
void ut_rx_node_real(const char *file, uint32_t line, enum helper_node_opcode opcode,
		     struct node_rx_pdu **ntf_ref, void *param);
void ut_rx_q_is_empty_real(const char *file, uint32_t line);

void encode_pdu(enum helper_pdu_opcode opcode, struct pdu_data *pdu, void *param);
