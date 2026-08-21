/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TESTIPC_H__
#define __TESTIPC_H__

/*
 * Simple message protocol for communication between ARM cores and DSP cores
 * on NXP i.MX RTxxx devices.
 *
 * Each message is a single uint32_t. Message layout is as follows:
 *
 * [31:8]: payload
 * [7:0]: opcode
 */

static inline uint32_t testipc_msg_make(uint8_t op, int32_t payload)
{
	return (uint32_t)op
		 | (((uint32_t)payload & 0x00FFFFFFU) << 8);
}

static inline uint8_t testipc_msg_get_op(uint32_t msg)
{
	return (uint8_t)msg;
}

static inline int32_t testipc_msg_get_payload(uint32_t msg)
{
	uint32_t p = msg >> 8;

	/* Sign extension */
	if (p & 0x00800000U) {
		p |= 0xFF000000U;
	}

	return (int32_t)p;
}

/* Opcodes sent DSP -> ARM */
#define AMP_OP_ALIVE       0x01U /* DSP booted and is running; payload = beacon counter */
#define AMP_OP_ECHO_RESP   0x02U /* Reply to AMP_OP_ECHO_REQ; payload = echoed value */
#define AMP_OP_IPI_ACK     0x03U /* Reply to a data-less IPI notification */
#define AMP_OP_AUDIO_DONE  0x04U /* Audio run finished OK; payload = transactions completed */
#define AMP_OP_ERROR       0x05U /* DSP hit an error; payload = truncated error code */

/* Opcodes sent ARM -> DSP */
#define AMP_OP_ECHO_REQ    0x81U /* Ask the DSP to echo the payload back */
#define AMP_OP_AUDIO_START 0x82U /* Ask the DSP to start the bounded audio run */

/* Payload magic used by the echo round-trip test. */
#define AMP_ECHO_MAGIC     0x000ABCDEU

/* Number of I2S transactions the audio test asks the DSP to perform. */
#define AMP_AUDIO_ITERATIONS 100U

int testipc_init(void);
int testipc_send(uint32_t msg);
int testipc_report_error(int retcode);
int testipc_recv(uint32_t *msg);
int testipc_recv_timeout(uint32_t *msg, k_timeout_t timeout);

#endif /* __TESTIPC_H__ */
