/******************************************************************************
*  Filename:       hw_adi.h
*  Revised:        2015-01-13 16:59:55 +0100 (Tue, 13 Jan 2015)
*  Revision:       42365
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

#ifndef __HW_ADI_H__
#define __HW_ADI_H__

//*****************************************************************************
//
// This file contains macros for controlling the ADI master and
// accessing ADI slave registers via the ADI Master.
// There are 3 categories of macros in this file:
//                 - macros that provide an offset to a register
//                   located within the DDI Master itself.
//                 - macros that define bits or bitfields
//                   within the DDI Master Registers.
//                 - macros that provide an "instruction offset"
//                   that are used when accessing a ADI Slave.
//
// The macros that that provide ADI Master register offsets and
// define bits and bitfields for those registers are the typical
// macros that appear in most hw_<module>.h header files.  In
// the following example ADI_O_SLAVECONF is a macro for a
// register offset and ADI_SLAVECONF_WAITFORACK is a macro for
// a bit in that register. This example code will set the WAITFORACK
// bit in register ADI_O_SLAVECONF of the ADI Master. (Note: this
// access the Master not the Slave).
//
//    HWREG(ADI3_BASE + ADI_O_SLAVECONF) |= ADI_SLAVECONF_WAITFORACK;
//
// The "instruction offset" macros are used to pass an instruction to
// the ADI Master when accessing ADI slave registers. These macros are
// only used when accessing ADI Slave Registers. (Remember ADI
// Master Registers are accessed normally).
//
// The instructions supported when accessing an ADI Slave Register follow:
//        - Direct Access to an ADI Slave register. I.e. read or
//          write the register.
//        - Set the specified bits in a ADI Slave register.
//        - Clear the specified bits in a ADI Slave register.
//        - Mask write of 4 bits to the a ADI Slave register.
//        - Mask write of 8 bits to the a ADI Slave register.
//        - Mask write of 16 bits to the a ADI Slave register.
//
// Note: only the "Direct Access" offset should be used when reading
// a ADI Slave register. Only 4-bit reads are supported and 8 bits write are
// supported natively. If accessing wider bitfields, the read/write operation
// will be spread out over a number of transactions. This is hidden for the
// user, but can potentially be very timeconsuming. Especially of running
// on a slow clock.
//
// The generic format of using these macros for a read follows:
//       // Read low 8-bits in ADI_SLAVE_OFF
//       myushortvar = HWREGB(ADI_MASTER_BASE + ADI_SLAVE_OFF + ADI_O_DIR);
//
//       // Read high 8-bits in ADI_SLAVE_OFF (data[31:16])
//       myushortvar = HWREGB(ADI_MASTER_BASE + ADI_SLAVE_OFF + ADI_O_DIR);
//
// Notes: In the above example:
//     - ADI_MASTER_BASE is the base address of the ADI Master defined
//       in the hw_memmap.h header file.
//     - ADI_SLAVE_OFF is the ADI Slave offset defined in the
//       hw_<adi_slave>.h header file (e.g. hw_adi_3_refsys_top.h for the refsys
//       module).
//     - ADI_O_DIR is the "instruction offset" macro defined in this
//       file that specifies the Direct Access instruction.
//
// Writes can use any of the "instruction macros".
// The following examples do a "direct write" to an ADI Slave register
// ADI_SLAVE_OFF using different size operands:
//
//     // ---------- DIRECT WRITES ----------
//     // Write 32-bits aligned
//     HWREG(ADI_MASTER_BASE + ADI_SLAVE_OFF + ADI_O_DIR) = 0x12345678;
//
//     // Write 16-bits aligned to high 16-bits then low 16-bits
//     // Add 2 to get to high 16-bits.
//     HWREGH(ADI_MASTER_BASE + ADI_SLAVE_OFF + ADI_O_DIR + 2) = 0xabcd;
//     HWREGH(ADI_MASTER_BASE + ADI_SLAVE_OFF + ADI_O_DIR) = 0xef01;
//
//     // Write each byte at ADI_SLAVE_OFF, one at a time.
//     // Add 1,2,or 3 to get to bytes 1,2, or 3.
//     HWREGB(ADI_MASTER_BASE + ADI_SLAVE_OFF + ADI_O_DIR) = 0x33;
//     HWREGB(ADI_MASTER_BASE + ADI_SLAVE_OFF + ADI_O_DIR + 1) = 0x44;
//     HWREGB(ADI_MASTER_BASE + ADI_SLAVE_OFF + ADI_O_DIR + 2) = 0x55;
//     HWREGB(ADI_MASTER_BASE + ADI_SLAVE_OFF + ADI_O_DIR + 3) = 0x66;
//
//     // ---------- SET/CLR ----------
//     The set and clear functions behave similarly to eachother. Each
//     can be performed on an 8-, 16-, or 32-bit operand.
//     Examples follow:
//     // Set all odd bits in a 32-bit words
//     HWREG(ADI_MASTER_BASE + ADI_SLAVE_OFF + ADI_O_SET) = 0xaaaaaaaa;
//
//     // Clear all bits in byte 2 (data[23:16]) using 32-bit operand
//     HWREG(DDI_MASTER_BASE + ADI_SLAVE_OFF + ADI_O_CLR) = 0x00ff0000;
//
//     // Set even bits in byte 2 (data[23:16]) using 8-bit operand
//     HWREGB(ADI_MASTER_BASE + ADI_SLAVE_OFF + 2 + ADI_O_CLR) = 0x55;
//
//     // ---------- MASKED WRITES ----------
//     The mask writes are a bit different. They operate on nibbles,
//     bytes, and 16-bit elements. Two operands are required; a 'mask'
//     and 'data'; The operands are concatenated and written to the master.
//     e.g. the mask and data are combined as follows for a 16 bit masked
//     write:
//           (mask << 16) | data;
//     Examples follow:
//
//     // Do an 4 bit masked write (Nibble) of 7 to data[3:0]).
//     // Byte write is needed.
//     HWREGB(ADI_MASTER_BASE + ADI_SLAVE_OFF + ADI_O_MASK4B01) = 0xf7;
//
//     // Do an 4 bit masked write of 4 to data[7:4]).
//     // Add 1 for next nibble
//     HWREGB(DDI_MASTER_BASE + DDI_SLAVE_OFF + ADI_O_MASK4B01 + 1) = 0xf4;
//
//*****************************************************************************

//*****************************************************************************
//
// The following are defines for the ADI master instruction offsets.
//
//*****************************************************************************
#define ADI_O_DIR             0x00000000  // Offset for the direct access
                                          // instruction
#define ADI_O_SET             0x00000010  // Offset for 'Set' instruction.
#define ADI_O_CLR             0x00000020  // Offset for 'Clear' instruction.
#define ADI_O_MASK4B          0x00000040  // Offset for 4-bit masked access.
                                          // Data bit[n] is written if mask
                                          // bit[n] is set ('1').
                                          // Bits 7:4 are mask. Bits 3:0 are data.
                                          // Requires 'byte' write.
#define ADI_O_MASK8B          0x00000060  // Offset for 8-bit masked access.
                                          // Data bit[n] is written if mask
                                          // bit[n] is set ('1'). Bits 15:8 are
                                          // mask. Bits 7:0 are data. Requires
                                          // 'short' write.
#define ADI_O_MASK16B         0x00000080  // Offset for 16-bit masked access.
                                          // Data bit[n] is written if mask
                                          // bit[n] is set ('1'). Bits 31:16
                                          // are mask. Bits 15:0 are data.
                                          // Requires 'long' write.

//*****************************************************************************
//
// The following are defines for the ADI register offsets.
//
//*****************************************************************************
#define ADI_O_SLAVESTAT         0x00000030  // ADI Slave status register
#define ADI_O_SLAVECONF         0x00000038  // ADI Master configuration

//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_SLAVESTAT register.
//
//*****************************************************************************
#define ADI_SLAVESTAT_DI_REQ    0x00000002  // Read current value of DI_REQ
                                            // signal. Writing 0 to this bit
                                            // forces a sync with slave,
                                            // ensuring that req will be 0. It
                                            // is recommended to write 0 to
                                            // this register before power down
                                            // of the master.
#define ADI_SLAVESTAT_DI_REQ_M  0x00000002
#define ADI_SLAVESTAT_DI_REQ_S  1
#define ADI_SLAVESTAT_DI_ACK    0x00000001  // Read current value of DI_ACK
                                            // signal
#define ADI_SLAVESTAT_DI_ACK_M  0x00000001
#define ADI_SLAVESTAT_DI_ACK_S  0
//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_SLAVECONF register.
//
//*****************************************************************************
#define ADI_SLAVECONF_CONFLOCK  0x00000080  // This register is no longer
                                            // accessible when this bit is set.
                                            // (unless sticky_bit_overwrite is
                                            // asserted on top module)
#define ADI_SLAVECONF_CONFLOCK_M \
                                0x00000080
#define ADI_SLAVECONF_CONFLOCK_S 7
#define ADI_SLAVECONF_WAITFORACK \
                                0x00000004  // A transaction on the ADI
                                            // interface does not end until ack
                                            // has been received from the slave
                                            // when this bit is set.

#define ADI_SLAVECONF_WAITFORACK_M \
                                0x00000004
#define ADI_SLAVECONF_WAITFORACK_S 2
#define ADI_SLAVECONF_ADICLKSPEED_M \
                                0x00000003  // Sets the period of an ADI
                                            // transactions. All transactions
                                            // takes an even number of clock
                                            // cycles,- ADI clock rising edge
                                            // occurs in the middle of the
                                            // period. Data and ctrl to slave
                                            // is set up in beginning of cycle,
                                            // and data from slave is read in
                                            // after the transaction 00: An ADI
                                            // transaction takes 2 master clock
                                            // cyclkes 01: An ADI transaction
                                            // takes 4 master clock cycles 10:
                                            // And ADI Transaction takes 8
                                            // master clock cycles 11: An ADI
                                            // transaction takes 16 master
                                            // clock cycles

#define ADI_SLAVECONF_ADICLKSPEED_S 0

//*****************************************************************************
//
// The following are defines pseudo-magic numbers that should go away.
// New code should not use these registers and old code should be ported
// to not use these.
//
//*****************************************************************************
#define ADI_O_DIR03             0x00000000  // Direct access for adi byte
                                            // offsets 0 to 3
#define ADI_O_DIR47             0x00000004  // Direct access for adi byte
                                            // offsets 4 to 7
#define ADI_O_DIR811            0x00000008  // Direct access for adi byte
                                            // offsets 8 to 11
#define ADI_O_DIR1215           0x0000000C  // Direct access for adi byte
                                            // offsets 12 to 15
#define ADI_O_SET03             0x00000010  // Set register for ADI byte
                                            // offsets 0 to 3
#define ADI_O_SET47             0x00000014  // Set register for ADI byte
                                            // offsets 4 to 7
#define ADI_O_SET811            0x00000018  // Set register for ADI byte
                                            // offsets 8 to 11
#define ADI_O_SET1215           0x0000001C  // Set register for ADI byte
                                            // offsets 12 to 15
#define ADI_O_CLR03             0x00000020  // Clear register for ADI byte
                                            // offsets 0 to 3
#define ADI_O_CLR47             0x00000024  // Clear register for ADI byte
                                            // offsets 4 to 7
#define ADI_O_CLR811            0x00000028  // Clear register for ADI byte
                                            // offsets 8 to 11
#define ADI_O_CLR1215           0x0000002C  // Clear register for ADI byte
                                            // offsets 12 to 15
#define ADI_O_SLAVESTAT         0x00000030  // ADI Slave status register
#define ADI_O_SLAVECONF         0x00000038  // ADI Master configuration
                                            // register
#define ADI_O_MASK4B01          0x00000040  // Masked access (4m/4d) for ADI
                                            // Registers at byte offsets 0 and
                                            // 1
#define ADI_O_MASK4B23          0x00000044  // Masked access (4m/4d) for ADI
                                            // Registers at byte offsets 2 and
                                            // 3
#define ADI_O_MASK4B45          0x00000048  // Masked access (4m/4d) for ADI
                                            // Registers at byte offsets 4 and
                                            // 5
#define ADI_O_MASK4B67          0x0000004C  // Masked access (4m/4d) for ADI
                                            // Registers at byte offsets 6 and
                                            // 7
#define ADI_O_MASK4B89          0x00000050  // Masked access (4m/4d) for ADI
                                            // Registers at byte offsets 8 and
                                            // 9
#define ADI_O_MASK4B1011        0x00000054  // Masked access (4m/4d) for ADI
                                            // Registers at byte offsets 10 and
                                            // 11
#define ADI_O_MASK4B1213        0x00000058  // Masked access (4m/4d) for ADI
                                            // Registers at byte offsets 12 and
                                            // 13
#define ADI_O_MASK4B1415        0x0000005C  // Masked access (4m/4d) for ADI
                                            // Registers at byte offsets 14 and
                                            // 15
#define ADI_O_MASK8B01          0x00000060  // Masked access (8m/8d) for ADI
                                            // Registers at byte offsets 0 and
                                            // 1
#define ADI_O_MASK8B23          0x00000064  // Masked access (8m/8d) for ADI
                                            // Registers at byte offsets 2 and
                                            // 3
#define ADI_O_MASK8B45          0x00000068  // Masked access (8m/8d) for ADI
                                            // Registers at byte offsets 4 and
                                            // 5
#define ADI_O_MASK8B67          0x0000006C  // Masked access (8m/8d) for ADI
                                            // Registers at byte offsets 6 and
                                            // 7
#define ADI_O_MASK8B89          0x00000070  // Masked access (8m/8d) for ADI
                                            // Registers at byte offsets 8 and
                                            // 9
#define ADI_O_MASK8B1011        0x00000074  // Masked access (8m/8d) for ADI
                                            // Registers at byte offsets 10 and
                                            // 11
#define ADI_O_MASK8B1213        0x00000078  // Masked access (8m/8d) for ADI
                                            // Registers at byte offsets 12 and
                                            // 13
#define ADI_O_MASK8B1415        0x0000007C  // Masked access (8m/8d) for ADI
                                            // Registers at byte offsets 14 and
                                            // 15
#define ADI_O_MASK16B01         0x00000080  // Masked access (16m/16d) for ADI
                                            // Registers at byte offsets 0 and
                                            // 1
#define ADI_O_MASK16B23         0x00000084  // Masked access (16m/16d) for ADI
                                            // Registers at byte offsets 2 and
                                            // 3
#define ADI_O_MASK16B45         0x00000088  // Masked access (16m/16d) for ADI
                                            // Registers at byte offsets 4 and
                                            // 5
#define ADI_O_MASK16B67         0x0000008C  // Masked access (16m/16d) for ADI
                                            // Registers at byte offsets 6 and
                                            // 7
#define ADI_O_MASK16B89         0x00000090  // Masked access (16m/16d) for ADI
                                            // Registers at byte offsets 8 and
                                            // 9
#define ADI_O_MASK16B1011       0x00000094  // Masked access (16m/16d) for ADI
                                            // Registers at byte offsets 10 and
                                            // 11
#define ADI_O_MASK16B1213       0x00000098  // Masked access (16m/16d) for ADI
                                            // Registers at byte offsets 12 and
                                            // 13
#define ADI_O_MASK16B1415       0x0000009C  // Masked access (16m/16d) for ADI
                                            // Registers at byte offsets 14 and
                                            // 15

//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_DIR03 register.
//
//*****************************************************************************
#define ADI_DIR03_B3_M          0xFF000000  // Direct access to ADI register 3
#define ADI_DIR03_B3_S          24
#define ADI_DIR03_B2_M          0x00FF0000  // Direct access to ADI register 2
#define ADI_DIR03_B2_S          16
#define ADI_DIR03_B1_M          0x0000FF00  // Direct access to ADI register 1
#define ADI_DIR03_B1_S          8
#define ADI_DIR03_B0_M          0x000000FF  // Direct access to ADI register 0
#define ADI_DIR03_B0_S          0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_DIR47 register.
//
//*****************************************************************************
#define ADI_DIR47_B3_M          0xFF000000  // Direct access to ADI register 7
#define ADI_DIR47_B3_S          24
#define ADI_DIR47_B2_M          0x00FF0000  // Direct access to ADI register 6
#define ADI_DIR47_B2_S          16
#define ADI_DIR47_B1_M          0x0000FF00  // Direct access to ADI register 5
#define ADI_DIR47_B1_S          8
#define ADI_DIR47_B0_M          0x000000FF  // Direct access to ADI register 4
#define ADI_DIR47_B0_S          0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_DIR811 register.
//
//*****************************************************************************
#define ADI_DIR811_B3_M         0xFF000000  // Direct access to ADI register
                                            // 11
#define ADI_DIR811_B3_S         24
#define ADI_DIR811_B2_M         0x00FF0000  // Direct access to ADI register
                                            // 10
#define ADI_DIR811_B2_S         16
#define ADI_DIR811_B1_M         0x0000FF00  // Direct access to ADI register 9
#define ADI_DIR811_B1_S         8
#define ADI_DIR811_B0_M         0x000000FF  // Direct access to ADI register 8
#define ADI_DIR811_B0_S         0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_DIR1215 register.
//
//*****************************************************************************
#define ADI_DIR1215_B3_M        0xFF000000  // Direct access to ADI register
                                            // 15
#define ADI_DIR1215_B3_S        24
#define ADI_DIR1215_B2_M        0x00FF0000  // Direct access to ADI register
                                            // 14
#define ADI_DIR1215_B2_S        16
#define ADI_DIR1215_B1_M        0x0000FF00  // Direct access to ADI register
                                            // 13
#define ADI_DIR1215_B1_S        8
#define ADI_DIR1215_B0_M        0x000000FF  // Direct access to ADI register
                                            // 12
#define ADI_DIR1215_B0_S        0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_SET03 register.
//
//*****************************************************************************
#define ADI_SET03_S3_M          0xFF000000  // A high bit value will set the
                                            // corresponding bit in ADI
                                            // register 3. Read returns 0.
#define ADI_SET03_S3_S          24
#define ADI_SET03_S2_M          0x00FF0000  // A high bit value will set the
                                            // corresponding bit in ADI
                                            // register 2. Read returns 0.
#define ADI_SET03_S2_S          16
#define ADI_SET03_S1_M          0x0000FF00  // A high bit value will set the
                                            // corresponding bit in ADI
                                            // register 1. Read returns 0.
#define ADI_SET03_S1_S          8
#define ADI_SET03_S0_M          0x000000FF  // A high bit value will set the
                                            // corresponding bit in ADI
                                            // register 0. Read returns 0.
#define ADI_SET03_S0_S          0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_SET47 register.
//
//*****************************************************************************
#define ADI_SET47_S3_M          0xFF000000  // A high bit value will set the
                                            // corresponding bit in ADI
                                            // register 7. Read returns 0.
#define ADI_SET47_S3_S          24
#define ADI_SET47_S2_M          0x00FF0000  // A high bit value will set the
                                            // corresponding bit in ADI
                                            // register 6. Read returns 0.
#define ADI_SET47_S2_S          16
#define ADI_SET47_S1_M          0x0000FF00  // A high bit value will set the
                                            // corresponding bit in ADI
                                            // register 5. Read returns 0.
#define ADI_SET47_S1_S          8
#define ADI_SET47_S0_M          0x000000FF  // A high bit value will set the
                                            // corresponding bit in ADI
                                            // register 4. Read returns 0.
#define ADI_SET47_S0_S          0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_SET811 register.
//
//*****************************************************************************
#define ADI_SET811_S3_M         0xFF000000  // A high bit value will set the
                                            // corresponding bit in ADI
                                            // register 11. Read returns 0.
#define ADI_SET811_S3_S         24
#define ADI_SET811_S2_M         0x00FF0000  // A high bit value will set the
                                            // corresponding bit in ADI
                                            // register 10. Read returns 0.
#define ADI_SET811_S2_S         16
#define ADI_SET811_S1_M         0x0000FF00  // A high bit value will set the
                                            // corresponding bit in ADI
                                            // register 9. Read returns 0.
#define ADI_SET811_S1_S         8
#define ADI_SET811_S0_M         0x000000FF  // A high bit value will set the
                                            // corresponding bit in ADI
                                            // register 8. Read returns 0.
#define ADI_SET811_S0_S         0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_SET1215 register.
//
//*****************************************************************************
#define ADI_SET1215_S3_M        0xFF000000  // A high bit value will set the
                                            // corresponding bit in ADI
                                            // register 15. Read returns 0.
#define ADI_SET1215_S3_S        24
#define ADI_SET1215_S2_M        0x00FF0000  // A high bit value will set the
                                            // corresponding bit in ADI
                                            // register 14. Read returns 0.
#define ADI_SET1215_S2_S        16
#define ADI_SET1215_S1_M        0x0000FF00  // A high bit value will set the
                                            // corresponding bit in ADI
                                            // register 13. Read returns 0.
#define ADI_SET1215_S1_S        8
#define ADI_SET1215_S0_M        0x000000FF  // A high bit value will set the
                                            // corresponding bit in ADI
                                            // register 12. Read returns 0.
#define ADI_SET1215_S0_S        0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_CLR03 register.
//
//*****************************************************************************
#define ADI_CLR03_S3_M          0xFF000000  // A high bit value will clear the
                                            // corresponding bit in ADI
                                            // register 3
#define ADI_CLR03_S3_S          24
#define ADI_CLR03_S2_M          0x00FF0000  // A high bit value will clear the
                                            // corresponding bit in ADI
                                            // register 2
#define ADI_CLR03_S2_S          16
#define ADI_CLR03_S1_M          0x0000FF00  // A high bit value will clear the
                                            // corresponding bit in ADI
                                            // register 1
#define ADI_CLR03_S1_S          8
#define ADI_CLR03_S0_M          0x000000FF  // A high bit value will clear the
                                            // corresponding bit in ADI
                                            // register 0
#define ADI_CLR03_S0_S          0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_CLR47 register.
//
//*****************************************************************************
#define ADI_CLR47_S3_M          0xFF000000  // A high bit value will clear the
                                            // corresponding bit in ADI
                                            // register 7
#define ADI_CLR47_S3_S          24
#define ADI_CLR47_S2_M          0x00FF0000  // A high bit value will clear the
                                            // corresponding bit in ADI
                                            // register 6
#define ADI_CLR47_S2_S          16
#define ADI_CLR47_S1_M          0x0000FF00  // A high bit value will clear the
                                            // corresponding bit in ADI
                                            // register 5
#define ADI_CLR47_S1_S          8
#define ADI_CLR47_S0_M          0x000000FF  // A high bit value will clear the
                                            // corresponding bit in ADI
                                            // register 4
#define ADI_CLR47_S0_S          0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_CLR811 register.
//
//*****************************************************************************
#define ADI_CLR811_S3_M         0xFF000000  // A high bit value will clear the
                                            // corresponding bit in ADI
                                            // register 11
#define ADI_CLR811_S3_S         24
#define ADI_CLR811_S2_M         0x00FF0000  // A high bit value will clear the
                                            // corresponding bit in ADI
                                            // register 10
#define ADI_CLR811_S2_S         16
#define ADI_CLR811_S1_M         0x0000FF00  // A high bit value will clear the
                                            // corresponding bit in ADI
                                            // register 9
#define ADI_CLR811_S1_S         8
#define ADI_CLR811_S0_M         0x000000FF  // A high bit value will clear the
                                            // corresponding bit in ADI
                                            // register 8
#define ADI_CLR811_S0_S         0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_CLR1215 register.
//
//*****************************************************************************
#define ADI_CLR1215_S3_M        0xFF000000  // A high bit value will clear the
                                            // corresponding bit in ADI
                                            // register 15
#define ADI_CLR1215_S3_S        24
#define ADI_CLR1215_S2_M        0x00FF0000  // A high bit value will clear the
                                            // corresponding bit in ADI
                                            // register 14
#define ADI_CLR1215_S2_S        16
#define ADI_CLR1215_S1_M        0x0000FF00  // A high bit value will clear the
                                            // corresponding bit in ADI
                                            // register 13
#define ADI_CLR1215_S1_S        8
#define ADI_CLR1215_S0_M        0x000000FF  // A high bit value will clear the
                                            // corresponding bit in ADI
                                            // register 12
#define ADI_CLR1215_S0_S        0
//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_SLAVESTAT register.
//
//*****************************************************************************
#define ADI_SLAVESTAT_DI_REQ    0x00000002  // Read current value of DI_REQ
                                            // signal. Writing 0 to this bit
                                            // forces a sync with slave,
                                            // ensuring that req will be 0. It
                                            // is recommended to write 0 to
                                            // this register before power down
                                            // of the master.
#define ADI_SLAVESTAT_DI_REQ_M  0x00000002
#define ADI_SLAVESTAT_DI_REQ_S  1
#define ADI_SLAVESTAT_DI_ACK    0x00000001  // Read current value of DI_ACK
                                            // signal
#define ADI_SLAVESTAT_DI_ACK_M  0x00000001
#define ADI_SLAVESTAT_DI_ACK_S  0
//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_SLAVECONF register.
//
//*****************************************************************************
#define ADI_SLAVECONF_CONFLOCK  0x00000080  // This register is no longer
                                            // accessible when this bit is set.
                                            // (unless sticky_bit_overwrite is
                                            // asserted on top module)
#define ADI_SLAVECONF_CONFLOCK_M \
                                0x00000080
#define ADI_SLAVECONF_CONFLOCK_S 7
#define ADI_SLAVECONF_WAITFORACK \
                                0x00000004  // A transaction on the ADI
                                            // interface does not end until ack
                                            // has been received from the slave
                                            // when this bit is set.

#define ADI_SLAVECONF_WAITFORACK_M \
                                0x00000004
#define ADI_SLAVECONF_WAITFORACK_S 2
#define ADI_SLAVECONF_ADICLKSPEED_M \
                                0x00000003  // Sets the period of an ADI
                                            // transactions. All transactions
                                            // takes an even number of clock
                                            // cycles,- ADI clock rising edge
                                            // occurs in the middle of the
                                            // period. Data and ctrl to slave
                                            // is set up in beginning of cycle,
                                            // and data from slave is read in
                                            // after the transaction 00: An ADI
                                            // transaction takes 2 master clock
                                            // cyclkes 01: An ADI transaction
                                            // takes 4 master clock cycles 10:
                                            // And ADI Transaction takes 8
                                            // master clock cycles 11: An ADI
                                            // transaction takes 16 master
                                            // clock cycles

#define ADI_SLAVECONF_ADICLKSPEED_S 0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_MASK4B01 register.
//
//*****************************************************************************
#define ADI_MASK4B01_M1H_M      0xF0000000  // Mask for bits [7:4] in ADI
                                            // register 1
#define ADI_MASK4B01_M1H_S      28
#define ADI_MASK4B01_D1H_M      0x0F000000  // Data for bits [7:4] in ADI
                                            // register 1, - only bits selected
                                            // by mask M1H will be affected by
                                            // access
#define ADI_MASK4B01_D1H_S      24
#define ADI_MASK4B01_M1L_M      0x00F00000  // Mask for bits [3:0] in ADI
                                            // register 1
#define ADI_MASK4B01_M1L_S      20
#define ADI_MASK4B01_D1L_M      0x000F0000  // Data for bits [3:0] in ADI
                                            // register 1, - only bits selected
                                            // by mask M1L will be affected by
                                            // access
#define ADI_MASK4B01_D1L_S      16
#define ADI_MASK4B01_M0H_M      0x0000F000  // Mask for bits [7:4] in ADI
                                            // register 0
#define ADI_MASK4B01_M0H_S      12
#define ADI_MASK4B01_D0H_M      0x00000F00  // Data for bits [7:4] in ADI
                                            // register 0, - only bits selected
                                            // by mask M0H will be affected by
                                            // access
#define ADI_MASK4B01_D0H_S      8
#define ADI_MASK4B01_M0L_M      0x000000F0  // Mask for bits [3:0] in ADI
                                            // register 0
#define ADI_MASK4B01_M0L_S      4
#define ADI_MASK4B01_D0L_M      0x0000000F  // Data for bits [3:0] in ADI
                                            // register 0, - only bits selected
                                            // by mask M0L will be affected by
                                            // access
#define ADI_MASK4B01_D0L_S      0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_MASK4B23 register.
//
//*****************************************************************************
#define ADI_MASK4B23_M1H_M      0xF0000000  // Mask for bits [7:4] in ADI
                                            // register 3
#define ADI_MASK4B23_M1H_S      28
#define ADI_MASK4B23_D1H_M      0x0F000000  // Data for bits [7:4] in ADI
                                            // register 3, - only bits selected
                                            // by mask M1H will be affected by
                                            // access
#define ADI_MASK4B23_D1H_S      24
#define ADI_MASK4B23_M1L_M      0x00F00000  // Mask for bits [3:0] in ADI
                                            // register 3
#define ADI_MASK4B23_M1L_S      20
#define ADI_MASK4B23_D1L_M      0x000F0000  // Data for bits [3:0] in ADI
                                            // register 3, - only bits selected
                                            // by mask M1L will be affected by
                                            // access
#define ADI_MASK4B23_D1L_S      16
#define ADI_MASK4B23_M0H_M      0x0000F000  // Mask for bits [7:4] in ADI
                                            // register 2
#define ADI_MASK4B23_M0H_S      12
#define ADI_MASK4B23_D0H_M      0x00000F00  // Data for bits [7:4] in ADI
                                            // register 2, - only bits selected
                                            // by mask M0H will be affected by
                                            // access
#define ADI_MASK4B23_D0H_S      8
#define ADI_MASK4B23_M0L_M      0x000000F0  // Mask for bits [3:0] in ADI
                                            // register 2
#define ADI_MASK4B23_M0L_S      4
#define ADI_MASK4B23_D0L_M      0x0000000F  // Data for bits [3:0] in ADI
                                            // register 2, - only bits selected
                                            // by mask M0L will be affected by
                                            // access
#define ADI_MASK4B23_D0L_S      0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_MASK4B45 register.
//
//*****************************************************************************
#define ADI_MASK4B45_M1H_M      0xF0000000  // Mask for bits [7:4] in ADI
                                            // register 5
#define ADI_MASK4B45_M1H_S      28
#define ADI_MASK4B45_D1H_M      0x0F000000  // Data for bits [7:4] in ADI
                                            // register 5, - only bits selected
                                            // by mask M1H will be affected by
                                            // access
#define ADI_MASK4B45_D1H_S      24
#define ADI_MASK4B45_M1L_M      0x00F00000  // Mask for bits [3:0] in ADI
                                            // register 5
#define ADI_MASK4B45_M1L_S      20
#define ADI_MASK4B45_D1L_M      0x000F0000  // Data for bits [3:0] in ADI
                                            // register 5, - only bits selected
                                            // by mask M1L will be affected by
                                            // access
#define ADI_MASK4B45_D1L_S      16
#define ADI_MASK4B45_M0H_M      0x0000F000  // Mask for bits [7:4] in ADI
                                            // register 4
#define ADI_MASK4B45_M0H_S      12
#define ADI_MASK4B45_D0H_M      0x00000F00  // Data for bits [7:4] in ADI
                                            // register 4, - only bits selected
                                            // by mask M0H will be affected by
                                            // access
#define ADI_MASK4B45_D0H_S      8
#define ADI_MASK4B45_M0L_M      0x000000F0  // Mask for bits [3:0] in ADI
                                            // register 4
#define ADI_MASK4B45_M0L_S      4
#define ADI_MASK4B45_D0L_M      0x0000000F  // Data for bits [3:0] in ADI
                                            // register 4, - only bits selected
                                            // by mask M0L will be affected by
                                            // access
#define ADI_MASK4B45_D0L_S      0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_MASK4B67 register.
//
//*****************************************************************************
#define ADI_MASK4B67_M1H_M      0xF0000000  // Mask for bits [7:4] in ADI
                                            // register 7
#define ADI_MASK4B67_M1H_S      28
#define ADI_MASK4B67_D1H_M      0x0F000000  // Data for bits [7:4] in ADI
                                            // register 7, - only bits selected
                                            // by mask M1H will be affected by
                                            // access
#define ADI_MASK4B67_D1H_S      24
#define ADI_MASK4B67_M1L_M      0x00F00000  // Mask for bits [3:0] in ADI
                                            // register 7
#define ADI_MASK4B67_M1L_S      20
#define ADI_MASK4B67_D1L_M      0x000F0000  // Data for bits [3:0] in ADI
                                            // register 7, - only bits selected
                                            // by mask M1L will be affected by
                                            // access
#define ADI_MASK4B67_D1L_S      16
#define ADI_MASK4B67_M0H_M      0x0000F000  // Mask for bits [7:4] in ADI
                                            // register 6
#define ADI_MASK4B67_M0H_S      12
#define ADI_MASK4B67_D0H_M      0x00000F00  // Data for bits [7:4] in ADI
                                            // register 6, - only bits selected
                                            // by mask M0H will be affected by
                                            // access
#define ADI_MASK4B67_D0H_S      8
#define ADI_MASK4B67_M0L_M      0x000000F0  // Mask for bits [3:0] in ADI
                                            // register 6
#define ADI_MASK4B67_M0L_S      4
#define ADI_MASK4B67_D0L_M      0x0000000F  // Data for bits [3:0] in ADI
                                            // register 6, - only bits selected
                                            // by mask M0L will be affected by
                                            // access
#define ADI_MASK4B67_D0L_S      0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_MASK4B89 register.
//
//*****************************************************************************
#define ADI_MASK4B89_M1H_M      0xF0000000  // Mask for bits [7:4] in ADI
                                            // register 9
#define ADI_MASK4B89_M1H_S      28
#define ADI_MASK4B89_D1H_M      0x0F000000  // Data for bits [7:4] in ADI
                                            // register 9, - only bits selected
                                            // by mask M1H will be affected by
                                            // access
#define ADI_MASK4B89_D1H_S      24
#define ADI_MASK4B89_M1L_M      0x00F00000  // Mask for bits [3:0] in ADI
                                            // register 9
#define ADI_MASK4B89_M1L_S      20
#define ADI_MASK4B89_D1L_M      0x000F0000  // Data for bits [3:0] in ADI
                                            // register 9, - only bits selected
                                            // by mask M1L will be affected by
                                            // access
#define ADI_MASK4B89_D1L_S      16
#define ADI_MASK4B89_M0H_M      0x0000F000  // Mask for bits [7:4] in ADI
                                            // register 8
#define ADI_MASK4B89_M0H_S      12
#define ADI_MASK4B89_D0H_M      0x00000F00  // Data for bits [7:4] in ADI
                                            // register 8, - only bits selected
                                            // by mask M0H will be affected by
                                            // access
#define ADI_MASK4B89_D0H_S      8
#define ADI_MASK4B89_M0L_M      0x000000F0  // Mask for bits [3:0] in ADI
                                            // register 8
#define ADI_MASK4B89_M0L_S      4
#define ADI_MASK4B89_D0L_M      0x0000000F  // Data for bits [3:0] in ADI
                                            // register 8, - only bits selected
                                            // by mask M0L will be affected by
                                            // access
#define ADI_MASK4B89_D0L_S      0
//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_MASK4B1011 register.
//
//*****************************************************************************
#define ADI_MASK4B1011_M1H_M    0xF0000000  // Mask for bits [7:4] in ADI
                                            // register 11
#define ADI_MASK4B1011_M1H_S    28
#define ADI_MASK4B1011_D1H_M    0x0F000000  // Data for bits [7:4] in ADI
                                            // register 11, - only bits
                                            // selected by mask M1H will be
                                            // affected by access
#define ADI_MASK4B1011_D1H_S    24
#define ADI_MASK4B1011_M1L_M    0x00F00000  // Mask for bits [3:0] in ADI
                                            // register 11
#define ADI_MASK4B1011_M1L_S    20
#define ADI_MASK4B1011_D1L_M    0x000F0000  // Data for bits [3:0] in ADI
                                            // register 11, - only bits
                                            // selected by mask M1L will be
                                            // affected by access
#define ADI_MASK4B1011_D1L_S    16
#define ADI_MASK4B1011_M0H_M    0x0000F000  // Mask for bits [7:4] in ADI
                                            // register 10
#define ADI_MASK4B1011_M0H_S    12
#define ADI_MASK4B1011_D0H_M    0x00000F00  // Data for bits [7:4] in ADI
                                            // register 10, - only bits
                                            // selected by mask M0H will be
                                            // affected by access
#define ADI_MASK4B1011_D0H_S    8
#define ADI_MASK4B1011_M0L_M    0x000000F0  // Mask for bits [3:0] in ADI
                                            // register 10
#define ADI_MASK4B1011_M0L_S    4
#define ADI_MASK4B1011_D0L_M    0x0000000F  // Data for bits [3:0] in ADI
                                            // register 10, - only bits
                                            // selected by mask M0L will be
                                            // affected by access
#define ADI_MASK4B1011_D0L_S    0
//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_MASK4B1213 register.
//
//*****************************************************************************
#define ADI_MASK4B1213_M1H_M    0xF0000000  // Mask for bits [7:4] in ADI
                                            // register 13
#define ADI_MASK4B1213_M1H_S    28
#define ADI_MASK4B1213_D1H_M    0x0F000000  // Data for bits [7:4] in ADI
                                            // register 13, - only bits
                                            // selected by mask M1H will be
                                            // affected by access
#define ADI_MASK4B1213_D1H_S    24
#define ADI_MASK4B1213_M1L_M    0x00F00000  // Mask for bits [3:0] in ADI
                                            // register 13
#define ADI_MASK4B1213_M1L_S    20
#define ADI_MASK4B1213_D1L_M    0x000F0000  // Data for bits [3:0] in ADI
                                            // register 13, - only bits
                                            // selected by mask M1L will be
                                            // affected by access
#define ADI_MASK4B1213_D1L_S    16
#define ADI_MASK4B1213_M0H_M    0x0000F000  // Mask for bits [7:4] in ADI
                                            // register 12
#define ADI_MASK4B1213_M0H_S    12
#define ADI_MASK4B1213_D0H_M    0x00000F00  // Data for bits [7:4] in ADI
                                            // register 12, - only bits
                                            // selected by mask M0H will be
                                            // affected by access
#define ADI_MASK4B1213_D0H_S    8
#define ADI_MASK4B1213_M0L_M    0x000000F0  // Mask for bits [3:0] in ADI
                                            // register 12
#define ADI_MASK4B1213_M0L_S    4
#define ADI_MASK4B1213_D0L_M    0x0000000F  // Data for bits [3:0] in ADI
                                            // register 12, - only bits
                                            // selected by mask M0L will be
                                            // affected by access
#define ADI_MASK4B1213_D0L_S    0
//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_MASK4B1415 register.
//
//*****************************************************************************
#define ADI_MASK4B1415_M1H_M    0xF0000000  // Mask for bits [7:4] in ADI
                                            // register 15
#define ADI_MASK4B1415_M1H_S    28
#define ADI_MASK4B1415_D1H_M    0x0F000000  // Data for bits [7:4] in ADI
                                            // register 15, - only bits
                                            // selected by mask M1H will be
                                            // affected by access
#define ADI_MASK4B1415_D1H_S    24
#define ADI_MASK4B1415_M1L_M    0x00F00000  // Mask for bits [3:0] in ADI
                                            // register 15
#define ADI_MASK4B1415_M1L_S    20
#define ADI_MASK4B1415_D1L_M    0x000F0000  // Data for bits [3:0] in ADI
                                            // register 15, - only bits
                                            // selected by mask M1L will be
                                            // affected by access
#define ADI_MASK4B1415_D1L_S    16
#define ADI_MASK4B1415_M0H_M    0x0000F000  // Mask for bits [7:4] in ADI
                                            // register 14
#define ADI_MASK4B1415_M0H_S    12
#define ADI_MASK4B1415_D0H_M    0x00000F00  // Data for bits [7:4] in ADI
                                            // register 14, - only bits
                                            // selected by mask M0H will be
                                            // affected by access
#define ADI_MASK4B1415_D0H_S    8
#define ADI_MASK4B1415_M0L_M    0x000000F0  // Mask for bits [3:0] in ADI
                                            // register 14
#define ADI_MASK4B1415_M0L_S    4
#define ADI_MASK4B1415_D0L_M    0x0000000F  // Data for bits [3:0] in ADI
                                            // register 14, - only bits
                                            // selected by mask M0L will be
                                            // affected by access
#define ADI_MASK4B1415_D0L_S    0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_MASK8B01 register.
//
//*****************************************************************************
#define ADI_MASK8B01_M1_M       0xFF000000  // Mask for ADI register 1
#define ADI_MASK8B01_M1_S       24
#define ADI_MASK8B01_D1_M       0x00FF0000  // Data for ADI register 1, - only
                                            // bits selected by mask M1 will be
                                            // affected by access
#define ADI_MASK8B01_D1_S       16
#define ADI_MASK8B01_M0_M       0x0000FF00  // Mask for ADI register 0
#define ADI_MASK8B01_M0_S       8
#define ADI_MASK8B01_D0_M       0x000000FF  // Data for ADI register 0, - only
                                            // bits selected by mask M0 will be
                                            // affected by access
#define ADI_MASK8B01_D0_S       0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_MASK8B23 register.
//
//*****************************************************************************
#define ADI_MASK8B23_M1_M       0xFF000000  // Mask for ADI register 3
#define ADI_MASK8B23_M1_S       24
#define ADI_MASK8B23_D1_M       0x00FF0000  // Data for ADI register 3, - only
                                            // bits selected by mask M1 will be
                                            // affected by access
#define ADI_MASK8B23_D1_S       16
#define ADI_MASK8B23_M0_M       0x0000FF00  // Mask for ADI register 2
#define ADI_MASK8B23_M0_S       8
#define ADI_MASK8B23_D0_M       0x000000FF  // Data for ADI register 2, - only
                                            // bits selected by mask M0 will be
                                            // affected by access
#define ADI_MASK8B23_D0_S       0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_MASK8B45 register.
//
//*****************************************************************************
#define ADI_MASK8B45_M1_M       0xFF000000  // Mask for ADI register 5
#define ADI_MASK8B45_M1_S       24
#define ADI_MASK8B45_D1_M       0x00FF0000  // Data for ADI register 5, - only
                                            // bits selected by mask M1 will be
                                            // affected by access
#define ADI_MASK8B45_D1_S       16
#define ADI_MASK8B45_M0_M       0x0000FF00  // Mask for ADI register 4
#define ADI_MASK8B45_M0_S       8
#define ADI_MASK8B45_D0_M       0x000000FF  // Data for ADI register 4, - only
                                            // bits selected by mask M0 will be
                                            // affected by access
#define ADI_MASK8B45_D0_S       0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_MASK8B67 register.
//
//*****************************************************************************
#define ADI_MASK8B67_M1_M       0xFF000000  // Mask for ADI register 7
#define ADI_MASK8B67_M1_S       24
#define ADI_MASK8B67_D1_M       0x00FF0000  // Data for ADI register 7, - only
                                            // bits selected by mask M1 will be
                                            // affected by access
#define ADI_MASK8B67_D1_S       16
#define ADI_MASK8B67_M0_M       0x0000FF00  // Mask for ADI register 6
#define ADI_MASK8B67_M0_S       8
#define ADI_MASK8B67_D0_M       0x000000FF  // Data for ADI register 6, - only
                                            // bits selected by mask M0 will be
                                            // affected by access
#define ADI_MASK8B67_D0_S       0
//*****************************************************************************
//
// The following are defines for the bit fields in the ADI_O_MASK8B89 register.
//
//*****************************************************************************
#define ADI_MASK8B89_M1_M       0xFF000000  // Mask for ADI register 9
#define ADI_MASK8B89_M1_S       24
#define ADI_MASK8B89_D1_M       0x00FF0000  // Data for ADI register 9, - only
                                            // bits selected by mask M1 will be
                                            // affected by access
#define ADI_MASK8B89_D1_S       16
#define ADI_MASK8B89_M0_M       0x0000FF00  // Mask for ADI register 8
#define ADI_MASK8B89_M0_S       8
#define ADI_MASK8B89_D0_M       0x000000FF  // Data for ADI register 8, - only
                                            // bits selected by mask M0 will be
                                            // affected by access
#define ADI_MASK8B89_D0_S       0
//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_MASK8B1011 register.
//
//*****************************************************************************
#define ADI_MASK8B1011_M1_M     0xFF000000  // Mask for ADI register 11
#define ADI_MASK8B1011_M1_S     24
#define ADI_MASK8B1011_D1_M     0x00FF0000  // Data for ADI register 11, -
                                            // only bits selected by mask M1
                                            // will be affected by access
#define ADI_MASK8B1011_D1_S     16
#define ADI_MASK8B1011_M0_M     0x0000FF00  // Mask for ADI register 10
#define ADI_MASK8B1011_M0_S     8
#define ADI_MASK8B1011_D0_M     0x000000FF  // Data for ADI register 10, -
                                            // only bits selected by mask M0
                                            // will be affected by access
#define ADI_MASK8B1011_D0_S     0
//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_MASK8B1213 register.
//
//*****************************************************************************
#define ADI_MASK8B1213_M1_M     0xFF000000  // Mask for ADI register 13
#define ADI_MASK8B1213_M1_S     24
#define ADI_MASK8B1213_D1_M     0x00FF0000  // Data for ADI register 13, -
                                            // only bits selected by mask M1
                                            // will be affected by access
#define ADI_MASK8B1213_D1_S     16
#define ADI_MASK8B1213_M0_M     0x0000FF00  // Mask for ADI register 12
#define ADI_MASK8B1213_M0_S     8
#define ADI_MASK8B1213_D0_M     0x000000FF  // Data for ADI register 12, -
                                            // only bits selected by mask M0
                                            // will be affected by access
#define ADI_MASK8B1213_D0_S     0
//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_MASK8B1415 register.
//
//*****************************************************************************
#define ADI_MASK8B1415_M1_M     0xFF000000  // Mask for ADI register 15
#define ADI_MASK8B1415_M1_S     24
#define ADI_MASK8B1415_D1_M     0x00FF0000  // Data for ADI register 15, -
                                            // only bits selected by mask M1
                                            // will be affected by access
#define ADI_MASK8B1415_D1_S     16
#define ADI_MASK8B1415_M0_M     0x0000FF00  // Mask for ADI register 14
#define ADI_MASK8B1415_M0_S     8
#define ADI_MASK8B1415_D0_M     0x000000FF  // Data for ADI register 14, -
                                            // only bits selected by mask M0
                                            // will be affected by access
#define ADI_MASK8B1415_D0_S     0
//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_MASK16B01 register.
//
//*****************************************************************************
#define ADI_MASK16B01_M_M       0xFFFF0000  // Mask for ADI register 0 and 1
#define ADI_MASK16B01_M_S       16
#define ADI_MASK16B01_D_M       0x0000FFFF  // Data for ADI register at
                                            // offsets 0 and 1, - only bits
                                            // selected by mask M will be
                                            // affected by access
#define ADI_MASK16B01_D_S       0
//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_MASK16B23 register.
//
//*****************************************************************************
#define ADI_MASK16B23_M_M       0xFFFF0000  // Mask for ADI register 2 and 3
#define ADI_MASK16B23_M_S       16
#define ADI_MASK16B23_D_M       0x0000FFFF  // Data for ADI register at
                                            // offsets 2 and 3, - only bits
                                            // selected by mask M will be
                                            // affected by access
#define ADI_MASK16B23_D_S       0
//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_MASK16B45 register.
//
//*****************************************************************************
#define ADI_MASK16B45_M_M       0xFFFF0000  // Mask for ADI register 4 and 5
#define ADI_MASK16B45_M_S       16
#define ADI_MASK16B45_D_M       0x0000FFFF  // Data for ADI register at
                                            // offsets 4 and 5, - only bits
                                            // selected by mask M will be
                                            // affected by access
#define ADI_MASK16B45_D_S       0
//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_MASK16B67 register.
//
//*****************************************************************************
#define ADI_MASK16B67_M_M       0xFFFF0000  // Mask for ADI register 6 and 7
#define ADI_MASK16B67_M_S       16
#define ADI_MASK16B67_D_M       0x0000FFFF  // Data for ADI register at
                                            // offsets 6 and 7, - only bits
                                            // selected by mask M will be
                                            // affected by access
#define ADI_MASK16B67_D_S       0
//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_MASK16B89 register.
//
//*****************************************************************************
#define ADI_MASK16B89_M_M       0xFFFF0000  // Mask for ADI register 8 and 9
#define ADI_MASK16B89_M_S       16
#define ADI_MASK16B89_D_M       0x0000FFFF  // Data for ADI register at
                                            // offsets 8 and 9, - only bits
                                            // selected by mask M will be
                                            // affected by access
#define ADI_MASK16B89_D_S       0
//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_MASK16B1011 register.
//
//*****************************************************************************
#define ADI_MASK16B1011_M_M     0xFFFF0000  // Mask for ADI register 10 and 11
#define ADI_MASK16B1011_M_S     16
#define ADI_MASK16B1011_D_M     0x0000FFFF  // Data for ADI register at
                                            // offsets 10 and 11, - only bits
                                            // selected by mask M will be
                                            // affected by access
#define ADI_MASK16B1011_D_S     0
//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_MASK16B1213 register.
//
//*****************************************************************************
#define ADI_MASK16B1213_M_M     0xFFFF0000  // Mask for ADI register 12 and 13
#define ADI_MASK16B1213_M_S     16
#define ADI_MASK16B1213_D_M     0x0000FFFF  // Data for ADI register at
                                            // offsets 12 and 13, - only bits
                                            // selected by mask M will be
                                            // affected by access
#define ADI_MASK16B1213_D_S     0
//*****************************************************************************
//
// The following are defines for the bit fields in the
// ADI_O_MASK16B1415 register.
//
//*****************************************************************************
#define ADI_MASK16B1415_M_M     0xFFFF0000  // Mask for ADI register 14 and 15
#define ADI_MASK16B1415_M_S     16
#define ADI_MASK16B1415_D_M     0x0000FFFF  // Data for ADI register at
                                            // offsets 14 and 15, - only bits
                                            // selected by mask M will be
                                            // affected by access
#define ADI_MASK16B1415_D_S     0

#endif // __HW_ADI_H__
