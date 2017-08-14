/**
 *
 * \file
 *
 * \brief This module contains NMC1000 SPI protocol bus APIs implementation.
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
#include "common/include/nm_common.h"

#ifdef CONF_WINC_USE_SPI

#define USE_OLD_SPI_SW

#include "bus_wrapper/include/nm_bus_wrapper.h"
#include "nmspi.h"

#define NMI_PERIPH_REG_BASE 0x1000
#define NMI_INTR_REG_BASE (NMI_PERIPH_REG_BASE+0xa00)
#define NMI_CHIPID	(NMI_PERIPH_REG_BASE)
#define NMI_PIN_MUX_0 (NMI_PERIPH_REG_BASE + 0x408)
#define NMI_INTR_ENABLE (NMI_INTR_REG_BASE)

#define NMI_SPI_REG_BASE 0xe800
#define NMI_SPI_CTL (NMI_SPI_REG_BASE)
#define NMI_SPI_MASTER_DMA_ADDR (NMI_SPI_REG_BASE+0x4)
#define NMI_SPI_MASTER_DMA_COUNT (NMI_SPI_REG_BASE+0x8)
#define NMI_SPI_SLAVE_DMA_ADDR (NMI_SPI_REG_BASE+0xc)
#define NMI_SPI_SLAVE_DMA_COUNT (NMI_SPI_REG_BASE+0x10)
#define NMI_SPI_TX_MODE (NMI_SPI_REG_BASE+0x20)
#define NMI_SPI_PROTOCOL_CONFIG (NMI_SPI_REG_BASE+0x24)
#define NMI_SPI_INTR_CTL (NMI_SPI_REG_BASE+0x2c)

#define NMI_SPI_PROTOCOL_OFFSET (NMI_SPI_PROTOCOL_CONFIG-NMI_SPI_REG_BASE)

#define SPI_BASE                NMI_SPI_REG_BASE

#define CMD_DMA_WRITE			0xc1
#define CMD_DMA_READ			0xc2
#define CMD_INTERNAL_WRITE		0xc3
#define CMD_INTERNAL_READ		0xc4
#define CMD_TERMINATE			0xc5
#define CMD_REPEAT				0xc6
#define CMD_DMA_EXT_WRITE		0xc7
#define CMD_DMA_EXT_READ		0xc8
#define CMD_SINGLE_WRITE		0xc9
#define CMD_SINGLE_READ			0xca
#define CMD_RESET				0xcf

#define N_OK					 1
#define N_FAIL					 0
#define N_RESET					-1
#define N_RETRY					-2

#define DATA_PKT_SZ_256 		256
#define DATA_PKT_SZ_512			512
#define DATA_PKT_SZ_1K			1024
#define DATA_PKT_SZ_4K			(4 * 1024)
#define DATA_PKT_SZ_8K			(8 * 1024)
#define DATA_PKT_SZ				DATA_PKT_SZ_8K

static uint8 	gu8Crc_off	=   0;

static sint8 nmi_spi_read(uint8* b, uint16 sz)
{
	struct nm_spi_rw spi;
	spi.in_buffer = NULL;
	spi.out_buffer = b;
	spi.size = sz;
	return nm_bus_ioctl(NM_BUS_IOCTL_RW, &spi);
}

static sint8 nmi_spi_write(uint8* b, uint16 sz)
{
	struct nm_spi_rw spi;
	spi.in_buffer = b;
	spi.out_buffer = NULL;
	spi.size = sz;
	return nm_bus_ioctl(NM_BUS_IOCTL_RW, &spi);
}

/********************************************

	Crc7

********************************************/

static const uint8 crc7_syndrome_table[256] = {
	0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
	0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
	0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26,
	0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
	0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d,
	0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
	0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14,
	0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
	0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b,
	0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
	0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42,
	0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
	0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69,
	0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
	0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70,
	0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
	0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e,
	0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
	0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67,
	0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
	0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
	0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
	0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55,
	0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
	0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a,
	0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
	0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03,
	0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
	0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28,
	0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
	0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31,
	0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79
};


static uint8 crc7_byte(uint8 crc, uint8 data)
{
	return crc7_syndrome_table[(crc << 1) ^ data];
}

