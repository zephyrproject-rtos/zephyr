/*
 * Copyright 2014-2015, Imagination Technologies Limited and/or its
 *                      affiliated group companies.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
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

#ifndef _MIPS_PRID_H_
#define _MIPS_PRID_H_

/*
 * MIPS CPU types
 */
#define	PRID_R2000	0x01	/* MIPS R2000 CPU		ISA I   */
#define	PRID_R3000	0x02	/* MIPS R3000 CPU		ISA I   */
#define	PRID_R6000	0x03	/* MIPS R6000 CPU		ISA II	*/
#define	PRID_R4000	0x04	/* MIPS R4000/4400 CPU		ISA III	*/
#define PRID_LR33K	0x05	/* LSI Logic R3000 derivate	ISA I	*/
#define	PRID_R6000A	0x06	/* MIPS R6000A CPU		ISA II	*/
#define	PRID_R3IDT	0x07	/* IDT R3000 derivates		ISA I	*/
#define	 PRID_R3IDT_R3041  0x07	  /* R3041 (cp_rev field) */
#define	 PRID_R3IDT_R36100 0x10	  /* R36100 (cp_rev field) */
#define	PRID_R10000	0x09	/* MIPS R10000/T5 CPU		ISA IV  */
#define	PRID_R4200	0x0a	/* MIPS R4200 CPU (ICE)		ISA III */
#define PRID_R4300	0x0b	/* NEC VR4300 CPU		ISA III */
#define PRID_R4100	0x0c	/* NEC VR4100 CPU		ISA III */
#define	PRID_R8000	0x10	/* MIPS R8000 Blackbird/TFP	ISA IV  */
#define	PRID_RC6457X	0x15	/* IDT RC6457X CPU		ISA IV  */
#define	PRID_RC3233X	0x18	/* IDT RC3233X CPU		ISA MIPS32 */
#define	PRID_R4600	0x20	/* QED R4600 Orion		ISA III */
#define	PRID_R4700	0x21	/* QED R4700 Orion		ISA III */
#define	PRID_R3900	0x22	/* Toshiba/Philips R3900 CPU	ISA I	*/
#define	PRID_R4650	0x22	/* QED R4650/R4640 CPU		ISA III */
#define	PRID_R5000	0x23	/* MIPS R5000 CPU		ISA IV  */
#define	PRID_RC3236X	0x26	/* IDT RC3236X CPU		ISA MIPS32 */
#define	PRID_RM7000	0x27	/* QED RM7000 CPU		ISA IV  */
#define	PRID_RM52XX	0x28	/* QED RM52XX CPU		ISA IV  */
#define	PRID_RC6447X	0x30	/* IDT RC6447X CPU		ISA III */
#define	PRID_R5400	0x54	/* NEC Vr5400 CPU		ISA IV  */
#define	PRID_R5500	0x55	/* NEC Vr5500 CPU		ISA IV  */
#define PRID_JADE	0x80	/* MIPS Jade (obsolete name)	ISA MIPS32 */
#define PRID_4KC	0x80	/* MIPS 4Kc (TLB)		ISA MIPS32 */
#define PRID_5KC	0x81	/* MIPS 5Kc			ISA MIPS64 */
#define PRID_20KC	0x82	/* MIPS 20Kc			ISA MIPS64 */
#define PRID_4KMP	0x83	/* MIPS 4Kp/4Km (FM)		ISA MIPS32 */
#define PRID_4KEC	0x84	/* MIPS 4KEc (TLB)		ISA MIPS32 */
#define PRID_4KEMP	0x85	/* MIPS 4KEm/4KEp (FM)		ISA MIPS32 */
#define PRID_4KSC	0x86	/* MIPS 4KSc 			ISA MIPS32 */
#define PRID_M4K	0x87	/* MIPS M4K			ISA MIPS32r2 */
#define PRID_25KF	0x88	/* MIPS 25Kf			ISA MIPS64 */
#define PRID_5KE	0x89	/* MIPS 5KE			ISA MIPS64r2 */
#define PRID_4KEC_R2	0x90	/* MIPS 4KEc (TLB)		ISA MIPS32r2 */
#define PRID_4KEMP_R2	0x91	/* MIPS 4KEm/4KEp (FM)		ISA MIPS32r2 */
#define PRID_4KSD	0x92	/* MIPS 4KSd			ISA MIPS32r2 */
#define PRID_24K	0x93	/* MIPS 24K			ISA MIPS32r2 */
#define PRID_34K	0x95	/* MIPS 34K			ISA MIPS32r2 */
#define PRID_24KE	0x96	/* MIPS 24KE			ISA MIPS32r2 */
#define PRID_74K	0x97	/* MIPS 74K			ISA MIPS32r2 */
#define PRID_1004K	0x99	/* MIPS 1004K			ISA MIPS32r2 */
#define PRID_1074K      0x9A    /* MIPS 1074K                   ISA MIPS32r2 */
#define PRID_M14K       0x9B    /* MIPS M14K                    ISA MIPS32r2 */
#define PRID_M14KC      0x9C    /* MIPS M14KC                   ISA MIPS32r2 */
#define PRID_M14KE      0x9D    /* MIPS M14KE                   ISA MIPS32r2 */
#define PRID_M14KEC     0x9E    /* MIPS M14KEC                  ISA MIPS32r2 */
#define PRID_INTERAPTIV_UP 0xA0    /* MIPS INTERAPTIV UP        ISA MIPS32r2 */
#define PRID_INTERAPTIV_MP 0xA1    /* MIPS INTERAPTIV MP        ISA MIPS32r2 */
#define PRID_PROAPTIV_UP   0xA2    /* MIPS PROAPTIV UP          ISA MIPS32r2 */
#define PRID_PROAPTIV_MP   0xA3    /* MIPS PROAPTIV MP          ISA MIPS32r2 */
#define PRID_M5100      0xA6    /* MIPS WARRIOR M5100           ISA MIPS32r2 */
#define PRID_M5150      0xA7    /* MIPS WARRIOR M5150           ISA MIPS32r2 */
#define PRID_P5600      0xA8    /* MIPS WARRIOR P5600           ISA MIPS32r2 */
#define PRID_I6400      0xA9    /* MIPS I6400                   ISA MIPS64r6 */

