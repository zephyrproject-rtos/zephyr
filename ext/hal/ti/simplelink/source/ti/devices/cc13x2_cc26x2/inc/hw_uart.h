/******************************************************************************
*  Filename:       hw_uart_h
*  Revised:        2018-05-14 12:24:52 +0200 (Mon, 14 May 2018)
*  Revision:       51990
*
* Copyright (c) 2015 - 2017, Texas Instruments Incorporated
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1) Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2) Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* 3) Neither the name of the ORGANIZATION nor the names of its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
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
*
******************************************************************************/

#ifndef __HW_UART_H__
#define __HW_UART_H__

//*****************************************************************************
//
// This section defines the register offsets of
// UART component
//
//*****************************************************************************
// Data
#define UART_O_DR                                                   0x00000000

// Status
#define UART_O_RSR                                                  0x00000004

// Error Clear
#define UART_O_ECR                                                  0x00000004

// Flag
#define UART_O_FR                                                   0x00000018

// Integer Baud-Rate Divisor
#define UART_O_IBRD                                                 0x00000024

// Fractional Baud-Rate Divisor
#define UART_O_FBRD                                                 0x00000028

// Line Control
#define UART_O_LCRH                                                 0x0000002C

// Control
#define UART_O_CTL                                                  0x00000030

// Interrupt FIFO Level Select
#define UART_O_IFLS                                                 0x00000034

// Interrupt Mask Set/Clear
#define UART_O_IMSC                                                 0x00000038

// Raw Interrupt Status
#define UART_O_RIS                                                  0x0000003C

// Masked Interrupt Status
#define UART_O_MIS                                                  0x00000040

// Interrupt Clear
#define UART_O_ICR                                                  0x00000044

// DMA Control
#define UART_O_DMACTL                                               0x00000048

//*****************************************************************************
//
// Register: UART_O_DR
//
//*****************************************************************************
// Field:    [11] OE
//
// UART Overrun Error:
// This bit is set to 1 if data is received and the receive FIFO is already
// full. The FIFO contents remain valid because no more data is written when
// the FIFO is full, , only the contents of the shift register are overwritten.
// This is cleared to 0 once there is an empty space in the FIFO and a new
// character can be written to it.
#define UART_DR_OE                                                  0x00000800
#define UART_DR_OE_BITN                                                     11
#define UART_DR_OE_M                                                0x00000800
#define UART_DR_OE_S                                                        11

// Field:    [10] BE
//
// UART Break Error:
// This bit is set to 1 if a break condition was detected, indicating that the
// received data input (UARTRXD input pin) was held LOW for longer than a
// full-word transmission time (defined as start, data, parity and stop bits).
// In FIFO mode, this error is associated with the character at the top of the
// FIFO (that is., the oldest received data character since last read). When a
// break occurs, a 0 character is loaded into the FIFO. The next character is
// enabled after the receive data input (UARTRXD input pin) goes to a 1
// (marking state), and the next valid start bit is received.
#define UART_DR_BE                                                  0x00000400
#define UART_DR_BE_BITN                                                     10
#define UART_DR_BE_M                                                0x00000400
#define UART_DR_BE_S                                                        10

// Field:     [9] PE
//
// UART Parity Error:
// When set to 1, it indicates that the parity of the received data character
// does not match the parity that the LCRH.EPS and LCRH.SPS select.
// In FIFO mode, this error is associated with the character at the top of the
// FIFO (that is, the oldest received data character since last read).
#define UART_DR_PE                                                  0x00000200
#define UART_DR_PE_BITN                                                      9
#define UART_DR_PE_M                                                0x00000200
#define UART_DR_PE_S                                                         9

// Field:     [8] FE
//
// UART Framing Error:
// When set to 1, it indicates that the received character did not have a valid
// stop bit (a valid stop bit is 1).
// In FIFO mode, this error is associated with the character at the top of the
// FIFO (that is., the oldest received data character since last read).
#define UART_DR_FE                                                  0x00000100
#define UART_DR_FE_BITN                                                      8
#define UART_DR_FE_M                                                0x00000100
#define UART_DR_FE_S                                                         8

// Field:   [7:0] DATA
//
// Data transmitted or received:
// On writes, the transmit data character is pushed into the FIFO.
// On reads, the oldest received data character since the last read is
// returned.
#define UART_DR_DATA_W                                                       8
#define UART_DR_DATA_M                                              0x000000FF
#define UART_DR_DATA_S                                                       0

//*****************************************************************************
//
// Register: UART_O_RSR
//
//*****************************************************************************
// Field:     [3] OE
//
// UART Overrun Error:
// This bit is set to 1 if data is received and the receive FIFO is already
// full. The FIFO contents remain valid because no more data is written when
// the FIFO is full, , only the contents of the shift register are overwritten.
// This is cleared to 0 once there is an empty space in the FIFO and a new
// character can be written to it.
#define UART_RSR_OE                                                 0x00000008
#define UART_RSR_OE_BITN                                                     3
#define UART_RSR_OE_M                                               0x00000008
#define UART_RSR_OE_S                                                        3

// Field:     [2] BE
//
// UART Break Error:
// This bit is set to 1 if a break condition was detected, indicating that the
// received data input (UARTRXD input pin) was held LOW for longer than a
// full-word transmission time (defined as start, data, parity and stop bits).
// When a break occurs, a 0 character is loaded into the FIFO. The next
// character is enabled after the receive data input (UARTRXD input pin) goes
// to a 1 (marking state), and the next valid start bit is received.
#define UART_RSR_BE                                                 0x00000004
#define UART_RSR_BE_BITN                                                     2
#define UART_RSR_BE_M                                               0x00000004
#define UART_RSR_BE_S                                                        2