static uint8 crc7(uint8 crc, const uint8 *buffer, uint32 len)
{
	while (len--)
		crc = crc7_byte(crc, *buffer++);
	return crc;
}

/********************************************

	Spi protocol Function

********************************************/

static sint8 spi_cmd(uint8 cmd, uint32 adr, uint32 u32data, uint32 sz,uint8 clockless)
{
	uint8 bc[9];
	uint8 len = 5;
	sint8 result = N_OK;

	bc[0] = cmd;
	switch (cmd) {
	case CMD_SINGLE_READ:				/* single word (4 bytes) read */
		bc[1] = (uint8)(adr >> 16);
		bc[2] = (uint8)(adr >> 8);
		bc[3] = (uint8)adr;
		len = 5;
		break;
	case CMD_INTERNAL_READ:			/* internal register read */
		bc[1] = (uint8)(adr >> 8);
		if(clockless)  bc[1] |= (1 << 7);
		bc[2] = (uint8)adr;
		bc[3] = 0x00;
		len = 5;
		break;
	case CMD_TERMINATE:					/* termination */
		bc[1] = 0x00;
		bc[2] = 0x00;
		bc[3] = 0x00;
		len = 5;
		break;
	case CMD_REPEAT:						/* repeat */
		bc[1] = 0x00;
		bc[2] = 0x00;
		bc[3] = 0x00;
		len = 5;
		break;
	case CMD_RESET:							/* reset */
		bc[1] = 0xff;
		bc[2] = 0xff;
		bc[3] = 0xff;
		len = 5;
		break;
	case CMD_DMA_WRITE:					/* dma write */
	case CMD_DMA_READ:					/* dma read */
		bc[1] = (uint8)(adr >> 16);
		bc[2] = (uint8)(adr >> 8);
		bc[3] = (uint8)adr;
		bc[4] = (uint8)(sz >> 8);
		bc[5] = (uint8)(sz);
		len = 7;
		break;
	case CMD_DMA_EXT_WRITE:		/* dma extended write */
	case CMD_DMA_EXT_READ:			/* dma extended read */
		bc[1] = (uint8)(adr >> 16);
		bc[2] = (uint8)(adr >> 8);
		bc[3] = (uint8)adr;
		bc[4] = (uint8)(sz >> 16);
		bc[5] = (uint8)(sz >> 8);
		bc[6] = (uint8)(sz);
		len = 8;
		break;
	case CMD_INTERNAL_WRITE:		/* internal register write */
		bc[1] = (uint8)(adr >> 8);
		if(clockless)  bc[1] |= (1 << 7);
		bc[2] = (uint8)(adr);
		bc[3] = (uint8)(u32data >> 24);
		bc[4] = (uint8)(u32data >> 16);
		bc[5] = (uint8)(u32data >> 8);
		bc[6] = (uint8)(u32data);
		len = 8;
		break;
	case CMD_SINGLE_WRITE:			/* single word write */
		bc[1] = (uint8)(adr >> 16);
		bc[2] = (uint8)(adr >> 8);
		bc[3] = (uint8)(adr);
		bc[4] = (uint8)(u32data >> 24);
		bc[5] = (uint8)(u32data >> 16);
		bc[6] = (uint8)(u32data >> 8);
		bc[7] = (uint8)(u32data);
		len = 9;
		break;
	default:
		result = N_FAIL;
		break;
	}

	if (result) {
		if (!gu8Crc_off)
			bc[len-1] = (crc7(0x7f, (const uint8 *)&bc[0], len-1)) << 1;
		else
			len-=1;

		if (M2M_SUCCESS != nmi_spi_write(bc, len)) {
			M2M_ERR("[nmi spi]: Failed cmd write, bus error...\n");
			result = N_FAIL;
		}
	}

	return result;
}

