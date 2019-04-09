/******************************************************************************
*  Filename:       hw_ddi.h
*  Revised:        2017-06-05 12:13:49 +0200 (Mon, 05 Jun 2017)
*  Revision:       49096
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

#ifndef __HW_DDI_H__
#define __HW_DDI_H__

//*****************************************************************************
//
// This file contains macros for controlling the DDI master and
// accessing DDI Slave registers via the DDI Master.
// There are 3 categories of macros in this file:
//                 - macros that provide an offset to a register
//                   located within the DDI Master itself.
//                 - macros that define bits or bitfields
//                   within the DDI Master Registers.
//                 - macros that provide an "instruction offset"
//                   that are used when accessing a DDI Slave.
//
// The macros that that provide DDI Master register offsets and
// define bits and bitfields for those registers are the typical
// macros that appear in most hw_<module>.h header files.  In
// the following example DDI_O_CFG is a macro for a
// register offset and DDI_CFG_WAITFORACK is a macro for
// a bit in that register. This example code will set the WAITFORACK
// bit in register DDI_O_CFG of the DDI Master. (Note: this
// access the Master not the Slave).
//
//    HWREG(AUX_OSCDDI_BASE + DDI_O_CFG) |= DDI_CFG_WAITFORACK;
//
//
// The "instruction offset" macros are used to pass an instruction to
// the DDI Master when accessing DDI slave registers. These macros are
// only used when accessing DDI Slave Registers. (Remember DDI
// Master Registers are accessed normally).
//
// The instructions supported when accessing a DDI Slave Regsiter follow:
//        - Direct Access to a DDI Slave register. I.e. read or
//          write the register.
//        - Set the specified bits in a DDI Slave register.
//        - Clear the specified bits in a DDI Slave register.
//        - Mask write of 4 bits to the a DDI Slave register.
//        - Mask write of 8 bits to the a DDI Slave register.
//        - Mask write of 16 bits to the a DDI Slave register.
//
// Note: only the "Direct Access" offset should be used when reading
// a DDI Slave register. Only 8- and 16-bit reads are supported.
//
// The generic format of using this marcos for a read follows:
//       // read low 16-bits in DDI_SLAVE_OFF
//       myushortvar = HWREGH(DDI_MASTER_BASE + DDI_SLAVE_OFF + DDI_O_DIR);
//
//       // read high 16-bits in DDI_SLAVE_OFF
//       // add 2 for data[31:16]
//       myushortvar = HWREGH(DDI_MASTER_BASE + DDI_SLAVE_OFF + 2 + DDI_O_DIR);

//       // read data[31:24] byte in DDI_SLAVE_OFF
//       // add 3 for data[31:24]
//       myuchar = HWREGB(DDI_MASTER_BASE + DDI_SLAVE_OFF + 3 + DDI_O_DIR);
//
// Notes: In the above example:
//     - DDI_MASTER_BASE is the base address of the DDI Master defined
//       in the hw_memmap.h header file.
//     - DDI_SLAVE_OFF is the DDI Slave offset defined in the
//       hw_<ddi_slave>.h header file (e.g. hw_osc_top.h for the oscsc
//       oscillator modules.
//     - DDI_O_DIR is the "instruction offset" macro defined in this
//       file that specifies the Direct Access instruction.
//
// Writes can use any of the "instruction macros".
// The following examples do a "direct write" to DDI Slave register
// DDI_SLAVE_OFF using different size operands:
//
//     // ---------- DIRECT WRITES ----------
//     // Write 32-bits aligned
//     HWREG(DDI_MASTER_BASE + DDI_SLAVE_OFF + DDI_O_DIR) = 0x12345678;

//     // Write 16-bits aligned to high 16-bits then low 16-bits
//     // Add 2 to get to high 16-bits.
//     HWREGH(DDI_MASTER_BASE + DDI_SLAVE_OFF + DDI_O_DIR + 2) = 0xabcd;
//     HWREGH(DDI_MASTER_BASE + DDI_SLAVE_OFF + DDI_O_DIR) = 0xef01;
//
//     // Write each byte at DDI_SLAVE_OFF, one at a time.
//     // Add 1,2,or 3 to get to bytes 1,2, or 3.
//     HWREGB(DDI_MASTER_BASE + DDI_SLAVE_OFF + DDI_O_DIR) = 0x33;
//     HWREGB(DDI_MASTER_BASE + DDI_SLAVE_OFF + DDI_O_DIR + 1) = 0x44;
//     HWREGB(DDI_MASTER_BASE + DDI_SLAVE_OFF + DDI_O_DIR + 2) = 0x55;
//     HWREGB(DDI_MASTER_BASE + DDI_SLAVE_OFF + DDI_O_DIR + 3) = 0x66;
//
//     // ---------- SET/CLR ----------
//     The set and clear functions behave similarly to eachother. Each
//     can be performed on an 8-, 16-, or 32-bit operand.
//     Examples follow:
//     // Set all odd bits in a 32-bit words
//     HWREG(DDI_MASTER_BASE + DDI_SLAVE_OFF + DDI_O_SET) = 0xaaaaaaaa;
//
//     // Clear all bits in byte 2 (data[23:16]) using 32-bit operand
//     HWREG(DDI_MASTER_BASE + DDI_SLAVE_OFF + DDI_O_CLR) = 0x00ff0000;
//
//     // Set even bits in byte 2 (data[23:16]) using 8-bit operand
//     HWREGB(DDI_MASTER_BASE + DDI_SLAVE_OFF  + 2 + DDI_O_CLR) = 0x55;
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
//     // Write 5555 to low 16-bits of DDI_SLAVE_OFF register
//     // a long write is needed (32-bits).
//     HWREG(DDI_MASTER_BASE + DDI_SLAVE_OFF + DDI_O_MASK16B) = 0xffff5555;

//     // Write 1AA to data bits 24:16 in high 16-bits of DDI_SLAVE_OFF register
//     // Note add 4 for high 16-bits at DDI_SLAVE_OFF; mask is 1ff!
//     HWREG(DDI_MASTER_BASE + DDI_SLAVE_OFF + DDI_O_MASK16B + 4) = 0x01ff01aa;
//
//     // Do an 8 bit masked write of 00 to low byte of register (data[7:0]).
//     // a short write is needed (16-bits).
//     HWREGH(DDI_MASTER_BASE + DDI_SLAVE_OFF + DDI_O_MASK16B) = 0xff00;
//
//     // Do an 8 bit masked write of 11 to byte 1 of register (data[15:8]).
//     // add 2 to get to byte 1.
//     HWREGH(DDI_MASTER_BASE + DDI_SLAVE_OFF + DDI_O_MASK16B + 2) = 0xff11;
//
//     // Do an 8 bit masked write of 33 to high byte of register (data[31:24]).
//     // add 6 to get to byte 3.
//     HWREGH(DDI_MASTER_BASE + DDI_SLAVE_OFF + DDI_O_MASK16B + 6) = 0xff33;
//
//     // Do an 4 bit masked write (Nibble) of 7 to data[3:0]).
//     // Byte write is needed.
//     HWREGB(DDI_MASTER_BASE + DDI_SLAVE_OFF + DDI_O_MASK16B) = 0xf7;
//
//     // Do an 4 bit masked write of 4 to data[7:4]).
//     // Add 1 for next nibble
//     HWREGB(DDI_MASTER_BASE + DDI_SLAVE_OFF + DDI_O_MASK16B + 1) = 0xf4;
//
//*****************************************************************************

//*****************************************************************************
//
// The following are defines for the DDI master instruction offsets.
//
//*****************************************************************************
#define DDI_O_DIR             0x00000000  // Offset for the direct access instruction
#define DDI_O_SET             0x00000080  // Offset for 'Set' instruction.
#define DDI_O_CLR             0x00000100  // Offset for 'Clear' instruction.
#define DDI_O_MASK4B          0x00000200  // Offset for 4-bit masked access.
                                          // Data bit[n] is written if mask bit[n] is set ('1').
                                          // Bits 7:4 are mask. Bits 3:0 are data.
                                          // Requires 'byte' write.
#define DDI_O_MASK8B          0x00000300  // Offset for 8-bit masked access.
                                          // Data bit[n] is written if mask bit[n] is set ('1').
                                          // Bits 15:8 are mask. Bits 7:0 are data.
                                          // Requires 'short' write.
#define DDI_O_MASK16B         0x00000400  // Offset for 16-bit masked access.
                                          // Data bit[n] is written if mask bit[n] is set ('1').
                                          // Bits 31:16 are mask. Bits 15:0 are data.
                                          // Requires 'long' write.



#endif // __HW_DDI_H__