// Field:     [1] PE
//
// UART Parity Error:
// When set to 1, it indicates that the parity of the received data character
// does not match the parity that the LCRH.EPS and LCRH.SPS select.
#define UART_RSR_PE                                                 0x00000002
#define UART_RSR_PE_BITN                                                     1
#define UART_RSR_PE_M                                               0x00000002
#define UART_RSR_PE_S                                                        1

// Field:     [0] FE
//
// UART Framing Error:
// When set to 1, it indicates that the received character did not have a valid
// stop bit (a valid stop bit is 1).
#define UART_RSR_FE                                                 0x00000001
#define UART_RSR_FE_BITN                                                     0
#define UART_RSR_FE_M                                               0x00000001
#define UART_RSR_FE_S                                                        0

//*****************************************************************************
//
// Register: UART_O_ECR
//
//*****************************************************************************
// Field:     [3] OE
//
// The framing (FE), parity (PE), break (BE) and overrun (OE) errors are
// cleared to 0 by any write to this register.
#define UART_ECR_OE                                                 0x00000008
#define UART_ECR_OE_BITN                                                     3
#define UART_ECR_OE_M                                               0x00000008
#define UART_ECR_OE_S                                                        3

// Field:     [2] BE
//
// The framing (FE), parity (PE), break (BE) and overrun (OE) errors are
// cleared to 0 by any write to this register.
#define UART_ECR_BE                                                 0x00000004
#define UART_ECR_BE_BITN                                                     2
#define UART_ECR_BE_M                                               0x00000004
#define UART_ECR_BE_S                                                        2

// Field:     [1] PE
//
// The framing (FE), parity (PE), break (BE) and overrun (OE) errors are
// cleared to 0 by any write to this register.
#define UART_ECR_PE                                                 0x00000002
#define UART_ECR_PE_BITN                                                     1
#define UART_ECR_PE_M                                               0x00000002
#define UART_ECR_PE_S                                                        1

// Field:     [0] FE
//
// The framing (FE), parity (PE), break (BE) and overrun (OE) errors are
// cleared to 0 by any write to this register.
#define UART_ECR_FE                                                 0x00000001
#define UART_ECR_FE_BITN                                                     0
#define UART_ECR_FE_M                                               0x00000001
#define UART_ECR_FE_S                                                        0

//*****************************************************************************
//
// Register: UART_O_FR
//
//*****************************************************************************
// Field:     [7] TXFE
//
// UART Transmit FIFO Empty:
// The meaning of this bit depends on the state of LCRH.FEN .
//   - If the FIFO is disabled, this bit is set when the transmit holding
// register is empty.
//   - If the FIFO is enabled, this bit is set when the transmit FIFO is empty.
// This bit does not indicate if there is data in the transmit shift register.
#define UART_FR_TXFE                                                0x00000080
#define UART_FR_TXFE_BITN                                                    7
#define UART_FR_TXFE_M                                              0x00000080
#define UART_FR_TXFE_S                                                       7

// Field:     [6] RXFF
//
// UART Receive FIFO Full:
// The meaning of this bit depends on the state of LCRH.FEN.
//   - If the FIFO is disabled, this bit is set when the receive holding
// register is full.
//   - If the FIFO is enabled, this bit is set when the receive FIFO is full.
#define UART_FR_RXFF                                                0x00000040
#define UART_FR_RXFF_BITN                                                    6
#define UART_FR_RXFF_M                                              0x00000040
#define UART_FR_RXFF_S                                                       6

// Field:     [5] TXFF
//
// UART Transmit FIFO Full:
// Transmit FIFO full. The meaning of this bit depends on the state of
// LCRH.FEN.
//   - If the FIFO is disabled, this bit is set when the transmit holding
// register is full.
//   - If the FIFO is enabled, this bit is set when the transmit FIFO is full.
#define UART_FR_TXFF                                                0x00000020
#define UART_FR_TXFF_BITN                                                    5
#define UART_FR_TXFF_M                                              0x00000020
#define UART_FR_TXFF_S                                                       5

// Field:     [4] RXFE
//
// UART Receive FIFO Empty:
// Receive FIFO empty. The meaning of this bit depends on the state of
// LCRH.FEN.
//   - If the FIFO is disabled, this bit is set when the receive holding
// register is empty.
//   - If the FIFO is enabled, this bit is set when the receive FIFO is empty.
#define UART_FR_RXFE                                                0x00000010
#define UART_FR_RXFE_BITN                                                    4
#define UART_FR_RXFE_M                                              0x00000010
#define UART_FR_RXFE_S                                                       4

// Field:     [3] BUSY
//
// UART Busy:
// If this bit is set to 1, the UART is busy transmitting data. This bit
// remains set until the complete byte, including all the stop bits, has been
// sent from the shift register.
// This bit is set as soon as the transmit FIFO becomes non-empty, regardless
// of whether the UART is enabled or not.
#define UART_FR_BUSY                                                0x00000008
#define UART_FR_BUSY_BITN                                                    3
#define UART_FR_BUSY_M                                              0x00000008
#define UART_FR_BUSY_S                                                       3

// Field:     [0] CTS
//
// Clear To Send:
// This bit is the complement of the active-low UART CTS input pin.
// That is, the bit is 1 when CTS input pin is LOW.
#define UART_FR_CTS                                                 0x00000001
#define UART_FR_CTS_BITN                                                     0
#define UART_FR_CTS_M                                               0x00000001
#define UART_FR_CTS_S                                                        0