static sint8 spi_cmd_rsp(uint8 cmd)
{
	uint8 rsp;
	sint8 result = N_OK;
	sint8 s8RetryCnt;

	/**
		Command/Control response
	**/
	if ((cmd == CMD_RESET) ||
		 (cmd == CMD_TERMINATE) ||
		 (cmd == CMD_REPEAT)) {
		if (M2M_SUCCESS != nmi_spi_read(&rsp, 1)) {
			result = N_FAIL;
			goto _fail_;
		}
	}

	/* wait for response */
	s8RetryCnt = 10;
	do
	{
		if (M2M_SUCCESS != nmi_spi_read(&rsp, 1)) {
			M2M_ERR("[nmi spi]: Failed cmd response read, bus error...\n");
			result = N_FAIL;
			goto _fail_;
		}
	} while((rsp != cmd) && (s8RetryCnt-- >0));

	/**
		State response
	**/
	/* wait for response */
	s8RetryCnt = 10;
	do
	{
		if (M2M_SUCCESS != nmi_spi_read(&rsp, 1)) {
			M2M_ERR("[nmi spi]: Failed cmd response read, bus error...\n");
			result = N_FAIL;
			goto _fail_;
		}
	} while((rsp != 0x00) && (s8RetryCnt-- >0));

_fail_:

	return result;
}

static sint8 spi_data_read(uint8 *b, uint16 sz,uint8 clockless)
{
	sint16 retry, ix, nbytes;
	sint8 result = N_OK;
	uint8 crc[2];
	uint8 rsp;

	/**
		Data
	**/
	ix = 0;
	do {
		if (sz <= DATA_PKT_SZ)
			nbytes = sz;
		else
			nbytes = DATA_PKT_SZ;

		/**
			Data Respnose header
		**/
		retry = 10;
		do {
			if (M2M_SUCCESS != nmi_spi_read(&rsp, 1)) {
				M2M_ERR("[nmi spi]: Failed data response read, bus error...\n");
				result = N_FAIL;
				break;
			}
			if (((rsp >> 4) & 0xf) == 0xf)
				break;
		} while (retry--);

		if (result == N_FAIL)
			break;

		if (retry <= 0) {
			M2M_ERR("[nmi spi]: Failed data response read...(%02x)\n", rsp);
			result = N_FAIL;
			break;
		}

		/**
			Read bytes
		**/
		if (M2M_SUCCESS != nmi_spi_read(&b[ix], nbytes)) {
			M2M_ERR("[nmi spi]: Failed data block read, bus error...\n");
			result = N_FAIL;
			break;
		}
		if(!clockless)
		{
			/**
			Read Crc
			**/
			if (!gu8Crc_off) {
				if (M2M_SUCCESS != nmi_spi_read(crc, 2)) {
					M2M_ERR("[nmi spi]: Failed data block crc read, bus error...\n");
					result = N_FAIL;
					break;
				}
			}
		}
		ix += nbytes;
		sz -= nbytes;

	} while (sz);

	return result;
}

static sint8 spi_data_write(uint8 *b, uint16 sz)
{
	sint16 ix;
	uint16 nbytes;
	sint8 result = 1;
	uint8 cmd, order, crc[2] = {0};
	//uint8 rsp;

	/**
		Data
	**/
	ix = 0;
	do {
		if (sz <= DATA_PKT_SZ)
			nbytes = sz;
		else
			nbytes = DATA_PKT_SZ;

		/**
			Write command
		**/
		cmd = 0xf0;
		if (ix == 0)  {
			if (sz <= DATA_PKT_SZ)
				order = 0x3;
			else
				order = 0x1;
		} else {
			if (sz <= DATA_PKT_SZ)
				order = 0x3;
			else
				order = 0x2;
		}
		cmd |= order;
		if (M2M_SUCCESS != nmi_spi_write(&cmd, 1)) {
			M2M_ERR("[nmi spi]: Failed data block cmd write, bus error...\n");
			result = N_FAIL;
			break;
		}

		/**
			Write data
		**/
		if (M2M_SUCCESS != nmi_spi_write(&b[ix], nbytes)) {
			M2M_ERR("[nmi spi]: Failed data block write, bus error...\n");
			result = N_FAIL;
			break;
		}

		/**
			Write Crc
		**/
		if (!gu8Crc_off) {
			if (M2M_SUCCESS != nmi_spi_write(crc, 2)) {
				M2M_ERR("[nmi spi]: Failed data block crc write, bus error...\n");
				result = N_FAIL;
				break;
			}
		}

		ix += nbytes;
		sz -= nbytes;
	} while (sz);


	return result;
}

