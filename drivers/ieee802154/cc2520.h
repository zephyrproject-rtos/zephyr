/* cc2520.h - IEEE 802.15.4 driver for TI CC2520 */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Copyright (c) 2011, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CC2520_CONST_H__
#define __CC2520_CONST_H__

/*
 * All constants are from the Chipcon cc2520 Data Sheet that at one
 * point in time could be found at
 * http://www.chipcon.com/files/cc2520_Data_Sheet_1_4.pdf
 *
 * The page numbers below refer to pages in this document.
 */

/* Page 27. */
enum cc2520_status_byte {
	CC2520_XOSC16M_STABLE	= 7,
	CC2520_RSSI_VALID	= 6,
	CC2520_EXCEPTION_CHA	= 5,
	CC2520_EXCEPTION_CHB	= 4,
	CC2520_DPU_H		= 3,
	CC2520_DPU_L		= 2,
	CC2520_TX_ACTIVE	= 1,
	CC2520_RX_ACTIVE	= 0,
};

#define TX_FRM_DONE      0x02
#define RX_FRM_DONE      0x01
#define RX_FRM_ABORTED   0x20
#define RX_FRM_UNDERFLOW 0x20

enum cc2520_radio_status {
	CC2520_STATUS_FIFO		= 7,
	CC2520_STATUS_FIFOP		= 6,
	CC2520_STATUS_SFD		= 5,
	CC2520_STATUS_CCA		= 4,
	CC2520_STATUS_SAMPLED_CCA	= 3,
	CC2520_STATUS_LOCK_STATUS	= 2,
	CC2520_STATUS_TX_ACTIVE		= 1,
	CC2520_STATUS_RX_ACTIVE		= 0,
};

/* Page 27. */
enum cc2520_memory_size {
	CC2520_RAM_SIZE		= 640,
	CC2520_FIFO_SIZE	= 128,
};

/* Page 29. */
enum cc2520_address {
	CC2520RAM_TXFIFO	= 0x100,
	CC2520RAM_RXFIFO	= 0x180,
	CC2520RAM_IEEEADDR	= 0x3EA,
	CC2520RAM_PANID		= 0x3F2,
	CC2520RAM_SHORTADDR	= 0x3F4,
};

/* Pages 112 & 113. */
enum cc2520_excflags0 {
	CC2520_EXCFLAGS0_RF_IDLE = 0,
	CC2520_EXCFLAGS0_TX_FRM_DONE,
	CC2520_EXCFLAGS0_TX_ACK_DONE,
	CC2520_EXCFLAGS0_TX_UNDERFLOW,
	CC2520_EXCFLAGS0_TX_OVERFLOW,
	CC2520_EXCFLAGS0_RX_UNDERFLOW,
	CC2520_EXCFLAGS0_RX_OVERFLOW,
	CC2520_EXCFLAGS0_RXENABLE_ZERO,
};

enum cc2520_excflags1 {
	CC2520_EXCFLAGS1_RX_FRM_DONE = 0,
	CC2520_EXCFLAGS1_RX_FRM_ACCEPTED,
	CC2520_EXCFLAGS1_SRC_MATCH_DONE,
	CC2520_EXCFLAGS1_SRC_MATCH_FOUND,
	CC2520_EXCFLAGS1_FIFOP,
	CC2520_EXCFLAGS1_SFD,
	CC2520_EXCFLAGS1_DPU_DONE_L,
	CC2520_EXCFLAGS1_DPU_DONE_H,
};

enum cc2520_excflags2 {
	CC2520_EXCFLAGS2_MEMADDR_ERROR = 0,
	CC2520_EXCFLAGS2_USAGE_ERROR,
	CC2520_EXCFLAGS2_OPERAND_ERROR,
	CC2520_EXCFLAGS2_SPI_ERROR,
	CC2520_EXCFLAGS2_RF_NO_LOCK,
	CC2520_EXCFLAGS2_RX_FRM_ABORTED,
	CC2520_EXCFLAGS2_RFBUFMOV_TIMEOUT,
	CC2520_EXCFLAGS2_UNUSED,
};

#define CC2520_FSMSTAT1_SAMPLED_CCA 0x08

/* IEEE 802.15.4 defined constants (2.4 GHz logical channels) */
#define MIN_CHANNEL		11    /* 2405 MHz */
#define MAX_CHANNEL		26    /* 2480 MHz */
#define CHANNEL_SPACING		5     /* MHz */