//*****************************************************************************
//
// Register: UART_O_IBRD
//
//*****************************************************************************
// Field:  [15:0] DIVINT
//
// The integer baud rate divisor:
// The baud rate divisor is calculated using the formula below:
// Baud rate divisor = (UART reference clock frequency) / (16 * Baud rate)
// Baud rate divisor must be minimum 1 and maximum 65535.
// That is, DIVINT=0 does not give a valid baud rate.
// Similarly, if DIVINT=0xFFFF, any non-zero values in FBRD.DIVFRAC will be
// illegal.
// A valid value must be written to this field before the UART can be used for
// RX or TX operations.
#define UART_IBRD_DIVINT_W                                                  16
#define UART_IBRD_DIVINT_M                                          0x0000FFFF
#define UART_IBRD_DIVINT_S                                                   0

//*****************************************************************************
//
// Register: UART_O_FBRD
//
//*****************************************************************************
// Field:   [5:0] DIVFRAC
//
// Fractional Baud-Rate Divisor:
// The baud rate divisor is calculated using the formula below:
// Baud rate divisor = (UART reference clock frequency) / (16 * Baud rate)
// Baud rate divisor must be minimum 1 and maximum 65535.
// That is, IBRD.DIVINT=0 does not give a valid baud rate.
// Similarly, if IBRD.DIVINT=0xFFFF, any non-zero values in DIVFRAC will be
// illegal.
// A valid value must be written to this field before the UART can be used for
// RX or TX operations.
#define UART_FBRD_DIVFRAC_W                                                  6
#define UART_FBRD_DIVFRAC_M                                         0x0000003F
#define UART_FBRD_DIVFRAC_S                                                  0

//*****************************************************************************
//
// Register: UART_O_LCRH
//
//*****************************************************************************
// Field:     [7] SPS
//
// UART Stick Parity Select:
//
// 0: Stick parity is disabled
// 1: The parity bit is transmitted and checked as invert of EPS field (i.e.
// the parity bit is transmitted and checked as 1 when EPS = 0).
//
// This bit has no effect when PEN disables parity checking and generation.
#define UART_LCRH_SPS                                               0x00000080
#define UART_LCRH_SPS_BITN                                                   7
#define UART_LCRH_SPS_M                                             0x00000080
#define UART_LCRH_SPS_S                                                      7

// Field:   [6:5] WLEN
//
// UART Word Length:
// These bits indicate the number of data bits transmitted or received in a
// frame.
// ENUMs:
// 8                        Word Length 8 bits
// 7                        Word Length 7 bits
// 6                        Word Length 6 bits
// 5                        Word Length 5 bits
#define UART_LCRH_WLEN_W                                                     2
#define UART_LCRH_WLEN_M                                            0x00000060
#define UART_LCRH_WLEN_S                                                     5
#define UART_LCRH_WLEN_8                                            0x00000060
#define UART_LCRH_WLEN_7                                            0x00000040
#define UART_LCRH_WLEN_6                                            0x00000020
#define UART_LCRH_WLEN_5                                            0x00000000

// Field:     [4] FEN
//
// UART Enable FIFOs
// ENUMs:
// EN                       Transmit and receive FIFO buffers are enabled
//                          (FIFO mode)
// DIS                      FIFOs are disabled (character mode) that is, the
//                          FIFOs become 1-byte-deep holding registers.
#define UART_LCRH_FEN                                               0x00000010
#define UART_LCRH_FEN_BITN                                                   4
#define UART_LCRH_FEN_M                                             0x00000010
#define UART_LCRH_FEN_S                                                      4
#define UART_LCRH_FEN_EN                                            0x00000010
#define UART_LCRH_FEN_DIS                                           0x00000000

// Field:     [3] STP2
//
// UART Two Stop Bits Select:
// If this bit is set to 1, two stop bits are transmitted at the end of the
// frame. The receive logic does not check for two stop bits being received.
#define UART_LCRH_STP2                                              0x00000008
#define UART_LCRH_STP2_BITN                                                  3
#define UART_LCRH_STP2_M                                            0x00000008
#define UART_LCRH_STP2_S                                                     3

// Field:     [2] EPS
//
// UART Even Parity Select
// ENUMs:
// EVEN                     Even parity: The UART generates or checks for an
//                          even number of 1s in the data and parity bits.
// ODD                      Odd parity: The UART generates or checks for an
//                          odd number of 1s in the data and parity bits.
#define UART_LCRH_EPS                                               0x00000004
#define UART_LCRH_EPS_BITN                                                   2
#define UART_LCRH_EPS_M                                             0x00000004
#define UART_LCRH_EPS_S                                                      2
#define UART_LCRH_EPS_EVEN                                          0x00000004
#define UART_LCRH_EPS_ODD                                           0x00000000

// Field:     [1] PEN
//
// UART Parity Enable
// This bit controls generation and checking of parity bit.
// ENUMs:
// EN                       Parity checking and generation is enabled.
// DIS                      Parity is disabled and no parity bit is added to
//                          the data frame
#define UART_LCRH_PEN                                               0x00000002
#define UART_LCRH_PEN_BITN                                                   1
#define UART_LCRH_PEN_M                                             0x00000002
#define UART_LCRH_PEN_S                                                      1
#define UART_LCRH_PEN_EN                                            0x00000002
#define UART_LCRH_PEN_DIS                                           0x00000000

