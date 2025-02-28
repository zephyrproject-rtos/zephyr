/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_ESPI_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_ESPI_H
typedef struct {
	volatile uint32_t EPSTS;
	volatile uint32_t EPCFG;
	volatile uint32_t EPINTEN;
	volatile uint32_t EPTXINFO;
	volatile uint32_t EPRXADRH;
	volatile uint32_t EPRXADRL;
	volatile uint32_t EPCPRADR;
	volatile uint32_t EPCPRVLD;
	volatile uint32_t EPBUF;
	volatile uint32_t EPBUFSZ;
	volatile uint32_t EPPTRCLR;
	volatile uint32_t ELMSG;
	volatile uint32_t EPMRDA;
	volatile uint32_t EPMRADRL;
	volatile uint32_t EPMRADRH;
	volatile uint32_t EPMRLEN;
	volatile uint32_t EVSTS;
	volatile uint32_t EVCFG;
	volatile uint32_t EVIDX2;
	volatile uint32_t EVIDX3;
	volatile uint32_t EVIDX7;
	volatile uint32_t EVIDX41;
	volatile uint32_t EVIDX42;
	volatile uint32_t EVIDX43;
	volatile uint32_t EVIDX44;
	volatile uint32_t EVIDX47;
	volatile uint32_t EVIDX4A;
	volatile uint32_t EVTXDAT;
	volatile uint32_t EVPVIDX;
	volatile uint32_t EVRXINTEN;
	volatile uint32_t EVTXINTEN;
	volatile uint32_t EOSTS;
	volatile uint32_t EOCFG;
	volatile uint32_t EORXINTEN;
	volatile uint32_t EORXBUF;
	volatile uint32_t EORXLEN;
	volatile uint32_t EOTXBUF;
	volatile uint32_t EOTXLEN;
	volatile uint32_t EOTXCTRL;
	volatile uint32_t EOTXINTEN;
	volatile uint32_t EFSTS;
	volatile uint32_t EFCONF;
	volatile uint32_t EMADR;
	volatile uint32_t EMTRLEN;
	volatile uint32_t EMBUF;
	volatile uint32_t EMCTRL;
	volatile uint32_t EMINTEN;
	volatile uint32_t ESBUF;
	volatile uint32_t ESINTEN;
	volatile uint32_t ESRXADR;
	volatile uint32_t ESRXLEN;
	volatile uint32_t ESPICFG;
	volatile uint32_t ERSTCFG;
	volatile uint32_t EVIDX51;
	volatile uint32_t EVIDX61;
	volatile uint32_t ESRXC;
	volatile uint32_t EFCFG2;
	volatile uint32_t EFCFG3;
	volatile uint32_t EFCFG4;
	volatile const uint32_t RESERVED[3];
	volatile uint32_t ELCTRL;
	volatile uint32_t ESBUF1;
	volatile uint32_t ESPRG0;
	volatile uint32_t ESPRG1;
	volatile uint32_t ESPDRT;
	volatile uint32_t ESP0STR;
	volatile uint32_t ESP1STR;
	volatile uint32_t ESP2STR;
	volatile uint32_t ESP3STR;
	volatile uint32_t ESP4STR;
	volatile uint32_t ESP5STR;
	volatile uint32_t ESP6STR;
	volatile uint32_t ESP7STR;
	volatile uint32_t ESP8STR;
	volatile uint32_t ESP9STR;
	volatile uint32_t ESPASTR;
	volatile uint32_t ESPBSTR;
	volatile uint32_t ESPCSTR;
	volatile uint32_t ESPDSTR;
	volatile uint32_t ESPESTR;
	volatile uint32_t ESPFSTR;
	volatile uint32_t ESP0LEN;
	volatile uint32_t ESP1LEN;
	volatile uint32_t ESP2LEN;
	volatile uint32_t ESP3LEN;
	volatile uint32_t ESP4LEN;
	volatile uint32_t ESP5LEN;
	volatile uint32_t ESP6LEN;
	volatile uint32_t ESP7LEN;
	volatile uint32_t ESP8LEN;
	volatile uint32_t ESP9LEN;
	volatile uint32_t ESPALEN;
	volatile uint32_t ESPBLEN;
	volatile uint32_t ESPCLEN;
	volatile uint32_t ESPDLEN;
	volatile uint32_t ESPELEN;
	volatile uint32_t ESPFLEN;
	volatile uint32_t ESWPRG0;
	volatile uint32_t ESRPRG0;
	volatile uint32_t ESWPRG1;
	volatile uint32_t ESRPRG1;
	volatile uint32_t ESWPRG2;
	volatile uint32_t ESRPRG2;
	volatile uint32_t ESWPRG3;
	volatile uint32_t ESRPRG3;
	volatile uint32_t ESWPRG4;
	volatile uint32_t ESRPRG4;
	volatile uint32_t ESWPRG5;
	volatile uint32_t ESRPRG5;
	volatile uint32_t ESWPRG6;
	volatile uint32_t ESRPRG6;
	volatile uint32_t ESWPRG7;
	volatile uint32_t ESRPRG7;
	volatile uint32_t ESWPRG8;
	volatile uint32_t ESRPRG8;
	volatile uint32_t ESWPRG9;
	volatile uint32_t ESRPRG9;
	volatile uint32_t ESWPRGA;
	volatile uint32_t ESRPRGA;
	volatile uint32_t ESWPRGB;
	volatile uint32_t ESRPRGB;
	volatile uint32_t ESWPRGC;
	volatile uint32_t ESRPRGC;
	volatile uint32_t ESWPRGD;
	volatile uint32_t ESRPRGD;
	volatile uint32_t ESWPRGE;
	volatile uint32_t ESRPRGE;
	volatile uint32_t ESWPRGF;
	volatile uint32_t ESRPRGF;
	volatile uint32_t ESPREN;
	volatile uint32_t ESPSTS;
	volatile uint32_t ESFLSZ;
	volatile uint32_t ESPINTEN;
	volatile uint32_t IOSHORTSTS;
	volatile uint32_t IOSHORTRDADDR;
	volatile uint32_t IOSHORTRDDATA;
	volatile uint32_t LDNCFG;
	volatile uint32_t ID0;
	volatile uint32_t ID1;
	volatile uint32_t VER;
} ESPI_Type;

