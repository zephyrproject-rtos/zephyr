/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Client API in this file is based on mbm_core.c from uC/Modbus Stack.
 *
 *                                uC/Modbus
 *                         The Embedded Modbus Stack
 *
 *      Copyright 2003-2020 Silicon Laboratories Inc. www.silabs.com
 *
 *                   SPDX-License-Identifier: APACHE-2.0
 *
 * This software is subject to an open source license and is distributed by
 *  Silicon Laboratories Inc. pursuant to the terms of the Apache License,
 *      Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
 */

/**
 * @brief MODBUS transport protocol API
 * @defgroup modbus MODBUS
 * @ingroup connectivity
 * @{
 */

#ifndef ZEPHYR_INCLUDE_MODBUS_H_
#define ZEPHYR_INCLUDE_MODBUS_H_

#include <zephyr/drivers/uart.h>
#include <zephyr/sys/slist.h>
#ifdef __cplusplus
extern "C" {
#endif

/** Length of MBAP Header */
#define MODBUS_MBAP_LENGTH		7
/** Length of MBAP Header plus function code */
#define MODBUS_MBAP_AND_FC_LENGTH	(MODBUS_MBAP_LENGTH + 1)

/** @name Modbus exception codes
 *  @{
 */
/** No exception */
#define MODBUS_EXC_NONE				0
/** Illegal function code */
#define MODBUS_EXC_ILLEGAL_FC			1
/** Illegal data address */
#define MODBUS_EXC_ILLEGAL_DATA_ADDR		2
/** Illegal data value */
#define MODBUS_EXC_ILLEGAL_DATA_VAL		3
/** Server device failure */
#define MODBUS_EXC_SERVER_DEVICE_FAILURE	4
/** Acknowledge */
#define MODBUS_EXC_ACK				5
/** Server device busy */
#define MODBUS_EXC_SERVER_DEVICE_BUSY		6
/** Memory parity error */
#define MODBUS_EXC_MEM_PARITY_ERROR		8
/** Gateway path unavailable */
#define MODBUS_EXC_GW_PATH_UNAVAILABLE		10
/** Gateway target device failed to respond */
#define MODBUS_EXC_GW_TARGET_FAILED_TO_RESP	11
/** @} */

/**
 * @brief Frame struct used internally and for raw ADU support.
 */
struct modbus_adu {
	/** Transaction Identifier */
	uint16_t trans_id;
	/** Protocol Identifier */
	uint16_t proto_id;
	/** Length of the data only (not the length of unit ID + PDU) */
	uint16_t length;
	/** Unit Identifier */
	uint8_t unit_id;
	/** Function Code */
	uint8_t fc;
	/** Transaction Data */
	uint8_t data[CONFIG_MODBUS_BUFFER_SIZE - 4];
	/** RTU CRC */
	uint16_t crc;
};

/**
 * @brief Coil read (FC01)
 *
 * Sends a Modbus message to read the status of coils from a server.
 *
 * @param iface      Modbus interface index
 * @param unit_id    Modbus unit ID of the server
 * @param start_addr Coil starting address
 * @param coil_tbl   Pointer to an array of bytes containing the value
 *                   of the coils read.
 *                   The format is:
 *
 *                                       MSB                               LSB
 *                                       B7   B6   B5   B4   B3   B2   B1   B0
 *                                       -------------------------------------
 *                       coil_tbl[0]     #8   #7                            #1
 *                       coil_tbl[1]     #16  #15                           #9
 *                            :
 *                            :
 *
 *                   Note that the array that will be receiving the coil
 *                   values must be greater than or equal to:
 *                   (num_coils - 1) / 8 + 1
 * @param num_coils  Quantity of coils to read
 *
 * @retval           0 If the function was successful
 */
int modbus_read_coils(const int iface,
		      const uint8_t unit_id,
		      const uint16_t start_addr,
		      uint8_t *const coil_tbl,
		      const uint16_t num_coils);

/**
 * @brief Read discrete inputs (FC02)
 *
 * Sends a Modbus message to read the status of discrete inputs from
 * a server.
 *
 * @param iface      Modbus interface index
 * @param unit_id    Modbus unit ID of the server
 * @param start_addr Discrete input starting address
 * @param di_tbl     Pointer to an array that will receive the state
 *                   of the discrete inputs.
 *                   The format of the array is as follows:
 *
 *                                     MSB                               LSB
 *                                     B7   B6   B5   B4   B3   B2   B1   B0
 *                                     -------------------------------------
 *                       di_tbl[0]     #8   #7                            #1
 *                       di_tbl[1]     #16  #15                           #9
 *                            :
 *                            :
 *
 *                   Note that the array that will be receiving the discrete
 *                   input values must be greater than or equal to:
 *                        (num_di - 1) / 8 + 1
 * @param num_di     Quantity of discrete inputs to read
 *
 * @retval           0 If the function was successful
 */
int modbus_read_dinputs(const int iface,
			const uint8_t unit_id,
			const uint16_t start_addr,
			uint8_t *const di_tbl,
			const uint16_t num_di);

/**
 * @brief Read holding registers (FC03)
 *
 * Sends a Modbus message to read the value of holding registers
 * from a server.
 *
 * @param iface      Modbus interface index
 * @param unit_id    Modbus unit ID of the server
 * @param start_addr Register starting address
 * @param reg_buf    Is a pointer to an array that will receive
 *                   the current values of the holding registers from
 *                   the server.  The array pointed to by 'reg_buf' needs
 *                   to be able to hold at least 'num_regs' entries.
 * @param num_regs   Quantity of registers to read
 *
 * @retval           0 If the function was successful
 */
int modbus_read_holding_regs(const int iface,
			     const uint8_t unit_id,
			     const uint16_t start_addr,
			     uint16_t *const reg_buf,
			     const uint16_t num_regs);

/**
 * @brief Read input registers (FC04)
 *
 * Sends a Modbus message to read the value of input registers from
 * a server.
 *
 * @param iface      Modbus interface index
 * @param unit_id    Modbus unit ID of the server
 * @param start_addr Register starting address
 * @param reg_buf    Is a pointer to an array that will receive
 *                   the current value of the holding registers
 *                   from the server.  The array pointed to by 'reg_buf'
 *                   needs to be able to hold at least 'num_regs' entries.
 * @param num_regs   Quantity of registers to read
 *
 * @retval           0 If the function was successful
 */
int modbus_read_input_regs(const int iface,
			   const uint8_t unit_id,
			   const uint16_t start_addr,
			   uint16_t *const reg_buf,
			   const uint16_t num_regs);

/**
 * @brief Write single coil (FC05)
 *
 * Sends a Modbus message to write the value of single coil to a server.
 *
 * @param iface      Modbus interface index
 * @param unit_id    Modbus unit ID of the server
 * @param coil_addr  Coils starting address
 * @param coil_state Is the desired state of the coil
 *
 * @retval           0 If the function was successful
 */
int modbus_write_coil(const int iface,
		      const uint8_t unit_id,
		      const uint16_t coil_addr,
		      const bool coil_state);

/**
 * @brief Write single holding register (FC06)
 *
 * Sends a Modbus message to write the value of single holding register
 * to a server unit.
 *
 * @param iface      Modbus interface index
 * @param unit_id    Modbus unit ID of the server
 * @param start_addr Coils starting address
 * @param reg_val    Desired value of the holding register
 *
 * @retval           0 If the function was successful
 */
int modbus_write_holding_reg(const int iface,
			     const uint8_t unit_id,
			     const uint16_t start_addr,
			     const uint16_t reg_val);

/**
 * @brief Read diagnostic (FC08)
 *
 * Sends a Modbus message to perform a diagnostic function of a server unit.
 *
 * @param iface      Modbus interface index
 * @param unit_id    Modbus unit ID of the server
 * @param sfunc      Diagnostic sub-function code
 * @param data       Sub-function data
 * @param data_out   Pointer to the data value
 *
 * @retval           0 If the function was successful
 */
int modbus_request_diagnostic(const int iface,
			      const uint8_t unit_id,
			      const uint16_t sfunc,
			      const uint16_t data,
			      uint16_t *const data_out);

/**
 * @brief Write coils (FC15)
 *
 * Sends a Modbus message to write to coils on a server unit.
 *
 * @param iface      Modbus interface index
 * @param unit_id    Modbus unit ID of the server
 * @param start_addr Coils starting address
 * @param coil_tbl   Pointer to an array of bytes containing the value
 *                   of the coils to write.
 *                   The format is:
 *
 *                                       MSB                               LSB
 *                                       B7   B6   B5   B4   B3   B2   B1   B0
 *                                       -------------------------------------
 *                       coil_tbl[0]     #8   #7                            #1
 *                       coil_tbl[1]     #16  #15                           #9
 *                            :
 *                            :
 *
 *                   Note that the array that will be receiving the coil
 *                   values must be greater than or equal to:
 *                   (num_coils - 1) / 8 + 1
 * @param num_coils  Quantity of coils to write
 *
 * @retval           0 If the function was successful
 */
int modbus_write_coils(const int iface,
		       const uint8_t unit_id,
		       const uint16_t start_addr,
		       uint8_t *const coil_tbl,
		       const uint16_t num_coils);

/**
 * @brief Write holding registers (FC16)
 *
 * Sends a Modbus message to write to integer holding registers
 * to a server unit.
 *
 * @param iface      Modbus interface index
 * @param unit_id    Modbus unit ID of the server
 * @param start_addr Register starting address
 * @param reg_buf    Is a pointer to an array containing
 *                   the value of the holding registers to write.
 *                   Note that the array containing the register values must
 *                   be greater than or equal to 'num_regs'
 * @param num_regs   Quantity of registers to write
 *
 * @retval           0 If the function was successful
 */
int modbus_write_holding_regs(const int iface,
			      const uint8_t unit_id,
			      const uint16_t start_addr,
			      uint16_t *const reg_buf,
			      const uint16_t num_regs);

/**
 * @brief Read floating-point holding registers (FC03)
 *
 * Sends a Modbus message to read the value of floating-point
 * holding registers from a server unit.
 *
 * @param iface      Modbus interface index
 * @param unit_id    Modbus unit ID of the server
 * @param start_addr Register starting address
 * @param reg_buf    Is a pointer to an array that will receive
 *                   the current values of the holding registers from
 *                   the server.  The array pointed to by 'reg_buf' needs
 *                   to be able to hold at least 'num_regs' entries.
 * @param num_regs   Quantity of registers to read
 *
 * @retval           0 If the function was successful
 */
int modbus_read_holding_regs_fp(const int iface,
				const uint8_t unit_id,
				const uint16_t start_addr,
				float *const reg_buf,
				const uint16_t num_regs);

/**
 * @brief Write floating-point holding registers (FC16)
 *
 * Sends a Modbus message to write to floating-point holding registers
 * to a server unit.
 *
 * @param iface      Modbus interface index
 * @param unit_id    Modbus unit ID of the server
 * @param start_addr Register starting address
 * @param reg_buf    Is a pointer to an array containing
 *                   the value of the holding registers to write.
 *                   Note that the array containing the register values must
 *                   be greater than or equal to 'num_regs'
 * @param num_regs   Quantity of registers to write
 *
 * @retval           0 If the function was successful
 */
int modbus_write_holding_regs_fp(const int iface,
				 const uint8_t unit_id,
				 const uint16_t start_addr,
				 float *const reg_buf,
				 const uint16_t num_regs);

/** Modbus Server User Callback structure */
struct modbus_user_callbacks {
	/** Coil read callback */
	int (*coil_rd)(uint16_t addr, bool *state);

	/** Coil write callback */
	int (*coil_wr)(uint16_t addr, bool state);

	/** Discrete Input read callback */
	int (*discrete_input_rd)(uint16_t addr, bool *state);

	/** Input Register read callback */
	int (*input_reg_rd)(uint16_t addr, uint16_t *reg);

	/** Floating Point Input Register read callback */
	int (*input_reg_rd_fp)(uint16_t addr, float *reg);

	/** Holding Register read callback */
	int (*holding_reg_rd)(uint16_t addr, uint16_t *reg);

	/** Holding Register write callback */
	int (*holding_reg_wr)(uint16_t addr, uint16_t reg);

	/** Floating Point Holding Register read callback */
	int (*holding_reg_rd_fp)(uint16_t addr, float *reg);

	/** Floating Point Holding Register write callback */
	int (*holding_reg_wr_fp)(uint16_t addr, float reg);
};

/**
 * @brief Get Modbus interface index according to interface name
 *
 * If there is more than one interface, it can be used to clearly
 * identify interfaces in the application.
 *
 * @param iface_name Modbus interface name
 *
 * @retval           Modbus interface index or negative error value.
 */
int modbus_iface_get_by_name(const char *iface_name);

/**
 * @brief ADU raw callback function signature
 *
 * @param iface      Modbus RTU interface index
 * @param adu        Pointer to the RAW ADU struct to send
 * @param user_data  Pointer to the user data
 *
 * @retval           0 If transfer was successful
 */
typedef int (*modbus_raw_cb_t)(const int iface, const struct modbus_adu *adu,
				void *user_data);

/**
 * @brief Custom function code handler function signature.
 *
 * Modbus allows user defined function codes which can be used to extend
 * the base protocol. These callbacks can also be used to implement
 * function codes currently not supported by Zephyr's Modbus subsystem.
 *
 * If an error occurs during the handling of the request, the handler should
 * signal this by setting excep_code to a modbus exception code.
 *
 * User data pointer can be used to pass state between subsequent calls to
 * the handler.
 *
 * @param iface      Modbus interface index
 * @param rx_adu     Pointer to the received ADU struct
 * @param tx_adu     Pointer to the outgoing ADU struct
 * @param excep_code Pointer to possible exception code
 * @param user_data  Pointer to user data
 *
 * @retval           true If response should be sent, false otherwise
 */
typedef bool (*modbus_custom_cb_t)(const int iface,
				const struct modbus_adu *const rx_adu,
				struct modbus_adu *const tx_adu,
				uint8_t *const excep_code,
				void *const user_data);

/** @cond INTERNAL_HIDDEN */
/**
 * @brief Custom function code definition.
 */
struct modbus_custom_fc {
	sys_snode_t node;
	modbus_custom_cb_t cb;
	void *user_data;
	uint8_t fc;
	uint8_t excep_code;
};
/** @endcond INTERNAL_HIDDEN */

/**
 * @brief Helper macro for initializing custom function code structs
 */
#define MODBUS_CUSTOM_FC_DEFINE(name, user_cb, user_fc, userdata)	\
	static struct modbus_custom_fc modbus_cfg_##name = {		\
		.cb = user_cb,						\
		.user_data = userdata,					\
		.fc = user_fc,						\
		.excep_code = MODBUS_EXC_NONE,				\
	}