// Field:     [0] BRK
//
// UART Send Break
// If this bit is set to 1, a low-level is continually output on the UARTTXD
// output pin, after completing transmission of the current character. For the
// proper execution of the break command, the
// software must set this bit for at least two complete frames. For normal use,
// this bit must be cleared to 0.
#define UART_LCRH_BRK                                               0x00000001
#define UART_LCRH_BRK_BITN                                                   0
#define UART_LCRH_BRK_M                                             0x00000001
#define UART_LCRH_BRK_S                                                      0

//*****************************************************************************
//
// Register: UART_O_CTL
//
//*****************************************************************************
// Field:    [15] CTSEN
//
// CTS hardware flow control enable
// ENUMs:
// EN                       CTS hardware flow control enabled
// DIS                      CTS hardware flow control disabled
#define UART_CTL_CTSEN                                              0x00008000
#define UART_CTL_CTSEN_BITN                                                 15
#define UART_CTL_CTSEN_M                                            0x00008000
#define UART_CTL_CTSEN_S                                                    15
#define UART_CTL_CTSEN_EN                                           0x00008000
#define UART_CTL_CTSEN_DIS                                          0x00000000

// Field:    [14] RTSEN
//
// RTS hardware flow control enable
// ENUMs:
// EN                       RTS hardware flow control enabled
// DIS                      RTS hardware flow control disabled
#define UART_CTL_RTSEN                                              0x00004000
#define UART_CTL_RTSEN_BITN                                                 14
#define UART_CTL_RTSEN_M                                            0x00004000
#define UART_CTL_RTSEN_S                                                    14
#define UART_CTL_RTSEN_EN                                           0x00004000
#define UART_CTL_RTSEN_DIS                                          0x00000000

// Field:    [11] RTS
//
// Request to Send
// This bit is the complement of the active-low UART RTS output. That is, when
// the bit is programmed to a 1 then RTS output on the pins is LOW.
#define UART_CTL_RTS                                                0x00000800
#define UART_CTL_RTS_BITN                                                   11
#define UART_CTL_RTS_M                                              0x00000800
#define UART_CTL_RTS_S                                                      11

// Field:     [9] RXE
//
// UART Receive Enable
// If the UART is disabled in the middle of reception, it completes the current
// character before stopping.
// ENUMs:
// EN                       UART Receive enabled
// DIS                      UART Receive disabled
#define UART_CTL_RXE                                                0x00000200
#define UART_CTL_RXE_BITN                                                    9
#define UART_CTL_RXE_M                                              0x00000200
#define UART_CTL_RXE_S                                                       9
#define UART_CTL_RXE_EN                                             0x00000200
#define UART_CTL_RXE_DIS                                            0x00000000

// Field:     [8] TXE
//
// UART Transmit Enable
// If the UART is disabled in the middle of transmission, it completes the
// current character before stopping.
// ENUMs:
// EN                       UART Transmit enabled
// DIS                      UART Transmit disabled
#define UART_CTL_TXE                                                0x00000100
#define UART_CTL_TXE_BITN                                                    8
#define UART_CTL_TXE_M                                              0x00000100
#define UART_CTL_TXE_S                                                       8
#define UART_CTL_TXE_EN                                             0x00000100
#define UART_CTL_TXE_DIS                                            0x00000000

// Field:     [7] LBE
//
// UART Loop Back Enable:
// Enabling the loop-back mode connects the UARTTXD output from the UART to
// UARTRXD input of the UART.
// ENUMs:
// EN                       Loop Back enabled
// DIS                      Loop Back disabled
#define UART_CTL_LBE                                                0x00000080
#define UART_CTL_LBE_BITN                                                    7
#define UART_CTL_LBE_M                                              0x00000080
#define UART_CTL_LBE_S                                                       7
#define UART_CTL_LBE_EN                                             0x00000080
#define UART_CTL_LBE_DIS                                            0x00000000

// Field:     [0] UARTEN
//
// UART Enable
// ENUMs:
// EN                       UART enabled
// DIS                      UART disabled
#define UART_CTL_UARTEN                                             0x00000001
#define UART_CTL_UARTEN_BITN                                                 0
#define UART_CTL_UARTEN_M                                           0x00000001
#define UART_CTL_UARTEN_S                                                    0
#define UART_CTL_UARTEN_EN                                          0x00000001
#define UART_CTL_UARTEN_DIS                                         0x00000000

//*****************************************************************************
//
// Register: UART_O_IFLS
//
//*****************************************************************************
// Field:   [5:3] RXSEL
//
// Receive interrupt FIFO level select:
// This field sets the trigger points for the receive interrupt. Values
// 0b101-0b111 are reserved.
// ENUMs:
// 7_8                      Receive FIFO becomes >= 7/8 full
// 6_8                      Receive FIFO becomes >= 3/4 full
// 4_8                      Receive FIFO becomes >= 1/2 full
// 2_8                      Receive FIFO becomes >= 1/4 full
// 1_8                      Receive FIFO becomes >= 1/8 full
#define UART_IFLS_RXSEL_W                                                    3
#define UART_IFLS_RXSEL_M                                           0x00000038
#define UART_IFLS_RXSEL_S                                                    3
#define UART_IFLS_RXSEL_7_8                                         0x00000020
#define UART_IFLS_RXSEL_6_8                                         0x00000018
#define UART_IFLS_RXSEL_4_8                                         0x00000010
#define UART_IFLS_RXSEL_2_8                                         0x00000008
#define UART_IFLS_RXSEL_1_8                                         0x00000000