/*
 * MIPS FPU types
 */
#define	PRID_SOFT	0x00	/* Software emulation		ISA I   */
#define	PRID_R2360	0x01	/* MIPS R2360 FPC		ISA I   */
#define	PRID_R2010	0x02	/* MIPS R2010 FPC		ISA I   */
#define	PRID_R3010	0x03	/* MIPS R3010 FPC		ISA I   */
#define	PRID_R6010	0x04	/* MIPS R6010 FPC		ISA II  */
#define	PRID_R4010	0x05	/* MIPS R4000/R4400 FPC		ISA II  */
#define PRID_LR33010	0x06	/* LSI Logic derivate		ISA I	*/
#define	PRID_R10010	0x09	/* MIPS R10000/T5 FPU		ISA IV  */
#define	PRID_R4210	0x0a	/* MIPS R4200 FPC (ICE)		ISA III */
#define PRID_UNKF1	0x0b	/* unnanounced product cpu	ISA III */
#define	PRID_R8010	0x10	/* MIPS R8000 Blackbird/TFP	ISA IV  */
#define	PRID_RC6457XF	0x15	/* IDT RC6457X FPU		ISA IV  */
#define	PRID_R4610	0x20	/* QED R4600 Orion		ISA III */
#define	PRID_R3SONY	0x21	/* Sony R3000 based FPU		ISA I   */
#define	PRID_R3910	0x22	/* Toshiba/Philips R3900 FPU	ISA I	*/
#define	PRID_R5010	0x23	/* MIPS R5000 FPU		ISA IV  */
#define	PRID_RM7000F	0x27	/* QED RM7000 FPU		ISA IV  */
#define	PRID_RM52XXF	0x28	/* QED RM52X FPU		ISA IV  */
#define	PRID_RC6447XF	0x30	/* IDT RC6447X FPU		ISA III */
#define	PRID_R5400F	0x54	/* NEC Vr5400 FPU		ISA IV  */
#define	PRID_R5500F	0x55	/* NEC Vr5500 FPU		ISA IV  */
#define PRID_20KCF	0x82	/* MIPS 20Kc FPU		ISA MIPS64 */
#define PRID_5KF	0x81	/* MIPS 5Kf FPU			ISA MIPS64 */
#define PRID_25KFF	0x88	/* MIPS 25Kf FPU		ISA MIPS64 */
#define PRID_5KEF	0x89	/* MIPS 5KEf FPU		ISA MIPS64r2 */
#define PRID_24KF	0x93	/* MIPS 24Kf FPU		ISA MIPS32r2 */
#define PRID_34KF	0x95	/* MIPS 34K FPU			ISA MIPS32r2 */
#define PRID_24KEF	0x96	/* MIPS 24KE FPU		ISA MIPS32r2 */
#define PRID_74KF	0x97	/* MIPS 74K FPU			ISA MIPS32r2 */

#endif /*  _MIPS_PRID_H_ */