/********************************************

	Spi Internal Read/Write Function

********************************************/

/********************************************

	Spi interfaces

********************************************/

static sint8 spi_write_reg(uint32 addr, uint32 u32data)
{
	sint8 result = N_OK;
	uint8 cmd = CMD_SINGLE_WRITE;
	uint8 clockless = 0;
	if (addr <= 0x30)
	{
		/**
		NMC1000 clockless registers.
		**/
		cmd = CMD_INTERNAL_WRITE;
		clockless = 1;
	}
	else
	{
		cmd = CMD_SINGLE_WRITE;
		clockless = 0;
	}

#if defined USE_OLD_SPI_SW
	result = spi_cmd(cmd, addr, u32data, 4, clockless);
	if (result != N_OK) {
		M2M_ERR("[nmi spi]: Failed cmd, write reg (%08x)...\n", (unsigned int)addr);
		return N_FAIL;
	}

	result = spi_cmd_rsp(cmd);
	if (result != N_OK) {
		M2M_ERR("[nmi spi]: Failed cmd response, write reg (%08x)...\n", (unsigned int)addr);
		spi_cmd(CMD_RESET, 0, 0, 0, 0);
		return N_FAIL;
	}

	return N_OK;
#else

	result = spi_cmd_complete(cmd, addr, (uint8*)&u32data, 4, clockless);
	if (result != N_OK) {
		M2M_ERR( "[nmi spi]: Failed cmd, write reg (%08x)...\n", addr);
	}

	return result;

#endif
}

static sint8 nm_spi_write(uint32 addr, uint8 *buf, uint16 size)
{
	sint8 result;
	uint8 cmd = CMD_DMA_EXT_WRITE;


	/**
		Command
	**/
#if defined USE_OLD_SPI_SW
	result = spi_cmd(cmd, addr, 0, size,0);
	if (result != N_OK) {
		M2M_ERR("[nmi spi]: Failed cmd, write block (%08x)...\n", (unsigned int)addr);
		return N_FAIL;
	}

	result = spi_cmd_rsp(cmd);
	if (result != N_OK) {
		M2M_ERR("[nmi spi ]: Failed cmd response, write block (%08x)...\n", (unsigned int)addr);
		spi_cmd(CMD_RESET, 0, 0, 0, 0);
		return N_FAIL;
	}
#else
	result = spi_cmd_complete(cmd, addr, NULL, size, 0);
	if (result != N_OK) {
		M2M_ERR( "[nmi spi]: Failed cmd, write block (%08x)...\n", addr);
		return N_FAIL;
	}
#endif

	/**
		Data
	**/
	result = spi_data_write(buf, size);
	if (result != N_OK) {
		M2M_ERR("[nmi spi]: Failed block data write...\n");
		spi_cmd(CMD_RESET, 0, 0, 0, 0);
	}

	return N_OK;
}

static sint8 spi_read_reg(uint32 addr, uint32 *u32data)
{
	sint8 result = N_OK;
	uint8 cmd = CMD_SINGLE_READ;
	uint8 tmp[4];
	uint8 clockless = 0;

	if (addr <= 0xff)
	{
		/**
		NMC1000 clockless registers.
		**/
		cmd = CMD_INTERNAL_READ;
		clockless = 1;
	}
	else
	{
		cmd = CMD_SINGLE_READ;
		clockless = 0;
	}

#if defined USE_OLD_SPI_SW
	result = spi_cmd(cmd, addr, 0, 4, clockless);
	if (result != N_OK) {
		M2M_ERR("[nmi spi]: Failed cmd, read reg (%08x)...\n", (unsigned int)addr);
		return N_FAIL;
	}

	result = spi_cmd_rsp(cmd);
	if (result != N_OK) {
		M2M_ERR("[nmi spi]: Failed cmd response, read reg (%08x)...\n", (unsigned int)addr);
		spi_cmd(CMD_RESET, 0, 0, 0, 0);
		return N_FAIL;
	}

	/* to avoid endianess issues */
	result = spi_data_read(&tmp[0], 4, clockless);
	if (result != N_OK) {
		M2M_ERR("[nmi spi]: Failed data read...\n");
		spi_cmd(CMD_RESET, 0, 0, 0, 0);
		return N_FAIL;
	}
#else
	result = spi_cmd_complete(cmd, addr, (uint8*)&tmp[0], 4, clockless);
	if (result != N_OK) {
		M2M_ERR( "[nmi spi]: Failed cmd, read reg (%08x)...\n", addr);
		return N_FAIL;
	}

#endif

	*u32data = tmp[0] |
		((uint32)tmp[1] << 8) |
		((uint32)tmp[2] << 16) |
		((uint32)tmp[3] << 24);

	return N_OK;
}