// Field:   [2:0] TXSEL
//
// Transmit interrupt FIFO level select:
// This field sets the trigger points for the transmit interrupt. Values
// 0b101-0b111 are reserved.
// ENUMs:
// 7_8                      Transmit FIFO becomes <= 7/8 full
// 6_8                      Transmit FIFO becomes <= 3/4 full
// 4_8                      Transmit FIFO becomes <= 1/2 full
// 2_8                      Transmit FIFO becomes <= 1/4 full
// 1_8                      Transmit FIFO becomes <= 1/8 full
#define UART_IFLS_TXSEL_W                                                    3
#define UART_IFLS_TXSEL_M                                           0x00000007
#define UART_IFLS_TXSEL_S                                                    0
#define UART_IFLS_TXSEL_7_8                                         0x00000004
#define UART_IFLS_TXSEL_6_8                                         0x00000003
#define UART_IFLS_TXSEL_4_8                                         0x00000002
#define UART_IFLS_TXSEL_2_8                                         0x00000001
#define UART_IFLS_TXSEL_1_8                                         0x00000000

//*****************************************************************************
//
// Register: UART_O_IMSC
//
//*****************************************************************************
// Field:    [11] EOTIM
//
// End of Transmission interrupt mask. A read returns the current mask for
// UART's EoT interrupt. On a write of 1, the mask of the EoT interrupt is set
// which means the interrupt state will be reflected in MIS.EOTMIS. A write of
// 0 clears the mask which means MIS.EOTMIS will not reflect the interrupt.
#define UART_IMSC_EOTIM                                             0x00000800
#define UART_IMSC_EOTIM_BITN                                                11
#define UART_IMSC_EOTIM_M                                           0x00000800
#define UART_IMSC_EOTIM_S                                                   11

// Field:    [10] OEIM
//
// Overrun error interrupt mask. A read returns the current mask for UART's
// overrun error interrupt. On a write of 1, the mask of the overrun error
// interrupt is set which means the interrupt state will be reflected in
// MIS.OEMIS. A write of 0 clears the mask which means MIS.OEMIS will not
// reflect the interrupt.
#define UART_IMSC_OEIM                                              0x00000400
#define UART_IMSC_OEIM_BITN                                                 10
#define UART_IMSC_OEIM_M                                            0x00000400
#define UART_IMSC_OEIM_S                                                    10

// Field:     [9] BEIM
//
// Break error interrupt mask. A read returns the current mask for UART's break
// error interrupt. On a write of 1, the mask of the overrun error interrupt is
// set which means the interrupt state will be reflected in MIS.BEMIS. A write
// of 0 clears the mask which means MIS.BEMIS will not reflect the interrupt.
#define UART_IMSC_BEIM                                              0x00000200
#define UART_IMSC_BEIM_BITN                                                  9
#define UART_IMSC_BEIM_M                                            0x00000200
#define UART_IMSC_BEIM_S                                                     9

// Field:     [8] PEIM
//
// Parity error interrupt mask. A read returns the current mask for UART's
// parity error interrupt. On a write of 1, the mask of the overrun error
// interrupt is set which means the interrupt state will be reflected in
// MIS.PEMIS. A write of 0 clears the mask which means MIS.PEMIS will not
// reflect the interrupt.
#define UART_IMSC_PEIM                                              0x00000100
#define UART_IMSC_PEIM_BITN                                                  8
#define UART_IMSC_PEIM_M                                            0x00000100
#define UART_IMSC_PEIM_S                                                     8

// Field:     [7] FEIM
//
// Framing error interrupt mask. A read returns the current mask for UART's
// framing error interrupt. On a write of 1, the mask of the overrun error
// interrupt is set which means the interrupt state will be reflected in
// MIS.FEMIS. A write of 0 clears the mask which means MIS.FEMIS will not
// reflect the interrupt.
#define UART_IMSC_FEIM                                              0x00000080
#define UART_IMSC_FEIM_BITN                                                  7
#define UART_IMSC_FEIM_M                                            0x00000080
#define UART_IMSC_FEIM_S                                                     7

// Field:     [6] RTIM
//
// Receive timeout interrupt mask. A read returns the current mask for UART's
// receive timeout interrupt. On a write of 1, the mask of the overrun error
// interrupt is set which means the interrupt state will be reflected in
// MIS.RTMIS. A write of 0 clears the mask which means this bitfield will not
// reflect the interrupt.
// The raw interrupt for receive timeout RIS.RTRIS cannot be set unless the
// mask is set (RTIM = 1). This is because the mask acts as an enable for power
// saving. That is, the same status can be read from MIS.RTMIS and RIS.RTRIS.
#define UART_IMSC_RTIM                                              0x00000040
#define UART_IMSC_RTIM_BITN                                                  6
#define UART_IMSC_RTIM_M                                            0x00000040
#define UART_IMSC_RTIM_S                                                     6

// Field:     [5] TXIM
//
// Transmit interrupt mask. A read returns the current mask for UART's transmit
// interrupt. On a write of 1, the mask of the overrun error interrupt is set
// which means the interrupt state will be reflected in MIS.TXMIS. A write of 0
// clears the mask which means MIS.TXMIS will not reflect the interrupt.
#define UART_IMSC_TXIM                                              0x00000020
#define UART_IMSC_TXIM_BITN                                                  5
#define UART_IMSC_TXIM_M                                            0x00000020
#define UART_IMSC_TXIM_S                                                     5

