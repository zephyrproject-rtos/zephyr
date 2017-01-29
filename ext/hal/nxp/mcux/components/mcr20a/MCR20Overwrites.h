/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file MCR20Overwrites.h
* Description: Overwrites header file for MCR20 Register values
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* o Redistributions of source code must retain the above copyright notice, this list
*   of conditions and the following disclaimer.
*
* o Redistributions in binary form must reproduce the above copyright notice, this
*   list of conditions and the following disclaimer in the documentation and/or
*   other materials provided with the distribution.
*
* o Neither the name of Freescale Semiconductor, Inc. nor the names of its
*   contributors may be used to endorse or promote products derived from this
*   software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef OVERWRITES_H_
#define OVERWRITES_H_

typedef struct overwrites_tag {
 char address;
 char data;
}overwrites_t;


/*****************************************************************************************************************/
//         This file is created exclusively for use with the transceiver 2.0 silicon
//         and is provided for the world to use. It contains a list of all
//         known overwrite values. Overwrite values are non-default register
//         values that configure the transceiver device to a more optimally performing
//         posture. It is expected that low level software (i.e. PHY) will
//         consume this file as a #include, and transfer the contents to the
//         the indicated addresses in the transceiver's memory space. This file has
//         at least one required entry, that being its own version current version
//         number, to be stored at transceiver's location 0x3B the
//         OVERWRITES_VERSION_NUMBER register. The RAM register is provided in
//         the transceiver address space to assist in future debug efforts. The
//         analyst may read this location (once device has been booted with
//         mysterious software) and have a good indication of what register
//         overwrites were performed (with all versions of the overwrites.h file
//         being archived forever at the Compass location shown above.
//
//     The transceiver has an indirect register (IAR) space. Write access to this space
//         requires 3 or more writes:
//         1st) the first write is an index value to the indirect (write Bit7=0, register access Bit 6=0) + 0x3E
//         2nd) IAR Register #0x00 - 0xFF.
//     3rd) The data to write
//         nth) Burst mode additional data if required.
//
//     Write access to direct space requires only a single address, data pair.

overwrites_t const overwrites_direct[] ={
{0x3B, 0x0C}, //version 0C: new value for ACKDELAY targeting 198us (23 May, 2013, Larry Roshak)
{0x23, 0x17}  //PA_PWR new default Power Step is "23"  
};

overwrites_t const overwrites_indirect[] ={
{0x31, 0x02}, //clear MISO_HIZ_EN (for single SPI master/slave pair) and SPI_PUL_EN (minimize HIB currents) 
{0x91, 0xB3}, //VCO_CTRL1 override VCOALC_REF_TX to 3                         
{0x92, 0x07}, //VCO_CTRL2 override VCOALC_REF_RX to 3, keep VCO_BUF_BOOST = 1 
{0x8A, 0x71}, //PA_TUNING override PA_COILTUNING to 001 (27 Nov 2012, D. Brown, on behalf of S. Eid)
{0x79, 0x2F}, //CHF_IBUF  Adjust the gm-C filter gain (+/- 6dB)         (21 Dec, 2012, on behalf of S. Soca)
{0x7A, 0x2F}, //CHF_QBUF  Adjust the gm-C filter gain (+/- 6dB)         (21 Dec, 2012, on behalf of S. Soca)
{0x7B, 0x24}, //CHF_IRIN  Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7C, 0x24}, //CHF_QRIN  Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7D, 0x24}, //CHF_IL    Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7E, 0x24}, //CHF_QL    Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7F, 0x32}, //CHF_CC1   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x80, 0x1D}, //CHF_CCL   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x81, 0x2D}, //CHF_CC2   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x82, 0x24}, //CHF_IROUT Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x83, 0x24}, //CHF_QROUT Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x64, 0x28}, //PA_CAL_DIS=1  Disabled PA calibration 
{0x52, 0x55}, //AGC_THR1 RSSI tune up 
{0x53, 0x2D}, //AGC_THR2 RSSI tune up 
{0x66, 0x5F}, //ATT_RSSI1 tune up     
{0x67, 0x8F}, //ATT_RSSI2 tune up     
{0x68, 0x61}, //RSSI_OFFSET 
{0x78, 0x03}, //CHF_PMAGAIN 
{0x22, 0x50}, //CCA1_THRESH 
{0x4D, 0x13}, //CORR_NVAL moved from 0x14 to 0x13 for 0.5 dB improved Rx Sensitivity 
{0x39, 0x3D}  //ACKDELAY new value targeting a delay of 198us (23 May, 2013, Larry Roshak)
};