static sint8 nm_spi_read(uint32 addr, uint8 *buf, uint16 size)
{
	uint8 cmd = CMD_DMA_EXT_READ;
	sint8 result;


	/**
		Command
	**/
#if defined USE_OLD_SPI_SW
	result = spi_cmd(cmd, addr, 0, size,0);
	if (result != N_OK) {
		M2M_ERR("[nmi spi]: Failed cmd, read block (%08x)...\n", (unsigned int)addr);
		return N_FAIL;
	}

	result = spi_cmd_rsp(cmd);
	if (result != N_OK) {
		M2M_ERR("[nmi spi]: Failed cmd response, read block (%08x)...\n", (unsigned int)addr);
		spi_cmd(CMD_RESET, 0, 0, 0, 0);
		return N_FAIL;
	}

	/**
		Data
	**/
	result = spi_data_read(buf, size,0);
	if (result != N_OK) {
		M2M_ERR("[nmi spi]: Failed block data read...\n");
		spi_cmd(CMD_RESET, 0, 0, 0, 0);
		return N_FAIL;
	}
#else
		result = spi_cmd_complete(cmd, addr, buf, size, 0);
		if (result != N_OK) {
			M2M_ERR("[nmi spi]: Failed cmd, read block (%08x)...\n", addr);
			return N_FAIL;
		}
#endif

	return N_OK;
}

/********************************************

	Bus interfaces

********************************************/

static void spi_init_pkt_sz(void)
{
	uint32 val32;

	/* Make sure SPI max. packet size fits the defined DATA_PKT_SZ.  */
	val32 = nm_spi_read_reg(SPI_BASE+0x24);
	val32 &= ~(0x7 << 4);
	switch(DATA_PKT_SZ)
	{
	case 256:  val32 |= (0 << 4); break;
	case 512:  val32 |= (1 << 4); break;
	case 1024: val32 |= (2 << 4); break;
	case 2048: val32 |= (3 << 4); break;
	case 4096: val32 |= (4 << 4); break;
	case 8192: val32 |= (5 << 4); break;

	}
	nm_spi_write_reg(SPI_BASE+0x24, val32);
}

/*
*	@fn		nm_spi_init
*	@brief	Initialize the SPI
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*	@author	M. Abdelmawla
*	@date	11 July 2012
*	@version	1.0
*/
sint8 nm_spi_init(void)
{
	uint32 chipid;
	uint32 reg =0;

	/**
		configure protocol
	**/
	gu8Crc_off = 0;

	// TODO: We can remove the CRC trials if there is a definite way to reset
	// the SPI to it's initial value.
	if (!spi_read_reg(NMI_SPI_PROTOCOL_CONFIG, &reg)) {
		/* Read failed. Try with CRC off. This might happen when module
		is removed but chip isn't reset*/
		gu8Crc_off = 1;
		M2M_ERR("[nmi spi]: Failed internal read protocol with CRC on, retyring with CRC off...\n");
		if (!spi_read_reg(NMI_SPI_PROTOCOL_CONFIG, &reg)){
			// Reaad failed with both CRC on and off, something went bad
			M2M_ERR( "[nmi spi]: Failed internal read protocol...\n");
			return 0;
		}
	}
	if(gu8Crc_off == 0)
	{
		reg &= ~0xc;	/* disable crc checking */
		reg &= ~0x70;
		reg |= (0x5 << 4);
		if (!spi_write_reg(NMI_SPI_PROTOCOL_CONFIG, reg)) {
			M2M_ERR( "[nmi spi]: Failed internal write protocol reg...\n");
			return 0;
		}
		gu8Crc_off = 1;
	}

	/**
		make sure can read back chip id correctly
	**/
	if (!spi_read_reg(0x1000, &chipid)) {
		M2M_ERR("[nmi spi]: Fail cmd read chip id...\n");
		return M2M_ERR_BUS_FAIL;
	}

	M2M_DBG("[nmi spi]: chipid (%08x)\n", (unsigned int)chipid);
	spi_init_pkt_sz();


	return M2M_SUCCESS;
}