// Field:     [4] RXIM
//
// Receive interrupt mask. A read returns the current mask for UART's receive
// interrupt. On a write of 1, the mask of the overrun error interrupt is set
// which means the interrupt state will be reflected in MIS.RXMIS. A write of 0
// clears the mask which means MIS.RXMIS will not reflect the interrupt.
#define UART_IMSC_RXIM                                              0x00000010
#define UART_IMSC_RXIM_BITN                                                  4
#define UART_IMSC_RXIM_M                                            0x00000010
#define UART_IMSC_RXIM_S                                                     4

// Field:     [1] CTSMIM
//
// Clear to Send (CTS) modem interrupt mask. A read returns the current mask
// for UART's clear to send interrupt. On a write of 1, the mask of the overrun
// error interrupt is set which means the interrupt state will be reflected in
// MIS.CTSMMIS. A write of 0 clears the mask which means MIS.CTSMMIS will not
// reflect the interrupt.
#define UART_IMSC_CTSMIM                                            0x00000002
#define UART_IMSC_CTSMIM_BITN                                                1
#define UART_IMSC_CTSMIM_M                                          0x00000002
#define UART_IMSC_CTSMIM_S                                                   1

//*****************************************************************************
//
// Register: UART_O_RIS
//
//*****************************************************************************
// Field:    [11] EOTRIS
//
// End of Transmission interrupt status:
// This field returns the raw interrupt state of UART's end of transmission
// interrupt. End of transmission flag is set when all the Transmit data in the
// FIFO and on the TX Line is tranmitted.
#define UART_RIS_EOTRIS                                             0x00000800
#define UART_RIS_EOTRIS_BITN                                                11
#define UART_RIS_EOTRIS_M                                           0x00000800
#define UART_RIS_EOTRIS_S                                                   11

// Field:    [10] OERIS
//
// Overrun error interrupt status:
// This field returns the raw interrupt state of UART's overrun error
// interrupt. Overrun error occurs if data is received and the receive FIFO is
// full.
#define UART_RIS_OERIS                                              0x00000400
#define UART_RIS_OERIS_BITN                                                 10
#define UART_RIS_OERIS_M                                            0x00000400
#define UART_RIS_OERIS_S                                                    10

// Field:     [9] BERIS
//
// Break error interrupt status:
// This field returns the raw interrupt state of UART's break error interrupt.
// Break error is set when a break condition is detected, indicating that the
// received data input (UARTRXD input pin) was held LOW for longer than a
// full-word transmission time (defined as start, data, parity and stop bits).
#define UART_RIS_BERIS                                              0x00000200
#define UART_RIS_BERIS_BITN                                                  9
#define UART_RIS_BERIS_M                                            0x00000200
#define UART_RIS_BERIS_S                                                     9

// Field:     [8] PERIS
//
// Parity error interrupt status:
// This field returns the raw interrupt state of UART's parity error interrupt.
// Parity error is set if the parity of the received data character does not
// match the parity that the LCRH.EPS and LCRH.SPS select.
#define UART_RIS_PERIS                                              0x00000100
#define UART_RIS_PERIS_BITN                                                  8
#define UART_RIS_PERIS_M                                            0x00000100
#define UART_RIS_PERIS_S                                                     8

// Field:     [7] FERIS
//
// Framing error interrupt status:
// This field returns the raw interrupt state of UART's framing error
// interrupt. Framing error is set if the received character does not have a
// valid stop bit (a valid stop bit is 1).
#define UART_RIS_FERIS                                              0x00000080
#define UART_RIS_FERIS_BITN                                                  7
#define UART_RIS_FERIS_M                                            0x00000080
#define UART_RIS_FERIS_S                                                     7

// Field:     [6] RTRIS
//
// Receive timeout interrupt status:
// This field returns the raw interrupt state of UART's receive timeout
// interrupt. The receive timeout interrupt is asserted when the receive FIFO
// is not empty, and no more data is received during a 32-bit period. The
// receive timeout interrupt is cleared either when the FIFO becomes empty
// through reading all the data, or when a 1 is written to ICR.RTIC.
// The raw interrupt for receive timeout cannot be set unless the mask is set
// (IMSC.RTIM = 1). This is because the mask acts as an enable for power
// saving. That is, the same status can be read from MIS.RTMIS and RTRIS.
#define UART_RIS_RTRIS                                              0x00000040
#define UART_RIS_RTRIS_BITN                                                  6
#define UART_RIS_RTRIS_M                                            0x00000040
#define UART_RIS_RTRIS_S                                                     6

// Field:     [5] TXRIS
//
// Transmit interrupt status:
// This field returns the raw interrupt state of UART's transmit interrupt.
// When FIFOs are enabled (LCRH.FEN = 1), the transmit interrupt is asserted if
// the number of bytes in transmit FIFO is equal to or lower than the
// programmed trigger level (IFLS.TXSEL). The transmit interrupt is cleared by
// writing data to the transmit FIFO until it becomes greater than the trigger
// level, or by clearing the interrupt through ICR.TXIC.
// When FIFOs are disabled (LCRH.FEN = 0), that is they have a depth of one
// location, the transmit interrupt is asserted if there is no data present in
// the transmitters single location. It is cleared by performing a single write
// to the transmit FIFO, or by clearing the interrupt through ICR.TXIC.
#define UART_RIS_TXRIS                                              0x00000020
#define UART_RIS_TXRIS_BITN                                                  5
#define UART_RIS_TXRIS_M                                            0x00000020
#define UART_RIS_TXRIS_S                                                     5

