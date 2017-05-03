/**
 *
 * \file
 *
 * \brief This module contains NMC1000 bus wrapper APIs declarations.
 *
 * Copyright (c) 2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#ifndef _NM_BUS_WRAPPER_H_
#define _NM_BUS_WRAPPER_H_

/**
 * BUS Type
 */
#define  NM_BUS_TYPE_I2C        ((u8_t)0)
#define  NM_BUS_TYPE_SPI        ((u8_t)1)
#define  NM_BUS_TYPE_UART       ((u8_t)2)
/**
 * IOCTL commands
 */
#define NM_BUS_IOCTL_R                  ((u8_t)0)       /*!< Read only ==> I2C/UART. Parameter:tstrNmI2cDefault/tstrNmUartDefault */
#define NM_BUS_IOCTL_W                  ((u8_t)1)       /*!< Write only ==> I2C/UART. Parameter type tstrNmI2cDefault/tstrNmUartDefault*/
#define NM_BUS_IOCTL_W_SPECIAL  ((u8_t)2)               /*!< Write two buffers within the same transaction
							(same start/stop conditions) ==> I2C only. Parameter:tstrNmI2cSpecial */
#define NM_BUS_IOCTL_RW                 ((u8_t)3)       /*!< Read/Write at the same time ==> SPI only. Parameter:tstrNmSpiRw */

#define NM_BUS_IOCTL_WR_RESTART ((u8_t)4)               /*!< Write buffer then made restart condition then read ==> I2C only. parameter:tstrNmI2cSpecial */
/**
 *	@struct	mm_bus_capabilities
 *	@brief	Structure holding bus capabilities information
 *	@sa	NM_BUS_TYPE_I2C, NM_BUS_TYPE_SPI
 */
struct mm_bus_capabilities {
	u16_t max_trx_size;     /*!< Maximum transfer size. Must be >= 16 bytes*/
};

/**
 *	@struct	nm_i2c_default
 *	@brief	Structure holding I2C default operation parameters
 *	@sa		NM_BUS_IOCTL_R, NM_BUS_IOCTL_W
 */
struct nm_i2c_default {
	u8_t slave_address;
	u8_t    *buffer;                /*!< Operation buffer */
	u16_t size;                     /*!< Operation size */
};

/**
 *	@struct	nm_i2c_special
 *	@brief	Structure holding I2C special operation parameters
 *	@sa		NM_BUS_IOCTL_W_SPECIAL
 */
struct nm_i2c_special {
	u8_t slave_address;
	u8_t   *buffer1;        /*!< pointer to the 1st buffer */
	u8_t   *buffer2;        /*!< pointer to the 2nd buffer */
	u16_t size1;            /*!< 1st buffer size */
	u16_t size2;            /*!< 2nd buffer size */
};

/**
 *	@struct	nm_spi_rw
 *	@brief	Structure holding SPI R/W parameters
 *	@sa		NM_BUS_IOCTL_RW
 */
struct nm_spi_rw {
	u8_t    *in_buffer;             /*!< pointer to input buffer.  Can be set to null and in this case zeros should be sent at MOSI */
	u8_t    *out_buffer;            /*!< pointer to output buffer.  Can be set to null and in this case data from MISO can be ignored  */
	u16_t size;                     /*!< Transfere size */
};


/*!< Bus capabilities. This structure must be declared at platform specific bus wrapper */
extern struct mm_bus_capabilities nm_bus_capabilities;


/**
 *	@fn		nm_bus_init
 *	@brief	Initialize the bus wrapper
 *	@return	ZERO in case of success and M2M_ERR_BUS_FAIL in case of failure
 */
s8_t nm_bus_init(void *);

/**
 *	@fn		nm_bus_ioctl
 *	@brief	send/receive from the bus
 *	@param [in]	u8Cmd
 *					IOCTL command for the operation
 *	@param [in]	parameter
 *					Arbitrary parameter depending on IOCTL
 *	@return	ZERO in case of success and M2M_ERR_BUS_FAIL in case of failure
 *	@note	For SPI only, it's important to be able to send/receive at the same time
 */
s8_t nm_bus_ioctl(u8_t cmd, void *parameter);

/**
 *	@fn		nm_bus_deinit
 *	@brief	De-initialize the bus wrapper
 *	@return	ZERO in case of success and M2M_ERR_BUS_FAIL in case of failure
 */
s8_t nm_bus_deinit(void);

/*
 *	@fn		nm_bus_reinit
 *	@brief		re-initialize the bus wrapper
 *	@param [in]	void *config	re-init configuration data
 *	@return		ZERO in case of success and M2M_ERR_BUS_FAIL in case of failure
 */
s8_t nm_bus_reinit(void *);
/*
 *	@fn			nm_bus_get_chip_type
 *	@brief		get chip type
 *	@return		ZERO in case of success and M2M_ERR_BUS_FAIL in case of failure
 */
#ifdef CONF_WINC_USE_UART
u8_t nm_bus_get_chip_type(void);
#endif

#endif  /*_NM_BUS_WRAPPER_H_*/
