/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @brief Header file for Atmel SAM3 I2C/TWI driver.
 *
 * Contains register definitions for the TWI controller.
 * This uses TWI instead of I2C to align with the datasheet.
 */

#ifndef _DRIVERS_I2C_ATMEL_SAM3_H_
#define _DRIVERS_I2C_ATMEL_SAM3_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Control Register
 */
#define TWI_CR_START		(1 << 0)	/* START for read      */
#define TWI_CR_STOP		(1 << 1)	/* Send STOP           */
#define TWI_CR_MSEN		(1 << 2)	/* Master enable       */
#define TWI_CR_MSDIS		(1 << 3)	/* Master disable      */
#define TWI_CR_SVEN		(1 << 4)	/* Slave enable        */
#define TWI_CR_SVDIS		(1 << 5)	/* Slave disable       */
#define TWI_CR_QUICK		(1 << 6)	/* SMBUS quick command */
#define TWI_CR_SWRST		(1 << 7)	/* Software reset      */

/*
 * Master Mode Register
 */
#define TWI_MMR_MREAD		(1 << 12)	/* 0 for write, 1 for read */

/* IADR is for internal address. This specifies how many bytes to use. */
#define TWI_MMR_IADRSZ_POS	(8)
#define TWI_MMR_IADRSZ_MASK	(3 << TWI_MMR_IADRSZ_POS)
#define TWI_MMR_IADRSZ_1_BYTE	(1 << 8)
#define TWI_MMR_IADRSZ_2_BYTE	(2 << 8)
#define TWI_MMR_IADRSZ_3_BYTE	(3 << 8)

/* DADR is for destination (slave) address in master mode */
#define TWI_MMR_DADR_POS	(16)
#define TWI_MMR_DADR_MASK	(0x7F << TWI_MMR_DADR_POS)

/*
 * Slave Mode Register
 */
#define TWI_MMR_SADR_POS	(16)
#define TWI_MMR_SADR_MASK	(0x7F << TWI_MMR_DADR_POS)

/*
 * Internal Address Register
 */
#define TWI_IADR_POS		(0)
#define TWI_IADR_MASK		(0xFFFFFF << TWI_IADR_POS)

/*
 * Clock Waveform Generator Register
 */
#define TWI_CWGR_CKDIV_POS	(16)
#define TWI_CWGR_CKDIV_MSGK	(0x07 << TWI_CWGR_CKDIV_POS)

#define TWI_CWGR_CHDIV_POS	(8)
#define TWI_CWGR_CHDIV_MASK	(0xFF << TWI_CWGR_CHDIV_POS)

#define TWI_CWGR_CLDIV_POS	(0)
#define TWI_CWGR_CLDIV_MASK	(0xFF << TWI_CWGR_CLDIV_POS)

/*
 * Status (SR),
 * Interrupt Enable (IER),
 * Interrupt Disable (IDR),
 * Interrupt Mask (IMR) registers
 */
#define TWI_IRQ_TXCOMP		(1 << 0)	/* Transfer complete      */
#define TWI_IRQ_RXRDY		(1 << 1)	/* RX ready               */
#define TWI_IRQ_TXRDY		(1 << 2)	/* TX ready               */
#define TWI_IRQ_SVREAD		(1 << 3)	/* Slave read             */
#define TWI_IRQ_SVACC		(1 << 4)	/* Slave access           */
#define TWI_IRQ_GACC		(1 << 5)	/* General call access    */
#define TWI_IRQ_OVRE		(1 << 6)	/* Overrun error          */
#define TWI_IRQ_NACK		(1 << 8)	/* No ACK                 */
#define TWI_IRQ_ARBLST		(1 << 9)	/* Arbitration lost       */
#define TWI_IRQ_SCLWS		(1 << 10)	/* Clock wait state       */
#define TWI_IRQ_EOSACC		(1 << 11)	/* End of slave access    */
#define TWI_IRQ_ENDRX		(1 << 12)	/* End of RX buffer (PDC) */
#define TWI_IRQ_ENDTX		(1 << 13)	/* End of TX buffer (PDC) */
#define TWI_IRQ_RXBUFF		(1 << 14)	/* RX buffer full (PDC)   */
#define TWI_IRQ_TXBUFE		(1 << 15)	/* TX buffer full (PDC)   */

#define TWI_IRQ_PDC	\
	(TWI_IRQ_ENDRX | TWI_IRQ_ENDTX | TWI_IRQ_RXBUFF | TWI_IRQ_TXBUFE)

/* Bits to disable all interrupts */
#define TWI_IRQ_DISABLE		(0x0000FF77)

/*
 * Receive Holding Register
 */
#define TWI_RHR_POS		(0)
#define TWI_RHR_MASK		(0xFF << TWI_RHR_POS)

/*
 * Transmit Holding Register
 */
#define TWI_THR_POS		(0)
#define TWI_THR_MASK		(0xFF << TWI_THR_POS)

#ifdef __cplusplus
}
#endif

#endif /* _DRIVERS_I2C_ATMEL_SAM3H_ */