// Field:     [4] RXRIS
//
// Receive interrupt status:
// This field returns the raw interrupt state of UART's receive interrupt.
// When FIFOs are enabled (LCRH.FEN = 1), the receive interrupt is asserted if
// the receive FIFO reaches the programmed trigger
// level (IFLS.RXSEL). The receive interrupt is cleared by reading data from
// the receive FIFO until it becomes less than the trigger level, or by
// clearing the interrupt through ICR.RXIC.
// When FIFOs are disabled (LCRH.FEN = 0), that is they have a depth of one
// location, the receive interrupt is asserted if data is received
// thereby filling the location. The receive interrupt is cleared by performing
// a single read of the receive FIFO, or by clearing the interrupt through
// ICR.RXIC.
#define UART_RIS_RXRIS                                              0x00000010
#define UART_RIS_RXRIS_BITN                                                  4
#define UART_RIS_RXRIS_M                                            0x00000010
#define UART_RIS_RXRIS_S                                                     4

// Field:     [1] CTSRMIS
//
// Clear to Send (CTS) modem interrupt status:
// This field returns the raw interrupt state of UART's clear to send
// interrupt.
#define UART_RIS_CTSRMIS                                            0x00000002
#define UART_RIS_CTSRMIS_BITN                                                1
#define UART_RIS_CTSRMIS_M                                          0x00000002
#define UART_RIS_CTSRMIS_S                                                   1

//*****************************************************************************
//
// Register: UART_O_MIS
//
//*****************************************************************************
// Field:    [11] EOTMIS
//
// End of Transmission interrupt status:
// This field returns the masked interrupt state of the overrun interrupt which
// is the AND product of raw interrupt state RIS.EOTRIS and the mask setting
// IMSC.EOTIM.
#define UART_MIS_EOTMIS                                             0x00000800
#define UART_MIS_EOTMIS_BITN                                                11
#define UART_MIS_EOTMIS_M                                           0x00000800
#define UART_MIS_EOTMIS_S                                                   11

// Field:    [10] OEMIS
//
// Overrun error masked interrupt status:
// This field returns the masked interrupt state of the overrun interrupt which
// is the AND product of raw interrupt state RIS.OERIS and the mask setting
// IMSC.OEIM.
#define UART_MIS_OEMIS                                              0x00000400
#define UART_MIS_OEMIS_BITN                                                 10
#define UART_MIS_OEMIS_M                                            0x00000400
#define UART_MIS_OEMIS_S                                                    10

// Field:     [9] BEMIS
//
// Break error masked interrupt status:
// This field returns the masked interrupt state of the break error interrupt
// which is the AND product of raw interrupt state RIS.BERIS and the mask
// setting IMSC.BEIM.
#define UART_MIS_BEMIS                                              0x00000200
#define UART_MIS_BEMIS_BITN                                                  9
#define UART_MIS_BEMIS_M                                            0x00000200
#define UART_MIS_BEMIS_S                                                     9

// Field:     [8] PEMIS
//
// Parity error masked interrupt status:
// This field returns the masked interrupt state of the parity error interrupt
// which is the AND product of raw interrupt state RIS.PERIS and the mask
// setting IMSC.PEIM.
#define UART_MIS_PEMIS                                              0x00000100
#define UART_MIS_PEMIS_BITN                                                  8
#define UART_MIS_PEMIS_M                                            0x00000100
#define UART_MIS_PEMIS_S                                                     8

// Field:     [7] FEMIS
//
// Framing error masked interrupt status: Returns the masked interrupt state of
// the framing error interrupt which is the AND product of raw interrupt state
// RIS.FERIS and the mask setting IMSC.FEIM.
#define UART_MIS_FEMIS                                              0x00000080
#define UART_MIS_FEMIS_BITN                                                  7
#define UART_MIS_FEMIS_M                                            0x00000080
#define UART_MIS_FEMIS_S                                                     7

// Field:     [6] RTMIS
//
// Receive timeout masked interrupt status:
// Returns the masked interrupt state of the receive timeout interrupt.
// The raw interrupt for receive timeout cannot be set unless the mask is set
// (IMSC.RTIM = 1). This is because the mask acts as an enable for power
// saving. That is, the same status can be read from RTMIS and RIS.RTRIS.
#define UART_MIS_RTMIS                                              0x00000040
#define UART_MIS_RTMIS_BITN                                                  6
#define UART_MIS_RTMIS_M                                            0x00000040
#define UART_MIS_RTMIS_S                                                     6

// Field:     [5] TXMIS
//
// Transmit masked interrupt status:
// This field returns the masked interrupt state of the transmit interrupt
// which is the AND product of raw interrupt state RIS.TXRIS and the mask
// setting IMSC.TXIM.
#define UART_MIS_TXMIS                                              0x00000020
#define UART_MIS_TXMIS_BITN                                                  5
#define UART_MIS_TXMIS_M                                            0x00000020
#define UART_MIS_TXMIS_S                                                     5

// Field:     [4] RXMIS
//
// Receive masked interrupt status:
// This field returns the masked interrupt state of the receive interrupt
// which is the AND product of raw interrupt state RIS.RXRIS and the mask
// setting IMSC.RXIM.
#define UART_MIS_RXMIS                                              0x00000010
#define UART_MIS_RXMIS_BITN                                                  4
#define UART_MIS_RXMIS_M                                            0x00000010
#define UART_MIS_RXMIS_S                                                     4

// Field:     [1] CTSMMIS
//
// Clear to Send (CTS) modem masked interrupt status:
// This field returns the masked interrupt state of the clear to send interrupt
// which is the AND product of raw interrupt state RIS.CTSRMIS and the mask
// setting IMSC.CTSMIM.
#define UART_MIS_CTSMMIS                                            0x00000002
#define UART_MIS_CTSMMIS_BITN                                                1
#define UART_MIS_CTSMMIS_M                                          0x00000002
#define UART_MIS_CTSMMIS_S                                                   1