/**
 * @brief Modbus interface mode
 */
enum modbus_mode {
	/** Modbus over serial line RTU mode */
	MODBUS_MODE_RTU,
	/** Modbus over serial line ASCII mode */
	MODBUS_MODE_ASCII,
	/** Modbus raw ADU mode */
	MODBUS_MODE_RAW,
};

/**
 * @brief Modbus serial line parameter
 */
struct modbus_serial_param {
	/** Baudrate of the serial line */
	uint32_t baud;
	/** parity UART's parity setting:
	 *    UART_CFG_PARITY_NONE,
	 *    UART_CFG_PARITY_EVEN,
	 *    UART_CFG_PARITY_ODD
	 */
	enum uart_config_parity parity;
	/** stop_bits UART's stop bits setting in non-compliant mode:
	 *    UART_CFG_STOP_BITS_0_5,
	 *    UART_CFG_STOP_BITS_1,
	 *    UART_CFG_STOP_BITS_1_5,
	 *    UART_CFG_STOP_BITS_2,
	 */
	enum uart_config_stop_bits stop_bits;
};

/**
 * @brief Modbus server parameter
 */
struct modbus_server_param {
	/** Pointer to the User Callback structure */
	struct modbus_user_callbacks *user_cb;
	/** Modbus unit ID of the server */
	uint8_t unit_id;
};