/* FREG definitions (BSET/BCLR supported) */
#define CC2520_FRMFILT0                0x000
#define CC2520_FRMFILT1                0x001
#define CC2520_SRCMATCH                0x002
#define CC2520_SRCSHORTEN0             0x004
#define CC2520_SRCSHORTEN1             0x005
#define CC2520_SRCSHORTEN2             0x006
#define CC2520_SRCEXTEN0               0x008
#define CC2520_SRCEXTEN1               0x009
#define CC2520_SRCEXTEN2               0x00A
#define CC2520_FRMCTRL0                0x00C
#define CC2520_FRMCTRL1                0x00D
#define CC2520_RXENABLE0               0x00E
#define CC2520_RXENABLE1               0x00F
#define CC2520_EXCFLAG0                0x010
#define CC2520_EXCFLAG1                0x011
#define CC2520_EXCFLAG2                0x012
#define CC2520_EXCMASKA0               0x014
#define CC2520_EXCMASKA1               0x015
#define CC2520_EXCMASKA2               0x016
#define CC2520_EXCMASKB0               0x018
#define CC2520_EXCMASKB1               0x019
#define CC2520_EXCMASKB2               0x01A
#define CC2520_EXCBINDX0               0x01C
#define CC2520_EXCBINDX1               0x01D
#define CC2520_EXCBINDY0               0x01E
#define CC2520_EXCBINDY1               0x01F
#define CC2520_GPIOCTRL0               0x020
#define CC2520_GPIOCTRL1               0x021
#define CC2520_GPIOCTRL2               0x022
#define CC2520_GPIOCTRL3               0x023
#define CC2520_GPIOCTRL4               0x024
#define CC2520_GPIOCTRL5               0x025
#define CC2520_GPIOPOLARITY            0x026
#define CC2520_GPIOCTRL                0x028
#define CC2520_DPUCON                  0x02A
#define CC2520_DPUSTAT                 0x02C
#define CC2520_FREQCTRL                0x02E
#define CC2520_FREQTUNE                0x02F
#define CC2520_TXPOWER                 0x030
#define CC2520_TXCTRL                  0x031
#define CC2520_FSMSTAT0                0x032
#define CC2520_FSMSTAT1                0x033
#define CC2520_FIFOPCTRL               0x034
#define CC2520_FSMCTRL                 0x035
#define CC2520_CCACTRL0                0x036
#define CC2520_CCACTRL1                0x037
#define CC2520_RSSI                    0x038
#define CC2520_RSSISTAT                0x039
#define CC2520_TXFIFO_BUF              0x03A
#define CC2520_RXFIRST                 0x03C
#define CC2520_RXFIFOCNT               0x03E
#define CC2520_TXFIFOCNT               0x03F

/* SREG definitions (BSET/BCLR unsupported) */
#define CC2520_CHIPID                  0x040
#define CC2520_VERSION                 0x042
#define CC2520_EXTCLOCK                0x044
#define CC2520_MDMCTRL0                0x046
#define CC2520_MDMCTRL1                0x047
#define CC2520_FREQEST                 0x048
#define CC2520_RXCTRL                  0x04A
#define CC2520_FSCTRL                  0x04C
#define CC2520_FSCAL0                  0x04E
#define CC2520_FSCAL1                  0x04F
#define CC2520_FSCAL2                  0x050
#define CC2520_FSCAL3                  0x051
#define CC2520_AGCCTRL0                0x052
#define CC2520_AGCCTRL1                0x053
#define CC2520_AGCCTRL2                0x054
#define CC2520_AGCCTRL3                0x055
#define CC2520_ADCTEST0                0x056
#define CC2520_ADCTEST1                0x057
#define CC2520_ADCTEST2                0x058
#define CC2520_MDMTEST0                0x05A
#define CC2520_MDMTEST1                0x05B
#define CC2520_DACTEST0                0x05C
#define CC2520_DACTEST1                0x05D
#define CC2520_ATEST                   0x05E
#define CC2520_DACTEST2                0x05F
#define CC2520_PTEST0                  0x060
#define CC2520_PTEST1                  0x061
#define CC2520_RESERVED                0x062
#define CC2520_DPUBIST                 0x07A
#define CC2520_ACTBIST                 0x07C
#define CC2520_RAMBIST                 0x07E

/* Instruction implementation */
#define CC2520_INS_SNOP                0x00
#define CC2520_INS_IBUFLD              0x02
#define CC2520_INS_SIBUFEX             0x03
#define CC2520_INS_SSAMPLECCA          0x04
#define CC2520_INS_SRES                0x0F
#define CC2520_INS_MEMRD               0x10
#define CC2520_INS_MEMWR               0x20
#define CC2520_INS_RXBUF               0x30
#define CC2520_INS_RXBUFCP             0x38
#define CC2520_INS_RXBUFMOV            0x32
#define CC2520_INS_TXBUF               0x3A
#define CC2520_INS_TXBUFCP             0x3E
#define CC2520_INS_RANDOM              0x3C
#define CC2520_INS_SXOSCON             0x40
#define CC2520_INS_STXCAL              0x41
#define CC2520_INS_SRXON               0x42
#define CC2520_INS_STXON               0x43
#define CC2520_INS_STXONCCA            0x44
#define CC2520_INS_SRFOFF              0x45
#define CC2520_INS_SXOSCOFF            0x46
#define CC2520_INS_SFLUSHRX            0x47
#define CC2520_INS_SFLUSHTX            0x48
#define CC2520_INS_SACK                0x49
#define CC2520_INS_SACKPEND            0x4A
#define CC2520_INS_SNACK               0x4B
#define CC2520_INS_SRXMASKBITSET       0x4C
#define CC2520_INS_SRXMASKBITCLR       0x4D
#define CC2520_INS_RXMASKAND           0x4E
#define CC2520_INS_RXMASKOR            0x4F
#define CC2520_INS_MEMCP               0x50
#define CC2520_INS_MEMCPR              0x52
#define CC2520_INS_MEMXCP              0x54
#define CC2520_INS_MEMXWR              0x56
#define CC2520_INS_BCLR                0x58
#define CC2520_INS_BSET                0x59
#define CC2520_INS_CTR                 0x60
#define CC2520_INS_CBCMAC              0x64
#define CC2520_INS_UCBCMAC             0x66
#define CC2520_INS_CCM                 0x68
#define CC2520_INS_UCCM                0x6A
#define CC2520_INS_ECB                 0x70
#define CC2520_INS_ECBO                0x72
#define CC2520_INS_ECBX                0x74
#define CC2520_INS_ECBXO               0x76
#define CC2520_INS_INC                 0x78
#define CC2520_INS_ABORT               0x7F
#define CC2520_INS_REGRD               0x80
#define CC2520_INS_REGWR               0xC0

#endif /* __CC2520_CONST_H__ */
