/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <net/buf.h>

/** Identifiers of the signature supported by the RPC */
enum {
	SIG_TYPE_NONE = 1,
	SIG_TYPE_S,
	SIG_TYPE_P,
	SIG_TYPE_S_B,
	SIG_TYPE_B_B_P,
	SIG_TYPE_S_P,
	SIG_TYPE_S_B_P,
	SIG_TYPE_S_B_B_P,
	SIG_TYPE_CONTROL = 0xFF
};

/**
 * RPC memory allocation function, must be implemented by the user of the RPC.
 *
 * This function is called by the RPC mechanism to allocate a buffer for
 * transmission of a serialized function.  The function should not fail.
 *
 * @param length Length of the buffer to allocate.
 *
 * @return Pointer to the allocated buffer, the allocation shall not fail,
 * error must be handled internally
 */
struct net_buf *rpc_alloc_cb(uint16_t length);

/**
 * RPC transmission function, must be implemented by the user of the RPC.
 *
 * @param buf Pointer to the buffer allocated for transmission
 * by @ref rpc_alloc_cb
 */
void rpc_transmit_cb(struct net_buf *buf);

/**
 * RPC initialization function that notifies the peer with an initialization
 * packet containing the information to check the compatibility of the
 * frameworks.
 *
 * @param version Local version to send to the peer.
 */
void rpc_init(uint32_t version);

/**
 * RPC initialization packet reception function, can optionally be implemented
 * by the user of the RPC.  If this function is not implemented, it will default
 * to an empty function.
 *
 * This function is called by the RPC mechanism when an initialization packet is
 * received from the connected peer.
 *
 * @param version Peer advertised version.
 * @param compatible True if the peer runs a compatible RPC framework.
 */
void rpc_init_cb(uint32_t version, bool compatible);

/** RPC serialize hash number generation.
 *
 * @return The unique identifier of the RPC deserialization.
 */
uint32_t rpc_serialize_hash(void);

/**
 * RPC serialization function to serialize a function that does not require any
 * parameter.
 *
 * @param fn_index Index of the function
 */
void rpc_serialize_none(uint8_t fn_index);

/**
 * RPC serialization function to serialize a function that expects a structure
 * as parameter.
 *
 * @param fn_index Index of the function
 * @param struct_data Pointer to the structure to serialize
 * @param struct_length Length of the structure to serialize
 */
void rpc_serialize_s(uint8_t fn_index, const void *struct_data,
		     uint8_t struct_length);

/**
 * RPC serialization function to serialize a function that expects a structure
 * and a pointer as parameters.
 *
 * @param fn_index Index of the function
 * @param struct_data Pointer to the structure to serialize
 * @param struct_length Length of the structure to serialize
 * @param p_priv Pointer to serialize
 */
void rpc_serialize_s_p(uint8_t fn_index, const void *struct_data,
		       uint8_t struct_length, void *p_priv);

/**
 * RPC serialization function to serialize a function that expects a pointer as
 * parameter.
 *
 * @param fn_index Index of the function
 * @param p_priv Pointer to serialize
 */
void rpc_serialize_p(uint8_t fn_index, void *p_priv);

/**
 * RPC serialization function to serialize a function that expects a structure
 * and a buffer as parameters.
 *
 * @param fn_index Index of the function
 * @param struct_data Pointer to the structure to serialize
 * @param struct_length Length of the structure to serialize
 * @param vbuf Pointer to the buffer to serialize
 * @param vbuf_length Length of the buffer to serialize
 */
void rpc_serialize_s_b(uint8_t fn_index, const void *struct_data,
		       uint8_t struct_length, const void *vbuf,
		       uint16_t vbuf_length);

/**
 * RPC serialization function to serialize a function that expects a structure
 * and a buffer as parameters.
 *
 * @param fn_index Index of the function
 * @param vbuf1 Pointer to the buffer1 to serialize
 * @param vbuf1_length Length of the buffer1 to serialize
 * @param vbuf2 Pointer to the buffer2 to serialize
 * @param vbuf2_length Length of the buffer2 to serialize
 * @param p_priv Pointer to serialize
 */
void rpc_serialize_b_b_p(uint8_t fn_index, const void *vbuf1,
			 uint16_t vbuf1_length, const void *vbuf2,
			 uint16_t vbuf2_length, void *p_priv);

/**
 * RPC serialization function to serialize a function that expects a structure
 * and a buffer as parameters.
 *
 * @param fn_index Index of the function
 * @param struct_data Pointer to the structure to serialize
 * @param struct_length Length of the structure to serialize
 * @param vbuf Pointer to the buffer to serialize
 * @param vbuf_length Length of the buffer to serialize
 * @param p_priv Pointer to serialize
 */
void rpc_serialize_s_b_p(uint8_t fn_index, const void *struct_data,
			 uint8_t struct_length, const void *vbuf,
			 uint16_t vbuf_length, void *p_priv);

/**
 * RPC serialization function to serialize a function that expects a structure
 * and a buffer as parameters.
 *
 * @param fn_index Index of the function
 * @param struct_data Pointer to the structure to serialize
 * @param struct_length Length of the structure to serialize
 * @param vbuf1 Pointer to the buffer1 to serialize
 * @param vbuf1_length Length of the buffer1 to serialize
 * @param vbuf2 Pointer to the buffer2 to serialize
 * @param vbuf2_length2 Length of the buffer2 to serialize
 * @param p_priv Pointer to serialize
 */
void rpc_serialize_s_b_b_p(uint8_t fn_index, const void *struct_data,
			   uint8_t struct_length, const void *vbuf1,
			   uint16_t vbuf1_length, const void *vbuf2,
			   uint16_t vbuf2_length, void *p_priv);

/**
 * RPC deserialization function, shall be invoked when a buffer is received
 * over the transport interface.
 *
 * @param buf Pointer to the received buffer
 */
void rpc_deserialize(struct net_buf *buf);

/** RPC deserialize hash number generation.
 *
 * @return The unique identifier of the RPC deserialization.
 */
uint32_t rpc_deserialize_hash(void);