struct modbus_raw_cb {
	modbus_raw_cb_t raw_tx_cb;
	void *user_data;
};

/**
 * @brief User parameter structure to configure Modbus interface
 *        as client or server.
 */
struct modbus_iface_param {
	/** Mode of the interface */
	enum modbus_mode mode;
	union {
		struct modbus_server_param server;
		/** Amount of time client will wait for
		 *  a response from the server.
		 */
		uint32_t rx_timeout;
	};
	union {
		/** Serial support parameter of the interface */
		struct modbus_serial_param serial;
		/** Pointer to raw ADU callback function */
		struct modbus_raw_cb rawcb;
	};
};

/**
 * @brief Configure Modbus Interface as raw ADU server
 *
 * @param iface      Modbus RTU interface index
 * @param param      Configuration parameter of the server interface
 *
 * @retval           0 If the function was successful
 */
int modbus_init_server(const int iface, struct modbus_iface_param param);

/**
 * @brief Configure Modbus Interface as raw ADU client
 *
 * @param iface      Modbus RTU interface index
 * @param param      Configuration parameter of the client interface
 *
 * @retval           0 If the function was successful
 */
int modbus_init_client(const int iface, struct modbus_iface_param param);

/**
 * @brief Disable Modbus Interface
 *
 * This function is called to disable Modbus interface.
 *
 * @param iface      Modbus interface index
 *
 * @retval           0 If the function was successful
 */
