/**
 *  Copyright (c) 2021 Golden Bits Software, Inc.
 *
 *  @file  auth_internal.h
 *
 *  @brief  Internal functions used by the authentication library.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_AUTH_INTERNAL_H_
#define ZEPHYR_INCLUDE_AUTH_INTERNAL_H_

/**
 * Thread params
 */
struct auth_thread_params {
	struct authenticate_conn *auth_conn;
	struct k_sem *thrd_sem;
};



/**
 * Defines to handle message fragmentation over the different transports.
 */

/* should be at large as the largest msg. */
#define XPORT_MAX_MESSAGE_SIZE          (650u)

/**
 * A message is broken up into multiple fragments.  Each fragement has
 * sync bytes, flags, and fragment length.
 */
#define XPORT_FRAG_SYNC_BYTE_HIGH       (0xA5)  /* bits 15-4 sync byte */
#define XPORT_FRAG_SYNC_BYTE_LOW        (0x90)
#define XPORT_FRAG_LOWBYTE_MASK         (0xF0)  /* bits 3-0 for flags */

#define XPORT_FRAG_SYNC_BITS            ((XPORT_FRAG_SYNC_BYTE_HIGH << 8u) | \
					                                XPORT_FRAG_SYNC_BYTE_LOW)
#define XPORT_FRAG_SYNC_MASK            (0xFFF0)

/**
 * Bitlfags used to indicate fragment order
 */
#define XPORT_FRAG_BEGIN                (0x1)
#define XPORT_FRAG_NEXT                 (0x2)
#define XPORT_FRAG_END                  (0x4)
#define XPORT_FRAG_UNUSED               (0x8)

#define XPORT_FRAG_HDR_BYTECNT          (sizeof(struct auth_message_frag_hdr))
#define XPORT_MIN_FRAGMENT              XPORT_FRAG_HDR_BYTECNT


#pragma pack(push, 1)
/**
 * Fragment header
 */
struct auth_message_frag_hdr {
	/* bits 15-4  are for fragment sync, bits 3-0 are flags */
	uint16_t sync_flags;    /* bytes to insure we're at a fragment */
	uint16_t payload_len;   /* number of bytes in the payload, does not
	                         * include the header. */
};

/**
 * One fragment, one or more fragments make up a message.
 */
struct auth_message_fragment {
	struct auth_message_frag_hdr hdr;
	uint8_t frag_payload[XPORT_MAX_MESSAGE_SIZE];
};
#pragma pack(pop)



/**
 * Starts the authentication thread.
 *
 * @param auth_conn Pointer to Authentication connection struct.
 *
 * @return  0 on success else negative error number.
 */
int auth_start_thread(struct authenticate_conn *auth_conn);


/**
 * Set the authentication status.
 *
 * @param auth_conn   Authentication connection struct.
 * @param status      Authentication status.
 */
void auth_lib_set_status(struct authenticate_conn *auth_conn, enum auth_status status);


/**
 * Initializes DTLS authentication method.
 *
 * @param auth_conn   Pointer to Authentication connection struct.
 * @param opt_params  Pointer to DTLS params which should contain certs and
 *                    associated private keys.
 *
 * @return  0 on success else one of AUTH_ERROR_* values.
 */
int auth_init_dtls_method(struct authenticate_conn *auth_conn,
			  struct auth_optional_param *opt_params);

/**
 * De-initialize, free any resources used during DTLS authentication.
 *
 * @param auth_conn Pointer to Authentication connection struct.
 *
 * return AUTH_SUCCESS on success, else AUTH_ERROR_* value.
 */
int auth_deinit_dtls(struct authenticate_conn *auth_conn);

/**
 * Initialize Challenge-Response method with additional parameters.
 *
 * @param auth_conn   Pointer to Authentication connection struct.
 * @param opt_params  Pointer to optional Challenge-Response params.
 *
 * @return  0 on success else one of AUTH_ERROR_* values.
 */
int auth_init_chalresp_method(struct authenticate_conn *auth_conn,
			      struct auth_optional_param *opt_params);

/**
 * De-initialize and free any resources used with the Challenge-Response
 * authentication method.
 *
 * @param auth_conn Pointer to Authentication connection struct.
 *
 * return AUTH_SUCCESS on success, else AUTH_ERROR_* value.
 */
int auth_deinit_chalresp(struct authenticate_conn *auth_conn);

/**
 *  Used by the client to send data bytes to the Peripheral.
 *  If necessary, will break up data into several sends depending
 *  on the MTU size.
 *
 * @param auth_conn   Pointer to Authentication connection struct.
 * @param buf         Buffer to send.
 * @param len         Buffer size.
 *
 * @return  Number of bytes sent or negative error value.
 */
int auth_client_tx(struct authenticate_conn *auth_conn, const unsigned char *buf,
		   size_t len);

/**
 * Used by the client to read data from the receive buffer.  Will not
 * block, if no bytes are available from the Peripheral returns 0.
 *
 * @param auth_conn  Pointer to Authentication connection struct.
 * @param buf        Buffer to copy byes into.
 * @param len        Buffer length.
 *
 * @return  Number of bytes returned, 0 if no bytes returned, or negative if
 *          an error occurred.
 */
int auth_client_recv(struct authenticate_conn *auth_conn, unsigned char *buf,
		     size_t len);