/*
*	@fn		nm_spi_init
*	@brief	DeInitialize the SPI
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*	@author	Samer Sarhan
*	@date	27 Feb 2015
*	@version	1.0
*/
sint8 nm_spi_deinit(void)
{
	gu8Crc_off = 0;
	return M2M_SUCCESS;
}

/*
*	@fn		nm_spi_read_reg
*	@brief	Read register
*	@param [in]	u32Addr
*				Register address
*	@return	Register value
*	@author	M. Abdelmawla
*	@date	11 July 2012
*	@version	1.0
*/
uint32 nm_spi_read_reg(uint32 u32Addr)
{
	uint32 u32Val;

	spi_read_reg(u32Addr, &u32Val);

	return u32Val;
}

/*
*	@fn		nm_spi_read_reg_with_ret
*	@brief	Read register with error code return
*	@param [in]	u32Addr
*				Register address
*	@param [out]	pu32RetVal
*				Pointer to u32 variable used to return the read value
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*	@author	M. Abdelmawla
*	@date	11 July 2012
*	@version	1.0
*/
sint8 nm_spi_read_reg_with_ret(uint32 u32Addr, uint32* pu32RetVal)
{
	sint8 s8Ret;

	s8Ret = spi_read_reg(u32Addr,pu32RetVal);

	if(N_OK == s8Ret) s8Ret = M2M_SUCCESS;
	else s8Ret = M2M_ERR_BUS_FAIL;

	return s8Ret;
}

/*
*	@fn		nm_spi_write_reg
*	@brief	write register
*	@param [in]	u32Addr
*				Register address
*	@param [in]	u32Val
*				Value to be written to the register
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*	@author	M. Abdelmawla
*	@date	11 July 2012
*	@version	1.0
*/
sint8 nm_spi_write_reg(uint32 u32Addr, uint32 u32Val)
{
	sint8 s8Ret;

	s8Ret = spi_write_reg(u32Addr, u32Val);

	if(N_OK == s8Ret) s8Ret = M2M_SUCCESS;
	else s8Ret = M2M_ERR_BUS_FAIL;

	return s8Ret;
}

/*
*	@fn		nm_spi_read_block
*	@brief	Read block of data
*	@param [in]	u32Addr
*				Start address
*	@param [out]	puBuf
*				Pointer to a buffer used to return the read data
*	@param [in]	u16Sz
*				Number of bytes to read. The buffer size must be >= u16Sz
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*	@author	M. Abdelmawla
*	@date	11 July 2012
*	@version	1.0
*/
sint8 nm_spi_read_block(uint32 u32Addr, uint8 *puBuf, uint16 u16Sz)
{
	sint8 s8Ret;

	s8Ret = nm_spi_read(u32Addr, puBuf, u16Sz);

	if(N_OK == s8Ret) s8Ret = M2M_SUCCESS;
	else s8Ret = M2M_ERR_BUS_FAIL;

	return s8Ret;
}

/*
*	@fn		nm_spi_write_block
*	@brief	Write block of data
*	@param [in]	u32Addr
*				Start address
*	@param [in]	puBuf
*				Pointer to the buffer holding the data to be written
*	@param [in]	u16Sz
*				Number of bytes to write. The buffer size must be >= u16Sz
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*	@author	M. Abdelmawla
*	@date	11 July 2012
*	@version	1.0
*/
sint8 nm_spi_write_block(uint32 u32Addr, uint8 *puBuf, uint16 u16Sz)
{
	sint8 s8Ret;

	s8Ret = nm_spi_write(u32Addr, puBuf, u16Sz);

	if(N_OK == s8Ret) s8Ret = M2M_SUCCESS;
	else s8Ret = M2M_ERR_BUS_FAIL;

	return s8Ret;
}

#endif