//*****************************************************************************
//
// Register: UART_O_ICR
//
//*****************************************************************************
// Field:    [11] EOTIC
//
// End of Transmission interrupt clear:
// Writing 1 to this field clears the overrun error interrupt (RIS.EOTRIS).
// Writing 0 has no effect.
#define UART_ICR_EOTIC                                              0x00000800
#define UART_ICR_EOTIC_BITN                                                 11
#define UART_ICR_EOTIC_M                                            0x00000800
#define UART_ICR_EOTIC_S                                                    11

// Field:    [10] OEIC
//
// Overrun error interrupt clear:
// Writing 1 to this field clears the overrun error interrupt (RIS.OERIS).
// Writing 0 has no effect.
#define UART_ICR_OEIC                                               0x00000400
#define UART_ICR_OEIC_BITN                                                  10
#define UART_ICR_OEIC_M                                             0x00000400
#define UART_ICR_OEIC_S                                                     10

// Field:     [9] BEIC
//
// Break error interrupt clear:
// Writing 1 to this field clears the break error interrupt (RIS.BERIS).
// Writing 0 has no effect.
#define UART_ICR_BEIC                                               0x00000200
#define UART_ICR_BEIC_BITN                                                   9
#define UART_ICR_BEIC_M                                             0x00000200
#define UART_ICR_BEIC_S                                                      9

// Field:     [8] PEIC
//
// Parity error interrupt clear:
// Writing 1 to this field clears the parity error interrupt (RIS.PERIS).
// Writing 0 has no effect.
#define UART_ICR_PEIC                                               0x00000100
#define UART_ICR_PEIC_BITN                                                   8
#define UART_ICR_PEIC_M                                             0x00000100
#define UART_ICR_PEIC_S                                                      8

// Field:     [7] FEIC
//
// Framing error interrupt clear:
// Writing 1 to this field clears the framing error interrupt (RIS.FERIS).
// Writing 0 has no effect.
#define UART_ICR_FEIC                                               0x00000080
#define UART_ICR_FEIC_BITN                                                   7
#define UART_ICR_FEIC_M                                             0x00000080
#define UART_ICR_FEIC_S                                                      7

// Field:     [6] RTIC
//
// Receive timeout interrupt clear:
// Writing 1 to this field clears the receive timeout interrupt (RIS.RTRIS).
// Writing 0 has no effect.
#define UART_ICR_RTIC                                               0x00000040
#define UART_ICR_RTIC_BITN                                                   6
#define UART_ICR_RTIC_M                                             0x00000040
#define UART_ICR_RTIC_S                                                      6

// Field:     [5] TXIC
//
// Transmit interrupt clear:
// Writing 1 to this field clears the transmit interrupt (RIS.TXRIS). Writing 0
// has no effect.
#define UART_ICR_TXIC                                               0x00000020
#define UART_ICR_TXIC_BITN                                                   5
#define UART_ICR_TXIC_M                                             0x00000020
#define UART_ICR_TXIC_S                                                      5

// Field:     [4] RXIC
//
// Receive interrupt clear:
// Writing 1 to this field clears the receive interrupt (RIS.RXRIS). Writing 0
// has no effect.
#define UART_ICR_RXIC                                               0x00000010
#define UART_ICR_RXIC_BITN                                                   4
#define UART_ICR_RXIC_M                                             0x00000010
#define UART_ICR_RXIC_S                                                      4

// Field:     [1] CTSMIC
//
// Clear to Send (CTS) modem interrupt clear:
// Writing 1 to this field clears the clear to send interrupt (RIS.CTSRMIS).
// Writing 0 has no effect.
#define UART_ICR_CTSMIC                                             0x00000002
#define UART_ICR_CTSMIC_BITN                                                 1
#define UART_ICR_CTSMIC_M                                           0x00000002
#define UART_ICR_CTSMIC_S                                                    1

//*****************************************************************************
//
// Register: UART_O_DMACTL
//
//*****************************************************************************
// Field:     [2] DMAONERR
//
// DMA on error. If this bit is set to 1, the DMA receive request outputs (for
// single and burst requests) are disabled when the UART error interrupt is
// asserted (more specifically if any of the error interrupts RIS.PERIS,
// RIS.BERIS, RIS.FERIS or RIS.OERIS are asserted).
#define UART_DMACTL_DMAONERR                                        0x00000004
#define UART_DMACTL_DMAONERR_BITN                                            2
#define UART_DMACTL_DMAONERR_M                                      0x00000004
#define UART_DMACTL_DMAONERR_S                                               2

// Field:     [1] TXDMAE
//
// Transmit DMA enable. If this bit is set to 1, DMA for the transmit FIFO is
// enabled.
#define UART_DMACTL_TXDMAE                                          0x00000002
#define UART_DMACTL_TXDMAE_BITN                                              1
#define UART_DMACTL_TXDMAE_M                                        0x00000002
#define UART_DMACTL_TXDMAE_S                                                 1

// Field:     [0] RXDMAE
//
// Receive DMA enable. If this bit is set to 1, DMA for the receive FIFO is
// enabled.
#define UART_DMACTL_RXDMAE                                          0x00000001
#define UART_DMACTL_RXDMAE_BITN                                              0
#define UART_DMACTL_RXDMAE_M                                        0x00000001
#define UART_DMACTL_RXDMAE_S                                                 0


#endif // __UART__