/**
 * Used by the Central to receive data from the Peripheral.  Will block until data is
 * received or a timeout has occurred.
 *
 * @param auth_conn  Pointer to Authentication connection struct.
 * @param buf        Buffer to copy byes into.
 * @param len        Buffer length.
 * @param timeout    Wait time in msecs for data, K_FOREVER or K_NO_WAIT.
 *
 * @return  Number of bytes returned, 0 if no bytes returned, or negative if
 *          an error occurred.
 */
int auth_client_recv_timeout(struct authenticate_conn *auth_conn, unsigned char *buf,
			     size_t len, uint32_t timeout);

/**
 * Used by the server to send data to the Central. Will break up buffer to max MTU
 * sizes if necessary and send multiple PDUs.  Uses Write Indications to get
 * acknowledgement from the Central before sending additional packet.
 *
 * @param auth_conn Pointer to Authentication connection struct.
 * @param buf       Data to send
 * @param len       Data length.
 *
 * @return  Number of bytes send, or negative if an error occurred.
 */
int auth_server_tx(struct authenticate_conn *auth_conn, const unsigned char *buf,
		   size_t len);

/**
 * Used by the server to read data from the receive buffer. Non-blocking.
 *
 * @param auth_conn Context, pointer to Authentication connection struct.
 * @param buf       Buffer to copy bytes into.
 * @param len       Number of bytes requested.
 *
 * @return Number of bytes returned, 0 if no bytes, of negative if an error occurred.
 */
int auth_server_recv(struct authenticate_conn *auth_conn, unsigned char *buf,
		     size_t len);

/**
 * Used by the server to read data from the receive buffer.  Blocking call.
 *
 * @param auth_conn  Context, pointer to Authentication connection struct.
 * @param buf        Buffer to copy bytes into.
 * @param len        Number of bytes requested.
 * @param timeout    Wait time in msecs for data, K_FOREVER or K_NO_WAIT.
 *
 * @return  Number of bytes returned, or -EAGAIN if timed out.
 */
int auth_server_recv_timeout(struct authenticate_conn *auth_conn, unsigned char *buf,
			     size_t len, uint32_t timeout);


/**
 * Used by the client to send data to Peripheral.
 *
 * @param conn  Pointer to Authentication connection struct.
 * @param data  Data to send.
 * @param len   Byte length of data.
 *
 * @return Number of bytes sent, negative number on error.
 */
int auth_client_tx(struct authenticate_conn *conn, const unsigned char *data,
		   size_t len);

/**
 * Used by the client to receive data to Peripheral.
 *
 * @param conn     Pointer to Authentication connection struct.
 * @param buf      Buffer to copy received bytes into.
 * @param rxbytes  Number of bytes requested.
 *
 * @return Number of bytes copied into the buffer. On error, negative error number.
 */
int auth_client_rx(struct authenticate_conn *conn, uint8_t *buf, size_t rxbytes);

/**
 * Used by server to send data to the client.
 *
 * @param conn  Pointer to Authentication connection struct.
 * @param data  Data to send.
 * @param len   Byte length of data.
 *
 * @return Number of bytes sent, negative number on error.
 */
int auth_server_tx(struct authenticate_conn *conn, const unsigned char *data,
		   size_t len);

/**
 * Used by server to receive data.
 *
 * @param conn  Pointer to Authentication connection struct.
 * @param buf   Buffer to copy received bytes into.
 * @param len   Number of bytes requested.
 *
 * @return Number of bytes copied (received) into the buffer.
 *         On error, negative error number.
 */
int auth_sever_rx(struct authenticate_conn *conn, uint8_t *buf, size_t len);

/**
 * Scans buffer to determine if a fragment is present.
 *
 * @param buffer            Buffer to scan.
 * @param buflen            Buffer length.
 * @param frag_beg_offset   Offset from buffer begin where fragment starts.
 * @param frag_byte_cnt     Number of bytes in this fragment.
 *
 * @return  true if full frame found, else false.
 */
bool auth_message_get_fragment(const uint8_t *buffer, uint16_t buflen,
			       uint16_t *frag_beg_offset, uint16_t *frag_byte_cnt);

/**
 * Used by lower transport to put received bytes into recv queue. Handle framing and
 * puts full message into receive queue. Handles reassembly of message fragments.
 *
 * @param xporthdl  Transport handle.
 * @param buff      Pointer to one frame.
 * @param buflen    Number of bytes in frame
 *
 * @return The number of bytes queued, can be less than requested.
 *         On error, negative value is returned.
 */
int auth_message_assemble(const auth_xport_hdl_t xporthdl, const uint8_t *buf,
			  size_t buflen);

/**
 * Swap the fragment header from Big Endian to the processor's byte
 * ordering.
 *
 * @param  frag_hdr  Pointer to message fragment header.
 */
void auth_message_hdr_to_cpu(struct auth_message_frag_hdr *frag_hdr);


/**
 * Swaps the fragment header bytes to Big Endian order.
 *
 * @param  frag_hdr  Pointer to message fragment header.
 */
void auth_message_hdr_to_be16(struct auth_message_frag_hdr *frag_hdr);

/**
 * Get random numbers
 *
 * @param buf  Buffer to return random bytes.
 * @param num  Number of random bytes requested.
 */
void auth_get_random(uint8_t *buf, size_t num);

#endif   /* ZEPHYR_INCLUDE_AUTH_INTERNAL_H_ */