/*
 * Copyright (c) 2022 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Run this test from zephyr directory as:
 *
 *     ./scripts/twister --coverage -p native_posix -v -T tests/bluetooth/ctrl_isoal/
 *
 */

#ifndef _ISOAL_TEST_COMMON_H_
#define _ISOAL_TEST_COMMON_H_


#define TEST_RX_PDU_PAYLOAD_MAX      (40)
#define TEST_RX_PDU_SIZE             (TEST_RX_PDU_PAYLOAD_MAX + 2)

#define TEST_RX_SDU_FRAG_PAYLOAD_MAX (100)
#define TEST_TX_PDU_PAYLOAD_MAX      (40)

#define TEST_TX_PDU_SIZE             (TEST_TX_PDU_PAYLOAD_MAX + 2)
#define TEST_TX_SDU_FRAG_PAYLOAD_MAX (100)


#define LLID_TO_STR(llid) (llid == PDU_BIS_LLID_COMPLETE_END ? "COMPLETE_END" : \
	(llid == PDU_BIS_LLID_START_CONTINUE ? "START_CONT" : \
		(llid == PDU_BIS_LLID_FRAMED ? "FRAMED" : \
			(llid == PDU_BIS_LLID_CTRL ? "CTRL" : "?????"))))

#define DU_ERR_TO_STR(err) (err == 1 ? "Bit Errors" : \
			(err == 2 ? "Data Lost" : \
			(err == 0 ? "OK" : "Undefined!")))

#define STATE_TO_STR(s) (s == BT_ISO_SINGLE ? "SINGLE" : \
	(s == BT_ISO_START ? "START" : \
		(s == BT_ISO_CONT ? "CONT" : \
			(s == BT_ISO_END ? "END" : "???"))))

#define ROLE_TO_STR(s) \
	((s) == ISOAL_ROLE_BROADCAST_SOURCE ? "Broadcast Source" : \
	 ((s) == ISOAL_ROLE_BROADCAST_SINK ? "Broadcast Sink" : \
	  ((s) == ISOAL_ROLE_PERIPHERAL ? "Peripheral" : \
	   ((s) == ISOAL_ROLE_CENTRAL ? "Central" : "Undefined"))))

#define FSM_TO_STR(s) (s == ISOAL_START ? "START" : \
	(s == ISOAL_CONTINUE ? "CONTINUE" : \
		(s == ISOAL_ERR_SPOOL ? "ERR SPOOL" : "???")))

#if defined(ISOAL_CONFIG_BUFFER_RX_SDUS_ENABLE)
#define COLLATED_RX_SDU_INFO(_non_buf, _buf) (_buf)
#else
#define COLLATED_RX_SDU_INFO(_non_buf, _buf) (_non_buf)
#endif /* ISOAL_CONFIG_BUFFER_RX_SDUS_ENABLE */

/* Maximum PDU payload for given number of PDUs */
#define MAX_FRAMED_PDU_PAYLOAD(_pdus)                                                              \
	(TEST_TX_PDU_PAYLOAD_MAX * _pdus) -                                                        \
		((PDU_ISO_SEG_HDR_SIZE * _pdus) + PDU_ISO_SEG_TIMEOFFSET_SIZE)

struct rx_pdu_meta_buffer {
	struct isoal_pdu_rx pdu_meta;
	struct node_rx_iso_meta meta;
	uint8_t pdu[TEST_RX_PDU_SIZE];
};

struct rx_sdu_frag_buffer {
	uint16_t write_loc;
	uint8_t sdu[TEST_RX_SDU_FRAG_PAYLOAD_MAX];
};


struct tx_pdu_meta_buffer {
	struct node_tx_iso node_tx;
	union{
		struct pdu_iso pdu;
		uint8_t pdu_payload[TEST_TX_PDU_PAYLOAD_MAX];
	};
};

struct tx_sdu_frag_buffer {
	struct isoal_sdu_tx sdu_tx;
	uint8_t sdu_payload[TEST_TX_SDU_FRAG_PAYLOAD_MAX];
};


extern void isoal_test_init_rx_pdu_buffer(struct rx_pdu_meta_buffer *buf);
extern void isoal_test_init_rx_sdu_buffer(struct rx_sdu_frag_buffer *buf);
extern void isoal_test_create_unframed_pdu(uint8_t llid, uint8_t *dataptr,
	uint8_t length, uint64_t payload_number, uint32_t timestamp,
	uint8_t  status, struct isoal_pdu_rx *pdu_meta);
extern uint16_t isoal_test_insert_segment(bool sc,
	bool cmplt, uint32_t time_offset, uint8_t *dataptr, uint8_t length,
	struct isoal_pdu_rx *pdu_meta);
extern void isoal_test_create_framed_pdu_base(uint64_t payload_number,
	uint32_t timestamp, uint8_t  status, struct isoal_pdu_rx *pdu_meta);
extern uint16_t isoal_test_add_framed_pdu_single(uint8_t *dataptr,
	uint8_t length, uint32_t time_offset, struct isoal_pdu_rx *pdu_meta);
extern uint16_t isoal_test_add_framed_pdu_start(uint8_t *dataptr,
	uint8_t length, uint32_t time_offset, struct isoal_pdu_rx *pdu_meta);
extern uint16_t isoal_test_add_framed_pdu_cont(uint8_t *dataptr,
	uint8_t length, struct isoal_pdu_rx *pdu_meta);
extern uint16_t isoal_test_add_framed_pdu_end(uint8_t *dataptr,
	uint8_t length, struct isoal_pdu_rx *pdu_meta);
extern void isoal_test_init_tx_pdu_buffer(struct tx_pdu_meta_buffer *buf);
extern void isoal_test_init_tx_sdu_buffer(struct tx_sdu_frag_buffer *buf);
extern void init_test_data_buffer(uint8_t *buf, uint16_t size);

#endif /* _ISOAL_TEST_COMMON_H_ */
