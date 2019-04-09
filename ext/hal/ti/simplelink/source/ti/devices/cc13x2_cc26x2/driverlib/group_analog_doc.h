/******************************************************************************
*  Filename:       group_analog_doc.h
*  Revised:        2016-08-30 14:34:13 +0200 (Tue, 30 Aug 2016)
*  Revision:       47080
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
//! \addtogroup analog_group
//! @{
//! \section sec_analog Introduction
//!
//! Access to registers in the analog domain of the device goes through master modules controlling slave
//! modules which contain the actual registers. The master module is located in the digital domain of the
//! device. The interfaces between master and slave modules are called ADI (Analog-to-Digital Interface)
//! and DDI (Digital-to-Digital Interface) depending on the type of module to access and thus the slave
//! modules are referred to as ADI slave and DDI slave.
//!
//! The ADI and DDI APIs provide access to these registers:
//! - <a href="../register_descriptions/ANATOP_MMAP/ADI_2_REFSYS.html" target="_blank">ADI_2_REFSYS</a> : Reference System for generating reference voltages and reference currents.
//!   - Reference system control
//!   - SOC LDO control
//! - <a href="../register_descriptions/ANATOP_MMAP/ADI_3_REFSYS.html" target="_blank">ADI_3_REFSYS</a> : Reference System for generating reference voltages and reference currents.
//!   - Reference system control
//!   - DC/DC control
//! - <a href="../register_descriptions/ANATOP_MMAP/ADI_4_AUX.html" target="_blank">ADI_4_AUX</a> : Controlling analog peripherals of AUX.
//!   - Multiplexers
//!   - Current source
//!   - Comparators
//!   - ADCs
//! - <a href="../register_descriptions/ANATOP_MMAP/DDI_0_OSC.html" target="_blank">DDI_0_OSC</a> : Controlling the oscillators (via AUX domain)
//!
//! The register descriptions of CPU memory map document the ADI/DDI masters. The register descriptions of
//! analog memory map document the ADI/DDI slaves. The ADI/DDI APIs allow the programmer to focus on the
//! slave registers of interest without being concerned with the ADI/DDI master part of the interface.
//!
//! Although the ADI/DDI APIs make the master "transparent" it can be useful to know a few details about
//! the ADI/DDI protocol and how the master handles transactions as it can affect how the system CPU performs.
//! - ADI protocol uses 8-bit write bus compared to 32-bit write bus in DDI. ADI protocol uses 4-bit read
//!   bus compared to 16-bit read bus in DDI. Hence a 32-bit read from an ADI register is translated into 8
//!   transactions in the ADI protocol.
//! - One transaction on the ADI/DDI protocol takes several clock cycles for the master to complete.
//! - ADI slave registers are 8-bit wide.
//! - DDI slave registers are 32-bit wide.
//! - ADI/DDI master supports multiple data width accesses seen from the system CPU
//!   (however, not all bit width accesses are supported by the APIs):
//!   - Read: 8, 16, 32-bit
//!   - Write
//!     - Direct (write, set, clear): 8, 16, 32-bit
//!     - Masked: 4, 8, 16-bit
//!
//! Making posted/buffered writes from the system CPU (default) to the ADI/DDI allows the system CPU to continue
//! while the ADI/DDI master handles the transactions on the ADI/DDI protocol. If using non-posted/non-buffered
//! writes the system CPU will wait for ADI/DDI master to complete the transactions to the slave before continuing
//! execution.
//!
//! Reading from ADI/DDI requires that all transactions on the ADI/DDI protocol have completed before the system CPU
//! receives the response thus the programmer must understand that the response time depends on the number of bytes
//! read. However, due to the 'set', 'clear' and 'masked write' features of the ADI/DDI most writes can be done
//! without the typical read-modify-write sequence thus reducing the need for reads to a minimum.
//!
//! Consequently, if making posted/buffered writes then the written value will not take effect in the
//! analog domain until some point later in time. An alternative to non-posted/non-buffered writes - in order to make
//! sure a written value has taken effect - is to read from the same ADI/DDI as the write as this will keep the system CPU
//! waiting until both the write and the read have completed.
//!
//! \note
//! Do NOT use masked write when writing bit fields spanning the "masked write boundary" i.e. the widest possible
//! masked write that the protocol supports (ADI = 4 bits, DDI = 16 bits). This will put the device into a
//! temporary state - which is potentially harmful to the device - as the bit field will be written over two transactions.
//! Thus to use masked writes:
//! - For ADI the bit field(s) must be within bit 0 to 3 (REG[3:0]) or bit 4 to 7 (REG[7:4]).
//! - For DDI the bit field(s) must be within bit 0 to 15 (REG[15:0]) or bit 16 to 31 (REG[31:16]).
//!
//! \note
//! If masked write is not allowed, a regular read-modify-write is necessary.
//!
//! @}