/* begin of deprecated versions

==VERSION 1==
(version 1 is empty)

==VERSION 2==
overwrites_t const overwrites_indirect[] ={
{0x31, 0x02}  //clear MISO_HIZ_EN (for single SPI master/slave pair) and SPI_PUL_EN (minimize HIB currents) 
};

==VERSION 3==
overwrites_t const overwrites_indirect[] ={
{0x31, 0x02}, //clear MISO_HIZ_EN (for single SPI master/slave pair) and SPI_PUL_EN (minimize HIB currents) 
{0x91, 0xB3}, //VCO_CTRL1: override VCOALC_REF_TX to 3 
{0x92, 0x07}  //VCO_CTRL2: override VCOALC_REF_RX to 3, keep VCO_BUF_BOOST = 1 
};

==VERSION 4==
overwrites_t const overwrites_direct[] ={
{0x3B, 0x04}  //version 04 is the current version: update PA_COILTUNING default 
};

overwrites_t const overwrites_indirect[] ={
{0x31, 0x02}, //clear MISO_HIZ_EN (for single SPI master/slave pair) and SPI_PUL_EN (minimize HIB currents) 
{0x91, 0xB3}, //VCO_CTRL1: override VCOALC_REF_TX to 3 
{0x92, 0x07}  //VCO_CTRL2: override VCOALC_REF_RX to 3, keep VCO_BUF_BOOST = 1 
{0x8A, 0x71}  //PA_TUNING: override PA_COILTUNING to 001 (27 Nov 2012, D. Brown, on behalf of S. Eid)
};

==VERSION 5==
overwrites_t const overwrites_direct[] ={
{0x3B, 0x05}  //version 05: updates Channel Filter Register set (21 Dec 2012, on behalf of S. Soca)
};

overwrites_t const overwrites_indirect[] ={
{0x31, 0x02}, //clear MISO_HIZ_EN (for single SPI master/slave pair) and SPI_PUL_EN (minimize HIB currents) 
{0x91, 0xB3}, //VCO_CTRL1 override VCOALC_REF_TX to 3                         
{0x92, 0x07}  //VCO_CTRL2 override VCOALC_REF_RX to 3, keep VCO_BUF_BOOST = 1 
{0x8A, 0x71}  //PA_TUNING override PA_COILTUNING to 001 (27 Nov 2012, D. Brown, on behalf of S. Eid)
{0x79, 0x2F}  //CHF_IBUF  Adjust the gm-C filter gain (+/- 6dB)         (21 Dec, 2012, on behalf of S. Soca)
{0x7A, 0x2F}  //CHF_QBUF  Adjust the gm-C filter gain (+/- 6dB)         (21 Dec, 2012, on behalf of S. Soca)
{0x7B, 0x24}  //CHF_IRIN  Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7C, 0x24}  //CHF_QRIN  Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7D, 0x24}  //CHF_IL    Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7E, 0x24}  //CHF_QL    Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x82, 0x24}  //CHF_IROUT Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x83, 0x24}  //CHF_QROUT Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7F, 0x32}  //CHF_CC1   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x80, 0x1D}  //CHF_CCL   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x81, 0x2D}  //CHF_CC2   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
};

==VERSION 6==
overwrites_t const overwrites_direct[] ={
{0x3B, 0x06}  //version 06: disable PA calibration 
};

overwrites_t const overwrites_indirect[] ={
{0x31, 0x02}, //clear MISO_HIZ_EN (for single SPI master/slave pair) and SPI_PUL_EN (minimize HIB currents) 
{0x91, 0xB3}, //VCO_CTRL1 override VCOALC_REF_TX to 3                         
{0x92, 0x07}  //VCO_CTRL2 override VCOALC_REF_RX to 3, keep VCO_BUF_BOOST = 1 
{0x8A, 0x71}  //PA_TUNING override PA_COILTUNING to 001 (27 Nov 2012, D. Brown, on behalf of S. Eid)
{0x79, 0x2F}  //CHF_IBUF  Adjust the gm-C filter gain (+/- 6dB)         (21 Dec, 2012, on behalf of S. Soca)
{0x7A, 0x2F}  //CHF_QBUF  Adjust the gm-C filter gain (+/- 6dB)         (21 Dec, 2012, on behalf of S. Soca)
{0x7B, 0x24}  //CHF_IRIN  Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7C, 0x24}  //CHF_QRIN  Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7D, 0x24}  //CHF_IL    Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7E, 0x24}  //CHF_QL    Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x82, 0x24}  //CHF_IROUT Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x83, 0x24}  //CHF_QROUT Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7F, 0x32}  //CHF_CC1   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x80, 0x1D}  //CHF_CCL   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x81, 0x2D}  //CHF_CC2   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x64, 0x28}  //PA_CAL_DIS=1  Disabled PA calibration 
};

==VERSION 7==
overwrites_t const overwrites_direct[] ={
{0x3B, 0x07}  //version 07: updated registers for ED/RSSI 
};

overwrites_t const overwrites_indirect[] ={
{0x31, 0x02}, //clear MISO_HIZ_EN (for single SPI master/slave pair) and SPI_PUL_EN (minimize HIB currents) 
{0x91, 0xB3}, //VCO_CTRL1 override VCOALC_REF_TX to 3                         
{0x92, 0x07},  //VCO_CTRL2 override VCOALC_REF_RX to 3, keep VCO_BUF_BOOST = 1 
{0x8A, 0x71},  //PA_TUNING override PA_COILTUNING to 001 (27 Nov 2012, D. Brown, on behalf of S. Eid)
{0x79, 0x2F},  //CHF_IBUF  Adjust the gm-C filter gain (+/- 6dB)         (21 Dec, 2012, on behalf of S. Soca)
{0x7A, 0x2F},  //CHF_QBUF  Adjust the gm-C filter gain (+/- 6dB)         (21 Dec, 2012, on behalf of S. Soca)
{0x7B, 0x24},  //CHF_IRIN  Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7C, 0x24},  //CHF_QRIN  Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7D, 0x24},  //CHF_IL    Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7E, 0x24},  //CHF_QL    Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x82, 0x24},  //CHF_IROUT Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x83, 0x24},  //CHF_QROUT Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7F, 0x32}, //CHF_CC1   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x80, 0x1D},  //CHF_CCL   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x81, 0x2D},  //CHF_CC2   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x64, 0x28},  //PA_CAL_DIS=1  Disabled PA calibration 
{0x52, 0x73},  //AGC_THR1 RSSI tune up 
{0x53, 0x2D}, //AGC_THR2 RSSI tune up 
{0x66, 0x5F}, //ATT_RSSI1 tune up 
{0x67, 0x8F}, //ATT_RSSI2 tune up 
{0x68, 0x60}, //RSSI_OFFSET 
{0x69, 0x65}  //RSSI_SLOPE 
};


==VERSION 8==
overwrites_t const overwrites_direct[] ={
{0x3B, 0x08}  //version 08: updated registers for ED/RSSI 
};

overwrites_t const overwrites_indirect[] ={
{0x31, 0x02}, //clear MISO_HIZ_EN (for single SPI master/slave pair) and SPI_PUL_EN (minimize HIB currents) 
{0x91, 0xB3}, //VCO_CTRL1 override VCOALC_REF_TX to 3                         
{0x92, 0x07}, //VCO_CTRL2 override VCOALC_REF_RX to 3, keep VCO_BUF_BOOST = 1 
{0x8A, 0x71}, //PA_TUNING override PA_COILTUNING to 001 (27 Nov 2012, D. Brown, on behalf of S. Eid)
{0x79, 0x2F}, //CHF_IBUF  Adjust the gm-C filter gain (+/- 6dB)         (21 Dec, 2012, on behalf of S. Soca)
{0x7A, 0x2F}, //CHF_QBUF  Adjust the gm-C filter gain (+/- 6dB)         (21 Dec, 2012, on behalf of S. Soca)
{0x7B, 0x24}, //CHF_IRIN  Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7C, 0x24}, //CHF_QRIN  Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7D, 0x24}, //CHF_IL    Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7E, 0x24}, //CHF_QL    Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x82, 0x24}, //CHF_IROUT Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x83, 0x24}, //CHF_QROUT Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7F, 0x32}, //CHF_CC1   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x80, 0x1D}, //CHF_CCL   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x81, 0x2D}, //CHF_CC2   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x64, 0x28}, //PA_CAL_DIS=1  Disabled PA calibration 
{0x52, 0x73}, //AGC_THR1 RSSI tune up 
{0x53, 0x2D}, //AGC_THR2 RSSI tune up 
{0x66, 0x5F}, //ATT_RSSI1 tune up     
{0x67, 0x8F}, //ATT_RSSI2 tune up     
{0x69, 0x65}  //RSSI_SLOPE            
{0x68, 0x61}, //RSSI_OFFSET 
{0x78, 0x03}  //CHF_PMAGAIN 
};


==VERSION 9==
overwrites_t const overwrites_direct[] ={
{0x3B, 0x09}  //version 09: updated registers for ED/RSSI and PowerStep 
{0x23, 0x17}  //PA_PWR new default value                                
};

overwrites_t const overwrites_indirect[] ={
{0x31, 0x02}, //clear MISO_HIZ_EN (for single SPI master/slave pair) and SPI_PUL_EN (minimize HIB currents) 
{0x91, 0xB3}, //VCO_CTRL1 override VCOALC_REF_TX to 3                         
{0x92, 0x07}, //VCO_CTRL2 override VCOALC_REF_RX to 3, keep VCO_BUF_BOOST = 1 
{0x8A, 0x71}, //PA_TUNING override PA_COILTUNING to 001 (27 Nov 2012, D. Brown, on behalf of S. Eid)
{0x79, 0x2F}, //CHF_IBUF  Adjust the gm-C filter gain (+/- 6dB)         (21 Dec, 2012, on behalf of S. Soca)
{0x7A, 0x2F}, //CHF_QBUF  Adjust the gm-C filter gain (+/- 6dB)         (21 Dec, 2012, on behalf of S. Soca)
{0x7B, 0x24}, //CHF_IRIN  Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7C, 0x24}, //CHF_QRIN  Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7D, 0x24}, //CHF_IL    Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7E, 0x24}, //CHF_QL    Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7F, 0x32}, //CHF_CC1   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x80, 0x1D}, //CHF_CCL   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x81, 0x2D}, //CHF_CC2   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x82, 0x24}, //CHF_IROUT Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x83, 0x24}, //CHF_QROUT Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x64, 0x28}, //PA_CAL_DIS=1  Disabled PA calibration 
{0x52, 0x55}, //AGC_THR1 RSSI tune up 
{0x53, 0x2D}, //AGC_THR2 RSSI tune up 
{0x66, 0x5F}, //ATT_RSSI1 tune up     
{0x67, 0x8F}, //ATT_RSSI2 tune up     
{0x68, 0x61}, //RSSI_OFFSET 
{0x78, 0x03}  //CHF_PMAGAIN 
};

==VERSION A==
overwrites_t const overwrites_direct[] ={
{0x3B, 0x0A}  //version 0A: updated registers for CCA 
{0x23, 0x17}  //PA_PWR new default Power Step is "23"  
};

overwrites_t const overwrites_indirect[] ={
{0x31, 0x02}, //clear MISO_HIZ_EN (for single SPI master/slave pair) and SPI_PUL_EN (minimize HIB currents) 
{0x91, 0xB3}, //VCO_CTRL1 override VCOALC_REF_TX to 3                         
{0x92, 0x07}, //VCO_CTRL2 override VCOALC_REF_RX to 3, keep VCO_BUF_BOOST = 1 
{0x8A, 0x71}, //PA_TUNING override PA_COILTUNING to 001 (27 Nov 2012, D. Brown, on behalf of S. Eid)
{0x79, 0x2F}, //CHF_IBUF  Adjust the gm-C filter gain (+/- 6dB)         (21 Dec, 2012, on behalf of S. Soca)
{0x7A, 0x2F}, //CHF_QBUF  Adjust the gm-C filter gain (+/- 6dB)         (21 Dec, 2012, on behalf of S. Soca)
{0x7B, 0x24}, //CHF_IRIN  Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7C, 0x24}, //CHF_QRIN  Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7D, 0x24}, //CHF_IL    Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7E, 0x24}, //CHF_QL    Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x7F, 0x32}, //CHF_CC1   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x80, 0x1D}, //CHF_CCL   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x81, 0x2D}, //CHF_CC2   Adjust the filter center frequency (+/- 1MHz) (21 Dec, 2012, on behalf of S. Soca)
{0x82, 0x24}, //CHF_IROUT Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x83, 0x24}, //CHF_QROUT Adjust the filter bandwidth (+/- 0.5MHz)      (21 Dec, 2012, on behalf of S. Soca)
{0x64, 0x28}, //PA_CAL_DIS=1  Disabled PA calibration 
{0x52, 0x55}, //AGC_THR1 RSSI tune up 
{0x53, 0x2D}, //AGC_THR2 RSSI tune up 
{0x66, 0x5F}, //ATT_RSSI1 tune up     
{0x67, 0x8F}, //ATT_RSSI2 tune up     
{0x68, 0x61}, //RSSI_OFFSET 
{0x78, 0x03}  //CHF_PMAGAIN 
{0x22, 0x50}  //CCA1_THRESH 
};

end of deprecated versions */


#endif  //OVERWRITES_H_