/* EPSTS */
#define ESPI_EPSTS_MWDONE_Pos       (0UL)
#define ESPI_EPSTS_MWDONE_Msk       BIT(ESPI_EPSTS_MWDONE_Pos)
#define ESPI_EPSTS_MWADMS_Pos       (1UL)
#define ESPI_EPSTS_MWADMS_Msk       BIT(ESPI_EPSTS_MWADMS_Pos)
#define ESPI_EPSTS_MRDONE_Pos       (2UL)
#define ESPI_EPSTS_MRDONE_Msk       BIT(ESPI_EPSTS_MRDONE_Pos)
#define ESPI_EPSTS_LTXDONE_Pos      (3UL)
#define ESPI_EPSTS_LTXDONE_Msk      BIT(ESPI_EPSTS_LTXDONE_Pos)
#define ESPI_EPSTS_CLRSTS_Pos       (4UL)
#define ESPI_EPSTS_CLRSTS_Msk       BIT(ESPI_EPSTS_CLRSTS_Pos)
/* EPCFG */
#define ESPI_EPCFG_CHEN_Pos         (0UL)
#define ESPI_EPCFG_CHEN_Msk         BIT(ESPI_EPCFG_CHEN_Pos)
#define ESPI_EPCFG_CHRDY_Pos        (1UL)
#define ESPI_EPCFG_CHRDY_Msk        BIT(ESPI_EPCFG_CHRDY_Pos)
#define ESPI_EPCFG_BMTREN_Pos       (2UL)
#define ESPI_EPCFG_BMTREN_Msk       BIT(ESPI_EPCFG_BMTREN_Pos)
#define ESPI_EPCFG_MXPLSUP_Pos      (4UL)
#define ESPI_EPCFG_MXPLSUP_Msk      GENMASK(6, 4)
#define ESPI_EPCFG_MXPLSEL_Pos      (8UL)
#define ESPI_EPCFG_MXPLSEL_Msk      GENMASK(10, 8)
#define ESPI_EPCFG_MXRDSZ_Pos       (12UL)
#define ESPI_EPCFG_MXRDSZ_Msk       GENMASK(14, 12)
/* EPINTEN */
#define ESPI_EPINTEN_CFGCHGEN_Pos   (0UL)
#define ESPI_EPINTEN_CFGCHGEN_Msk   BIT(ESPI_EPINTEN_CFGCHGEN_Pos)
#define ESPI_EPINTEN_MEMWREN_Pos    (1UL)
#define ESPI_EPINTEN_MEMWREN_Msk    BIT(ESPI_EPINTEN_MEMWREN_Pos)
#define ESPI_EPINTEN_MEMRDEN_Pos    (2UL)
#define ESPI_EPINTEN_MEMRDEN_Msk    BIT(ESPI_EPINTEN_MEMRDEN_Pos)
/* EPRXINFO */
#define ESPI_EPRXINFO_LENGTH_Pos    (0UL)
#define ESPI_EPRXINFO_LENGTH_Msk    GENMASK(11, 0)
#define ESPI_EPRXINFO_TAG_Pos       (12UL)
#define ESPI_EPRXINFO_TAG_Msk       GENMASK(15, 12)
#define ESPI_EPRXINFO_CYCLE_Pos     (16UL)
#define ESPI_EPRXINFO_CYCLE_Msk     GENMASK(23, 16)
#define ESPI_EPRXINFO_OPCODE_Pos    (24UL)
#define ESPI_EPRXINFO_OPCODE_Msk    GENMASK(31, 24)
/* EPCPRVLD */
#define ESPI_EPCPRVLD_VALIDEN_Pos   (0UL)
#define ESPI_EPCPRVLD_VALIDEN_Msk   BIT(ESPI_EPCPRVLD_VALIDEN_Pos)
/* EPBUFSZ */
#define ESPI_EPBUFSZ_SIZE_Pos       (0UL)
#define ESPI_EPBUFSZ_SIZE_Msk       GENMASK(3, 0)
/* EPPTRCLR */
#define ESPI_EPPTRCLR_WRCLR_Pos     (0UL)
#define ESPI_EPPTRCLR_WRCLR_Msk     BIT(ESPI_EPPTRCLR_WRCLR_Pos)
#define ESPI_EPPTRCLR_RDCLR_Pos     (1UL)
#define ESPI_EPPTRCLR_RDCLR_Msk     BIT(ESPI_EPPTRCLR_RDCLR_Pos)
/* ELMSG */
#define ESPI_ELMSG_MSGDAT_Pos       (0UL)
#define ESPI_ELMSG_MSGDAT_Msk       GENMASK(15, 0)
#define ESPI_ELMSG_MSGCODE_Pos      (16UL)
#define ESPI_ELMSG_MSGCODE_Msk      GENMASK(31, 16)
/* EPMRLEN */
#define ESPI_EPMRLEN_RXLEN_Pos      (0UL)
#define ESPI_EPMRLEN_RXLEN_Msk      GENMASK(11, 0)
/* EVSTS */
#define ESPI_EVSTS_RXEPT_Pos        (0UL)
#define ESPI_EVSTS_RXEPT_Msk        BIT(ESPI_EVSTS_RXEPT_Pos)
#define ESPI_EVSTS_RXOVR_Pos        (1UL)
#define ESPI_EVSTS_RXOVR_Msk        BIT(ESPI_EVSTS_RXOVR_Pos)
#define ESPI_EVSTS_TXFULL_Pos       (2UL)
#define ESPI_EVSTS_TXFULL_Msk       BIT(ESPI_EVSTS_TXFULL_Pos)
#define ESPI_EVSTS_ILLCHG_Pos       (3UL)
#define ESPI_EVSTS_ILLCHG_Msk       BIT(ESPI_EVSTS_ILLCHG_Pos)
#define ESPI_EVSTS_IDX2CHG_Pos      (4UL)
#define ESPI_EVSTS_IDX2CHG_Msk      BIT(ESPI_EVSTS_IDX2CHG_Pos)
#define ESPI_EVSTS_IDX3CHG_Pos      (5UL)
#define ESPI_EVSTS_IDX3CHG_Msk      BIT(ESPI_EVSTS_IDX3CHG_Pos)
#define ESPI_EVSTS_IDX7CHG_Pos      (6UL)
#define ESPI_EVSTS_IDX7CHG_Msk      BIT(ESPI_EVSTS_IDX7CHG_Pos)
#define ESPI_EVSTS_IDX41CHG_Pos     (7UL)
#define ESPI_EVSTS_IDX41CHG_Msk     BIT(ESPI_EVSTS_IDX41CHG_Pos)
#define ESPI_EVSTS_IDX42CHG_Pos     (8UL)
#define ESPI_EVSTS_IDX42CHG_Msk     BIT(ESPI_EVSTS_IDX42CHG_Pos)
#define ESPI_EVSTS_IDX43CHG_Pos     (9UL)
#define ESPI_EVSTS_IDX43CHG_Msk     BIT(ESPI_EVSTS_IDX43CHG_Pos)
#define ESPI_EVSTS_IDX44CHG_Pos     (10UL)
#define ESPI_EVSTS_IDX44CHG_Msk     BIT(ESPI_EVSTS_IDX44CHG_Pos)
#define ESPI_EVSTS_IDX47CHG_Pos     (11UL)
#define ESPI_EVSTS_IDX47CHG_Msk     BIT(ESPI_EVSTS_IDX47CHG_Pos)
#define ESPI_EVSTS_IDX4ACHG_Pos     (12UL)
#define ESPI_EVSTS_IDX4ACHG_Msk     BIT(ESPI_EVSTS_IDX4ACHG_Pos)
#define ESPI_EVSTS_IDX51CHG_Pos     (13UL)
#define ESPI_EVSTS_IDX51CHG_Msk     BIT(ESPI_EVSTS_IDX51CHG_Pos)
#define ESPI_EVSTS_IDX61CHG_Pos     (14UL)
#define ESPI_EVSTS_IDX61CHG_Msk     BIT(ESPI_EVSTS_IDX61CHG_Pos)
#define ESPI_EVSTS_RXIDXCHG_Pos     (15UL)
#define ESPI_EVSTS_RXIDXCHG_Msk     BIT(ESPI_EVSTS_RXIDXCHG_Pos)
#define ESPI_EVSTS_TXDONE_Pos       (16UL)
#define ESPI_EVSTS_TXDONE_Msk       BIT(ESPI_EVSTS_TXDONE_Pos)
#define ESPI_EVSTS_RXIDXCLR_Pos     (17UL)
#define ESPI_EVSTS_RXIDXCLR_Msk     BIT(ESPI_EVSTS_RXIDXCLR_Pos)
/* EVCFG */
#define ESPI_EVCFG_CHEN_Pos         (0UL)
#define ESPI_EVCFG_CHEN_Msk         BIT(ESPI_EVCFG_CHEN_Pos)
#define ESPI_EVCFG_CHRDY_Pos        (1UL)
#define ESPI_EVCFG_CHRDY_Msk        BIT(ESPI_EVCFG_CHRDY_Pos)
#define ESPI_EVCFG_MAXSUP_Pos       (8UL)
#define ESPI_EVCFG_MAXSUP_Msk       GENMASK(13, 8)
#define ESPI_EVCFG_MAXCNT_Pos       (16UL)
#define ESPI_EVCFG_MAXCNT_Msk       GENMASK(21, 16)
/* EVIDX2 */
#define ESPI_EVIDX2_RXDAT_Pos       (0UL)
#define ESPI_EVIDX2_RXDAT_Msk       GENMASK(7, 0)
/* EVIDX3 */
#define ESPI_EVIDX3_RXDAT_Pos       (0UL)
#define ESPI_EVIDX3_RXDAT_Msk       GENMASK(7, 0)
/* EVIDX7 */
#define ESPI_EVIDX7_RXDAT_Pos       (0UL)
#define ESPI_EVIDX7_RXDAT_Msk       GENMASK(7, 0)
/* EVIDX41 */
#define ESPI_EVIDX41_RXDAT_Pos      (0UL)
#define ESPI_EVIDX41_RXDAT_Msk      GENMASK(7, 0)
/* EVIDX42 */
#define ESPI_EVIDX42_RXDAT_Pos      (0UL)
#define ESPI_EVIDX42_RXDAT_Msk      GENMASK(7, 0)
/* EVIDX43 */
#define ESPI_EVIDX43_RXDAT_Pos      (0UL)
#define ESPI_EVIDX43_RXDAT_Msk      GENMASK(7, 0)
/* EVIDX44 */
#define ESPI_EVIDX44_RXDAT_Pos      (0UL)
#define ESPI_EVIDX44_RXDAT_Msk      GENMASK(7, 0)
/* EVIDX47 */
#define ESPI_EVIDX47_RXDAT_Pos      (0UL)
#define ESPI_EVIDX47_RXDAT_Msk      GENMASK(7, 0)
/* EVIDX4A */
#define ESPI_EVIDX4A_RXDAT_Pos      (0UL)
#define ESPI_EVIDX4A_RXDAT_Msk      GENMASK(7, 0)
/* EVTXDAT */
#define ESPI_EVTXDAT_TXDAT_Pos      (0UL)
#define ESPI_EVTXDAT_TXDAT_Msk      GENMASK(7, 0)
#define ESPI_EVTXDAT_TXIDX_Pos      (8UL)
#define ESPI_EVTXDAT_TXIDX_Msk      GENMASK(15, 8)
/* EVPVIDX */
#define ESPI_EVPVIDX_PVIDX_Pos      (0UL)
#define ESPI_EVPVIDX_PVIDX_Msk      GENMASK(7, 0)
/* EVRXINTEN */
#define ESPI_EVRXINTEN_CFGCHGEN_Pos (0UL)
#define ESPI_EVRXINTEN_CFGCHGEN_Msk BIT(ESPI_EVRXINTEN_CFGCHGEN_Pos)
#define ESPI_EVRXINTEN_RXCHGEN_Pos  (2UL)
#define ESPI_EVRXINTEN_RXCHGEN_Msk  BIT(ESPI_EVRXINTEN_RXCHGEN_Pos)
/* EVTXINTEN */
#define ESPI_EVTXINTEN_TXMPTEN_Pos  (0UL)
#define ESPI_EVTXINTEN_TXMPTEN_Msk  BIT(ESPI_EVTXINTEN_TXMPTEN_Pos)
/* EOSTS */
#define ESPI_EOSTS_RXPND_Pos        (0UL)
#define ESPI_EOSTS_RXPND_Msk        BIT(ESPI_EOSTS_RXPND_Pos)
#define ESPI_EOSTS_RXDONE_Pos       (1UL)
#define ESPI_EOSTS_RXDONE_Msk       BIT(ESPI_EOSTS_RXDONE_Pos)
#define ESPI_EOSTS_TXPND_Pos        (2UL)
#define ESPI_EOSTS_TXPND_Msk        BIT(ESPI_EOSTS_TXPND_Pos)
#define ESPI_EOSTS_TXDONE_Pos       (3UL)
#define ESPI_EOSTS_TXDONE_Msk       BIT(ESPI_EOSTS_TXDONE_Pos)
#define ESPI_EOSTS_CFGENCHG_Pos     (4UL)
#define ESPI_EOSTS_CFGENCHG_Msk     BIT(ESPI_EOSTS_CFGENCHG_Pos)
/* EOCFG */
#define ESPI_EOCFG_CHEN_Pos         (0UL)
#define ESPI_EOCFG_CHEN_Msk         BIT(ESPI_EOCFG_CHEN_Pos)
#define ESPI_EOCFG_CHRDY_Pos        (1UL)
#define ESPI_EOCFG_CHRDY_Msk        BIT(ESPI_EOCFG_CHRDY_Pos)
#define ESPI_EOCFG_MXSZSUP_Pos      (4UL)
#define ESPI_EOCFG_MXSZSUP_Msk      GENMASK(6, 4)
#define ESPI_EOCFG_MXSZSEL_Pos      (8UL)
#define ESPI_EOCFG_MXSZSEL_Msk      GENMASK(10, 8)
/* EORXINTEN */
#define ESPI_EORXINTEN_CHENCHG_Pos  (0UL)
#define ESPI_EORXINTEN_CHENCHG_Msk  BIT(ESPI_EORXINTEN_CHENCHG_Pos)
#define ESPI_EORXINTEN_RXEN_Pos     (1UL)
#define ESPI_EORXINTEN_RXEN_Msk     BIT(ESPI_EORXINTEN_RXEN_Pos)
/* EORXLEN */
#define ESPI_EORXLEN_LENGTH_Pos     (0UL)
#define ESPI_EORXLEN_LENGTH_Msk     GENMASK(11, 0)
/* EOTXLEN */
#define ESPI_EOTXLEN_LENGTH_Pos     (0UL)
#define ESPI_EOTXLEN_LENGTH_Msk     GENMASK(11, 0)
/* EOTXCTRL */
#define ESPI_EOTXCTRL_TXSTR_Pos     (0UL)
#define ESPI_EOTXCTRL_TXSTR_Msk     BIT(ESPI_EOTXCTRL_TXSTR_Pos)
/* EOTXINTEN */
#define ESPI_EOTXINTEN_TXEN_Pos     (0UL)
#define ESPI_EOTXINTEN_TXEN_Msk     BIT(ESPI_EOTXINTEN_TXEN_Pos)
/* EFSTS */
#define ESPI_EFSTS_MAFTXDN_Pos      (0UL)
#define ESPI_EFSTS_MAFTXDN_Msk      BIT(ESPI_EFSTS_MAFTXDN_Pos)
#define ESPI_EFSTS_MAFREOVR_Pos     (1UL)
#define ESPI_EFSTS_MAFREOVR_Msk     BIT(ESPI_EFSTS_MAFREOVR_Pos)
#define ESPI_EFSTS_MAFREUDR_Pos     (2UL)
#define ESPI_EFSTS_MAFREUDR_Msk     BIT(ESPI_EFSTS_MAFREUDR_Pos)
#define ESPI_EFSTS_SAFDONE_Pos      (3UL)
#define ESPI_EFSTS_SAFDONE_Msk      BIT(ESPI_EFSTS_SAFDONE_Pos)
#define ESPI_EFSTS_SAFRW_Pos        (4UL)
#define ESPI_EFSTS_SAFRW_Msk        BIT(ESPI_EFSTS_SAFRW_Pos)
#define ESPI_EFSTS_SAFERS_Pos       (5UL)
#define ESPI_EFSTS_SAFERS_Msk       BIT(ESPI_EFSTS_SAFERS_Pos)
#define ESPI_EFSTS_CHENCHG_Pos      (6UL)
#define ESPI_EFSTS_CHENCHG_Msk      BIT(ESPI_EFSTS_CHENCHG_Pos)
#define ESPI_EFSTS_OP2_Pos          (7UL)
#define ESPI_EFSTS_OP2_Msk          BIT(ESPI_EFSTS_OP2_Pos)
/* EFCONF */
#define ESPI_EFCONF_CHEN_Pos        (0UL)
#define ESPI_EFCONF_CHEN_Msk        BIT(ESPI_EFCONF_CHEN_Pos)
#define ESPI_EFCONF_CHRDY_Pos       (1UL)
#define ESPI_EFCONF_CHRDY_Msk       BIT(ESPI_EFCONF_CHRDY_Pos)
#define ESPI_EFCONF_ERBLKSZ_Pos     (2UL)
#define ESPI_EFCONF_ERBLKSZ_Msk     GENMASK(4, 2)
#define ESPI_EFCONF_MXPLSUP_Pos     (5UL)
#define ESPI_EFCONF_MXPLSUP_Msk     GENMASK(7, 5)
#define ESPI_EFCONF_MXPLSEL_Pos     (8UL)
#define ESPI_EFCONF_MXPLSEL_Msk     GENMASK(10, 8)
#define ESPI_EFCONF_SHAREMD_Pos     (11UL)
#define ESPI_EFCONF_SHAREMD_Msk     BIT(ESPI_EFCONF_SHAREMD_Pos)
#define ESPI_EFCONF_MXRDSZ_Pos      (12UL)
#define ESPI_EFCONF_MXRDSZ_Msk      GENMASK(14, 12)
#define ESPI_EFCONF_SHARECAPSP_Pos  (16UL)
#define ESPI_EFCONF_SHARECAPSP_Msk  GENMASK(17, 16)
#define ESPI_EFCONF_RPMCCNT1_Pos    (20UL)
#define ESPI_EFCONF_RPMCCNT1_Msk    GENMASK(23, 20)
#define ESPI_EFCONF_RPMCOP1_Pos     (24UL)
#define ESPI_EFCONF_RPMCOP1_Msk     GENMASK(31, 24)
/* EMTRLEN */
#define ESPI_EMTRLEN_TRLEN_Pos      (0UL)
#define ESPI_EMTRLEN_TRLEN_Msk      GENMASK(11, 0)
/* EMCTRL */
#define ESPI_EMCTRL_START_Pos       (0UL)
#define ESPI_EMCTRL_START_Msk       BIT(ESPI_EMCTRL_START_Pos)
#define ESPI_EMCTRL_MDSEL_Pos       (1UL)
#define ESPI_EMCTRL_MDSEL_Msk       GENMASK(2, 1)
/* EMINTEN */
#define ESPI_EMINTEN_CHENCHG_Pos    (0UL)
#define ESPI_EMINTEN_CHENCHG_Msk    BIT(ESPI_EMINTEN_CHENCHG_Pos)
#define ESPI_EMINTEN_TRDONEEN_Pos   (1UL)
#define ESPI_EMINTEN_TRDONEEN_Msk   BIT(ESPI_EMINTEN_TRDONEEN_Pos)
/* ESINTEN */
#define ESPI_ESINTEN_TRDONEEN_Pos   (0UL)
#define ESPI_ESINTEN_TRDONEEN_Msk   BIT(ESPI_ESINTEN_TRDONEEN_Pos)
#define ESPI_ESINTEN_ERASEEN_Pos    (1UL)
#define ESPI_ESINTEN_ERASEEN_Msk    BIT(ESPI_ESINTEN_ERASEEN_Pos)
#define ESPI_ESINTEN_RPMCEN_Pos     (2UL)
#define ESPI_ESINTEN_RPMCEN_Msk     BIT(ESPI_ESINTEN_RPMCEN_Pos)
/* ESRXLEN */
#define ESPI_ESRXLEN_LENGTH_Pos     (0UL)
#define ESPI_ESRXLEN_LENGTH_Msk     GENMASK(11, 0)
/* ESPICFG */
#define ESPI_ESPICFG_CHSUP_Pos      (0UL)
#define ESPI_ESPICFG_CHSUP_Msk      GENMASK(7, 0)
#define ESPI_ESPICFG_MXWAITALW_Pos  (12UL)
#define ESPI_ESPICFG_MXWAITALW_Msk  GENMASK(15, 12)
#define ESPI_ESPICFG_MXFREQSUP_Pos  (16UL)
#define ESPI_ESPICFG_MXFREQSUP_Msk  GENMASK(18, 16)
#define ESPI_ESPICFG_ODALRSUP_Pos   (19UL)
#define ESPI_ESPICFG_ODALRSUP_Msk   BIT(ESPI_ESPICFG_ODALRSUP_Pos)
#define ESPI_ESPICFG_OPFREQ_Pos     (20UL)
#define ESPI_ESPICFG_OPFREQ_Msk     GENMASK(22, 20)
#define ESPI_ESPICFG_ODALRSEL_Pos   (23UL)
#define ESPI_ESPICFG_ODALRSEL_Msk   BIT(ESPI_ESPICFG_ODALRSEL_Pos)
#define ESPI_ESPICFG_IOSUP_Pos      (24UL)
#define ESPI_ESPICFG_IOSUP_Msk      GENMASK(25, 24)
#define ESPI_ESPICFG_IOSEL_Pos      (26UL)
#define ESPI_ESPICFG_IOSEL_Msk      GENMASK(27, 26)
#define ESPI_ESPICFG_ALRMODE_Pos    (28UL)
#define ESPI_ESPICFG_ALRMODE_Msk    BIT(ESPI_ESPICFG_ALRMODE_Pos)
#define ESPI_ESPICFG_RTCINBMC_Pos   (29UL)
#define ESPI_ESPICFG_RTCINBMC_Msk   BIT(ESPI_ESPICFG_RTCINBMC_Pos)
#define ESPI_ESPICFG_RSPMDFEN_Pos   (30UL)
#define ESPI_ESPICFG_RSPMDFEN_Msk   BIT(ESPI_ESPICFG_RSPMDFEN_Pos)
#define ESPI_ESPICFG_CRCCHKEN_Pos   (31UL)
#define ESPI_ESPICFG_CRCCHKEN_Msk   BIT(ESPI_ESPICFG_CRCCHKEN_Pos)
/* ERSTCFG */
#define ESPI_ERSTCFG_RSTMONEN_Pos   (0UL)
#define ESPI_ERSTCFG_RSTMONEN_Msk   BIT(ESPI_ERSTCFG_RSTMONEN_Pos)
#define ESPI_ERSTCFG_RSTPOL_Pos     (1UL)
#define ESPI_ERSTCFG_RSTPOL_Msk     BIT(ESPI_ERSTCFG_RSTPOL_Pos)
#define ESPI_ERSTCFG_RSTINTEN_Pos   (2UL)
#define ESPI_ERSTCFG_RSTINTEN_Msk   BIT(ESPI_ERSTCFG_RSTINTEN_Pos)
#define ESPI_ERSTCFG_RSTSTS_Pos     (3UL)
#define ESPI_ERSTCFG_RSTSTS_Msk     BIT(ESPI_ERSTCFG_RSTSTS_Pos)
/* EVIDX51 */
#define ESPI_EVIDX51_RXDAT_Pos      (0UL)
#define ESPI_EVIDX51_RXDAT_Msk      GENMASK(7, 0)
/* EVIDX61 */
#define ESPI_EVIDX61_RXDAT_Pos      (0UL)
#define ESPI_EVIDX61_RXDAT_Msk      GENMASK(7, 0)
/* EFCFG2 */
#define ESPI_EFCFG2_MXRDSZ_Pos      (0UL)
#define ESPI_EFCFG2_MXRDSZ_Msk      GENMASK(2, 0)
#define ESPI_EFCFG2_ERBLKSZ_Pos     (8UL)
#define ESPI_EFCFG2_ERBLKSZ_Msk     GENMASK(15, 8)
#define ESPI_EFCFG2_RPMCSP_Pos      (16UL)
#define ESPI_EFCFG2_RPMCSP_Msk      GENMASK(21, 16)
#define ESPI_EFCFG2_NUMRPMC_Pos     (22UL)
#define ESPI_EFCFG2_NUMRPMC_Msk     GENMASK(23, 22)
/* EFCFG3 */
#define ESPI_EFCFG3_RPMCCNT2_Pos    (20UL)
#define ESPI_EFCFG3_RPMCCNT2_Msk    GENMASK(23, 20)
#define ESPI_EFCFG3_RPMCOP2_Pos     (24UL)
#define ESPI_EFCFG3_RPMCOP2_Msk     GENMASK(31, 24)
/* EFCFG4 */
#define ESPI_EFCFG4_RPMCCNT3_Pos    (4UL)
#define ESPI_EFCFG4_RPMCCNT3_Msk    GENMASK(7, 4)
#define ESPI_EFCFG4_RPMCOP3_Pos     (8UL)
#define ESPI_EFCFG4_RPMCOP3_Msk     GENMASK(15, 8)
#define ESPI_EFCFG4_RPMCCNT4_Pos    (20UL)
#define ESPI_EFCFG4_RPMCCNT4_Msk    GENMASK(23, 20)
#define ESPI_EFCFG4_RPMCOP4_Pos     (24UL)
#define ESPI_EFCFG4_RPMCOP4_Msk     GENMASK(31, 24)
/* ELCTRL */
#define ESPI_ELCTRL_TXSTR_Pos       (0UL)
#define ESPI_ELCTRL_TXSTR_Msk       BIT(ESPI_ELCTRL_TXSTR_Pos)
/* ESPRG0 */
#define ESPI_ESPRG0_TAG0GRP_Pos     (0UL)
#define ESPI_ESPRG0_TAG0GRP_Msk     GENMASK(3, 0)
#define ESPI_ESPRG0_TAG1GRP_Pos     (4UL)
#define ESPI_ESPRG0_TAG1GRP_Msk     GENMASK(7, 4)
#define ESPI_ESPRG0_TAG2GRP_Pos     (8UL)
#define ESPI_ESPRG0_TAG2GRP_Msk     GENMASK(11, 8)
#define ESPI_ESPRG0_TAG3GRP_Pos     (12UL)
#define ESPI_ESPRG0_TAG3GRP_Msk     GENMASK(15, 12)
#define ESPI_ESPRG0_TAG4GRP_Pos     (16UL)
#define ESPI_ESPRG0_TAG4GRP_Msk     GENMASK(19, 16)
#define ESPI_ESPRG0_TAG5GRP_Pos     (20UL)
#define ESPI_ESPRG0_TAG5GRP_Msk     GENMASK(23, 20)
#define ESPI_ESPRG0_TAG6GRP_Pos     (24UL)
#define ESPI_ESPRG0_TAG6GRP_Msk     GENMASK(27, 24)
#define ESPI_ESPRG0_TAG7GRP_Pos     (28UL)
#define ESPI_ESPRG0_TAG7GRP_Msk     GENMASK(31, 28)
/* ESPRG1 */
#define ESPI_ESPRG1_TAG8GRP_Pos     (0UL)
#define ESPI_ESPRG1_TAG8GRP_Msk     GENMASK(2, 0)
#define ESPI_ESPRG1_TAG9GRP_Pos     (4UL)
#define ESPI_ESPRG1_TAG9GRP_Msk     GENMASK(6, 4)
#define ESPI_ESPRG1_TAGAGRP_Pos     (8UL)
#define ESPI_ESPRG1_TAGAGRP_Msk     GENMASK(10, 8)
#define ESPI_ESPRG1_TAGBGRP_Pos     (12UL)
#define ESPI_ESPRG1_TAGBGRP_Msk     GENMASK(14, 12)
#define ESPI_ESPRG1_TAGCGRP_Pos     (16UL)
#define ESPI_ESPRG1_TAGCGRP_Msk     GENMASK(18, 16)
#define ESPI_ESPRG1_TAGDGRP_Pos     (20UL)
#define ESPI_ESPRG1_TAGDGRP_Msk     GENMASK(22, 20)
#define ESPI_ESPRG1_TAGEGRP_Pos     (24UL)
#define ESPI_ESPRG1_TAGEGRP_Msk     GENMASK(26, 24)
#define ESPI_ESPRG1_TAGFGRP_Pos     (28UL)
#define ESPI_ESPRG1_TAGFGRP_Msk     GENMASK(30, 28)
/* ESPDRT */
#define ESPI_ESPDRT_TAGDRT_Pos      (0UL)
#define ESPI_ESPDRT_TAGDRT_Msk      BIT(ESPI_ESPDRT_TAGDRT_Pos)
/* ESP0STR */
#define ESPI_ESP0STR_ADDR_Pos       (0UL)
#define ESPI_ESP0STR_ADDR_Msk       GENMASK(19, 0)
/* ESP1STR */
#define ESPI_ESP1STR_ADDR_Pos       (0UL)
#define ESPI_ESP1STR_ADDR_Msk       GENMASK(19, 0)
/* ESP2STR */
#define ESPI_ESP2STR_ADDR_Pos       (0UL)
#define ESPI_ESP2STR_ADDR_Msk       GENMASK(19, 0)
/* ESP3STR */
#define ESPI_ESP3STR_ADDR_Pos       (0UL)
#define ESPI_ESP3STR_ADDR_Msk       GENMASK(19, 0)
/* ESP4STR */
#define ESPI_ESP4STR_ADDR_Pos       (0UL)
#define ESPI_ESP4STR_ADDR_Msk       GENMASK(19, 0)
/* ESP5STR */
#define ESPI_ESP5STR_ADDR_Pos       (0UL)
#define ESPI_ESP5STR_ADDR_Msk       GENMASK(19, 0)
/* ESP6STR */
#define ESPI_ESP6STR_ADDR_Pos       (0UL)
#define ESPI_ESP6STR_ADDR_Msk       GENMASK(19, 0)
/* ESP7STR */
#define ESPI_ESP7STR_ADDR_Pos       (0UL)
#define ESPI_ESP7STR_ADDR_Msk       GENMASK(19, 0)
/* ESP8STR */
#define ESPI_ESP8STR_ADDR_Pos       (0UL)
#define ESPI_ESP8STR_ADDR_Msk       GENMASK(19, 0)
/* ESP9STR */
#define ESPI_ESP9STR_ADDR_Pos       (0UL)
#define ESPI_ESP9STR_ADDR_Msk       GENMASK(19, 0)
/* ESPASTR */
#define ESPI_ESPASTR_ADDR_Pos       (0UL)
#define ESPI_ESPASTR_ADDR_Msk       GENMASK(19, 0)
/* ESPBSTR */
#define ESPI_ESPBSTR_ADDR_Pos       (0UL)
#define ESPI_ESPBSTR_ADDR_Msk       GENMASK(19, 0)
/* ESPCSTR */
#define ESPI_ESPCSTR_ADDR_Pos       (0UL)
#define ESPI_ESPCSTR_ADDR_Msk       GENMASK(19, 0)
/* ESPDSTR */
#define ESPI_ESPDSTR_ADDR_Pos       (0UL)
#define ESPI_ESPDSTR_ADDR_Msk       GENMASK(19, 0)
/* ESPESTR */
#define ESPI_ESPESTR_ADDR_Pos       (0UL)
#define ESPI_ESPESTR_ADDR_Msk       GENMASK(19, 0)
/* ESPFSTR */
#define ESPI_ESPFSTR_ADDR_Pos       (0UL)
#define ESPI_ESPFSTR_ADDR_Msk       GENMASK(19, 0)
/* ESPFSTR */
#define ESPI_ESPFSTR_ADDR_Pos       (0UL)
#define ESPI_ESPFSTR_ADDR_Msk       GENMASK(19, 0)
/* ESP0LEN */
#define ESPI_ESP0LEN_LEN_Pos        (0UL)
#define ESPI_ESP0LEN_LEN_Msk        GENMASK(19, 0)
/* ESP1LEN */
#define ESPI_ESP1LEN_LEN_Pos        (0UL)
#define ESPI_ESP1LEN_LEN_Msk        GENMASK(19, 0)
/* ESP2LEN */
#define ESPI_ESP2LEN_LEN_Pos        (0UL)
#define ESPI_ESP2LEN_LEN_Msk        GENMASK(19, 0)
/* ESP3LEN */
#define ESPI_ESP3LEN_LEN_Pos        (0UL)
#define ESPI_ESP3LEN_LEN_Msk        GENMASK(19, 0)
/* ESP4LEN */
#define ESPI_ESP4LEN_LEN_Pos        (0UL)
#define ESPI_ESP4LEN_LEN_Msk        GENMASK(19, 0)
/* ESP5LEN */
#define ESPI_ESP5LEN_LEN_Pos        (0UL)
#define ESPI_ESP5LEN_LEN_Msk        GENMASK(19, 0)
/* ESP6LEN */
#define ESPI_ESP6LEN_LEN_Pos        (0UL)
#define ESPI_ESP6LEN_LEN_Msk        GENMASK(19, 0)
/* ESP7LEN */
#define ESPI_ESP7LEN_LEN_Pos        (0UL)
#define ESPI_ESP7LEN_LEN_Msk        GENMASK(19, 0)
/* ESP8LEN */
#define ESPI_ESP8LEN_LEN_Pos        (0UL)
#define ESPI_ESP8LEN_LEN_Msk        GENMASK(19, 0)
/* ESP9LEN */
#define ESPI_ESP9LEN_LEN_Pos        (0UL)
#define ESPI_ESP9LEN_LEN_Msk        GENMASK(19, 0)
/* ESPALEN */
#define ESPI_ESPALEN_LEN_Pos        (0UL)
#define ESPI_ESPALEN_LEN_Msk        GENMASK(19, 0)
/* ESPBLEN */
#define ESPI_ESPBLEN_LEN_Pos        (0UL)
#define ESPI_ESPBLEN_LEN_Msk        GENMASK(19, 0)
/* ESPCLEN */
#define ESPI_ESPCLEN_LEN_Pos        (0UL)
#define ESPI_ESPCLEN_LEN_Msk        GENMASK(19, 0)
/* ESPDLEN */
#define ESPI_ESPDLEN_LEN_Pos        (0UL)
#define ESPI_ESPDLEN_LEN_Msk        GENMASK(19, 0)
/* ESPELEN */
#define ESPI_ESPELEN_LEN_Pos        (0UL)
#define ESPI_ESPELEN_LEN_Msk        GENMASK(19, 0)
/* ESPFLEN */
#define ESPI_ESPFLEN_LEN_Pos        (0UL)
#define ESPI_ESPFLEN_LEN_Msk        GENMASK(19, 0)
/* ESWPRG0 */
#define ESPI_ESWPRG0_GWREN_Pos      (0UL)
#define ESPI_ESWPRG0_GWREN_Msk      BIT(ESPI_ESWPRG0_GWREN_Pos)
/* ESWPRG1 */
#define ESPI_ESWPRG1_GWREN_Pos      (0UL)
#define ESPI_ESWPRG1_GWREN_Msk      BIT(ESPI_ESWPRG1_GWREN_Pos)
/* ESWPRG2 */
#define ESPI_ESWPRG2_GWREN_Pos      (0UL)
#define ESPI_ESWPRG2_GWREN_Msk      BIT(ESPI_ESWPRG2_GWREN_Pos)
/* ESWPRG3 */
#define ESPI_ESWPRG3_GWREN_Pos      (0UL)
#define ESPI_ESWPRG3_GWREN_Msk      BIT(ESPI_ESWPRG3_GWREN_Pos)
/* ESWPRG4 */
#define ESPI_ESWPRG4_GWREN_Pos      (0UL)
#define ESPI_ESWPRG4_GWREN_Msk      BIT(ESPI_ESWPRG4_GWREN_Pos)
/* ESWPRG5 */
#define ESPI_ESWPRG5_GWREN_Pos      (0UL)
#define ESPI_ESWPRG5_GWREN_Msk      BIT(ESPI_ESWPRG5_GWREN_Pos)
/* ESWPRG6 */
#define ESPI_ESWPRG6_GWREN_Pos      (0UL)
#define ESPI_ESWPRG6_GWREN_Msk      BIT(ESPI_ESWPRG6_GWREN_Pos)
/* ESWPRG7 */
#define ESPI_ESWPRG7_GWREN_Pos      (0UL)
#define ESPI_ESWPRG7_GWREN_Msk      BIT(ESPI_ESWPRG7_GWREN_Pos)
/* ESWPRG8 */
#define ESPI_ESWPRG8_GWREN_Pos      (0UL)
#define ESPI_ESWPRG8_GWREN_Msk      BIT(ESPI_ESWPRG8_GWREN_Pos)
/* ESWPRG9 */
#define ESPI_ESWPRG9_GWREN_Pos      (0UL)
#define ESPI_ESWPRG9_GWREN_Msk      BIT(ESPI_ESWPRG9_GWREN_Pos)
/* ESWPRGA */
#define ESPI_ESWPRGA_GWREN_Pos      (0UL)
#define ESPI_ESWPRGA_GWREN_Msk      BIT(ESPI_ESWPRGA_GWREN_Pos)
/* ESWPRGB */
#define ESPI_ESWPRGB_GWREN_Pos      (0UL)
#define ESPI_ESWPRGB_GWREN_Msk      BIT(ESPI_ESWPRGB_GWREN_Pos)
/* ESWPRGC */
#define ESPI_ESWPRGC_GWREN_Pos      (0UL)
#define ESPI_ESWPRGC_GWREN_Msk      BIT(ESPI_ESWPRGC_GWREN_Pos)
/* ESWPRGD */
#define ESPI_ESWPRGD_GWREN_Pos      (0UL)
#define ESPI_ESWPRGD_GWREN_Msk      BIT(ESPI_ESWPRGD_GWREN_Pos)
/* ESWPRGE */
#define ESPI_ESWPRGE_GWREN_Pos      (0UL)
#define ESPI_ESWPRGE_GWREN_Msk      BIT(ESPI_ESWPRGE_GWREN_Pos)
/* ESWPRGF */
#define ESPI_ESWPRGF_GWREN_Pos      (0UL)
#define ESPI_ESWPRGF_GWREN_Msk      BIT(ESPI_ESWPRGF_GWREN_Pos)
/* ESRPRG0 */
#define ESPI_ESRPRG0_GRDEN_Pos      (0UL)
#define ESPI_ESRPRG0_GRDEN_Msk      BIT(ESPI_ESRPRG0_GRDEN_Pos)
/* ESRPRG1 */
#define ESPI_ESRPRG1_GRDEN_Pos      (0UL)
#define ESPI_ESRPRG1_GRDEN_Msk      BIT(ESPI_ESRPRG1_GRDEN_Pos)
/* ESRPRG2 */
#define ESPI_ESRPRG2_GRDEN_Pos      (0UL)
#define ESPI_ESRPRG2_GRDEN_Msk      BIT(ESPI_ESRPRG2_GRDEN_Pos)
/* ESRPRG3 */
#define ESPI_ESRPRG3_GRDEN_Pos      (0UL)
#define ESPI_ESRPRG3_GRDEN_Msk      BIT(ESPI_ESRPRG3_GRDEN_Pos)
/* ESRPRG4 */
#define ESPI_ESRPRG4_GRDEN_Pos      (0UL)
#define ESPI_ESRPRG4_GRDEN_Msk      BIT(ESPI_ESRPRG4_GRDEN_Pos)
/* ESRPRG5 */
#define ESPI_ESRPRG5_GRDEN_Pos      (0UL)
#define ESPI_ESRPRG5_GRDEN_Msk      BIT(ESPI_ESRPRG5_GRDEN_Pos)
/* ESRPRG6 */
#define ESPI_ESRPRG6_GRDEN_Pos      (0UL)
#define ESPI_ESRPRG6_GRDEN_Msk      BIT(ESPI_ESRPRG6_GRDEN_Pos)
/* ESRPRG7 */
#define ESPI_ESRPRG7_GRDEN_Pos      (0UL)
#define ESPI_ESRPRG7_GRDEN_Msk      BIT(ESPI_ESRPRG7_GRDEN_Pos)
/* ESRPRG8 */
#define ESPI_ESRPRG8_GRDEN_Pos      (0UL)
#define ESPI_ESRPRG8_GRDEN_Msk      BIT(ESPI_ESRPRG8_GRDEN_Pos)
/* ESRPRG9 */
#define ESPI_ESRPRG9_GRDEN_Pos      (0UL)
#define ESPI_ESRPRG9_GRDEN_Msk      BIT(ESPI_ESRPRG9_GRDEN_Pos)
/* ESRPRGA */
#define ESPI_ESRPRGA_GRDEN_Pos      (0UL)
#define ESPI_ESRPRGA_GRDEN_Msk      BIT(ESPI_ESRPRGA_GRDEN_Pos)
/* ESRPRGB */
#define ESPI_ESRPRGB_GRDEN_Pos      (0UL)
#define ESPI_ESRPRGB_GRDEN_Msk      BIT(ESPI_ESRPRGB_GRDEN_Pos)
/* ESRPRGC */
#define ESPI_ESRPRGC_GRDEN_Pos      (0UL)
#define ESPI_ESRPRGC_GRDEN_Msk      BIT(ESPI_ESRPRGC_GRDEN_Pos)
/* ESRPRGD */
#define ESPI_ESRPRGD_GRDEN_Pos      (0UL)
#define ESPI_ESRPRGD_GRDEN_Msk      BIT(ESPI_ESRPRGD_GRDEN_Pos)
/* ESRPRGE */
#define ESPI_ESRPRGE_GRDEN_Pos      (0UL)
#define ESPI_ESRPRGE_GRDEN_Msk      BIT(ESPI_ESRPRGE_GRDEN_Pos)
/* ESRPRGF */
#define ESPI_ESRPRGF_GRDEN_Pos      (0UL)
#define ESPI_ESRPRGF_GRDEN_Msk      BIT(ESPI_ESRPRGF_GRDEN_Pos)
/* ESPREN */
#define ESPI_ESPREN_EN_Pos          (0UL)
#define ESPI_ESPREN_EN_Msk          BIT(ESPI_ESPREN_EN_Pos)
/* ESPSTS */
#define ESPI_ESPSTS_OVSIZE_Pos      (0UL)
#define ESPI_ESPSTS_OVSIZE_Msk      BIT(ESPI_ESPSTS_OVSIZE_Pos)
#define ESPI_ESPSTS_HIT_Pos         (1UL)
#define ESPI_ESPSTS_HIT_Msk         BIT(ESPI_ESPSTS_HIT_Pos)
#define ESPI_ESPSTS_CRS4K_Pos       (2UL)
#define ESPI_ESPSTS_CRS4K_Msk       BIT(ESPI_ESPSTS_CRS4K_Pos)
/* ESPINTEN */
#define ESPI_ESPINTEN_OVSIZEEN_Pos  (0UL)
#define ESPI_ESPINTEN_OVSIZEEN_Msk  BIT(ESPI_ESPINTEN_OVSIZEEN_Pos)
#define ESPI_ESPINTEN_HITEN_Pos     (1UL)
#define ESPI_ESPINTEN_HITEN_Msk     BIT(ESPI_ESPINTEN_HITEN_Pos)
#define ESPI_ESPINTEN_CRS4KEN_Pos   (2UL)
#define ESPI_ESPINTEN_CRS4KEN_Msk   BIT(ESPI_ESPINTEN_CRS4KEN_Pos)
/* IOSHORTSTS */
#define ESPI_IOSHORTSTS_BYTES_Pos   (0UL)
#define ESPI_IOSHORTSTS_BYTES_Msk   GENMASK(1, 0)
#define ESPI_IOSHORTSTS_TYPE_Pos    (2UL)
#define ESPI_IOSHORTSTS_TYPE_Msk    BIT(ESPI_IOSHORTSTS_TYPE_Pos)
#define ESPI_IOSHORTSTS_ACCEPT_Pos  (3UL)
#define ESPI_IOSHORTSTS_ACCEPT_Msk  BIT(ESPI_IOSHORTSTS_ACCEPT_Pos)
/* IOSHORTRDADDR */
#define ESPI_IOSHORTRDADDR_ADDR_Pos (0UL)
#define ESPI_IOSHORTRDADDR_ADDR_Msk GENMASK(15, 0)
/* LDNCFG */
#define ESPI_LDNCFG_IDX_Pos         (0UL)
#define ESPI_LDNCFG_IDX_Msk         GENMASK(15, 0)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_ESPI_H */