int modbus_disable(const uint8_t iface);

/**
 * @brief Submit raw ADU
 *
 * @param iface      Modbus RTU interface index
 * @param adu        Pointer to the RAW ADU struct that is received
 *
 * @retval           0 If transfer was successful
 */
int modbus_raw_submit_rx(const int iface, const struct modbus_adu *adu);

/**
 * @brief Put MBAP header into a buffer
 *
 * @param adu        Pointer to the RAW ADU struct
 * @param header     Pointer to the buffer in which MBAP header
 *                   will be placed.
 */
void modbus_raw_put_header(const struct modbus_adu *adu, uint8_t *header);

/**
 * @brief Get MBAP header from a buffer
 *
 * @param adu        Pointer to the RAW ADU struct
 * @param header     Pointer to the buffer containing MBAP header
 */
void modbus_raw_get_header(struct modbus_adu *adu, const uint8_t *header);

/**
 * @brief Set Server Device Failure exception
 *
 * This function modifies ADU passed by the pointer.
 *
 * @param adu        Pointer to the RAW ADU struct
 */
void modbus_raw_set_server_failure(struct modbus_adu *adu);

/**
 * @brief Use interface as backend to send and receive ADU
 *
 * This function overwrites ADU passed by the pointer and generates
 * exception responses if backend interface is misconfigured or
 * target device is unreachable.
 *
 * @param iface      Modbus client interface index
 * @param adu        Pointer to the RAW ADU struct
 *
 * @retval           0 If transfer was successful
 */
int modbus_raw_backend_txn(const int iface, struct modbus_adu *adu);

/**
 * @brief Register a user-defined function code handler.
 *
 * The Modbus specification allows users to define standard function codes
 * missing from Zephyr's Modbus implementation as well as add non-standard
 * function codes in the ranges 65 to 72 and 100 to 110 (decimal), as per
 * specification.
 *
 * This function registers a new handler at runtime for the given
 * function code.
 *
 * @param iface        Modbus client interface index
 * @param custom_fc    User defined function code and callback pair
 *
 * @retval           0 on success
 */
int modbus_register_user_fc(const int iface, struct modbus_custom_fc *custom_fc);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_MODBUS_H_ */
