/*
 * Copyright (c) 2022 Schlumberger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MEMORY_CONTROLLER_INFINEON_XMC4XXX_EBU_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MEMORY_CONTROLLER_INFINEON_XMC4XXX_EBU_H_

#define EBU_CLC_SYNC_Pos   16
#define EBU_CLC_DIV2_Pos   17
#define EBU_CLC_EBUDIV_Pos 18

#define EBU_MODCON_SDTRI_Pos	2
#define EBU_MODCON_EXTLOCK_Pos	4
#define EBU_MODCON_ARBSYNC_Pos	5
#define EBU_MODCON_ARBMODE_Pos	6
#define EBU_MODCON_TIMEOUTC_Pos 8
#define EBU_MODCON_ALE_Pos	31

#define EBU_USERCON_ADDIO_Pos 16
#define EBU_USERCON_ADVIO_Pos 25

#define EBU_BUSRCON0_FETBLEN_Pos 0
#define EBU_BUSRCON0_FBBMSEL_Pos 3
#define EBU_BUSRCON0_BFSSS_Pos	 4
#define EBU_BUSRCON0_FDBKEN_Pos	 5
#define EBU_BUSRCON0_BFCMSEL_Pos 6
#define EBU_BUSRCON0_NAA_Pos	 7
#define EBU_BUSRCON0_ECSE_Pos	 16
#define EBU_BUSRCON0_EBSE_Pos	 17
#define EBU_BUSRCON0_DBA_Pos	 18
#define EBU_BUSRCON0_WAITINV_Pos 19
#define EBU_BUSRCON0_BCGEN_Pos	 20
#define EBU_BUSRCON0_WAIT_Pos	 24
#define EBU_BUSRCON0_AAP_Pos	 26

#define EBU_BUSRAP0_RDDTACS_Pos	 0
#define EBU_BUSRAP0_RDRECOVC_Pos 4
#define EBU_BUSRAP0_WAITRDC_Pos	 7
#define EBU_BUSRAP0_DATAC_Pos	 12
#define EBU_BUSRAP0_EXTCLOCK_Pos 16
#define EBU_BUSRAP0_EXTDATA_Pos	 18
#define EBU_BUSRAP0_CMDDELAY_Pos 20
#define EBU_BUSRAP0_AHOLDC_Pos	 24
#define EBU_BUSRAP0_ADDRC_Pos	 28

#define EBU_BUSWCON0_FETBLEN_Pos 0
#define EBU_BUSWCON0_FBBMSEL_Pos 3
#define EBU_BUSWCON0_NAA_Pos	 7
#define EBU_BUSWCON0_ECSE_Pos	 16
#define EBU_BUSWCON0_EBSE_Pos	 17
#define EBU_BUSWCON0_WAITINV_Pos 19
#define EBU_BUSWCON0_BCGEN_Pos	 20
#define EBU_BUSWCON0_WAIT_Pos	 24
#define EBU_BUSWCON0_AAP_Pos	 26
#define EBU_BUSWCON0_LOCKCS_Pos	 27

#define EBU_BUSWAP0_WRDTACS_Pos	 0
#define EBU_BUSWAP0_WRRECOVC_Pos 4
#define EBU_BUSWAP0_WAITWRC_Pos	 7
#define EBU_BUSWAP0_DATAC_Pos	 12
#define EBU_BUSWAP0_EXTCLOCK_Pos 16
#define EBU_BUSWAP0_EXTDATA_Pos	 18
#define EBU_BUSWAP0_CMDDELAY_Pos 20
#define EBU_BUSWAP0_AHOLDC_Pos	 24
#define EBU_BUSWAP0_ADDRC_Pos	 28

#define EBU_SDRMCON_CRAS_Pos	 0
#define EBU_SDRMCON_CRFSH_Pos	 4
#define EBU_SDRMCON_CRSC_Pos	 8
#define EBU_SDRMCON_CRP_Pos	 10
#define EBU_SDRMCON_AWIDTH_Pos	 12
#define EBU_SDRMCON_CRCD_Pos	 14
#define EBU_SDRMCON_CRC_Pos	 16
#define EBU_SDRMCON_ROWM_Pos	 19
#define EBU_SDRMCON_BANKM_Pos	 22
#define EBU_SDRMCON_CRCE_Pos	 25
#define EBU_SDRMCON_CLKDIS_Pos	 28
#define EBU_SDRMCON_PWR_MODE_Pos 29
#define EBU_SDRMCON_SDCMSEL_Pos	 31

#define EBU_SDRMOD_BURSTL_Pos	 0
#define EBU_SDRMOD_BTYP_Pos	 3
#define EBU_SDRMOD_CASLAT_Pos	 4
#define EBU_SDRMOD_OPMODE_Pos	 7
#define EBU_SDRMOD_COLDSTART_Pos 15
#define EBU_SDRMOD_XOPM_Pos	 16
#define EBU_SDRMOD_XBA_Pos	 28

#define EBU_SDRMREF_REFRESHC_Pos    0
#define EBU_SDRMREF_REFRESHR_Pos    6
#define EBU_SDRMREF_SELFREXST_Pos   9
#define EBU_SDRMREF_SELFREX_Pos	    10
#define EBU_SDRMREF_SELFRENST_Pos   11
#define EBU_SDRMREF_AUTOSELFR_Pos   13
#define EBU_SDRMREF_ERFSHC_Pos	    14
#define EBU_SDRMREF_SELFREX_DLY_Pos 16
#define EBU_SDRMREF_ARFSH_Pos	    24
#define EBU_SDRMREF_RES_DLY_Pos	    25

/* Register setters */

#define XMC4XXX_EBU_SET_CLC(sync, div2, ebudiv)                                                    \
	((sync) << EBU_CLC_SYNC_Pos | (div2) << EBU_CLC_DIV2_Pos | (ebudiv) << EBU_CLC_EBUDIV_Pos)

#define XMC4XXX_EBU_SET_MODCON(sdtri, extlock, arbsync, arbmode, timeoutc, ale)                    \
	((sdtri) << EBU_MODCON_SDTRI_Pos | (extlock) << EBU_MODCON_EXTLOCK_Pos |                   \
	 (arbsync) << EBU_MODCON_ARBSYNC_Pos | (arbmode) << EBU_MODCON_ARBMODE_Pos |               \
	 (timeoutc) << EBU_MODCON_TIMEOUTC_Pos | (ale) << EBU_MODCON_ALE_Pos)

#define XMC4XXX_EBU_SET_USERCON(addio, advio)                                                      \
	((addio) << EBU_USERCON_ADDIO_Pos | (advio) << EBU_USERCON_ADVIO_Pos)

#define XMC4XXX_EBU_SET_BUSRCON(fetblen, fbbmsel, bfsss, fdbken, bfcmsel, naa, ecse, ebse, dba,    \
				waitinv, bcgen, wait, aap)                                         \
	((fetblen) << EBU_BUSRCON0_FETBLEN_Pos | (fbbmsel) << EBU_BUSRCON0_FBBMSEL_Pos |           \
	 (bfsss) << EBU_BUSRCON0_BFSSS_Pos | (fdbken) << EBU_BUSRCON0_FDBKEN_Pos |                 \
	 (bfcmsel) << EBU_BUSRCON0_BFCMSEL_Pos | (naa) << EBU_BUSRCON0_NAA_Pos |                   \
	 (ecse) << EBU_BUSRCON0_ECSE_Pos | (ebse) << EBU_BUSRCON0_EBSE_Pos |                       \
	 (dba) << EBU_BUSRCON0_DBA_Pos | (waitinv) << EBU_BUSRCON0_WAITINV_Pos |                   \
	 (bcgen) << EBU_BUSRCON0_BCGEN_Pos | (wait) << EBU_BUSRCON0_WAIT_Pos |                     \
	 (aap) << EBU_BUSRCON0_AAP_Pos)

#define XMC4XXX_EBU_SET_BUSWCON(fetblen, fbbmsel, naa, ecse, ebse, waitinv, bcgen, wait, aap,      \
				lockcs)                                                            \
	((fetblen) << EBU_BUSWCON0_FETBLEN_Pos | (fbbmsel) << EBU_BUSWCON0_FBBMSEL_Pos |           \
	 (naa) << EBU_BUSWCON0_NAA_Pos | (ecse) << EBU_BUSWCON0_ECSE_Pos |                         \
	 (ebse) << EBU_BUSWCON0_EBSE_Pos | (waitinv) << EBU_BUSWCON0_WAITINV_Pos |                 \
	 (bcgen) << EBU_BUSWCON0_BCGEN_Pos | (wait) << EBU_BUSWCON0_WAIT_Pos |                     \
	 (aap) << EBU_BUSWCON0_AAP_Pos | (lockcs) << EBU_BUSWCON0_LOCKCS_Pos)

#define XMC4XXX_EBU_SET_BUSRAP(rddtacs, rdrecovc, waitrdc, datac, extclock, extdata, cmddelay,     \
			       aholdc, addrc)                                                      \
	((rddtacs) << EBU_BUSRAP0_RDDTACS_Pos | (rdrecovc) << EBU_BUSRAP0_RDRECOVC_Pos |           \
	 (waitrdc) << EBU_BUSRAP0_WAITRDC_Pos | (datac) << EBU_BUSRAP0_DATAC_Pos |                 \
	 (extclock) << EBU_BUSRAP0_EXTCLOCK_Pos | (extdata) << EBU_BUSRAP0_EXTDATA_Pos |           \
	 (cmddelay) << EBU_BUSRAP0_CMDDELAY_Pos | (aholdc) << EBU_BUSRAP0_AHOLDC_Pos |             \
	 (addrc) << EBU_BUSRAP0_ADDRC_Pos)

#define XMC4XXX_EBU_SET_BUSWAP(wrdtacs, wrrecovc, waitwrc, datac, extclock, extdata, cmddelay,     \
			       aholdc, addrc)                                                      \
	((wrdtacs) << EBU_BUSWAP0_WRDTACS_Pos | (wrrecovc) << EBU_BUSWAP0_WRRECOVC_Pos |           \
	 (waitwrc) << EBU_BUSWAP0_WAITWRC_Pos | (datac) << EBU_BUSWAP0_DATAC_Pos |                 \
	 (extclock) << EBU_BUSWAP0_EXTCLOCK_Pos | (extdata) << EBU_BUSWAP0_EXTDATA_Pos |           \
	 (cmddelay) << EBU_BUSWAP0_CMDDELAY_Pos | (aholdc) << EBU_BUSWAP0_AHOLDC_Pos |             \
	 (addrc) << EBU_BUSWAP0_ADDRC_Pos)

#define XMC4XXX_EBU_SET_SDRMCON(sdcmsel, pwr_mode, clkdis, crce, bankm, rowm, crc, crcd, awidth,   \
				crp, crsc, crfsh, cras)                                            \
	((sdcmsel) << EBU_SDRMCON_SDCMSEL_Pos | (pwr_mode) << EBU_SDRMCON_PWR_MODE_Pos |           \
	 (clkdis) << EBU_SDRMCON_CLKDIS_Pos | (crce) << EBU_SDRMCON_CRCE_Pos |                     \
	 (bankm) << EBU_SDRMCON_BANKM_Pos | (rowm) << EBU_SDRMCON_ROWM_Pos |                       \
	 (crc) << EBU_SDRMCON_CRC_Pos | (crcd) << EBU_SDRMCON_CRCD_Pos |                           \
	 (awidth) << EBU_SDRMCON_AWIDTH_Pos | (crp) << EBU_SDRMCON_CRP_Pos |                       \
	 (crsc) << EBU_SDRMCON_CRSC_Pos | (crfsh) << EBU_SDRMCON_CRFSH_Pos |                       \
	 (cras) << EBU_SDRMCON_CRAS_Pos)

#define XMC4XXX_EBU_SDRMOD(xba, xopm, coldstart, opmode, caslat, burstl)                           \
	((xba) << EBU_SDRMOD_XBA_Pos | (xopm) << EBU_SDRMOD_XOPM_Pos |                             \
	 (coldstart) << EBU_SDRMOD_COLDSTART_Pos | (opmode) << EBU_SDRMOD_OPMODE_Pos |             \
	 (caslat) << EBU_SDRMOD_CASLAT_Pos | (burstl) << EBU_SDRMOD_BURSTL_Pos)

#define XMC4XXX_EBU_SDRMREF(res_dly, arfsh, selfrex_dly, erfshc, autoselfr, selfrex, refreshr,     \
			    refreshc)                                                              \
	((res_dly) << EBU_SDRMREF_RES_DLY_Pos | (arfsh) << EBU_SDRMREF_ARFSH_Pos |                 \
	 (selfrex_dly) << EBU_SDRMREF_SELFREX_DLY_Pos | (erfshc) << EBU_SDRMREF_ERFSHC_Pos |       \
	 (autoselfr) << EBU_SDRMREF_AUTOSELFR_Pos | (selfrex) << EBU_SDRMREF_SELFREX_Pos |         \
	 (refreshr) << EBU_SDRMREF_REFRESHR_Pos | (refreshc) << EBU_SDRMREF_REFRESHC_Pos)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MEMORY_CONTROLLER_INFINEON_XMC4XXX_EBU_H_ */
