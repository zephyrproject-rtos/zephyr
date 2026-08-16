/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODEM_UBX_KEYS_
#define ZEPHYR_MODEM_UBX_KEYS_

/** Message output configuration (UART1) */
enum ubx_key_msg_out_uart1 {
	/** Output rate of the NMEA-GX-DTM message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_DTM_UART1 = 0x209100a7,
	/** Output rate of the NMEA-GX-GBS message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_GBS_UART1 = 0x209100de,
	/** Output rate of the NMEA-GX-GGA message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_GGA_UART1 = 0x209100bb,
	/** Output rate of the NMEA-GX-GLL message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_GLL_UART1 = 0x209100ca,
	/** Output rate of the NMEA-GX-GNS message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_GNS_UART1 = 0x209100b6,
	/** Output rate of the NMEA-GX-GRS message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_GRS_UART1 = 0x209100cf,
	/** Output rate of the NMEA-GX-GSA message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_GSA_UART1 = 0x209100c0,
	/** Output rate of the NMEA-GX-GST message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_GST_UART1 = 0x209100d4,
	/** Output rate of the NMEA-GX-GSV message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_GSV_UART1 = 0x209100c5,
	/** Output rate of the NMEA-GX-RLM message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_RLM_UART1 = 0x20910401,
	/** Output rate of the NMEA-GX-RMC message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_RMC_UART1 = 0x209100ac,
	/** Output rate of the NMEA-GX-THS message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_THS_UART1 = 0x209100e3,
	/** The output rate of the NMEA-GX-UTC message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_UTC_UART1 = 0x209106d0,
	/** Output rate of the NMEA-GX-VLW message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_VLW_UART1 = 0x209100e8,
	/** Output rate of the NMEA-GX-VTG message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_VTG_UART1 = 0x209100b1,
	/** Output rate of the NMEA-GX-ZDA message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_ZDA_UART1 = 0x209100d9,
	/** Output rate of the NMEA-NAV2-GX-GGA message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GGA_UART1 = 0x20910662,
	/** Output rate of the NMEA-NAV2-GX-GLL message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GLL_UART1 = 0x20910671,
	/** Output rate of the NMEA-NAV2-GX-GNS message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GNS_UART1 = 0x2091065d,
	/** Output rate of the NMEA-NAV2-GX-GSA message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GSA_UART1 = 0x20910667,
	/** Output rate of the NMEA-NAV2-GX-RMC message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_RMC_UART1 = 0x20910653,
	/** Output rate of the NMEA-NAV2-GX-VTG message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_VTG_UART1 = 0x20910658,
	/** Output rate of the NMEA-NAV2-GX-ZDA message on the UART1 port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_ZDA_UART1 = 0x20910680,
	/** Output rate of the NMEA-GX-PUBX00 message on the UART1 port */
	UBX_KEY_MSG_OUT_PUBX_POLYP_UART1 = 0x209100ed,
	/** Output rate of the NMEA-GX-PUBX03 message on the UART1 port */
	UBX_KEY_MSG_OUT_PUBX_POLYS_UART1 = 0x209100f2,
	/** Output rate of the NMEA-GX-PUBX04 message on the UART1 port */
	UBX_KEY_MSG_OUT_PUBX_POLYT_UART1 = 0x209100f7,
	/** Output rate of the NMEA-GX-PUBX50 message on the UART1 port */
	UBX_KEY_MSG_OUT_PUBX_POLYV_UART1 = 0x209103ef,
	/** Output rate of the RTCM-3X-TYPE1005 message on the UART1 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1005_UART1 = 0x209102be,
	/** Output rate of the RTCM-3X-TYPE1006 message on the UART1 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1006_UART1 = 0x209102c3,
	/** Output rate of the RTCM-3X-TYPE1074 message on the UART1 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1074_UART1 = 0x2091035f,
	/** Output rate of the RTCM-3X-TYPE1077 message on the UART1 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1077_UART1 = 0x209102cd,
	/** Output rate of the RTCM-3X-TYPE1084 message on the UART1 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1084_UART1 = 0x20910364,
	/** Output rate of the RTCM-3X-TYPE1087 message on the UART1 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1087_UART1 = 0x209102d2,
	/** Output rate of the RTCM-3X-TYPE1094 message on the UART1 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1094_UART1 = 0x20910369,
	/** Output rate of the RTCM-3X-TYPE1097 message on the UART1 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1097_UART1 = 0x20910319,
	/** Output rate of the RTCM-3X-TYPE1124 message on the UART1 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1124_UART1 = 0x2091036e,
	/** Output rate of the RTCM-3X-TYPE1127 message on the UART1 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1127_UART1 = 0x209102d7,
	/** Output rate of the RTCM-3X-TYPE1230 message on the UART1 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1230_UART1 = 0x20910304,
	/** Output rate of the RTCM-3X-TYPE4072_0 message on the UART1 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE4072_0_UART1 = 0x209102ff,
	/** Output rate of the RTCM-3X-TYPE4072_1 message on the UART1 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE4072_1_UART1 = 0x20910382,
	/** Output rate of the UBX-AID-ALM message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_AID_ALM_UART1 = 0x2091016f,
	/** Output rate of the UBX-AID-AOP message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_AID_AOP_UART1 = 0x2091026e,
	/** Output rate of the UBX-AID-EPH message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_AID_EPH_UART1 = 0x20910165,
	/** Output rate of the UBX-AID-INI message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_AID_INI_UART1 = 0x209100fc,
	/** Output rate of the UBX-ESF-ALG message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_ESF_ALG_UART1 = 0x20910110,
	/** Output rate of the UBX-ESF-CAL message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_ESF_CAL_UART1 = 0x209106ad,
	/** Output rate of the UBX-ESF-INS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_ESF_INS_UART1 = 0x20910115,
	/** Output rate of the UBX-ESF-MEAS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_ESF_MEAS_UART1 = 0x20910278,
	/** Output rate of the UBX-ESF-RAW message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_ESF_RAW_UART1 = 0x209102a0,
	/** Output rate of the UBX-ESF-STATUS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_ESF_STATUS_UART1 = 0x20910106,
	/** Output rate of the UBX-HNR-ATT message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_HNR_ATT_UART1 = 0x20910378,
	/** Output rate of the UBX-HNR-INS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_HNR_INS_UART1 = 0x20910373,
	/** Output rate of the UBX-HNR-PVT message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_HNR_PVT_UART1 = 0x2091028c,
	/** Output rate of the UBX-LOG-INFO message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_LOG_INFO_UART1 = 0x2091025a,
	/** Output rate of the UBX-MON-COMMS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_MON_COMMS_UART1 = 0x20910350,
	/** Output rate of the UBX-MON-HW2 message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_MON_HW2_UART1 = 0x209101ba,
	/** Output rate of the UBX-MON-HW3 message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_MON_HW3_UART1 = 0x20910355,
	/** Output rate of the UBX-MON-HW message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_MON_HW_UART1 = 0x209101b5,
	/** Output rate of the UBX-MON-INST message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_MON_INST_UART1 = 0x209103cc,
	/** Output rate of the UBX-MON-IO message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_MON_IO_UART1 = 0x209101a6,
	/** Output rate of the UBX-MON-MSGPP message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_MON_MSGPP_UART1 = 0x20910197,
	/** Output rate of the UBX_MON_PMP message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_MON_PMP_UART1 = 0x20910323,
	/** Output rate of the UBX-MON-RF message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_MON_RF_UART1 = 0x2091035a,
	/** Output rate of the UBX-MON-RXBUF message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_MON_RXBUF_UART1 = 0x209101a1,
	/** Output rate of the UBX-MON-RXR message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_MON_RXR_UART1 = 0x20910188,
	/** Output rate of the UBX-MON-SPAN message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_MON_SPAN_UART1 = 0x2091038c,
	/** Output rate of the UBX-MON-SYS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_MON_SYS_UART1 = 0x2091069e,
	/** Output rate of the UBX-MON-TXBUF message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_MON_TXBUF_UART1 = 0x2091019c,
	/** Output rate of the UBX-NAV2-CLOCK message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_CLOCK_UART1 = 0x20910431,
	/** Output rate of the UBX-NAV2-COV message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_COV_UART1 = 0x20910436,
	/** Output rate of the UBX-NAV2-DOP message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_DOP_UART1 = 0x20910466,
	/** Output rate of the UBX-NAV2-EELL message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_EELL_UART1 = 0x20910471,
	/** Output rate of the UBX-NAV2-EOE message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_EOE_UART1 = 0x20910566,
	/** Output rate of the UBX-NAV2-ODO message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_ODO_UART1 = 0x20910476,
	/** Output rate of the UBX-NAV2-POSECEF message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_POSECEF_UART1 = 0x20910481,
	/** Output rate of the UBX-NAV2-POSLLH message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_POSLLH_UART1 = 0x20910486,
	/** Output rate of the UBX-NAV2-PVAT message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_PVAT_UART1 = 0x20910630,
	/** Output rate of the UBX-NAV2-PVT message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_PVT_UART1 = 0x20910491,
	/** Output rate of the UBX-NAV2-SAT message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SAT_UART1 = 0x20910496,
	/** Output rate of the UBX-NAV2-SBAS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SBAS_UART1 = 0x20910501,
	/** Output rate of the UBX-NAV2-SIG message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SIG_UART1 = 0x20910506,
	/** Output rate of the UBX-NAV2-SLAS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SLAS_UART1 = 0x20910511,
	/** Output rate of the UBX-NAV2-STATUS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_STATUS_UART1 = 0x20910516,
	/** Output rate of the UBX-NAV2-SVIN message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SVIN_UART1 = 0x20910521,
	/** Output rate of the UBX-NAV2-TIMEBDS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEBDS_UART1 = 0x20910526,
	/** Output rate of the UBX-NAV2-TIMEGAL message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEGAL_UART1 = 0x20910531,
	/** Output rate of the UBX-NAV2-TIMEGLO message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEGLO_UART1 = 0x20910536,
	/** Output rate of the UBX-NAV2-TIMEGPS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEGPS_UART1 = 0x20910541,
	/** Output rate of the UBX-NAV2-TIMELS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMELS_UART1 = 0x20910546,
	/** Output rate of the UBX-NAV2-TIMENAVIC message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMENAVIC_UART1 = 0x209106a8,
	/** Output rate of the UBX-NAV2-TIMEQZSS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEQZSS_UART1 = 0x20910576,
	/** Output rate of the UBX-NAV2-TIMEUTC message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEUTC_UART1 = 0x20910551,
	/** Output rate of the UBX-NAV2-VELECEF message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_VELECEF_UART1 = 0x20910556,
	/** Output rate of the UBX-NAV2-VELNED message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_VELNED_UART1 = 0x20910561,
	/** Output rate of the UBX-NAV-AOPSTATUS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_AOPSTATUS_UART1 = 0x2091007a,
	/** Output rate of the UBX-NAV-ATT message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_ATT_UART1 = 0x20910020,
	/** Output rate of the UBX_NAV_CFB message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_CFB_UART1 = 0x209106d5,
	/** Output rate of the UBX-NAV-CLOCK message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_CLOCK_UART1 = 0x20910066,
	/** Output rate of the UBX-NAV-COV message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_COV_UART1 = 0x20910084,
	/** Output rate of the UBX-NAV-DAHEADING message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_DAHEADING_UART1 = 0x209103e0,
	/** Output rate of the UBX-NAV-DGPS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_DGPS_UART1 = 0x20910075,
	/** Output rate of the UBX-NAV-DOP message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_DOP_UART1 = 0x20910039,
	/** Output rate of the UBX-NAV-EELL message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_EELL_UART1 = 0x20910314,
	/** Output rate of the UBX-NAV-EOE message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_EOE_UART1 = 0x20910160,
	/** Output rate of the UBX-NAV-GEOFENCE message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_GEOFENCE_UART1 = 0x209100a2,
	/** Output rate of the UBX-NAV-HPPOSECEF message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_HPPOSECEF_UART1 = 0x2091002f,
	/** Output rate of the UBX-NAV-HPPOSLLH message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_HPPOSLLH_UART1 = 0x20910034,
	/** Output rate of the UBX-NAV-NMI message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_NMI_UART1 = 0x20910591,
	/** Output rate of the UBX-NAV-ODO message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_ODO_UART1 = 0x2091007f,
	/** Output rate of the UBX-NAV-ORB message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_ORB_UART1 = 0x20910011,
	/** Output rate of the UBX-NAV-PL message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_PL_UART1 = 0x20910416,
	/** Output rate of the UBX-NAV-POSECEF message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_POSECEF_UART1 = 0x20910025,
	/** Output rate of the UBX-NAV-POSLLH message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_POSLLH_UART1 = 0x2091002a,
	/** Output rate of the UBX-NAV-PVAT message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_PVAT_UART1 = 0x2091062b,
	/** Output rate of the UBX-NAV-PVT message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_PVT_UART1 = 0x20910007,
	/** Output rate of the UBX-NAV-RELPOSNED message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_RELPOSNED_UART1 = 0x2091008e,
	/** Output rate of the UBX-NAV-SAT message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_SAT_UART1 = 0x20910016,
	/** Output rate of the UBX-NAV-SBAS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_SBAS_UART1 = 0x2091006b,
	/** Output rate of the UBX-NAV-SIG message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_SIG_UART1 = 0x20910346,
	/** Output rate of the UBX-NAV-SLAS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_SLAS_UART1 = 0x20910337,
	/** Output rate of the UBX-NAV-SOL message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_SOL_UART1 = 0x20910002,
	/** Output rate of the UBX-NAV-STATUS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_STATUS_UART1 = 0x2091001b,
	/** Output rate of the UBX-NAV-SVINFO message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_SVINFO_UART1 = 0x2091000c,
	/** Output rate of the UBX-NAV-SVIN message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_SVIN_UART1 = 0x20910089,
	/** Output rate of the UBX-NAV-TIMEBDS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEBDS_UART1 = 0x20910052,
	/** Output rate of the UBX-NAV-TIMEGAL message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEGAL_UART1 = 0x20910057,
	/** Output rate of the UBX-NAV-TIMEGLO message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEGLO_UART1 = 0x2091004d,
	/** Output rate of the UBX-NAV-TIMEGPS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEGPS_UART1 = 0x20910048,
	/** Output rate of the UBX-NAV-TIMELS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMELS_UART1 = 0x20910061,
	/** Output rate of the UBX-NAV-TIMENAVIC message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMENAVIC_UART1 = 0x209106a3,
	/** Output rate of the UBX-NAV-TIMEQZSS message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEQZSS_UART1 = 0x20910387,
	/** Output rate of the UBX-NAV-TIMETRUSTED message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMETRUSTED_UART1 = 0x209103a9,
	/** Output rate of the UBX-NAV-TIMEUTC message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEUTC_UART1 = 0x2091005c,
	/** Output rate of the UBX-NAV-VELECEF message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_VELECEF_UART1 = 0x2091003e,
	/** Output rate of the UBX-NAV-VELNED message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_NAV_VELNED_UART1 = 0x20910043,
	/** Output rate of the UBX-RXM-ALM message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_ALM_UART1 = 0x20910174,
	/** Output rate of the UBX-RXM-COR message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_COR_UART1 = 0x209106b7,
	/** Output rate of the UBX-RXM-EPH message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_EPH_UART1 = 0x2091016a,
	/** Output rate of the UBX-RXM-IMES message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_IMES_UART1 = 0x2091015b,
	/** Output rate of the UBX-RXM-MEAS20 message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEAS20_UART1 = 0x20910644,
	/** Output rate of the UBX-RXM-MEAS50 message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEAS50_UART1 = 0x20910649,
	/** Output rate of the UBX-RXM-MEASC12 message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASC12_UART1 = 0x2091063f,
	/** Output rate of the UBX-RXM-MEASD12 message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASD12_UART1 = 0x2091063a,
	/** Output rate of the UBX-RXM-MEASX2 message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASX2_UART1 = 0x209103c2,
	/** Output rate of the UBX-RXM-MEASX message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASX_UART1 = 0x20910205,
	/** Output rate of the UBX_RXM_PMP message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_PMP_UART1 = 0x2091031e,
	/** output rate of the UBX-RXM-QZSSL6 message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_QZSSL6_UART1 = 0x2091033b,
	/** Output rate of the UBX-RXM-RAWX message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_RAWX_UART1 = 0x209102a5,
	/** Output rate of the UBX-RXM-RLM message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_RLM_UART1 = 0x2091025f,
	/** Output rate of the UBX-RXM-RTCM message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_RTCM_UART1 = 0x20910269,
	/** Output rate of the UBX-RXM-SFRBX message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_SFRBX_UART1 = 0x20910232,
	/** Output rate of the UBX-RXM-SPARTN message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_SPARTN_UART1 = 0x20910606,
	/** Output rate of the UBX-RXM-SVSI message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_SVSI_UART1 = 0x20910151,
	/** Output rate of the UBX-RXM-TM message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_RXM_TM_UART1 = 0x20910611,
	/** Output rate of the UBX-SEC-OSNMA message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_SEC_OSNMA_UART1 = 0x209106cb,
	/** Output rate of the UBX-SEC-SIGLOG message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_SEC_SIGLOG_UART1 = 0x2091068a,
	/** Output rate of the UBX-SEC-SIG message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_SEC_SIG_UART1 = 0x20910635,
	/** Output rate of the UBX-TIM-SVIN message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_TIM_SVIN_UART1 = 0x20910098,
	/** Output rate of the UBX-TIM-TM2 message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_TIM_TM2_UART1 = 0x20910179,
	/** Output rate of the UBX-TIM-TP message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_TIM_TP_UART1 = 0x2091017e,
	/** Output rate of the UBX-TIM-VRFY message on the UART1 port */
	UBX_KEY_MSG_OUT_UBX_TIM_VRFY_UART1 = 0x20910093,
};

/** Message output configuration (I2C) */
enum ubx_key_msg_out_i2c {
	/** Output rate of the NMEA-GX-DTM message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_DTM_I2C = 0x209100a6,
	/** Output rate of the NMEA-GX-GBS message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_GBS_I2C = 0x209100dd,
	/** Output rate of the NMEA-GX-GGA message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_GGA_I2C = 0x209100ba,
	/** Output rate of the NMEA-GX-GLL message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_GLL_I2C = 0x209100c9,
	/** Output rate of the NMEA-GX-GNS message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_GNS_I2C = 0x209100b5,
	/** Output rate of the NMEA-GX-GRS message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_GRS_I2C = 0x209100ce,
	/** Output rate of the NMEA-GX-GSA message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_GSA_I2C = 0x209100bf,
	/** Output rate of the NMEA-GX-GST message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_GST_I2C = 0x209100d3,
	/** Output rate of the NMEA-GX-GSV message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_GSV_I2C = 0x209100c4,
	/** Output rate of the NMEA-GX-RLM message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_RLM_I2C = 0x20910400,
	/** Output rate of the NMEA-GX-RMC message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_RMC_I2C = 0x209100ab,
	/** Output rate of the NMEA-GX-THS message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_THS_I2C = 0x209100e2,
	/** The output rate of the NMEA-GX-UTC message on the I2C port. */
	UBX_KEY_MSG_OUT_NMEA_UTC_I2C = 0x209106cf,
	/** Output rate of the NMEA-GX-VLW message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_VLW_I2C = 0x209100e7,
	/** Output rate of the NMEA-GX-VTG message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_VTG_I2C = 0x209100b0,
	/** Output rate of the NMEA-GX-ZDA message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_ZDA_I2C = 0x209100d8,
	/** Output rate of the NMEA-NAV2-GX-GGA message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GGA_I2C = 0x20910661,
	/** Output rate of the NMEA-NAV2-GX-GLL message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GLL_I2C = 0x20910670,
	/** Output rate of the NMEA-NAV2-GX-GNS message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GNS_I2C = 0x2091065c,
	/** Output rate of the NMEA-NAV2-GX-GSA message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GSA_I2C = 0x20910666,
	/** Output rate of the NMEA-NAV2-GX-RMC message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_RMC_I2C = 0x20910652,
	/** Output rate of the NMEA-NAV2-GX-VTG message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_VTG_I2C = 0x20910657,
	/** Output rate of the NMEA-NAV2-GX-ZDA message on the I2C port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_ZDA_I2C = 0x2091067f,
	/** Output rate of the NMEA-GX-PUBX00 message on the I2C port */
	UBX_KEY_MSG_OUT_PUBX_POLYP_I2C = 0x209100ec,
	/** Output rate of the NMEA-GX-PUBX03 message on the I2C port */
	UBX_KEY_MSG_OUT_PUBX_POLYS_I2C = 0x209100f1,
	/** Output rate of the NMEA-GX-PUBX04 message on the I2C port */
	UBX_KEY_MSG_OUT_PUBX_POLYT_I2C = 0x209100f6,
	/** Output rate of the NMEA-GX-PUBX50 message on the I2C port */
	UBX_KEY_MSG_OUT_PUBX_POLYV_I2C = 0x209103ee,
	/** Output rate of the RTCM-3X-TYPE1005 message on the I2C port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1005_I2C = 0x209102bd,
	/** Output rate of the RTCM-3X-TYPE1006 message on the I2C port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1006_I2C = 0x209102c2,
	/** Output rate of the RTCM-3X-TYPE1074 message on the I2C port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1074_I2C = 0x2091035e,
	/** Output rate of the RTCM-3X-TYPE1077 message on the I2C port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1077_I2C = 0x209102cc,
	/** Output rate of the RTCM-3X-TYPE1084 message on the I2C port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1084_I2C = 0x20910363,
	/** Output rate of the RTCM-3X-TYPE1087 message on the I2C port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1087_I2C = 0x209102d1,
	/** Output rate of the RTCM-3X-TYPE1094 message on the I2C port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1094_I2C = 0x20910368,
	/** Output rate of the RTCM-3X-TYPE1097 message on the I2C port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1097_I2C = 0x20910318,
	/** Output rate of the RTCM-3X-TYPE1124 message on the I2C port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1124_I2C = 0x2091036d,
	/** Output rate of the RTCM-3X-TYPE1127 message on the I2C port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1127_I2C = 0x209102d6,
	/** Output rate of the RTCM-3X-TYPE1230 message on the I2C port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1230_I2C = 0x20910303,
	/** Output rate of the RTCM-3X-TYPE4072_0 message on the I2C port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE4072_0_I2C = 0x209102fe,
	/** Output rate of the RTCM-3X-TYPE4072_1 message on the I2C port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE4072_1_I2C = 0x20910381,
	/** Output rate of the UBX-AID-ALM message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_AID_ALM_I2C = 0x2091016e,
	/** Output rate of the UBX-AID-AOP message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_AID_AOP_I2C = 0x2091026d,
	/** Output rate of the UBX-AID-EPH message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_AID_EPH_I2C = 0x20910164,
	/** Output rate of the UBX-AID-INI message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_AID_INI_I2C = 0x209100fb,
	/** Output rate of the UBX-ESF-ALG message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_ESF_ALG_I2C = 0x2091010f,
	/** Output rate of the UBX-ESF-CAL message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_ESF_CAL_I2C = 0x209106ac,
	/** Output rate of the UBX-ESF-INS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_ESF_INS_I2C = 0x20910114,
	/** Output rate of the UBX-ESF-MEAS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_ESF_MEAS_I2C = 0x20910277,
	/** Output rate of the UBX-ESF-RAW message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_ESF_RAW_I2C = 0x2091029f,
	/** Output rate of the UBX-ESF-STATUS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_ESF_STATUS_I2C = 0x20910105,
	/** Output rate of the UBX-HNR-ATT message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_HNR_ATT_I2C = 0x20910377,
	/** Output rate of the UBX-HNR-INS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_HNR_INS_I2C = 0x20910372,
	/** Output rate of the UBX-HNR-PVT message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_HNR_PVT_I2C = 0x2091028b,
	/** Output rate of the UBX-LOG-INFO message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_LOG_INFO_I2C = 0x20910259,
	/** Output rate of the UBX-MON-COMMS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_MON_COMMS_I2C = 0x2091034f,
	/** Output rate of the UBX-MON-HW2 message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_MON_HW2_I2C = 0x209101b9,
	/** Output rate of the UBX-MON-HW3 message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_MON_HW3_I2C = 0x20910354,
	/** Output rate of the UBX-MON-HW message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_MON_HW_I2C = 0x209101b4,
	/** Output rate of the UBX-MON-INST message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_MON_INST_I2C = 0x209103cb,
	/** Output rate of the UBX-MON-IO message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_MON_IO_I2C = 0x209101a5,
	/** Output rate of the UBX-MON-MSGPP message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_MON_MSGPP_I2C = 0x20910196,
	/** Output rate of the UBX_MON_PMP message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_MON_PMP_I2C = 0x20910322,
	/** Output rate of the UBX-MON-RF message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_MON_RF_I2C = 0x20910359,
	/** Output rate of the UBX-MON-RXBUF message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_MON_RXBUF_I2C = 0x209101a0,
	/** Output rate of the UBX-MON-RXR message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_MON_RXR_I2C = 0x20910187,
	/** Output rate of the UBX-MON-SPAN message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_MON_SPAN_I2C = 0x2091038b,
	/** Output rate of the UBX-MON-SYS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_MON_SYS_I2C = 0x2091069d,
	/** Output rate of the UBX-MON-TXBUF message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_MON_TXBUF_I2C = 0x2091019b,
	/** Output rate of the UBX-NAV2-CLOCK message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_CLOCK_I2C = 0x20910430,
	/** Output rate of the UBX-NAV2-COV message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_COV_I2C = 0x20910435,
	/** Output rate of the UBX-NAV2-DOP message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_DOP_I2C = 0x20910465,
	/** Output rate of the UBX-NAV2-EELL message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_EELL_I2C = 0x20910470,
	/** Output rate of the UBX-NAV2-EOE message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_EOE_I2C = 0x20910565,
	/** Output rate of the UBX-NAV2-ODO message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_ODO_I2C = 0x20910475,
	/** Output rate of the UBX-NAV2-POSECEF message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_POSECEF_I2C = 0x20910480,
	/** Output rate of the UBX-NAV2-POSLLH message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_POSLLH_I2C = 0x20910485,
	/** Output rate of the UBX-NAV2-PVAT message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_PVAT_I2C = 0x2091062f,
	/** Output rate of the UBX-NAV2-PVT message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_PVT_I2C = 0x20910490,
	/** Output rate of the UBX-NAV2-SAT message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SAT_I2C = 0x20910495,
	/** Output rate of the UBX-NAV2-SBAS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SBAS_I2C = 0x20910500,
	/** Output rate of the UBX-NAV2-SIG message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SIG_I2C = 0x20910505,
	/** Output rate of the UBX-NAV2-SLAS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SLAS_I2C = 0x20910510,
	/** Output rate of the UBX-NAV2-STATUS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_STATUS_I2C = 0x20910515,
	/** Output rate of the UBX-NAV2-SVIN message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SVIN_I2C = 0x20910520,
	/** Output rate of the UBX-NAV2-TIMEBDS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEBDS_I2C = 0x20910525,
	/** Output rate of the UBX-NAV2-TIMEGAL message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEGAL_I2C = 0x20910530,
	/** Output rate of the UBX-NAV2-TIMEGLO message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEGLO_I2C = 0x20910535,
	/** Output rate of the UBX-NAV2-TIMEGPS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEGPS_I2C = 0x20910540,
	/** Output rate of the UBX-NAV2-TIMELS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMELS_I2C = 0x20910545,
	/** Output rate of the UBX-NAV2-TIMENAVIC message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMENAVIC_I2C = 0x209106a7,
	/** Output rate of the UBX-NAV2-TIMEQZSS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEQZSS_I2C = 0x20910575,
	/** Output rate of the UBX-NAV2-TIMEUTC message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEUTC_I2C = 0x20910550,
	/** Output rate of the UBX-NAV2-VELECEF message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_VELECEF_I2C = 0x20910555,
	/** Output rate of the UBX-NAV2-VELNED message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV2_VELNED_I2C = 0x20910560,
	/** Output rate of the UBX-NAV-AOPSTATUS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_AOPSTATUS_I2C = 0x20910079,
	/** Output rate of the UBX-NAV-ATT message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_ATT_I2C = 0x2091001f,
	/** Output rate of the UBX_NAV_CFB message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_CFB_I2C = 0x209106d4,
	/** Output rate of the UBX-NAV-CLOCK message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_CLOCK_I2C = 0x20910065,
	/** Output rate of the UBX-NAV-COV message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_COV_I2C = 0x20910083,
	/** Output rate of the UBX-NAV-DAHEADING message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_DAHEADING_I2C = 0x209103df,
	/** Output rate of the UBX-NAV-DGPS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_DGPS_I2C = 0x20910074,
	/** Output rate of the UBX-NAV-DOP message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_DOP_I2C = 0x20910038,
	/** Output rate of the UBX-NAV-EELL message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_EELL_I2C = 0x20910313,
	/** Output rate of the UBX-NAV-EOE message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_EOE_I2C = 0x2091015f,
	/** Output rate of the UBX-NAV-GEOFENCE message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_GEOFENCE_I2C = 0x209100a1,
	/** Output rate of the UBX-NAV-HPPOSECEF message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_HPPOSECEF_I2C = 0x2091002e,
	/** Output rate of the UBX-NAV-HPPOSLLH message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_HPPOSLLH_I2C = 0x20910033,
	/** Output rate of the UBX-NAV-NMI message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_NMI_I2C = 0x20910590,
	/** Output rate of the UBX-NAV-ODO message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_ODO_I2C = 0x2091007e,
	/** Output rate of the UBX-NAV-ORB message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_ORB_I2C = 0x20910010,
	/** Output rate of the UBX-NAV-PL message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_PL_I2C = 0x20910415,
	/** Output rate of the UBX-NAV-POSECEF message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_POSECEF_I2C = 0x20910024,
	/** Output rate of the UBX-NAV-POSLLH message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_POSLLH_I2C = 0x20910029,
	/** Output rate of the UBX-NAV-PVAT message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_PVAT_I2C = 0x2091062a,
	/** Output rate of the UBX-NAV-PVT message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_PVT_I2C = 0x20910006,
	/** Output rate of the UBX-NAV-RELPOSNED message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_RELPOSNED_I2C = 0x2091008d,
	/** Output rate of the UBX-NAV-SAT message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_SAT_I2C = 0x20910015,
	/** Output rate of the UBX-NAV-SBAS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_SBAS_I2C = 0x2091006a,
	/** Output rate of the UBX-NAV-SIG message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_SIG_I2C = 0x20910345,
	/** Output rate of the UBX-NAV-SLAS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_SLAS_I2C = 0x20910336,
	/** Output rate of the UBX-NAV-SOL message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_SOL_I2C = 0x20910001,
	/** Output rate of the UBX-NAV-STATUS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_STATUS_I2C = 0x2091001a,
	/** Output rate of the UBX-NAV-SVINFO message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_SVINFO_I2C = 0x2091000b,
	/** Output rate of the UBX-NAV-SVIN message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_SVIN_I2C = 0x20910088,
	/** Output rate of the UBX-NAV-TIMEBDS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEBDS_I2C = 0x20910051,
	/** Output rate of the UBX-NAV-TIMEGAL message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEGAL_I2C = 0x20910056,
	/** Output rate of the UBX-NAV-TIMEGLO message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEGLO_I2C = 0x2091004c,
	/** Output rate of the UBX-NAV-TIMEGPS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEGPS_I2C = 0x20910047,
	/** Output rate of the UBX-NAV-TIMELS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMELS_I2C = 0x20910060,
	/** Output rate of the UBX-NAV-TIMENAVIC message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMENAVIC_I2C = 0x209106a2,
	/** Output rate of the UBX-NAV-TIMEQZSS message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEQZSS_I2C = 0x20910386,
	/** Output rate of the UBX-NAV-TIMETRUSTED message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMETRUSTED_I2C = 0x209103a8,
	/** Output rate of the UBX-NAV-TIMEUTC message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEUTC_I2C = 0x2091005b,
	/** Output rate of the UBX-NAV-VELECEF message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_VELECEF_I2C = 0x2091003d,
	/** Output rate of the UBX-NAV-VELNED message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_NAV_VELNED_I2C = 0x20910042,
	/** Output rate of the UBX-RXM-ALM message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_ALM_I2C = 0x20910173,
	/** Output rate of the UBX-RXM-COR message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_COR_I2C = 0x209106b6,
	/** Output rate of the UBX-RXM-EPH message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_EPH_I2C = 0x20910169,
	/** Output rate of the UBX-RXM-IMES message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_IMES_I2C = 0x2091015a,
	/** Output rate of the UBX-RXM-MEAS20 message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEAS20_I2C = 0x20910643,
	/** Output rate of the UBX-RXM-MEAS50 message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEAS50_I2C = 0x20910648,
	/** Output rate of the UBX-RXM-MEASC12 message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASC12_I2C = 0x2091063e,
	/** Output rate of the UBX-RXM-MEASD12 message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASD12_I2C = 0x20910639,
	/** Output rate of the UBX-RXM-MEASX2 message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASX2_I2C = 0x209103c1,
	/** Output rate of the UBX-RXM-MEASX message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASX_I2C = 0x20910204,
	/** Output rate of the UBX_RXM_PMP message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_PMP_I2C = 0x2091031d,
	/** output rate of the UBX-RXM-QZSSL6 message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_QZSSL6_I2C = 0x2091033f,
	/** Output rate of the UBX-RXM-RAWX message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_RAWX_I2C = 0x209102a4,
	/** Output rate of the UBX-RXM-RLM message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_RLM_I2C = 0x2091025e,
	/** Output rate of the UBX-RXM-RTCM message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_RTCM_I2C = 0x20910268,
	/** Output rate of the UBX-RXM-SFRBX message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_SFRBX_I2C = 0x20910231,
	/** Output rate of the UBX-RXM-SPARTN message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_SPARTN_I2C = 0x20910605,
	/** Output rate of the UBX-RXM-SVSI message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_SVSI_I2C = 0x20910150,
	/** Output rate of the UBX-RXM-TM message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_RXM_TM_I2C = 0x20910610,
	/** Output rate of the UBX-SEC-OSNMA message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_SEC_OSNMA_I2C = 0x209106ca,
	/** Output rate of the UBX-SEC-SIGLOG message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_SEC_SIGLOG_I2C = 0x20910689,
	/** Output rate of the UBX-SEC-SIG message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_SEC_SIG_I2C = 0x20910634,
	/** Output rate of the UBX-TIM-SVIN message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_TIM_SVIN_I2C = 0x20910097,
	/** Output rate of the UBX-TIM-TM2 message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_TIM_TM2_I2C = 0x20910178,
	/** Output rate of the UBX-TIM-TP message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_TIM_TP_I2C = 0x2091017d,
	/** Output rate of the UBX-TIM-VRFY message on the I2C port */
	UBX_KEY_MSG_OUT_UBX_TIM_VRFY_I2C = 0x20910092,
};

/** Message output configuration (UART2) */
enum ubx_key_msg_out_uart2 {
	/** Output rate of the NMEA-GX-DTM message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_DTM_UART2 = 0x209100a8,
	/** Output rate of the NMEA-GX-GBS message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_GBS_UART2 = 0x209100df,
	/** Output rate of the NMEA-GX-GGA message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_GGA_UART2 = 0x209100bc,
	/** Output rate of the NMEA-GX-GLL message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_GLL_UART2 = 0x209100cb,
	/** Output rate of the NMEA-GX-GNS message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_GNS_UART2 = 0x209100b7,
	/** Output rate of the NMEA-GX-GRS message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_GRS_UART2 = 0x209100d0,
	/** Output rate of the NMEA-GX-GSA message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_GSA_UART2 = 0x209100c1,
	/** Output rate of the NMEA-GX-GST message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_GST_UART2 = 0x209100d5,
	/** Output rate of the NMEA-GX-GSV message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_GSV_UART2 = 0x209100c6,
	/** Output rate of the NMEA-GX-RLM message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_RLM_UART2 = 0x20910402,
	/** Output rate of the NMEA-GX-RMC message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_RMC_UART2 = 0x209100ad,
	/** Output rate of the NMEA-GX-THS message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_THS_UART2 = 0x209100e4,
	/** The output rate of the NMEA-GX-UTC message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_UTC_UART2 = 0x209106d1,
	/** Output rate of the NMEA-GX-VLW message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_VLW_UART2 = 0x209100e9,
	/** Output rate of the NMEA-GX-VTG message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_VTG_UART2 = 0x209100b2,
	/** Output rate of the NMEA-GX-ZDA message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_ZDA_UART2 = 0x209100da,
	/** Output rate of the NMEA-NAV2-GX-GGA message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GGA_UART2 = 0x20910663,
	/** Output rate of the NMEA-NAV2-GX-GLL message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GLL_UART2 = 0x20910672,
	/** Output rate of the NMEA-NAV2-GX-GNS message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GNS_UART2 = 0x2091065e,
	/** Output rate of the NMEA-NAV2-GX-GSA message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GSA_UART2 = 0x20910668,
	/** Output rate of the NMEA-NAV2-GX-RMC message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_RMC_UART2 = 0x20910654,
	/** Output rate of the NMEA-NAV2-GX-VTG message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_VTG_UART2 = 0x20910659,
	/** Output rate of the NMEA-NAV2-GX-ZDA message on the UART2 port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_ZDA_UART2 = 0x20910681,
	/** Output rate of the NMEA-GX-PUBX00 message on the UART2 port */
	UBX_KEY_MSG_OUT_PUBX_POLYP_UART2 = 0x209100ee,
	/** Output rate of the NMEA-GX-PUBX03 message on the UART2 port */
	UBX_KEY_MSG_OUT_PUBX_POLYS_UART2 = 0x209100f3,
	/** Output rate of the NMEA-GX-PUBX04 message on the UART2 port */
	UBX_KEY_MSG_OUT_PUBX_POLYT_UART2 = 0x209100f8,
	/** Output rate of the NMEA-GX-PUBX50 message on the UART2 port */
	UBX_KEY_MSG_OUT_PUBX_POLYV_UART2 = 0x209103f0,
	/** Output rate of the RTCM-3X-TYPE1005 message on the UART2 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1005_UART2 = 0x209102bf,
	/** Output rate of the RTCM-3X-TYPE1006 message on the UART2 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1006_UART2 = 0x209102c4,
	/** Output rate of the RTCM-3X-TYPE1074 message on the UART2 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1074_UART2 = 0x20910360,
	/** Output rate of the RTCM-3X-TYPE1077 message on the UART2 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1077_UART2 = 0x209102ce,
	/** Output rate of the RTCM-3X-TYPE1084 message on the UART2 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1084_UART2 = 0x20910365,
	/** Output rate of the RTCM-3X-TYPE1087 message on the UART2 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1087_UART2 = 0x209102d3,
	/** Output rate of the RTCM-3X-TYPE1094 message on the UART2 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1094_UART2 = 0x2091036a,
	/** Output rate of the RTCM-3X-TYPE1097 message on the UART2 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1097_UART2 = 0x2091031a,
	/** Output rate of the RTCM-3X-TYPE1124 message on the UART2 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1124_UART2 = 0x2091036f,
	/** Output rate of the RTCM-3X-TYPE1127 message on the UART2 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1127_UART2 = 0x209102d8,
	/** Output rate of the RTCM-3X-TYPE1230 message on the UART2 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1230_UART2 = 0x20910305,
	/** Output rate of the RTCM-3X-TYPE4072_0 message on the UART2 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE4072_0_UART2 = 0x20910300,
	/** Output rate of the RTCM-3X-TYPE4072_1 message on the UART2 port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE4072_1_UART2 = 0x20910383,
	/** Output rate of the UBX-AID-ALM message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_AID_ALM_UART2 = 0x20910170,
	/** Output rate of the UBX-AID-AOP message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_AID_AOP_UART2 = 0x2091026f,
	/** Output rate of the UBX-AID-EPH message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_AID_EPH_UART2 = 0x20910166,
	/** Output rate of the UBX-AID-INI message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_AID_INI_UART2 = 0x209100fd,
	/** Output rate of the UBX-ESF-ALG message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_ESF_ALG_UART2 = 0x20910111,
	/** Output rate of the UBX-ESF-CAL message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_ESF_CAL_UART2 = 0x209106ae,
	/** Output rate of the UBX-ESF-INS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_ESF_INS_UART2 = 0x20910116,
	/** Output rate of the UBX-ESF-MEAS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_ESF_MEAS_UART2 = 0x20910279,
	/** Output rate of the UBX-ESF-RAW message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_ESF_RAW_UART2 = 0x209102a1,
	/** Output rate of the UBX-ESF-STATUS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_ESF_STATUS_UART2 = 0x20910107,
	/** Output rate of the UBX-HNR-ATT message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_HNR_ATT_UART2 = 0x20910379,
	/** Output rate of the UBX-HNR-INS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_HNR_INS_UART2 = 0x20910374,
	/** Output rate of the UBX-HNR-PVT message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_HNR_PVT_UART2 = 0x2091028d,
	/** Output rate of the UBX-LOG-INFO message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_LOG_INFO_UART2 = 0x2091025b,
	/** Output rate of the UBX-MON-COMMS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_MON_COMMS_UART2 = 0x20910351,
	/** Output rate of the UBX-MON-HW2 message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_MON_HW2_UART2 = 0x209101bb,
	/** Output rate of the UBX-MON-HW3 message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_MON_HW3_UART2 = 0x20910356,
	/** Output rate of the UBX-MON-HW message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_MON_HW_UART2 = 0x209101b6,
	/** Output rate of the UBX-MON-INST message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_MON_INST_UART2 = 0x209103cd,
	/** Output rate of the UBX-MON-IO message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_MON_IO_UART2 = 0x209101a7,
	/** Output rate of the UBX-MON-MSGPP message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_MON_MSGPP_UART2 = 0x20910198,
	/** Output rate of the UBX_MON_PMP message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_MON_PMP_UART2 = 0x20910324,
	/** Output rate of the UBX-MON-RF message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_MON_RF_UART2 = 0x2091035b,
	/** Output rate of the UBX-MON-RXBUF message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_MON_RXBUF_UART2 = 0x209101a2,
	/** Output rate of the UBX-MON-RXR message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_MON_RXR_UART2 = 0x20910189,
	/** Output rate of the UBX-MON-SPAN message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_MON_SPAN_UART2 = 0x2091038d,
	/** Output rate of the UBX-MON-SYS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_MON_SYS_UART2 = 0x2091069f,
	/** Output rate of the UBX-MON-TXBUF message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_MON_TXBUF_UART2 = 0x2091019d,
	/** Output rate of the UBX-NAV2-CLOCK message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_CLOCK_UART2 = 0x20910432,
	/** Output rate of the UBX-NAV2-COV message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_COV_UART2 = 0x20910437,
	/** Output rate of the UBX-NAV2-DOP message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_DOP_UART2 = 0x20910467,
	/** Output rate of the UBX-NAV2-EELL message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_EELL_UART2 = 0x20910472,
	/** Output rate of the UBX-NAV2-EOE message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_EOE_UART2 = 0x20910567,
	/** Output rate of the UBX-NAV2-ODO message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_ODO_UART2 = 0x20910477,
	/** Output rate of the UBX-NAV2-POSECEF message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_POSECEF_UART2 = 0x20910482,
	/** Output rate of the UBX-NAV2-POSLLH message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_POSLLH_UART2 = 0x20910487,
	/** Output rate of the UBX-NAV2-PVAT message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_PVAT_UART2 = 0x20910631,
	/** Output rate of the UBX-NAV2-PVT message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_PVT_UART2 = 0x20910492,
	/** Output rate of the UBX-NAV2-SAT message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SAT_UART2 = 0x20910497,
	/** Output rate of the UBX-NAV2-SBAS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SBAS_UART2 = 0x20910502,
	/** Output rate of the UBX-NAV2-SIG message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SIG_UART2 = 0x20910507,
	/** Output rate of the UBX-NAV2-SLAS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SLAS_UART2 = 0x20910512,
	/** Output rate of the UBX-NAV2-STATUS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_STATUS_UART2 = 0x20910517,
	/** Output rate of the UBX-NAV2-SVIN message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SVIN_UART2 = 0x20910522,
	/** Output rate of the UBX-NAV2-TIMEBDS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEBDS_UART2 = 0x20910527,
	/** Output rate of the UBX-NAV2-TIMEGAL message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEGAL_UART2 = 0x20910532,
	/** Output rate of the UBX-NAV2-TIMEGLO message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEGLO_UART2 = 0x20910537,
	/** Output rate of the UBX-NAV2-TIMEGPS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEGPS_UART2 = 0x20910542,
	/** Output rate of the UBX-NAV2-TIMELS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMELS_UART2 = 0x20910547,
	/** Output rate of the UBX-NAV2-TIMENAVIC message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMENAVIC_UART2 = 0x209106a9,
	/** Output rate of the UBX-NAV2-TIMEQZSS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEQZSS_UART2 = 0x20910577,
	/** Output rate of the UBX-NAV2-TIMEUTC message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEUTC_UART2 = 0x20910552,
	/** Output rate of the UBX-NAV2-VELECEF message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_VELECEF_UART2 = 0x20910557,
	/** Output rate of the UBX-NAV2-VELNED message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV2_VELNED_UART2 = 0x20910562,
	/** Output rate of the UBX-NAV-AOPSTATUS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_AOPSTATUS_UART2 = 0x2091007b,
	/** Output rate of the UBX-NAV-ATT message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_ATT_UART2 = 0x20910021,
	/** Output rate of the UBX_NAV_CFB message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_CFB_UART2 = 0x209106d6,
	/** Output rate of the UBX-NAV-CLOCK message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_CLOCK_UART2 = 0x20910067,
	/** Output rate of the UBX-NAV-COV message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_COV_UART2 = 0x20910085,
	/** Output rate of the UBX-NAV-DAHEADING message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_DAHEADING_UART2 = 0x209103e1,
	/** Output rate of the UBX-NAV-DGPS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_DGPS_UART2 = 0x20910076,
	/** Output rate of the UBX-NAV-DOP message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_DOP_UART2 = 0x2091003a,
	/** Output rate of the UBX-NAV-EELL message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_EELL_UART2 = 0x20910315,
	/** Output rate of the UBX-NAV-EOE message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_EOE_UART2 = 0x20910161,
	/** Output rate of the UBX-NAV-GEOFENCE message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_GEOFENCE_UART2 = 0x209100a3,
	/** Output rate of the UBX-NAV-HPPOSECEF message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_HPPOSECEF_UART2 = 0x20910030,
	/** Output rate of the UBX-NAV-HPPOSLLH message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_HPPOSLLH_UART2 = 0x20910035,
	/** Output rate of the UBX-NAV-NMI message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_NMI_UART2 = 0x20910592,
	/** Output rate of the UBX-NAV-ODO message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_ODO_UART2 = 0x20910080,
	/** Output rate of the UBX-NAV-ORB message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_ORB_UART2 = 0x20910012,
	/** Output rate of the UBX-NAV-PL message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_PL_UART2 = 0x20910417,
	/** Output rate of the UBX-NAV-POSECEF message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_POSECEF_UART2 = 0x20910026,
	/** Output rate of the UBX-NAV-POSLLH message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_POSLLH_UART2 = 0x2091002b,
	/** Output rate of the UBX-NAV-PVAT message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_PVAT_UART2 = 0x2091062c,
	/** Output rate of the UBX-NAV-PVT message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_PVT_UART2 = 0x20910008,
	/** Output rate of the UBX-NAV-RELPOSNED message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_RELPOSNED_UART2 = 0x2091008f,
	/** Output rate of the UBX-NAV-SAT message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_SAT_UART2 = 0x20910017,
	/** Output rate of the UBX-NAV-SBAS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_SBAS_UART2 = 0x2091006c,
	/** Output rate of the UBX-NAV-SIG message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_SIG_UART2 = 0x20910347,
	/** Output rate of the UBX-NAV-SLAS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_SLAS_UART2 = 0x20910338,
	/** Output rate of the UBX-NAV-SOL message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_SOL_UART2 = 0x20910003,
	/** Output rate of the UBX-NAV-STATUS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_STATUS_UART2 = 0x2091001c,
	/** Output rate of the UBX-NAV-SVINFO message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_SVINFO_UART2 = 0x2091000d,
	/** Output rate of the UBX-NAV-SVIN message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_SVIN_UART2 = 0x2091008a,
	/** Output rate of the UBX-NAV-TIMEBDS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEBDS_UART2 = 0x20910053,
	/** Output rate of the UBX-NAV-TIMEGAL message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEGAL_UART2 = 0x20910058,
	/** Output rate of the UBX-NAV-TIMEGLO message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEGLO_UART2 = 0x2091004e,
	/** Output rate of the UBX-NAV-TIMEGPS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEGPS_UART2 = 0x20910049,
	/** Output rate of the UBX-NAV-TIMELS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMELS_UART2 = 0x20910062,
	/** Output rate of the UBX-NAV-TIMENAVIC message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMENAVIC_UART2 = 0x209106a4,
	/** Output rate of the UBX-NAV-TIMEQZSS message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEQZSS_UART2 = 0x20910388,
	/** Output rate of the UBX-NAV-TIMETRUSTED message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMETRUSTED_UART2 = 0x209103aa,
	/** Output rate of the UBX-NAV-TIMEUTC message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEUTC_UART2 = 0x2091005d,
	/** Output rate of the UBX-NAV-VELECEF message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_VELECEF_UART2 = 0x2091003f,
	/** Output rate of the UBX-NAV-VELNED message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_NAV_VELNED_UART2 = 0x20910044,
	/** Output rate of the UBX-RXM-ALM message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_ALM_UART2 = 0x20910175,
	/** Output rate of the UBX-RXM-COR message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_COR_UART2 = 0x209106b8,
	/** Output rate of the UBX-RXM-EPH message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_EPH_UART2 = 0x2091016b,
	/** Output rate of the UBX-RXM-IMES message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_IMES_UART2 = 0x2091015c,
	/** Output rate of the UBX-RXM-MEAS20 message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEAS20_UART2 = 0x20910645,
	/** Output rate of the UBX-RXM-MEAS50 message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEAS50_UART2 = 0x2091064a,
	/** Output rate of the UBX-RXM-MEASC12 message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASC12_UART2 = 0x20910640,
	/** Output rate of the UBX-RXM-MEASD12 message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASD12_UART2 = 0x2091063b,
	/** Output rate of the UBX-RXM-MEASX2 message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASX2_UART2 = 0x209103c3,
	/** Output rate of the UBX-RXM-MEASX message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASX_UART2 = 0x20910206,
	/** Output rate of the UBX_RXM_PMP message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_PMP_UART2 = 0x2091031f,
	/** output rate of the UBX-RXM-QZSSL6 message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_QZSSL6_UART2 = 0x2091033c,
	/** Output rate of the UBX-RXM-RAWX message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_RAWX_UART2 = 0x209102a6,
	/** Output rate of the UBX-RXM-RLM message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_RLM_UART2 = 0x20910260,
	/** Output rate of the UBX-RXM-RTCM message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_RTCM_UART2 = 0x2091026a,
	/** Output rate of the UBX-RXM-SFRBX message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_SFRBX_UART2 = 0x20910233,
	/** Output rate of the UBX-RXM-SPARTN message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_SPARTN_UART2 = 0x20910607,
	/** Output rate of the UBX-RXM-SVSI message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_SVSI_UART2 = 0x20910152,
	/** Output rate of the UBX-RXM-TM message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_RXM_TM_UART2 = 0x20910612,
	/** Output rate of the UBX-SEC-OSNMA message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_SEC_OSNMA_UART2 = 0x209106cc,
	/** Output rate of the UBX-SEC-SIGLOG message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_SEC_SIGLOG_UART2 = 0x2091068b,
	/** Output rate of the UBX-SEC-SIG message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_SEC_SIG_UART2 = 0x20910636,
	/** Output rate of the UBX-TIM-SVIN message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_TIM_SVIN_UART2 = 0x20910099,
	/** Output rate of the UBX-TIM-TM2 message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_TIM_TM2_UART2 = 0x2091017a,
	/** Output rate of the UBX-TIM-TP message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_TIM_TP_UART2 = 0x2091017f,
	/** Output rate of the UBX-TIM-VRFY message on the UART2 port */
	UBX_KEY_MSG_OUT_UBX_TIM_VRFY_UART2 = 0x20910094,
};

/** Message output configuration (SPI) */
enum ubx_key_msg_out_spi {
	/** Output rate of the NMEA-GX-DTM message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_DTM_SPI = 0x209100aa,
	/** Output rate of the NMEA-GX-GBS message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_GBS_SPI = 0x209100e1,
	/** Output rate of the NMEA-GX-GGA message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_GGA_SPI = 0x209100be,
	/** Output rate of the NMEA-GX-GLL message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_GLL_SPI = 0x209100cd,
	/** Output rate of the NMEA-GX-GNS message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_GNS_SPI = 0x209100b9,
	/** Output rate of the NMEA-GX-GRS message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_GRS_SPI = 0x209100d2,
	/** Output rate of the NMEA-GX-GSA message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_GSA_SPI = 0x209100c3,
	/** Output rate of the NMEA-GX-GST message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_GST_SPI = 0x209100d7,
	/** Output rate of the NMEA-GX-GSV message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_GSV_SPI = 0x209100c8,
	/** Output rate of the NMEA-GX-RLM message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_RLM_SPI = 0x20910404,
	/** Output rate of the NMEA-GX-RMC message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_RMC_SPI = 0x209100af,
	/** Output rate of the NMEA-GX-THS message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_THS_SPI = 0x209100e6,
	/** The output rate of the NMEA-GX-UTC message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_UTC_SPI = 0x209106d3,
	/** Output rate of the NMEA-GX-VLW message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_VLW_SPI = 0x209100eb,
	/** Output rate of the NMEA-GX-VTG message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_VTG_SPI = 0x209100b4,
	/** Output rate of the NMEA-GX-ZDA message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_ZDA_SPI = 0x209100dc,
	/** Output rate of the NMEA-NAV2-GX-GGA message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GGA_SPI = 0x20910665,
	/** Output rate of the NMEA-NAV2-GX-GLL message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GLL_SPI = 0x20910674,
	/** Output rate of the NMEA-NAV2-GX-GNS message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GNS_SPI = 0x20910660,
	/** Output rate of the NMEA-NAV2-GX-GSA message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GSA_SPI = 0x2091066a,
	/** Output rate of the NMEA-NAV2-GX-RMC message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_RMC_SPI = 0x20910656,
	/** Output rate of the NMEA-NAV2-GX-VTG message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_VTG_SPI = 0x2091065b,
	/** Output rate of the NMEA-NAV2-GX-ZDA message on the SPI port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_ZDA_SPI = 0x20910683,
	/** Output rate of the NMEA-GX-PUBX00 message on the SPI port */
	UBX_KEY_MSG_OUT_PUBX_POLYP_SPI = 0x209100f0,
	/** Output rate of the NMEA-GX-PUBX03 message on the SPI port */
	UBX_KEY_MSG_OUT_PUBX_POLYS_SPI = 0x209100f5,
	/** Output rate of the NMEA-GX-PUBX04 message on the SPI port */
	UBX_KEY_MSG_OUT_PUBX_POLYT_SPI = 0x209100fa,
	/** Output rate of the NMEA-GX-PUBX50 message on the SPI port */
	UBX_KEY_MSG_OUT_PUBX_POLYV_SPI = 0x209103f2,
	/** Output rate of the RTCM-3X-TYPE1005 message on the SPI port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1005_SPI = 0x209102c1,
	/** Output rate of the RTCM-3X-TYPE1006 message on the SPI port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1006_SPI = 0x209102c6,
	/** Output rate of the RTCM-3X-TYPE1074 message on the SPI port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1074_SPI = 0x20910362,
	/** Output rate of the RTCM-3X-TYPE1077 message on the SPI port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1077_SPI = 0x209102d0,
	/** Output rate of the RTCM-3X-TYPE1084 message on the SPI port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1084_SPI = 0x20910367,
	/** Output rate of the RTCM-3X-TYPE1087 message on the SPI port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1087_SPI = 0x209102d5,
	/** Output rate of the RTCM-3X-TYPE1094 message on the SPI port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1094_SPI = 0x2091036c,
	/** Output rate of the RTCM-3X-TYPE1097 message on the SPI port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1097_SPI = 0x2091031c,
	/** Output rate of the RTCM-3X-TYPE1124 message on the SPI port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1124_SPI = 0x20910371,
	/** Output rate of the RTCM-3X-TYPE1127 message on the SPI port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1127_SPI = 0x209102da,
	/** Output rate of the RTCM-3X-TYPE1230 message on the SPI port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1230_SPI = 0x20910307,
	/** Output rate of the RTCM-3X-TYPE4072_0 message on the SPI port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE4072_0_SPI = 0x20910302,
	/** Output rate of the RTCM-3X-TYPE4072_1 message on the SPI port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE4072_1_SPI = 0x20910385,
	/** Output rate of the UBX-AID-ALM message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_AID_ALM_SPI = 0x20910172,
	/** Output rate of the UBX-AID-AOP message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_AID_AOP_SPI = 0x20910271,
	/** Output rate of the UBX-AID-EPH message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_AID_EPH_SPI = 0x20910168,
	/** Output rate of the UBX-AID-INI message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_AID_INI_SPI = 0x209100ff,
	/** Output rate of the UBX-ESF-ALG message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_ESF_ALG_SPI = 0x20910113,
	/** Output rate of the UBX-ESF-CAL message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_ESF_CAL_SPI = 0x209106b0,
	/** Output rate of the UBX-ESF-INS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_ESF_INS_SPI = 0x20910118,
	/** Output rate of the UBX-ESF-MEAS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_ESF_MEAS_SPI = 0x2091027b,
	/** Output rate of the UBX-ESF-RAW message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_ESF_RAW_SPI = 0x209102a3,
	/** Output rate of the UBX-ESF-STATUS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_ESF_STATUS_SPI = 0x20910109,
	/** Output rate of the UBX-HNR-ATT message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_HNR_ATT_SPI = 0x2091037b,
	/** Output rate of the UBX-HNR-INS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_HNR_INS_SPI = 0x20910376,
	/** Output rate of the UBX-HNR-PVT message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_HNR_PVT_SPI = 0x2091028f,
	/** Output rate of the UBX-LOG-INFO message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_LOG_INFO_SPI = 0x2091025d,
	/** Output rate of the UBX-MON-COMMS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_MON_COMMS_SPI = 0x20910353,
	/** Output rate of the UBX-MON-HW2 message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_MON_HW2_SPI = 0x209101bd,
	/** Output rate of the UBX-MON-HW3 message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_MON_HW3_SPI = 0x20910358,
	/** Output rate of the UBX-MON-HW message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_MON_HW_SPI = 0x209101b8,
	/** Output rate of the UBX-MON-INST message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_MON_INST_SPI = 0x209103cf,
	/** Output rate of the UBX-MON-IO message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_MON_IO_SPI = 0x209101a9,
	/** Output rate of the UBX-MON-MSGPP message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_MON_MSGPP_SPI = 0x2091019a,
	/** Output rate of the UBX_MON_PMP message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_MON_PMP_SPI = 0x20910326,
	/** Output rate of the UBX-MON-RF message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_MON_RF_SPI = 0x2091035d,
	/** Output rate of the UBX-MON-RXBUF message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_MON_RXBUF_SPI = 0x209101a4,
	/** Output rate of the UBX-MON-RXR message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_MON_RXR_SPI = 0x2091018b,
	/** Output rate of the UBX-MON-SPAN message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_MON_SPAN_SPI = 0x2091038f,
	/** Output rate of the UBX-MON-SYS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_MON_SYS_SPI = 0x209106a1,
	/** Output rate of the UBX-MON-TXBUF message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_MON_TXBUF_SPI = 0x2091019f,
	/** Output rate of the UBX-NAV2-CLOCK message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_CLOCK_SPI = 0x20910434,
	/** Output rate of the UBX-NAV2-COV message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_COV_SPI = 0x20910439,
	/** Output rate of the UBX-NAV2-DOP message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_DOP_SPI = 0x20910469,
	/** Output rate of the UBX-NAV2-EELL message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_EELL_SPI = 0x20910474,
	/** Output rate of the UBX-NAV2-EOE message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_EOE_SPI = 0x20910569,
	/** Output rate of the UBX-NAV2-ODO message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_ODO_SPI = 0x20910479,
	/** Output rate of the UBX-NAV2-POSECEF message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_POSECEF_SPI = 0x20910484,
	/** Output rate of the UBX-NAV2-POSLLH message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_POSLLH_SPI = 0x20910489,
	/** Output rate of the UBX-NAV2-PVAT message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_PVAT_SPI = 0x20910633,
	/** Output rate of the UBX-NAV2-PVT message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_PVT_SPI = 0x20910494,
	/** Output rate of the UBX-NAV2-SAT message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SAT_SPI = 0x20910499,
	/** Output rate of the UBX-NAV2-SBAS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SBAS_SPI = 0x20910504,
	/** Output rate of the UBX-NAV2-SIG message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SIG_SPI = 0x20910509,
	/** Output rate of the UBX-NAV2-SLAS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SLAS_SPI = 0x20910514,
	/** Output rate of the UBX-NAV2-STATUS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_STATUS_SPI = 0x20910519,
	/** Output rate of the UBX-NAV2-SVIN message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SVIN_SPI = 0x20910524,
	/** Output rate of the UBX-NAV2-TIMEBDS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEBDS_SPI = 0x20910529,
	/** Output rate of the UBX-NAV2-TIMEGAL message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEGAL_SPI = 0x20910534,
	/** Output rate of the UBX-NAV2-TIMEGLO message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEGLO_SPI = 0x20910539,
	/** Output rate of the UBX-NAV2-TIMEGPS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEGPS_SPI = 0x20910544,
	/** Output rate of the UBX-NAV2-TIMELS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMELS_SPI = 0x20910549,
	/** Output rate of the UBX-NAV2-TIMENAVIC message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMENAVIC_SPI = 0x209106ab,
	/** Output rate of the UBX-NAV2-TIMEQZSS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEQZSS_SPI = 0x20910579,
	/** Output rate of the UBX-NAV2-TIMEUTC message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEUTC_SPI = 0x20910554,
	/** Output rate of the UBX-NAV2-VELECEF message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_VELECEF_SPI = 0x20910559,
	/** Output rate of the UBX-NAV2-VELNED message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV2_VELNED_SPI = 0x20910564,
	/** Output rate of the UBX-NAV-AOPSTATUS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_AOPSTATUS_SPI = 0x2091007d,
	/** Output rate of the UBX-NAV-ATT message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_ATT_SPI = 0x20910023,
	/** Output rate of the UBX_NAV_CFB message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_CFB_SPI = 0x209106d8,
	/** Output rate of the UBX-NAV-CLOCK message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_CLOCK_SPI = 0x20910069,
	/** Output rate of the UBX-NAV-COV message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_COV_SPI = 0x20910087,
	/** Output rate of the UBX-NAV-DAHEADING message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_DAHEADING_SPI = 0x209103e3,
	/** Output rate of the UBX-NAV-DGPS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_DGPS_SPI = 0x20910078,
	/** Output rate of the UBX-NAV-DOP message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_DOP_SPI = 0x2091003c,
	/** Output rate of the UBX-NAV-EELL message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_EELL_SPI = 0x20910317,
	/** Output rate of the UBX-NAV-EOE message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_EOE_SPI = 0x20910163,
	/** Output rate of the UBX-NAV-GEOFENCE message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_GEOFENCE_SPI = 0x209100a5,
	/** Output rate of the UBX-NAV-HPPOSECEF message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_HPPOSECEF_SPI = 0x20910032,
	/** Output rate of the UBX-NAV-HPPOSLLH message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_HPPOSLLH_SPI = 0x20910037,
	/** Output rate of the UBX-NAV-NMI message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_NMI_SPI = 0x20910594,
	/** Output rate of the UBX-NAV-ODO message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_ODO_SPI = 0x20910082,
	/** Output rate of the UBX-NAV-ORB message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_ORB_SPI = 0x20910014,
	/** Output rate of the UBX-NAV-PL message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_PL_SPI = 0x20910419,
	/** Output rate of the UBX-NAV-POSECEF message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_POSECEF_SPI = 0x20910028,
	/** Output rate of the UBX-NAV-POSLLH message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_POSLLH_SPI = 0x2091002d,
	/** Output rate of the UBX-NAV-PVAT message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_PVAT_SPI = 0x2091062e,
	/** Output rate of the UBX-NAV-PVT message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_PVT_SPI = 0x2091000a,
	/** Output rate of the UBX-NAV-RELPOSNED message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_RELPOSNED_SPI = 0x20910091,
	/** Output rate of the UBX-NAV-SAT message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_SAT_SPI = 0x20910019,
	/** Output rate of the UBX-NAV-SBAS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_SBAS_SPI = 0x2091006e,
	/** Output rate of the UBX-NAV-SIG message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_SIG_SPI = 0x20910349,
	/** Output rate of the UBX-NAV-SLAS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_SLAS_SPI = 0x2091033a,
	/** Output rate of the UBX-NAV-SOL message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_SOL_SPI = 0x20910005,
	/** Output rate of the UBX-NAV-STATUS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_STATUS_SPI = 0x2091001e,
	/** Output rate of the UBX-NAV-SVINFO message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_SVINFO_SPI = 0x2091000f,
	/** Output rate of the UBX-NAV-SVIN message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_SVIN_SPI = 0x2091008c,
	/** Output rate of the UBX-NAV-TIMEBDS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEBDS_SPI = 0x20910055,
	/** Output rate of the UBX-NAV-TIMEGAL message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEGAL_SPI = 0x2091005a,
	/** Output rate of the UBX-NAV-TIMEGLO message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEGLO_SPI = 0x20910050,
	/** Output rate of the UBX-NAV-TIMEGPS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEGPS_SPI = 0x2091004b,
	/** Output rate of the UBX-NAV-TIMELS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMELS_SPI = 0x20910064,
	/** Output rate of the UBX-NAV-TIMENAVIC message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMENAVIC_SPI = 0x209106a6,
	/** Output rate of the UBX-NAV-TIMEQZSS message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEQZSS_SPI = 0x2091038a,
	/** Output rate of the UBX-NAV-TIMETRUSTED message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMETRUSTED_SPI = 0x209103ac,
	/** Output rate of the UBX-NAV-TIMEUTC message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEUTC_SPI = 0x2091005f,
	/** Output rate of the UBX-NAV-VELECEF message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_VELECEF_SPI = 0x20910041,
	/** Output rate of the UBX-NAV-VELNED message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_NAV_VELNED_SPI = 0x20910046,
	/** Output rate of the UBX-RXM-ALM message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_ALM_SPI = 0x20910177,
	/** Output rate of the UBX-RXM-COR message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_COR_SPI = 0x209106ba,
	/** Output rate of the UBX-RXM-EPH message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_EPH_SPI = 0x2091016d,
	/** Output rate of the UBX-RXM-IMES message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_IMES_SPI = 0x2091015e,
	/** Output rate of the UBX-RXM-MEAS20 message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEAS20_SPI = 0x20910647,
	/** Output rate of the UBX-RXM-MEAS50 message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEAS50_SPI = 0x2091064c,
	/** Output rate of the UBX-RXM-MEASC12 message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASC12_SPI = 0x20910642,
	/** Output rate of the UBX-RXM-MEASD12 message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASD12_SPI = 0x2091063d,
	/** Output rate of the UBX-RXM-MEASX2 message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASX2_SPI = 0x209103c5,
	/** Output rate of the UBX-RXM-MEASX message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASX_SPI = 0x20910208,
	/** Output rate of the UBX_RXM_PMP message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_PMP_SPI = 0x20910321,
	/** output rate of the UBX-RXM-QZSSL6 message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_QZSSL6_SPI = 0x2091033e,
	/** Output rate of the UBX-RXM-RAWX message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_RAWX_SPI = 0x209102a8,
	/** Output rate of the UBX-RXM-RLM message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_RLM_SPI = 0x20910262,
	/** Output rate of the UBX-RXM-RTCM message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_RTCM_SPI = 0x2091026c,
	/** Output rate of the UBX-RXM-SFRBX message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_SFRBX_SPI = 0x20910235,
	/** Output rate of the UBX-RXM-SPARTN message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_SPARTN_SPI = 0x20910609,
	/** Output rate of the UBX-RXM-SVSI message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_SVSI_SPI = 0x20910154,
	/** Output rate of the UBX-RXM-TM message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_RXM_TM_SPI = 0x20910614,
	/** Output rate of the UBX-SEC-OSNMA message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_SEC_OSNMA_SPI = 0x209106ce,
	/** Output rate of the UBX-SEC-SIGLOG message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_SEC_SIGLOG_SPI = 0x2091068d,
	/** Output rate of the UBX-SEC-SIG message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_SEC_SIG_SPI = 0x20910638,
	/** Output rate of the UBX-TIM-SVIN message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_TIM_SVIN_SPI = 0x2091009b,
	/** Output rate of the UBX-TIM-TM2 message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_TIM_TM2_SPI = 0x2091017c,
	/** Output rate of the UBX-TIM-TP message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_TIM_TP_SPI = 0x20910181,
	/** Output rate of the UBX-TIM-VRFY message on the SPI port */
	UBX_KEY_MSG_OUT_UBX_TIM_VRFY_SPI = 0x20910096,
};

/** Message output configuration (USB) */
enum ubx_key_msg_out_usb {
	/** Output rate of the NMEA-GX-DTM message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_DTM_USB = 0x209100a9,
	/** Output rate of the NMEA-GX-GBS message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_GBS_USB = 0x209100e0,
	/** Output rate of the NMEA-GX-GGA message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_GGA_USB = 0x209100bd,
	/** Output rate of the NMEA-GX-GLL message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_GLL_USB = 0x209100cc,
	/** Output rate of the NMEA-GX-GNS message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_GNS_USB = 0x209100b8,
	/** Output rate of the NMEA-GX-GRS message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_GRS_USB = 0x209100d1,
	/** Output rate of the NMEA-GX-GSA message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_GSA_USB = 0x209100c2,
	/** Output rate of the NMEA-GX-GST message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_GST_USB = 0x209100d6,
	/** Output rate of the NMEA-GX-GSV message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_GSV_USB = 0x209100c7,
	/** Output rate of the NMEA-GX-RLM message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_RLM_USB = 0x20910403,
	/** Output rate of the NMEA-GX-RMC message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_RMC_USB = 0x209100ae,
	/** Output rate of the NMEA-GX-THS message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_THS_USB = 0x209100e5,
	/** The output rate of the NMEA-GX-UTC message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_UTC_USB = 0x209106d2,
	/** Output rate of the NMEA-GX-VLW message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_VLW_USB = 0x209100ea,
	/** Output rate of the NMEA-GX-VTG message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_VTG_USB = 0x209100b3,
	/** Output rate of the NMEA-GX-ZDA message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_ZDA_USB = 0x209100db,
	/** Output rate of the NMEA-NAV2-GX-GGA message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GGA_USB = 0x20910664,
	/** Output rate of the NMEA-NAV2-GX-GLL message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GLL_USB = 0x20910673,
	/** Output rate of the NMEA-NAV2-GX-GNS message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GNS_USB = 0x2091065f,
	/** Output rate of the NMEA-NAV2-GX-GSA message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_GSA_USB = 0x20910669,
	/** Output rate of the NMEA-NAV2-GX-RMC message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_RMC_USB = 0x20910655,
	/** Output rate of the NMEA-NAV2-GX-VTG message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_VTG_USB = 0x2091065a,
	/** Output rate of the NMEA-NAV2-GX-ZDA message on the USB port */
	UBX_KEY_MSG_OUT_NMEA_NAV2_ZDA_USB = 0x20910682,
	/** Output rate of the NMEA-GX-PUBX00 message on the USB port */
	UBX_KEY_MSG_OUT_PUBX_POLYP_USB = 0x209100ef,
	/** Output rate of the NMEA-GX-PUBX03 message on the USB port */
	UBX_KEY_MSG_OUT_PUBX_POLYS_USB = 0x209100f4,
	/** Output rate of the NMEA-GX-PUBX04 message on the USB port */
	UBX_KEY_MSG_OUT_PUBX_POLYT_USB = 0x209100f9,
	/** Output rate of the NMEA-GX-PUBX50 message on the USB port */
	UBX_KEY_MSG_OUT_PUBX_POLYV_USB = 0x209103f1,
	/** Output rate of the RTCM-3X-TYPE1005 message on the USB port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1005_USB = 0x209102c0,
	/** Output rate of the RTCM-3X-TYPE1006 message on the USB port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1006_USB = 0x209102c5,
	/** Output rate of the RTCM-3X-TYPE1074 message on the USB port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1074_USB = 0x20910361,
	/** Output rate of the RTCM-3X-TYPE1077 message on the USB port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1077_USB = 0x209102cf,
	/** Output rate of the RTCM-3X-TYPE1084 message on the USB port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1084_USB = 0x20910366,
	/** Output rate of the RTCM-3X-TYPE1087 message on the USB port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1087_USB = 0x209102d4,
	/** Output rate of the RTCM-3X-TYPE1094 message on the USB port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1094_USB = 0x2091036b,
	/** Output rate of the RTCM-3X-TYPE1097 message on the USB port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1097_USB = 0x2091031b,
	/** Output rate of the RTCM-3X-TYPE1124 message on the USB port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1124_USB = 0x20910370,
	/** Output rate of the RTCM-3X-TYPE1127 message on the USB port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1127_USB = 0x209102d9,
	/** Output rate of the RTCM-3X-TYPE1230 message on the USB port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE1230_USB = 0x20910306,
	/** Output rate of the RTCM-3X-TYPE4072_0 message on the USB port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE4072_0_USB = 0x20910301,
	/** Output rate of the RTCM-3X-TYPE4072_1 message on the USB port */
	UBX_KEY_MSG_OUT_RTCM_3X_TYPE4072_1_USB = 0x20910384,
	/** Output rate of the UBX-AID-ALM message on the USB port */
	UBX_KEY_MSG_OUT_UBX_AID_ALM_USB = 0x20910171,
	/** Output rate of the UBX-AID-AOP message on the USB port */
	UBX_KEY_MSG_OUT_UBX_AID_AOP_USB = 0x20910270,
	/** Output rate of the UBX-AID-EPH message on the USB port */
	UBX_KEY_MSG_OUT_UBX_AID_EPH_USB = 0x20910167,
	/** Output rate of the UBX-AID-INI message on the USB port */
	UBX_KEY_MSG_OUT_UBX_AID_INI_USB = 0x209100fe,
	/** Output rate of the UBX-ESF-ALG message on the USB port */
	UBX_KEY_MSG_OUT_UBX_ESF_ALG_USB = 0x20910112,
	/** Output rate of the UBX-ESF-CAL message on the USB port */
	UBX_KEY_MSG_OUT_UBX_ESF_CAL_USB = 0x209106af,
	/** Output rate of the UBX-ESF-INS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_ESF_INS_USB = 0x20910117,
	/** Output rate of the UBX-ESF-MEAS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_ESF_MEAS_USB = 0x2091027a,
	/** Output rate of the UBX-ESF-RAW message on the USB port */
	UBX_KEY_MSG_OUT_UBX_ESF_RAW_USB = 0x209102a2,
	/** Output rate of the UBX-ESF-STATUS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_ESF_STATUS_USB = 0x20910108,
	/** Output rate of the UBX-HNR-ATT message on the USB port */
	UBX_KEY_MSG_OUT_UBX_HNR_ATT_USB = 0x2091037a,
	/** Output rate of the UBX-HNR-INS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_HNR_INS_USB = 0x20910375,
	/** Output rate of the UBX-HNR-PVT message on the USB port */
	UBX_KEY_MSG_OUT_UBX_HNR_PVT_USB = 0x2091028e,
	/** Output rate of the UBX-LOG-INFO message on the USB port */
	UBX_KEY_MSG_OUT_UBX_LOG_INFO_USB = 0x2091025c,
	/** Output rate of the UBX-MON-COMMS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_MON_COMMS_USB = 0x20910352,
	/** Output rate of the UBX-MON-HW2 message on the USB port */
	UBX_KEY_MSG_OUT_UBX_MON_HW2_USB = 0x209101bc,
	/** Output rate of the UBX-MON-HW3 message on the USB port */
	UBX_KEY_MSG_OUT_UBX_MON_HW3_USB = 0x20910357,
	/** Output rate of the UBX-MON-HW message on the USB port */
	UBX_KEY_MSG_OUT_UBX_MON_HW_USB = 0x209101b7,
	/** Output rate of the UBX-MON-INST message on the USB port */
	UBX_KEY_MSG_OUT_UBX_MON_INST_USB = 0x209103ce,
	/** Output rate of the UBX-MON-IO message on the USB port */
	UBX_KEY_MSG_OUT_UBX_MON_IO_USB = 0x209101a8,
	/** Output rate of the UBX-MON-MSGPP message on the USB port */
	UBX_KEY_MSG_OUT_UBX_MON_MSGPP_USB = 0x20910199,
	/** Output rate of the UBX_MON_PMP message on the USB port */
	UBX_KEY_MSG_OUT_UBX_MON_PMP_USB = 0x20910325,
	/** Output rate of the UBX-MON-RF message on the USB port */
	UBX_KEY_MSG_OUT_UBX_MON_RF_USB = 0x2091035c,
	/** Output rate of the UBX-MON-RXBUF message on the USB port */
	UBX_KEY_MSG_OUT_UBX_MON_RXBUF_USB = 0x209101a3,
	/** Output rate of the UBX-MON-RXR message on the USB port */
	UBX_KEY_MSG_OUT_UBX_MON_RXR_USB = 0x2091018a,
	/** Output rate of the UBX-MON-SPAN message on the USB port */
	UBX_KEY_MSG_OUT_UBX_MON_SPAN_USB = 0x2091038e,
	/** Output rate of the UBX-MON-SYS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_MON_SYS_USB = 0x209106a0,
	/** Output rate of the UBX-MON-TXBUF message on the USB port */
	UBX_KEY_MSG_OUT_UBX_MON_TXBUF_USB = 0x2091019e,
	/** Output rate of the UBX-NAV2-CLOCK message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_CLOCK_USB = 0x20910433,
	/** Output rate of the UBX-NAV2-COV message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_COV_USB = 0x20910438,
	/** Output rate of the UBX-NAV2-DOP message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_DOP_USB = 0x20910468,
	/** Output rate of the UBX-NAV2-EELL message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_EELL_USB = 0x20910473,
	/** Output rate of the UBX-NAV2-EOE message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_EOE_USB = 0x20910568,
	/** Output rate of the UBX-NAV2-ODO message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_ODO_USB = 0x20910478,
	/** Output rate of the UBX-NAV2-POSECEF message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_POSECEF_USB = 0x20910483,
	/** Output rate of the UBX-NAV2-POSLLH message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_POSLLH_USB = 0x20910488,
	/** Output rate of the UBX-NAV2-PVAT message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_PVAT_USB = 0x20910632,
	/** Output rate of the UBX-NAV2-PVT message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_PVT_USB = 0x20910493,
	/** Output rate of the UBX-NAV2-SAT message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SAT_USB = 0x20910498,
	/** Output rate of the UBX-NAV2-SBAS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SBAS_USB = 0x20910503,
	/** Output rate of the UBX-NAV2-SIG message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SIG_USB = 0x20910508,
	/** Output rate of the UBX-NAV2-SLAS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SLAS_USB = 0x20910513,
	/** Output rate of the UBX-NAV2-STATUS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_STATUS_USB = 0x20910518,
	/** Output rate of the UBX-NAV2-SVIN message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_SVIN_USB = 0x20910523,
	/** Output rate of the UBX-NAV2-TIMEBDS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEBDS_USB = 0x20910528,
	/** Output rate of the UBX-NAV2-TIMEGAL message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEGAL_USB = 0x20910533,
	/** Output rate of the UBX-NAV2-TIMEGLO message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEGLO_USB = 0x20910538,
	/** Output rate of the UBX-NAV2-TIMEGPS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEGPS_USB = 0x20910543,
	/** Output rate of the UBX-NAV2-TIMELS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMELS_USB = 0x20910548,
	/** Output rate of the UBX-NAV2-TIMENAVIC message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMENAVIC_USB = 0x209106aa,
	/** Output rate of the UBX-NAV2-TIMEQZSS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEQZSS_USB = 0x20910578,
	/** Output rate of the UBX-NAV2-TIMEUTC message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_TIMEUTC_USB = 0x20910553,
	/** Output rate of the UBX-NAV2-VELECEF message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_VELECEF_USB = 0x20910558,
	/** Output rate of the UBX-NAV2-VELNED message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV2_VELNED_USB = 0x20910563,
	/** Output rate of the UBX-NAV-AOPSTATUS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_AOPSTATUS_USB = 0x2091007c,
	/** Output rate of the UBX-NAV-ATT message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_ATT_USB = 0x20910022,
	/** Output rate of the UBX_NAV_CFB message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_CFB_USB = 0x209106d7,
	/** Output rate of the UBX-NAV-CLOCK message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_CLOCK_USB = 0x20910068,
	/** Output rate of the UBX-NAV-COV message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_COV_USB = 0x20910086,
	/** Output rate of the UBX-NAV-DAHEADING message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_DAHEADING_USB = 0x209103e2,
	/** Output rate of the UBX-NAV-DGPS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_DGPS_USB = 0x20910077,
	/** Output rate of the UBX-NAV-DOP message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_DOP_USB = 0x2091003b,
	/** Output rate of the UBX-NAV-EELL message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_EELL_USB = 0x20910316,
	/** Output rate of the UBX-NAV-EOE message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_EOE_USB = 0x20910162,
	/** Output rate of the UBX-NAV-GEOFENCE message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_GEOFENCE_USB = 0x209100a4,
	/** Output rate of the UBX-NAV-HPPOSECEF message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_HPPOSECEF_USB = 0x20910031,
	/** Output rate of the UBX-NAV-HPPOSLLH message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_HPPOSLLH_USB = 0x20910036,
	/** Output rate of the UBX-NAV-NMI message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_NMI_USB = 0x20910593,
	/** Output rate of the UBX-NAV-ODO message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_ODO_USB = 0x20910081,
	/** Output rate of the UBX-NAV-ORB message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_ORB_USB = 0x20910013,
	/** Output rate of the UBX-NAV-PL message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_PL_USB = 0x20910418,
	/** Output rate of the UBX-NAV-POSECEF message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_POSECEF_USB = 0x20910027,
	/** Output rate of the UBX-NAV-POSLLH message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_POSLLH_USB = 0x2091002c,
	/** Output rate of the UBX-NAV-PVAT message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_PVAT_USB = 0x2091062d,
	/** Output rate of the UBX-NAV-PVT message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_PVT_USB = 0x20910009,
	/** Output rate of the UBX-NAV-RELPOSNED message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_RELPOSNED_USB = 0x20910090,
	/** Output rate of the UBX-NAV-SAT message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_SAT_USB = 0x20910018,
	/** Output rate of the UBX-NAV-SBAS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_SBAS_USB = 0x2091006d,
	/** Output rate of the UBX-NAV-SIG message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_SIG_USB = 0x20910348,
	/** Output rate of the UBX-NAV-SLAS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_SLAS_USB = 0x20910339,
	/** Output rate of the UBX-NAV-SOL message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_SOL_USB = 0x20910004,
	/** Output rate of the UBX-NAV-STATUS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_STATUS_USB = 0x2091001d,
	/** Output rate of the UBX-NAV-SVINFO message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_SVINFO_USB = 0x2091000e,
	/** Output rate of the UBX-NAV-SVIN message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_SVIN_USB = 0x2091008b,
	/** Output rate of the UBX-NAV-TIMEBDS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEBDS_USB = 0x20910054,
	/** Output rate of the UBX-NAV-TIMEGAL message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEGAL_USB = 0x20910059,
	/** Output rate of the UBX-NAV-TIMEGLO message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEGLO_USB = 0x2091004f,
	/** Output rate of the UBX-NAV-TIMEGPS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEGPS_USB = 0x2091004a,
	/** Output rate of the UBX-NAV-TIMELS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMELS_USB = 0x20910063,
	/** Output rate of the UBX-NAV-TIMENAVIC message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMENAVIC_USB = 0x209106a5,
	/** Output rate of the UBX-NAV-TIMEQZSS message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEQZSS_USB = 0x20910389,
	/** Output rate of the UBX-NAV-TIMETRUSTED message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMETRUSTED_USB = 0x209103ab,
	/** Output rate of the UBX-NAV-TIMEUTC message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_TIMEUTC_USB = 0x2091005e,
	/** Output rate of the UBX-NAV-VELECEF message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_VELECEF_USB = 0x20910040,
	/** Output rate of the UBX-NAV-VELNED message on the USB port */
	UBX_KEY_MSG_OUT_UBX_NAV_VELNED_USB = 0x20910045,
	/** Output rate of the UBX-RXM-ALM message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_ALM_USB = 0x20910176,
	/** Output rate of the UBX-RXM-COR message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_COR_USB = 0x209106b9,
	/** Output rate of the UBX-RXM-EPH message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_EPH_USB = 0x2091016c,
	/** Output rate of the UBX-RXM-IMES message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_IMES_USB = 0x2091015d,
	/** Output rate of the UBX-RXM-MEAS20 message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEAS20_USB = 0x20910646,
	/** Output rate of the UBX-RXM-MEAS50 message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEAS50_USB = 0x2091064b,
	/** Output rate of the UBX-RXM-MEASC12 message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASC12_USB = 0x20910641,
	/** Output rate of the UBX-RXM-MEASD12 message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASD12_USB = 0x2091063c,
	/** Output rate of the UBX-RXM-MEASX2 message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASX2_USB = 0x209103c4,
	/** Output rate of the UBX-RXM-MEASX message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_MEASX_USB = 0x20910207,
	/** Output rate of the UBX_RXM_PMP message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_PMP_USB = 0x20910320,
	/** output rate of the UBX-RXM-QZSSL6 message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_QZSSL6_USB = 0x2091033d,
	/** Output rate of the UBX-RXM-RAWX message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_RAWX_USB = 0x209102a7,
	/** Output rate of the UBX-RXM-RLM message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_RLM_USB = 0x20910261,
	/** Output rate of the UBX-RXM-RTCM message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_RTCM_USB = 0x2091026b,
	/** Output rate of the UBX-RXM-SFRBX message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_SFRBX_USB = 0x20910234,
	/** Output rate of the UBX-RXM-SPARTN message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_SPARTN_USB = 0x20910608,
	/** Output rate of the UBX-RXM-SVSI message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_SVSI_USB = 0x20910153,
	/** Output rate of the UBX-RXM-TM message on the USB port */
	UBX_KEY_MSG_OUT_UBX_RXM_TM_USB = 0x20910613,
	/** Output rate of the UBX-SEC-OSNMA message on the USB port */
	UBX_KEY_MSG_OUT_UBX_SEC_OSNMA_USB = 0x209106cd,
	/** Output rate of the UBX-SEC-SIGLOG message on the USB port */
	UBX_KEY_MSG_OUT_UBX_SEC_SIGLOG_USB = 0x2091068c,
	/** Output rate of the UBX-SEC-SIG message on the USB port */
	UBX_KEY_MSG_OUT_UBX_SEC_SIG_USB = 0x20910637,
	/** Output rate of the UBX-TIM-SVIN message on the USB port */
	UBX_KEY_MSG_OUT_UBX_TIM_SVIN_USB = 0x2091009a,
	/** Output rate of the UBX-TIM-TM2 message on the USB port */
	UBX_KEY_MSG_OUT_UBX_TIM_TM2_USB = 0x2091017b,
	/** Output rate of the UBX-TIM-TP message on the USB port */
	UBX_KEY_MSG_OUT_UBX_TIM_TP_USB = 0x20910180,
	/** Output rate of the UBX-TIM-VRFY message on the USB port */
	UBX_KEY_MSG_OUT_UBX_TIM_VRFY_USB = 0x20910095,
};

/** Navigation and measurement rate configuration */
enum ubx_key_rate {
	/** Nominal time between GNSS measurements */
	UBX_KEY_RATE_MEAS = 0x30210001,
	/** Ratio of number of measurements to number of navigation solutions */
	UBX_KEY_RATE_NAV = 0x30210002,
	/** Time system to which measurements are aligned */
	UBX_KEY_RATE_TIMEREF = 0x20210003,
	/** Output rate of priority navigation mode messages */
	UBX_KEY_RATE_NAV_PRIO = 0x20210004,
};

/** Standard precision navigation configuration */
enum ubx_key_nav_cfg {
	/** Position fix mode */
	UBX_KEY_NAV_CFG_FIX_MODE = 0x20110011,
	/** Dynamic platform model */
	UBX_KEY_NAV_CFG_DYN_MODEL = 0x20110021,
};

/** UART1 protocol configuration */
enum ubx_key_uart1_proto {
	/** Flag to indicate if UBX should be an input protocol on UART1 */
	UBX_KEY_UART1_PROTO_IN_UBX = 0x10730001,
	/** Flag to indicate if NMEA should be an input protocol on UART1 */
	UBX_KEY_UART1_PROTO_IN_NMEA = 0x10730002,
	/** Flag to indicate if RTCM2X should be an input protocol on UART1 */
	UBX_KEY_UART1_PROTO_IN_RTCM2X = 0x10730003,
	/** Flag to indicate if RTCM3X should be an input protocol on UART1 */
	UBX_KEY_UART1_PROTO_IN_RTCM3X = 0x10730004,
	/** Flag to indicate if SPARTN should be an input protocol on UART1 */
	UBX_KEY_UART1_PROTO_IN_SPARTN = 0x10730005,
	/** Flag to indicate if UBX should be an output protocol on UART1 */
	UBX_KEY_UART1_PROTO_OUT_UBX = 0x10740001,
	/** Flag to indicate if NMEA should be an output protocol on UART1 */
	UBX_KEY_UART1_PROTO_OUT_NMEA = 0x10740002,
	/** Flag to indicate if RTCM3X should be an output protocol on UART1 */
	UBX_KEY_UART1_PROTO_OUT_RTCM3X = 0x10740004,
};

/** I2C protocol configuration */
enum ubx_key_uart1_proto_i2c {
	/** Flag to indicate if UBX should be an input protocol on I2C */
	UBX_KEY_I2C_PROTO_IN_UBX = 0x10710001,
	/** Flag to indicate if NMEA should be an input protocol on I2C */
	UBX_KEY_I2C_PROTO_IN_NMEA = 0x10710002,
	/** Flag to indicate if RTCM2X should be an input protocol on I2C */
	UBX_KEY_I2C_PROTO_IN_RTCM2X = 0x10710003,
	/** Flag to indicate if RTCM3X should be an input protocol on I2C */
	UBX_KEY_I2C_PROTO_IN_RTCM3X = 0x10710004,
	/** Flag to indicate if SPARTN should be an input protocol on I2C */
	UBX_KEY_I2C_PROTO_IN_SPARTN = 0x10710005,
	/** Flag to indicate if UBX should be an output protocol on I2C */
	UBX_KEY_I2C_PROTO_OUT_UBX = 0x10720001,
	/** Flag to indicate if NMEA should be an output protocol on I2C */
	UBX_KEY_I2C_PROTO_OUT_NMEA = 0x10720002,
	/** Flag to indicate if RTCM3X should be an output protocol on I2C */
	UBX_KEY_I2C_PROTO_OUT_RTCM3X = 0x10720004,
};

/** UART2 protocol configuration */
enum ubx_key_uart2_proto {
	/** Flag to indicate if UBX should be an input protocol on UART2 */
	UBX_KEY_UART2_PROTO_IN_UBX = 0x10750001,
	/** Flag to indicate if NMEA should be an input protocol on UART2 */
	UBX_KEY_UART2_PROTO_IN_NMEA = 0x10750002,
	/** Flag to indicate if RTCM2X should be an input protocol on UART2 */
	UBX_KEY_UART2_PROTO_IN_RTCM2X = 0x10750003,
	/** Flag to indicate if RTCM3X should be an input protocol on UART2 */
	UBX_KEY_UART2_PROTO_IN_RTCM3X = 0x10750004,
	/** Flag to indicate if SPARTN should be an input protocol on UART2 */
	UBX_KEY_UART2_PROTO_IN_SPARTN = 0x10750005,
	/** Flag to indicate if UBX should be an output protocol on UART2 */
	UBX_KEY_UART2_PROTO_OUT_UBX = 0x10760001,
	/** Flag to indicate if NMEA should be an output protocol on UART2 */
	UBX_KEY_UART2_PROTO_OUT_NMEA = 0x10760002,
	/** Flag to indicate if RTCM3X should be an output protocol on UART2 */
	UBX_KEY_UART2_PROTO_OUT_RTCM3X = 0x10760004,
};

/** SPI protocol configuration */
enum ubx_key_spi_proto {
	/** Flag to indicate if UBX should be an input protocol on SPI */
	UBX_KEY_SPI_PROTO_IN_UBX = 0x10790001,
	/** Flag to indicate if NMEA should be an input protocol on SPI */
	UBX_KEY_SPI_PROTO_IN_NMEA = 0x10790002,
	/** Flag to indicate if RTCM2X should be an input protocol on SPI */
	UBX_KEY_SPI_PROTO_IN_RTCM2X = 0x10790003,
	/** Flag to indicate if RTCM3X should be an input protocol on SPI */
	UBX_KEY_SPI_PROTO_IN_RTCM3X = 0x10790004,
	/** Flag to indicate if SPARTN should be an input protocol on SPI */
	UBX_KEY_SPI_PROTO_IN_SPARTN = 0x10790005,
	/** Flag to indicate if UBX should be an output protocol on SPI */
	UBX_KEY_SPI_PROTO_OUT_UBX = 0x107a0001,
	/** Flag to indicate if NMEA should be an output protocol on SPI */
	UBX_KEY_SPI_PROTO_OUT_NMEA = 0x107a0002,
	/** Flag to indicate if RTCM3X should be an output protocol on SPI */
	UBX_KEY_SPI_PROTO_OUT_RTCM3X = 0x107a0004,
};

/** High precision navigation configuration */
enum ubx_key_nav_hp_cfg {
	/** Differential corrections mode */
	UBX_KEY_NAV_HP_CFG_GNSS_MODE = 0x20140011,
};

/** Standard precision navigation configuration */
enum ubx_key_navspg {
	/** Initial fix must be a 3D fix */
	UBX_KEY_NAVSPG_INIFIX3D = 0x10110013,
	/** GPS week rollover number */
	UBX_KEY_NAVSPG_WKNROLLOVER = 0x30110017,
	/** Use precise point positioning (PPP) */
	UBX_KEY_NAVSPG_USE_PPP = 0x10110019,
	/** UTC standard to be used */
	UBX_KEY_NAVSPG_UTCSTANDARD = 0x2011001c,
	/** Acknowledge assistance input messages */
	UBX_KEY_NAVSPG_ACKAIDING = 0x10110025,
	/** Use user geodetic datum parameters */
	UBX_KEY_NAVSPG_USE_USRDAT = 0x10110061,
	/** Geodetic datum semi-major axis */
	UBX_KEY_NAVSPG_USRDAT_MAJA = 0x50110062,
	/** Geodetic datum 1.0 / flattening */
	UBX_KEY_NAVSPG_USRDAT_FLAT = 0x50110063,
	/** Geodetic datum X axis shift at the origin */
	UBX_KEY_NAVSPG_USRDAT_DX = 0x40110064,
	/** Geodetic datum Y axis shift at the origin */
	UBX_KEY_NAVSPG_USRDAT_DY = 0x40110065,
	/** Geodetic datum Z axis shift at the origin */
	UBX_KEY_NAVSPG_USRDAT_DZ = 0x40110066,
	/** Geodetic datum rotation about the X axis */
	UBX_KEY_NAVSPG_USRDAT_ROTX = 0x40110067,
	/** Geodetic datum rotation about the Y axis () */
	UBX_KEY_NAVSPG_USRDAT_ROTY = 0x40110068,
	/** Geodetic datum rotation about the Z axis */
	UBX_KEY_NAVSPG_USRDAT_ROTZ = 0x40110069,
	/** Geodetic datum scale factor */
	UBX_KEY_NAVSPG_USRDAT_SCALE = 0x4011006a,
	/** Minimum number of satellites for navigation */
	UBX_KEY_NAVSPG_INFIL_MINSVS = 0x201100a1,
	/** Maximum number of satellites for navigation */
	UBX_KEY_NAVSPG_INFIL_MAXSVS = 0x201100a2,
	/** Minimum satellite signal level for navigation */
	UBX_KEY_NAVSPG_INFIL_MINCNO = 0x201100a3,
	/** Minimum elevation for a GNSS satellite to be used in navigation */
	UBX_KEY_NAVSPG_INFIL_MINELEV = 0x201100a4,
	/** Number of satellites required to have C/N0 above threshold for a fix to be attempted */
	UBX_KEY_NAVSPG_INFIL_NCNOTHRS = 0x201100aa,
	/** C/N0 threshold for deciding whether to attempt a fix */
	UBX_KEY_NAVSPG_INFIL_CNOTHRS = 0x201100ab,
	/** Output filter position DOP mask (threshold) */
	UBX_KEY_NAVSPG_OUTFIL_PDOP = 0x301100b1,
	/** Output filter time DOP mask (threshold) */
	UBX_KEY_NAVSPG_OUTFIL_TDOP = 0x301100b2,
	/** Output filter position accuracy mask (threshold) */
	UBX_KEY_NAVSPG_OUTFIL_PACC = 0x301100b3,
	/** Output filter time accuracy mask (threshold) */
	UBX_KEY_NAVSPG_OUTFIL_TACC = 0x301100b4,
	/** Output filter frequency accuracy mask (threshold) */
	UBX_KEY_NAVSPG_OUTFIL_FACC = 0x301100b5,
	/** Fixed altitude (mean sea level) for 2D fix mode */
	UBX_KEY_NAVSPG_CONSTR_ALT = 0x401100c1,
	/** Fixed altitude variance for 2D mode */
	UBX_KEY_NAVSPG_CONSTR_ALTVAR = 0x401100c2,
	/** Limit for propagating the state after NOFIX */
	UBX_KEY_NAVSPG_CONSTR_PROPLIMIT = 0x401100c3,
	/** DGNSS timeout. Maximum value is 255. */
	UBX_KEY_NAVSPG_CONSTR_DGNSSTO = 0x201100c4,
	/** DGNSS timeout value scale for CFG-NAVSPG-CONSTR_DGNSSTO */
	UBX_KEY_NAVSPG_CONSTR_DGNSSTO_SCALE = 0x201100c5,
	/** Permanently attenuated signal compensation mode */
	UBX_KEY_NAVSPG_SIGATTCOMP = 0x201100d6,
	/** Enable Protection level */
	UBX_KEY_NAVSPG_PL_ENA = 0x101100d7,
	/** Enable using only signals with authenticated navigation data */
	UBX_KEY_NAVSPG_ONLY_AUTHDATA = 0x101100dd,
	/** Maximum trusted time accuracy */
	UBX_KEY_NAVSPG_MAX_TIMETRUSTED_ACC = 0x301100de,
	/** Offset between the dual-antenna baseline heading and vehicle forward axis */
	UBX_KEY_NAVSPG_DAHEADING_OFFSET = 0x401100e4,
};

/** Satellite systems (GNSS) signal configuration */
enum ubx_key_signal {
	/** GPS enable */
	UBX_KEY_SIGNAL_GPS_ENA = 0x1031001f,
	/** GPS L1C/A */
	UBX_KEY_SIGNAL_GPS_L1CA_ENA = 0x10310001,
	/** GPS L2C */
	UBX_KEY_SIGNAL_GPS_L2C_ENA = 0x10310003,
	/** GPS L5 */
	UBX_KEY_SIGNAL_GPS_L5_ENA = 0x10310004,
	/** SBAS enable */
	UBX_KEY_SIGNAL_SBAS_ENA = 0x10310020,
	/** SBAS L1C/A */
	UBX_KEY_SIGNAL_SBAS_L1CA_ENA = 0x10310005,
	/** Galileo enable */
	UBX_KEY_SIGNAL_GAL_ENA = 0x10310021,
	/** Galileo E1 */
	UBX_KEY_SIGNAL_GAL_E1_ENA = 0x10310007,
	/** Galileo E5a */
	UBX_KEY_SIGNAL_GAL_E5A_ENA = 0x10310009,
	/** Galileo E5b */
	UBX_KEY_SIGNAL_GAL_E5B_ENA = 0x1031000a,
	/** Galileo E6 */
	UBX_KEY_SIGNAL_GAL_E6_ENA = 0x1031000b,
	/** BeiDou Enable */
	UBX_KEY_SIGNAL_BDS_ENA = 0x10310022,
	/** BeiDou B1I */
	UBX_KEY_SIGNAL_BDS_B1_ENA = 0x1031000d,
	/** BeiDou B1C */
	UBX_KEY_SIGNAL_BDS_B1C_ENA = 0x1031000f,
	/** BeiDou B2I */
	UBX_KEY_SIGNAL_BDS_B2_ENA = 0x1031000e,
	/** BeiDou B2a */
	UBX_KEY_SIGNAL_BDS_B2A_ENA = 0x10310028,
	/** BeiDou B3I */
	UBX_KEY_SIGNAL_BDS_B3_ENA = 0x10310010,
	/** QZSS enable */
	UBX_KEY_SIGNAL_QZSS_ENA = 0x10310024,
	/** QZSS L1C/A */
	UBX_KEY_SIGNAL_QZSS_L1CA_ENA = 0x10310012,
	/** QZSS L1C/B */
	UBX_KEY_SIGNAL_QZSS_L1CB_ENA = 0x10310039,
	/** QZSS L1S */
	UBX_KEY_SIGNAL_QZSS_L1S_ENA = 0x10310014,
	/** QZSS L2C */
	UBX_KEY_SIGNAL_QZSS_L2C_ENA = 0x10310015,
	/** QZSS L5 */
	UBX_KEY_SIGNAL_QZSS_L5_ENA = 0x10310017,
	/** GLONASS enable */
	UBX_KEY_SIGNAL_GLO_ENA = 0x10310025,
	/** GLONASS L1 */
	UBX_KEY_SIGNAL_GLO_L1_ENA = 0x10310018,
	/** GLONASS L2 */
	UBX_KEY_SIGNAL_GLO_L2_ENA = 0x1031001a,
	/** NavIC enable */
	UBX_KEY_SIGNAL_NAVIC_ENA = 0x10310026,
	/** NavIC L5 */
	UBX_KEY_SIGNAL_NAVIC_L5_ENA = 0x1031001d,
	/** L-Band enable */
	UBX_KEY_SIGNAL_LBAND_ENA = 0x1031002a,
	/** L-Band PMP */
	UBX_KEY_SIGNAL_LBAND_PMP_ENA = 0x1031002f,
	/** Active signal plan */
	UBX_KEY_SIGNAL_PLAN = 0x2031003a,
};

/** AssistNow Autonomous and Offline configuration */
enum ubx_key_ana {
	/** Use AssistNow Autonomous */
	UBX_KEY_ANA_USE_ANA = 0x10230001,
	/** Maximum acceptable (modeled) orbit error */
	UBX_KEY_ANA_ORBMAXERR = 0x30230002,
};

/** Batched output configuration */
enum ubx_key_batch {
	/** Enable data batching */
	UBX_KEY_BATCH_ENABLE = 0x10260013,
	/** Enable PIO notification */
	UBX_KEY_BATCH_PIOENABLE = 0x10260014,
	/** Maximum entries in buffer */
	UBX_KEY_BATCH_MAXENTRIES = 0x30260015,
	/** Buffer fill level warning threshold */
	UBX_KEY_BATCH_WARNTHRS = 0x30260016,
	/** PIO is active low */
	UBX_KEY_BATCH_PIOACTIVELOW = 0x10260018,
	/** PIO ID for buffer level notification */
	UBX_KEY_BATCH_PIOID = 0x20260019,
	/** Include extra PVT data */
	UBX_KEY_BATCH_EXTRAPVT = 0x1026001a,
	/** Include odometer data */
	UBX_KEY_BATCH_EXTRAODO = 0x1026001b,
};

/** BeiDou system configuration */
enum ubx_key_bds {
	/** Enable only the given BDS D1/D2 navigation data streams, ignoring the others */
	UBX_KEY_BDS_D1D2_NAVDATA = 0x20340009,
	/** Use BeiDou geostationary satellites (PRN 1-5 and 59-63) */
	UBX_KEY_BDS_USE_GEO_PRN = 0x10340014,
};

/** Confidence Bound estimation */
enum ubx_key_cfb {
	/** Enable Confidence Bound estimation */
	UBX_KEY_CFB_ENABLE = 0x100c0001,
	/** Value for the average window size */
	UBX_KEY_CFB_WINDOW_SIZE = 0x400c0007,
};

/** Galileo system configuration */
enum ubx_key_gal {
	/** Enable using Galileo Open Service Navigation Message Authentication (OSNMA) protocol */
	UBX_KEY_GAL_USE_OSNMA = 0x10350005,
	/** Minimum equivalent tag length */
	UBX_KEY_GAL_OSNMA_MINTAGLENGTH = 0x20350007,
	/** Apply the time synchronization requirement */
	UBX_KEY_GAL_OSNMA_TIMESYNC = 0x10350009,
	/** Use I/NAV as primary source */
	UBX_KEY_GAL_OSNMA_INAVPRIM = 0x10350010,
};

/** Geofencing configuration */
enum ubx_key_geofence {
	/** Required confidence level for state evaluation */
	UBX_KEY_GEOFENCE_CONFLVL = 0x20240011,
	/** Use PIO combined fence state output */
	UBX_KEY_GEOFENCE_USE_PIO = 0x10240012,
	/** PIO pin polarity */
	UBX_KEY_GEOFENCE_PINPOL = 0x20240013,
	/** PIO pin number */
	UBX_KEY_GEOFENCE_PIN = 0x20240014,
	/** Number of vertices for polygon 1: 0 = disabled, 1..20 = enabled with defined amount of
	 *  vertices
	 */
	UBX_KEY_GEOFENCE_POLYGON1_NUM_VERTICES = 0x20240017,
	/** Number of vertices for polygon 2: 0 = disabled, 1..20 = enabled with defined amount of
	 *  vertices
	 */
	UBX_KEY_GEOFENCE_POLYGON2_NUM_VERTICES = 0x20240018,
	/** Number of vertices for polygon 3: 0 = disabled, 1..20 = enabled with defined amount of
	 *  vertices
	 */
	UBX_KEY_GEOFENCE_POLYGON3_NUM_VERTICES = 0x20240019,
	/** Number of vertices for polygon 4: 0 = disabled, 1..20 = enabled with defined amount of
	 * vertices
	 */
	UBX_KEY_GEOFENCE_POLYGON4_NUM_VERTICES = 0x2024001a,
	/** Number of vertices for polygon 5: 0 = disabled, 1..20 = enabled with defined amount of
	 *  vertices
	 */
	UBX_KEY_GEOFENCE_POLYGON5_NUM_VERTICES = 0x2024001b,
	/** Use first geofence */
	UBX_KEY_GEOFENCE_USE_FENCE1 = 0x10240020,
	/** Latitude of the first geofence circle center */
	UBX_KEY_GEOFENCE_FENCE1_LAT = 0x40240021,
	/** Longitude of the first geofence circle center */
	UBX_KEY_GEOFENCE_FENCE1_LON = 0x40240022,
	/** Radius of the first geofence circle */
	UBX_KEY_GEOFENCE_FENCE1_RAD = 0x40240023,
	/** Use second geofence */
	UBX_KEY_GEOFENCE_USE_FENCE2 = 0x10240030,
	/** Latitude of the second geofence circle center */
	UBX_KEY_GEOFENCE_FENCE2_LAT = 0x40240031,
	/** Longitude of the second geofence circle center */
	UBX_KEY_GEOFENCE_FENCE2_LON = 0x40240032,
	/** Radius of the second geofence circle */
	UBX_KEY_GEOFENCE_FENCE2_RAD = 0x40240033,
	/** Use third geofence */
	UBX_KEY_GEOFENCE_USE_FENCE3 = 0x10240040,
	/** Latitude of the third geofence circle center */
	UBX_KEY_GEOFENCE_FENCE3_LAT = 0x40240041,
	/** Longitude of the third geofence circle center */
	UBX_KEY_GEOFENCE_FENCE3_LON = 0x40240042,
	/** Radius of the third geofence circle */
	UBX_KEY_GEOFENCE_FENCE3_RAD = 0x40240043,
	/** Use fourth geofence */
	UBX_KEY_GEOFENCE_USE_FENCE4 = 0x10240050,
	/** Latitude of the fourth geofence circle center */
	UBX_KEY_GEOFENCE_FENCE4_LAT = 0x40240051,
	/** Longitude of the fourth geofence circle center */
	UBX_KEY_GEOFENCE_FENCE4_LON = 0x40240052,
	/** Radius of the fourth geofence circle */
	UBX_KEY_GEOFENCE_FENCE4_RAD = 0x40240053,
	/** Latitude of the 1st geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX1_LAT = 0x40240061,
	/** Longitude of the 1st geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX1_LON = 0x40240062,
	/** Latitude of the 2nd geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX2_LAT = 0x40240063,
	/** Longitude of the 2nd geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX2_LON = 0x40240064,
	/** Latitude of the 3rd geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX3_LAT = 0x40240065,
	/** Longitude of the 3rd geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX3_LON = 0x40240066,
	/** Latitude of the 4th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX4_LAT = 0x40240067,
	/** Longitude of the 4th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX4_LON = 0x40240068,
	/** Latitude of the 5th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX5_LAT = 0x40240069,
	/** Longitude of the 5th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX5_LON = 0x4024006a,
	/** Latitude of the 6th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX6_LAT = 0x4024006b,
	/** Longitude of the 6th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX6_LON = 0x4024006c,
	/** Latitude of the 7th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX7_LAT = 0x4024006d,
	/** Longitude of the 7th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX7_LON = 0x4024006e,
	/** Latitude of the 8th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX8_LAT = 0x4024006f,
	/** Longitude of the 8th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX8_LON = 0x40240070,
	/** Latitude of the 9th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX9_LAT = 0x40240071,
	/** Longitude of the 9th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX9_LON = 0x40240072,
	/** Latitude of the 10th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX10_LAT = 0x40240073,
	/** Longitude of the 10th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX10_LON = 0x40240074,
	/** Latitude of the 11th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX11_LAT = 0x40240075,
	/** Longitude of the 11th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX11_LON = 0x40240076,
	/** Latitude of the 12th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX12_LAT = 0x40240077,
	/** Longitude of the 12th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX12_LON = 0x40240078,
	/** Latitude of the 13th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX13_LAT = 0x40240079,
	/** Longitude of the 13th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX13_LON = 0x4024007a,
	/** Latitude of the 14th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX14_LAT = 0x4024007b,
	/** Longitude of the 14th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX14_LON = 0x4024007c,
	/** Latitude of the 15th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX15_LAT = 0x4024007d,
	/** Longitude of the 15th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX15_LON = 0x4024007e,
	/** Latitude of the 16th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX16_LAT = 0x4024007f,
	/** Longitude of the 16th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX16_LON = 0x40240080,
	/** Latitude of the 17th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX17_LAT = 0x40240081,
	/** Longitude of the 17th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX17_LON = 0x40240082,
	/** Latitude of the 18th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX18_LAT = 0x40240083,
	/** Longitude of the 18th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX18_LON = 0x40240084,
	/** Latitude of the 19th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX19_LAT = 0x40240085,
	/** Longitude of the 19th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX19_LON = 0x40240086,
	/** Latitude of the 20th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX20_LAT = 0x40240087,
	/** Longitude of the 20th geofence vertex */
	UBX_KEY_GEOFENCE_VERTEX20_LON = 0x40240088,
};

/** Hardware configuration */
enum ubx_key_hw {
	/** Active antenna voltage control flag */
	UBX_KEY_HW_ANT_CFG_VOLTCTRL = 0x10a3002e,
	/** Short antenna detection flag */
	UBX_KEY_HW_ANT_CFG_SHORTDET = 0x10a3002f,
	/** Short antenna detection polarity */
	UBX_KEY_HW_ANT_CFG_SHORTDET_POL = 0x10a30030,
	/** Open antenna detection flag */
	UBX_KEY_HW_ANT_CFG_OPENDET = 0x10a30031,
	/** Open antenna detection polarity */
	UBX_KEY_HW_ANT_CFG_OPENDET_POL = 0x10a30032,
	/** Power down antenna flag */
	UBX_KEY_HW_ANT_CFG_PWRDOWN = 0x10a30033,
	/** Power down antenna logic polarity */
	UBX_KEY_HW_ANT_CFG_PWRDOWN_POL = 0x10a30034,
	/** Automatic recovery from short state flag */
	UBX_KEY_HW_ANT_CFG_RECOVER = 0x10a30035,
	/** Antenna switch PIO number */
	UBX_KEY_HW_ANT_SUP_SWITCH_PIN = 0x20a30036,
	/** Antenna short detection PIO number */
	UBX_KEY_HW_ANT_SUP_SHORT_PIN = 0x20a30037,
	/** Antenna open detection PIO number */
	UBX_KEY_HW_ANT_SUP_OPEN_PIN = 0x20a30038,
	/** ANT on->short timeout[us] */
	UBX_KEY_HW_ANT_ON_SHORT_US = 0x30a3003c,
	/** Select Wake-On-Motion mode */
	UBX_KEY_HW_SENS_WOM_MODE = 0x20a30063,
	/** Wake-On-Motion threshold */
	UBX_KEY_HW_SENS_WOM_THLD = 0x20a30064,
	/** Antenna supervisor engine selection */
	UBX_KEY_HW_ANT_SUP_ENGINE = 0x20a30054,
	/** Antenna supervisor MADC engine short detection threshold */
	UBX_KEY_HW_ANT_SUP_SHORT_THR = 0x20a30055,
	/** Antenna supervisor MADC engine open detection threshold */
	UBX_KEY_HW_ANT_SUP_OPEN_THR = 0x20a30056,
	/** Mode for internal LNA */
	UBX_KEY_HW_RF_LNA_MODE = 0x20a30057,
	/** Low Gain Mode for internal LNA RF1 */
	UBX_KEY_HW_RF1_LNA_MODE_LOWGAIN = 0x10a3006a,
	/** Low Gain Mode for internal LNA RF2 */
	UBX_KEY_HW_RF2_LNA_MODE_LOWGAIN = 0x10a3006b,
	/** Low Gain Mode for internal LNA RF3 */
	UBX_KEY_HW_RF3_LNA_MODE_LOWGAIN = 0x10a3006c,
};

/** Configuration of the I2C interface */
enum ubx_key_i2c {
	/** I2C address of the receiver (7 bits) */
	UBX_KEY_I2C_ADDRESS = 0x20510001,
	/** Flag to disable timeouting the interface after 1.5 s */
	UBX_KEY_I2C_EXTENDEDTIMEOUT = 0x10510002,
	/** Flag to indicate if the I2C interface should be enabled */
	UBX_KEY_I2C_ENABLED = 0x10510003,
	/** Flag to indicate if the internal Pull-Ups (SDA and SCL lines) should be disabled */
	UBX_KEY_I2C_PULL_UPS_DISABLED = 0x1051000b,
};

/** Information message configuration */
enum ubx_key_infmsg {
	/** Information message enable flags for the UBX protocol on the I2C interface */
	UBX_KEY_INFMSG_UBX_I2C = 0x20920001,
	/** Information message enable flags for the UBX protocol on the UART1 interface */
	UBX_KEY_INFMSG_UBX_UART1 = 0x20920002,
	/** Information message enable flags for the UBX protocol on the UART2 interface */
	UBX_KEY_INFMSG_UBX_UART2 = 0x20920003,
	/** Information message enable flags for the UBX protocol on the USB interface */
	UBX_KEY_INFMSG_UBX_USB = 0x20920004,
	/** Information message enable flags for the UBX protocol on the SPI interface */
	UBX_KEY_INFMSG_UBX_SPI = 0x20920005,
	/** Information message enable flags for the NMEA protocol on the I2C interface */
	UBX_KEY_INFMSG_NMEA_I2C = 0x20920006,
	/** Information message enable flags for the NMEA protocol on the UART1 interface */
	UBX_KEY_INFMSG_NMEA_UART1 = 0x20920007,
	/** Information message enable flags for the NMEA protocol on the UART2 interface */
	UBX_KEY_INFMSG_NMEA_UART2 = 0x20920008,
	/** Information message enable flags for the NMEA protocol on the USB interface */
	UBX_KEY_INFMSG_NMEA_USB = 0x20920009,
	/** Information message enable flags for the NMEA protocol on the SPI interface */
	UBX_KEY_INFMSG_NMEA_SPI = 0x2092000a,
};

/** Jamming and interference monitor configuration */
enum ubx_key_itfm {
	/** Broadband jamming detection threshold */
	UBX_KEY_ITFM_BBTHRESHOLD = 0x20410001,
	/** CW jamming detection threshold */
	UBX_KEY_ITFM_CWTHRESHOLD = 0x20410002,
	/** Enable interference detection */
	UBX_KEY_ITFM_ENABLE = 0x1041000d,
	/** Antenna setting */
	UBX_KEY_ITFM_ANTSETTING = 0x20410010,
	/** Scan auxiliary bands */
	UBX_KEY_ITFM_ENABLE_AUX = 0x10410013,
};

/** Data logger configuration */
enum ubx_key_logfilter {
	/** Recording enabled */
	UBX_KEY_LOGFILTER_RECORD_ENA = 0x10de0002,
	/** Once per wake up */
	UBX_KEY_LOGFILTER_ONCE_PER_WAKE_UP_ENA = 0x10de0003,
	/** Apply all filter settings */
	UBX_KEY_LOGFILTER_APPLY_ALL_FILTERS = 0x10de0004,
	/** Minimum time interval between logged positions */
	UBX_KEY_LOGFILTER_MIN_INTERVAL = 0x30de0005,
	/** Time threshold */
	UBX_KEY_LOGFILTER_TIME_THRS = 0x30de0006,
	/** Speed threshold */
	UBX_KEY_LOGFILTER_SPEED_THRS = 0x30de0007,
	/** Position threshold */
	UBX_KEY_LOGFILTER_POSITION_THRS = 0x40de0008,
};

/** Motion detector configuration */
enum ubx_key_mot {
	/** Static hold speed threshold, below which the receiver is considered to be stationary */
	UBX_KEY_MOT_GNSSSPEED_THRS = 0x20250038,
	/** Static hold distance threshold, within which the receiver is considered to be stationary
	 */
	UBX_KEY_MOT_GNSSDIST_THRS = 0x3025003b,
	/** Averaging window for IMU measurements in noisy setups. */
	UBX_KEY_MOT_IMU_FILT_WINDOW = 0x30250016,
};

/** Secondary output configuration */
enum ubx_key_nav2 {
	/** Enable secondary (NAV2) output */
	UBX_KEY_NAV2_OUT_ENABLED = 0x10170001,
	/** Use SBAS integrity information in the secondary output */
	UBX_KEY_NAV2_SBAS_USE_INTEGRITY = 0x10170002,
	/** Enable using only signals with authenticated navigation data in the secondary output */
	UBX_KEY_NAV2_NAVSPG_ONLY_AUTHDATA = 0x10170003,
};

/** Navigation corrections configuration */
enum ubx_key_navcor {
	/** Enable/disable HOST corrections */
	UBX_KEY_NAVCOR_ENABLE_HOST = 0x100d0001,
	/** Enable/disable Galileo HAS corrections */
	UBX_KEY_NAVCOR_ENABLE_GAL_HAS = 0x100d0002,
};

/** Satellite Mask Configuration */
enum ubx_key_navmask {
	/** Satellite mask for the GPS system */
	UBX_KEY_NAVMASK_SV_MASK_GPS = 0x50180013,
	/** Satellite mask for the GALILEO system */
	UBX_KEY_NAVMASK_SV_MASK_GAL = 0x50180014,
	/** Satellite mask for the GLONASS system */
	UBX_KEY_NAVMASK_SV_MASK_GLO = 0x50180015,
	/** Satellite mask for the BeiDou system */
	UBX_KEY_NAVMASK_SV_MASK_BDS = 0x50180016,
	/** Satellite mask for the QZSS system */
	UBX_KEY_NAVMASK_SV_MASK_QZSS = 0x50180017,
	/** Satellite mask for the NavIC system */
	UBX_KEY_NAVMASK_SV_MASK_NAVIC = 0x50180018,
	/** Elevation masks for azimuth range 0 <= az < 20 deg */
	UBX_KEY_NAVMASK_EL_MASK_000_020 = 0x50180001,
	/** Elevation masks for azimuth range 20 <= az < 40 deg */
	UBX_KEY_NAVMASK_EL_MASK_020_040 = 0x50180002,
	/** Elevation masks for azimuth range 40 <= az < 60 deg */
	UBX_KEY_NAVMASK_EL_MASK_040_060 = 0x50180003,
	/** Elevation masks for azimuth range 60 <= az < 80 deg */
	UBX_KEY_NAVMASK_EL_MASK_060_080 = 0x50180004,
	/** Elevation masks for azimuth range 80 <= az < 100 deg */
	UBX_KEY_NAVMASK_EL_MASK_080_100 = 0x50180005,
	/** Elevation masks for azimuth range 100 <= az < 120 deg */
	UBX_KEY_NAVMASK_EL_MASK_100_120 = 0x50180006,
	/** Elevation masks for azimuth range 120 <= az < 140 deg */
	UBX_KEY_NAVMASK_EL_MASK_120_140 = 0x50180007,
	/** Elevation masks for azimuth range 140 <= az < 160 deg */
	UBX_KEY_NAVMASK_EL_MASK_140_160 = 0x50180008,
	/** Elevation masks for azimuth range 160 <= az < 180 deg */
	UBX_KEY_NAVMASK_EL_MASK_160_180 = 0x50180009,
	/** Elevation masks for azimuth range 180 <= az < 200 deg */
	UBX_KEY_NAVMASK_EL_MASK_180_200 = 0x5018000a,
	/** Elevation masks for azimuth range 200 <= az < 220 deg */
	UBX_KEY_NAVMASK_EL_MASK_200_220 = 0x5018000b,
	/** Elevation masks for azimuth range 220 <= az < 240 deg */
	UBX_KEY_NAVMASK_EL_MASK_220_240 = 0x5018000c,
	/** Elevation masks for azimuth range 240 <= az < 260 deg */
	UBX_KEY_NAVMASK_EL_MASK_240_260 = 0x5018000d,
	/** Elevation masks for azimuth range 260 <= az < 280 deg */
	UBX_KEY_NAVMASK_EL_MASK_260_280 = 0x5018000e,
	/** Elevation masks for azimuth range 280 <= az < 300 deg */
	UBX_KEY_NAVMASK_EL_MASK_280_300 = 0x5018000f,
	/** Elevation masks for azimuth range 300 <= az < 320 deg */
	UBX_KEY_NAVMASK_EL_MASK_300_320 = 0x50180010,
	/** Elevation masks for azimuth range 320 <= az < 340 deg */
	UBX_KEY_NAVMASK_EL_MASK_320_340 = 0x50180011,
	/** Elevation masks for azimuth range 340 <= az < 360 deg */
	UBX_KEY_NAVMASK_EL_MASK_340_360 = 0x50180012,
};

/** NMEA protocol configuration */
enum ubx_key_nmea {
	/** NMEA protocol version */
	UBX_KEY_NMEA_PROTVER = 0x20930001,
	/** Maximum number of SVs to report per Talker ID */
	UBX_KEY_NMEA_MAXSVS = 0x20930002,
	/** Enable compatibility mode */
	UBX_KEY_NMEA_COMPAT = 0x10930003,
	/** Enable considering mode */
	UBX_KEY_NMEA_CONSIDER = 0x10930004,
	/** Enable strict limit to 82 characters maximum NMEA message length */
	UBX_KEY_NMEA_LIMIT82 = 0x10930005,
	/** Enable high precision mode */
	UBX_KEY_NMEA_HIGHPREC = 0x10930006,
	/** Display configuration for SVs that do not have value defined in NMEA */
	UBX_KEY_NMEA_SVNUMBERING = 0x20930007,
	/** Disable reporting of GPS satellites */
	UBX_KEY_NMEA_FILT_GPS = 0x10930011,
	/** Disable reporting of SBAS satellites */
	UBX_KEY_NMEA_FILT_SBAS = 0x10930012,
	/** Disable reporting of Galileo satellites */
	UBX_KEY_NMEA_FILT_GAL = 0x10930013,
	/** Disable reporting of QZSS satellites */
	UBX_KEY_NMEA_FILT_QZSS = 0x10930015,
	/** Disable reporting of GLONASS satellites */
	UBX_KEY_NMEA_FILT_GLO = 0x10930016,
	/** Disable reporting of BeiDou satellites */
	UBX_KEY_NMEA_FILT_BDS = 0x10930017,
	/** Disable reporting of NavIC satellites */
	UBX_KEY_NMEA_FILT_NAVIC = 0x10930018,
	/** Enable position output for failed or invalid fixes */
	UBX_KEY_NMEA_OUT_INVFIX = 0x10930021,
	/** Enable position output for invalid fixes */
	UBX_KEY_NMEA_OUT_MSKFIX = 0x10930022,
	/** Enable time output for invalid times */
	UBX_KEY_NMEA_OUT_INVTIME = 0x10930023,
	/** Enable date output for invalid dates */
	UBX_KEY_NMEA_OUT_INVDATE = 0x10930024,
	/** Restrict output to GPS satellites only */
	UBX_KEY_NMEA_OUT_ONLYGPS = 0x10930025,
	/** Enable course over ground output even if it is frozen */
	UBX_KEY_NMEA_OUT_FROZENCOG = 0x10930026,
	/** Main Talker ID */
	UBX_KEY_NMEA_MAINTALKERID = 0x20930031,
	/** Talker ID for GSV NMEA messages */
	UBX_KEY_NMEA_GSVTALKERID = 0x20930032,
	/** BeiDou Talker ID */
	UBX_KEY_NMEA_BDSTALKERID = 0x30930033,
};

/** Odometer and low-speed course over ground filter configuration */
enum ubx_key_odo {
	/** Use odometer */
	UBX_KEY_ODO_USE_ODO = 0x10220001,
	/** Use low-speed course over ground filter */
	UBX_KEY_ODO_USE_COG = 0x10220002,
	/** Output low-pass filtered velocity */
	UBX_KEY_ODO_OUTLPVEL = 0x10220003,
	/** Output low-pass filtered course over ground (heading) */
	UBX_KEY_ODO_OUTLPCOG = 0x10220004,
	/** Odometer profile configuration */
	UBX_KEY_ODO_PROFILE = 0x20220005,
	/** Upper speed limit for low-speed course over ground filter */
	UBX_KEY_ODO_COGMAXSPEED = 0x20220021,
	/** Maximum acceptable position accuracy for computing low-speed filtered course over ground
	 */
	UBX_KEY_ODO_COGMAXPOSACC = 0x20220022,
	/** Velocity low-pass filter level */
	UBX_KEY_ODO_VELLPGAIN = 0x20220031,
	/** Course over ground low-pass filter level (at speed < 8 m/s) */
	UBX_KEY_ODO_COGLPGAIN = 0x20220032,
};

/** Configuration for receiver power management */
enum ubx_key_pm {
	/** General mode of operation. */
	UBX_KEY_PM_OPERATEMODE = 0x20d00001,
	/** Position update period for Long Interval Tracking. */
	UBX_KEY_PM_POSUPDATEPERIOD = 0x40d00002,
	/** Acquisition period used if the receiver previously failed to achieve a position fix. */
	UBX_KEY_PM_ACQPERIOD = 0x40d00003,
	/** Maximum time to spend in Acquisition state */
	UBX_KEY_PM_MAXACQTIME = 0x20d00007,
	/** Update ephemeris regularly. */
	UBX_KEY_PM_UPDATEEPH = 0x10d0000a,
	/** EXTINT pin select */
	UBX_KEY_PM_EXTINTSEL = 0x20d0000b,
	/** EXTINT pin control (Wake) */
	UBX_KEY_PM_EXTINTWAKE = 0x10d0000c,
	/** EXTINT pin control (Backup) */
	UBX_KEY_PM_EXTINTBACKUP = 0x10d0000d,
	/** EXTINT pin control (Inactive) */
	UBX_KEY_PM_EXTINTINACTIVE = 0x10d0000e,
	/** Inactivity time out on EXTINT pin if enabled */
	UBX_KEY_PM_EXTINTINACTIVITY = 0x40d0000f,
	/** Limit peak current */
	UBX_KEY_PM_LIMITPEAKCURR = 0x10d00010,
	/** Time between automatic SOS BACKUP operations */
	UBX_KEY_PM_AUTOSAVEPERIOD = 0x20d0001d,
	/** Enable the selective L5 mode */
	UBX_KEY_PM_SELECTIVE_L5 = 0x10d0001e,
	/** Acquisition period used if the receiver previously failed to achieve a position fix. */
	UBX_KEY_PM_POSRETRYPERIOD = 0x40d00021,
};

/** Point to multipoint (PMP) configuration */
enum ubx_key_pmp {
	/** Center frequency */
	UBX_KEY_PMP_CENTER_FREQUENCY = 0x40b10011,
	/** Search window */
	UBX_KEY_PMP_SEARCH_WINDOW = 0x30b10012,
	/** Use service ID */
	UBX_KEY_PMP_USE_SERVICE_ID = 0x10b10016,
	/** Service identifier */
	UBX_KEY_PMP_SERVICE_ID = 0x30b10017,
	/** Data rate */
	UBX_KEY_PMP_DATA_RATE = 0x30b10013,
	/** Use descrambler */
	UBX_KEY_PMP_USE_DESCRAMBLER = 0x10b10014,
	/** Descrambler initialization */
	UBX_KEY_PMP_DESCRAMBLER_INIT = 0x30b10015,
	/** Use prescrambling */
	UBX_KEY_PMP_USE_PRESCRAMBLING = 0x10b10019,
	/** Unique word */
	UBX_KEY_PMP_UNIQUE_WORD = 0x50b1001a,
};

/** QZSS system configuration */
enum ubx_key_qzss {
	/** Apply QZSS SLAS DGNSS corrections */
	UBX_KEY_QZSS_USE_SLAS_DGNSS = 0x10370005,
	/** Use QZSS SLAS data when it is in test mode (SLAS msg 0) */
	UBX_KEY_QZSS_USE_SLAS_TESTMODE = 0x10370006,
	/** Raim out measurements that are not corrected by QZSS SLAS, if at least 5 measurements
	 *  are corrected
	 */
	UBX_KEY_QZSS_USE_SLAS_RAIM_UNCORR = 0x10370007,
	/** Maximum baseline distance to closest GMS */
	UBX_KEY_QZSS_SLAS_MAX_BASELINE = 0x30370008,
	/** QZSS L6 SV Id to be decoded by channel A */
	UBX_KEY_QZSS_L6_SVIDA = 0x20370020,
	/** QZSS L6 SV Id to be decoded by channel B */
	UBX_KEY_QZSS_L6_SVIDB = 0x20370030,
	/** QZSS L6 messages to be decoded by channel A */
	UBX_KEY_QZSS_L6_MSGA = 0x20370050,
	/** QZSS L6 messages to be decoded by channel B */
	UBX_KEY_QZSS_L6_MSGB = 0x20370060,
	/** QZSS L6 message Reed-Solomon decoder mode */
	UBX_KEY_QZSS_L6_RSDECODER = 0x20370080,
};

/** Remote inventory */
enum ubx_key_rinv {
	/** Dump data at startup */
	UBX_KEY_RINV_DUMP = 0x10c70001,
	/** Data is binary */
	UBX_KEY_RINV_BINARY = 0x10c70002,
	/** Size of data */
	UBX_KEY_RINV_DATA_SIZE = 0x20c70003,
	/** Data bytes 1-8 (LSB) */
	UBX_KEY_RINV_CHUNK0 = 0x50c70004,
	/** Data bytes 9-16 */
	UBX_KEY_RINV_CHUNK1 = 0x50c70005,
	/** Data bytes 17-24 */
	UBX_KEY_RINV_CHUNK2 = 0x50c70006,
	/** Data bytes 25-30 (MSB) */
	UBX_KEY_RINV_CHUNK3 = 0x50c70007,
};

/** RTCM protocol configuration */
enum ubx_key_rtcm {
	/** RTCM DF003 (Reference station ID) output value */
	UBX_KEY_RTCM_DF003_OUT = 0x30090001,
	/** RTCM DF003 (Reference station ID) input value */
	UBX_KEY_RTCM_DF003_IN = 0x30090008,
	/** RTCM input filter configuration based on RTCM DF003 (Reference station ID) value */
	UBX_KEY_RTCM_DF003_IN_FILTER = 0x20090009,
	/** Sign of the Phaserange rate in RTCM MSM7 and MSM5 output messages */
	UBX_KEY_RTCM_MSM_OUT_RANGERATE_SIGN = 0x10090011,
	/** RTCM DF028 (antenna height) output value */
	UBX_KEY_RTCM_DF028_OUT = 0x30090010,
	/** Reduced GNSS observables timeout */
	UBX_KEY_RTCM_REDUCED_GNSS_TIMEOUT = 0x40090012,
	/** Epoch complete timeout */
	UBX_KEY_RTCM_EPOCH_COMPLETE_TIMEOUT = 0x40090013,
};

/** SBAS configuration */
enum ubx_key_sbas {
	/** Use SBAS data when it is in test mode (SBAS msg 0) */
	UBX_KEY_SBAS_USE_TESTMODE = 0x10360002,
	/** Use SBAS GEOs as a ranging source (for navigation) */
	UBX_KEY_SBAS_USE_RANGING = 0x10360003,
	/** Use SBAS differential corrections */
	UBX_KEY_SBAS_USE_DIFFCORR = 0x10360004,
	/** Use SBAS integrity information */
	UBX_KEY_SBAS_USE_INTEGRITY = 0x10360005,
	/** Accept corrections from SBAS SV, even if not self included in PRN MASK */
	UBX_KEY_SBAS_ACCEPT_NOT_IN_PRNMASK = 0x30360008,
	/** Use SBAS ionosphere correction only */
	UBX_KEY_SBAS_USE_IONOONLY = 0x10360007,
	/** SBAS PRN search configuration */
	UBX_KEY_SBAS_PRNSCANMASK = 0x50360006,
};

/** Security configuration */
enum ubx_key_sec {
	/** Configuration lockdown */
	UBX_KEY_SEC_CFG_LOCK = 0x10f60009,
	/** Configuration lockdown exempted group 1 */
	UBX_KEY_SEC_CFG_LOCK_UNLOCKGRP1 = 0x30f6000a,
	/** Configuration lockdown exempted group 2 */
	UBX_KEY_SEC_CFG_LOCK_UNLOCKGRP2 = 0x30f6000b,
	/** Disabling the simulated signal spoofing detection. */
	UBX_KEY_SEC_SPOOFDET_SIM_SIG_DIS = 0x10f6005d,
	/** When set, go for a more sensitive jamming detection (at the cost of increased false
	 *  alarm rate)
	 */
	UBX_KEY_SEC_JAMDET_SENSITIVITY_HI = 0x10f60051,
};

/** Sensor fusion (SF) core configuration */
enum ubx_key_sfcore {
	/** Use ADR/UDR sensor fusion */
	UBX_KEY_SFCORE_USE_SF = 0x10080001,
	/** X coordinate of IMU-to-CRP lever-arm in the installation frame */
	UBX_KEY_SFCORE_IMU2CRP_LA_X = 0x30080002,
	/** Y coordinate of IMU-to-CRP lever-arm in the installation frame */
	UBX_KEY_SFCORE_IMU2CRP_LA_Y = 0x30080003,
	/** Z coordinate of IMU-to-CRP lever-arm in the installation frame */
	UBX_KEY_SFCORE_IMU2CRP_LA_Z = 0x30080004,
	/** Rate of navigation solution output */
	UBX_KEY_SFCORE_HNR_RATE = 0x2008001a,
};

/** Sensor fusion (SF) inertial measurement unit (IMU) configuration */
enum ubx_key_sfimu {
	/** Time period between each update for the saved temperature gyroscope bias table */
	UBX_KEY_SFIMU_GYRO_TC_UPDATE_PERIOD = 0x30060007,
	/** Gyroscope sensor RMS threshold */
	UBX_KEY_SFIMU_GYRO_RMSTHDL = 0x20060008,
	/** Nominal gyroscope sensor data sampling frequency */
	UBX_KEY_SFIMU_GYRO_FREQUENCY = 0x20060009,
	/** Gyroscope sensor data latency due to e.g. CAN bus */
	UBX_KEY_SFIMU_GYRO_LATENCY = 0x3006000a,
	/** Gyroscope sensor data accuracy */
	UBX_KEY_SFIMU_GYRO_ACCURACY = 0x3006000b,
	/** Accelerometer RMS threshold */
	UBX_KEY_SFIMU_ACCEL_RMSTHDL = 0x20060015,
	/** Nominal accelerometer sensor data sampling frequency */
	UBX_KEY_SFIMU_ACCEL_FREQUENCY = 0x20060016,
	/** Accelerometer sensor data latency due to e.g. CAN bus */
	UBX_KEY_SFIMU_ACCEL_LATENCY = 0x30060017,
	/** Accelerometer sensor data accuracy */
	UBX_KEY_SFIMU_ACCEL_ACCURACY = 0x30060018,
	/** IMU enabled */
	UBX_KEY_SFIMU_IMU_EN = 0x1006001d,
	/** SCL PIO of the IMU I2C */
	UBX_KEY_SFIMU_IMU_I2C_SCL_PIO = 0x2006001e,
	/** SDA PIO of the IMU I2C */
	UBX_KEY_SFIMU_IMU_I2C_SDA_PIO = 0x2006001f,
	/** X coordinate of IMU-to-ANT lever-arm in the installation frame */
	UBX_KEY_SFIMU_IMU2ANT_LA_X = 0x30060020,
	/** Y coordinate of IMU-to-ANT lever-arm in the installation frame */
	UBX_KEY_SFIMU_IMU2ANT_LA_Y = 0x30060021,
	/** Z coordinate of IMU-to-ANT lever-arm in the installation frame */
	UBX_KEY_SFIMU_IMU2ANT_LA_Z = 0x30060022,
	/** Enable automatic IMU-mount alignment */
	UBX_KEY_SFIMU_AUTO_MNTALG_ENA = 0x10060027,
	/** User-defined IMU-mount yaw angle [0, 36000] */
	UBX_KEY_SFIMU_IMU_MNTALG_YAW = 0x4006002d,
	/** User-defined IMU-mount pitch angle [-9000, 9000] */
	UBX_KEY_SFIMU_IMU_MNTALG_PITCH = 0x3006002e,
	/** User-defined IMU-mount roll angle [-18000, 18000] */
	UBX_KEY_SFIMU_IMU_MNTALG_ROLL = 0x3006002f,
	/** User-defined IMU mount alignment angles tolerance level */
	UBX_KEY_SFIMU_IMU_MNTALG_TOLERANCE = 0x20060030,
};

/** Sensor fusion (SF) odometer configuration */
enum ubx_key_sfodo {
	/** Use combined rear wheel ticks instead of the single tick */
	UBX_KEY_SFODO_COMBINE_TICKS = 0x10070001,
	/** Use speed measurements */
	UBX_KEY_SFODO_USE_SPEED = 0x10070003,
	/** Disable automatic estimation of maximum absolute wheel tick counter */
	UBX_KEY_SFODO_DIS_AUTOCOUNTMAX = 0x10070004,
	/** Disable automatic wheel tick direction pin polarity detection */
	UBX_KEY_SFODO_DIS_AUTODIRPINPOL = 0x10070005,
	/** Disable automatic receiver reconfiguration for processing speed data instead of wheel
	 *  tick data
	 */
	UBX_KEY_SFODO_DIS_AUTOSPEED = 0x10070006,
	/** Wheel tick scale factor */
	UBX_KEY_SFODO_FACTOR = 0x40070007,
	/** Wheel tick quantization */
	UBX_KEY_SFODO_QUANT_ERROR = 0x40070008,
	/** Wheel tick counter maximum value */
	UBX_KEY_SFODO_COUNT_MAX = 0x40070009,
	/** Wheel tick data latency due to e.g. CAN bus */
	UBX_KEY_SFODO_LATENCY = 0x3007000a,
	/** Nominal wheel tick data frequency (0 = not set) */
	UBX_KEY_SFODO_FREQUENCY = 0x2007000b,
	/** Count both rising and falling edges on wheel tick signal */
	UBX_KEY_SFODO_CNT_BOTH_EDGES = 0x1007000d,
	/** Speed sensor dead band (0 = not set) */
	UBX_KEY_SFODO_SPEED_BAND = 0x3007000e,
	/** Wheel tick signal enabled */
	UBX_KEY_SFODO_USE_WT_PIN = 0x1007000f,
	/** Wheel tick direction pin polarity */
	UBX_KEY_SFODO_DIR_PINPOL = 0x10070010,
	/** Disable automatic use of wheel tick/speed data received over the software interface */
	UBX_KEY_SFODO_DIS_AUTOSW = 0x10070011,
	/** X coordinate of IMU-to-VRP lever-arm in the installation frame */
	UBX_KEY_SFODO_IMU2VRP_LA_X = 0x30070012,
	/** Y coordinate of IMU-to-VRP lever-arm in the installation frame */
	UBX_KEY_SFODO_IMU2VRP_LA_Y = 0x30070013,
	/** Z coordinate of IMU-to-VRP lever-arm in the installation frame */
	UBX_KEY_SFODO_IMU2VRP_LA_Z = 0x30070014,
	/** Do not use directional information */
	UBX_KEY_SFODO_DIS_DIR_INFO = 0x1007001c,
};

/** SPARTN configuration */
enum ubx_key_spartn {
	/** Selector for source SPARTN stream */
	UBX_KEY_SPARTN_USE_SOURCE = 0x20a70001,
};

/** Configuration of the SPI interface */
enum ubx_key_spi {
	/** Number of bytes containing 0xFF to receive before switching off reception. Range: 0
	 *  (mechanism off)
	 */
	UBX_KEY_SPI_MAXFF = 0x20640001,
	/** Clock polarity select: 0: Active Hight Clock, SCLK idles low, 1: Active Low Clock, SCLK
	 *  idles high
	 */
	UBX_KEY_SPI_CPOLARITY = 0x10640002,
	/** Clock phase select: 0: Data captured on first edge of SCLK, 1: Data captured on second
	 *  edge of SCLK
	 */
	UBX_KEY_SPI_CPHASE = 0x10640003,
	/** Flag to disable timeouting the interface after 1.5s */
	UBX_KEY_SPI_EXTENDEDTIMEOUT = 0x10640005,
	/** Flag to indicate if the SPI interface should be enabled */
	UBX_KEY_SPI_ENABLED = 0x10640006,
};

/** Time mode configuration */
enum ubx_key_tmode {
	/** Receiver mode */
	UBX_KEY_TMODE_MODE = 0x20030001,
	/** Determines whether the ARP position is given in ECEF or LAT/LON/HEIGHT? */
	UBX_KEY_TMODE_POS_TYPE = 0x20030002,
	/** ECEF X coordinate of the ARP position. */
	UBX_KEY_TMODE_ECEF_X = 0x40030003,
	/** ECEF Y coordinate of the ARP position. */
	UBX_KEY_TMODE_ECEF_Y = 0x40030004,
	/** ECEF Z coordinate of the ARP position. */
	UBX_KEY_TMODE_ECEF_Z = 0x40030005,
	/** High-precision ECEF X coordinate of the ARP position. */
	UBX_KEY_TMODE_ECEF_X_HP = 0x20030006,
	/** High-precision ECEF Y coordinate of the ARP position. */
	UBX_KEY_TMODE_ECEF_Y_HP = 0x20030007,
	/** High-precision ECEF Z coordinate of the ARP position. */
	UBX_KEY_TMODE_ECEF_Z_HP = 0x20030008,
	/** Latitude of the ARP position. */
	UBX_KEY_TMODE_LAT = 0x40030009,
	/** Longitude of the ARP position. */
	UBX_KEY_TMODE_LON = 0x4003000a,
	/** Height of the ARP position. */
	UBX_KEY_TMODE_HEIGHT = 0x4003000b,
	/** High-precision latitude of the ARP position */
	UBX_KEY_TMODE_LAT_HP = 0x2003000c,
	/** High-precision longitude of the ARP position. */
	UBX_KEY_TMODE_LON_HP = 0x2003000d,
	/** High-precision height of the ARP position. */
	UBX_KEY_TMODE_HEIGHT_HP = 0x2003000e,
	/** Fixed position 3D accuracy */
	UBX_KEY_TMODE_FIXED_POS_ACC = 0x4003000f,
	/** Survey-in minimum duration */
	UBX_KEY_TMODE_SVIN_MIN_DUR = 0x40030010,
	/** Survey-in position accuracy limit */
	UBX_KEY_TMODE_SVIN_ACC_LIMIT = 0x40030011,
};

/** Time pulse configuration */
enum ubx_key_tp {
	/** Determines whether the time pulse is interpreted as frequency or period */
	UBX_KEY_TP_PULSE_DEF = 0x20050023,
	/** Determines whether the time pulse length is interpreted as length or pulse ratio */
	UBX_KEY_TP_PULSE_LENGTH_DEF = 0x20050030,
	/** Antenna cable delay in [ns] */
	UBX_KEY_TP_ANT_CABLEDELAY = 0x30050001,
	/** Time pulse period (TP1) in [us] */
	UBX_KEY_TP_PERIOD_TP1 = 0x40050002,
	/** Time pulse period when locked to GNSS time (TP1) in [us] */
	UBX_KEY_TP_PERIOD_LOCK_TP1 = 0x40050003,
	/** Time pulse frequency (TP1) in [Hz] */
	UBX_KEY_TP_FREQ_TP1 = 0x40050024,
	/** Time pulse frequency when locked to GNSS time (TP1) in [Hz] */
	UBX_KEY_TP_FREQ_LOCK_TP1 = 0x40050025,
	/** Time pulse length (TP1) in [us] */
	UBX_KEY_TP_LEN_TP1 = 0x40050004,
	/** Time pulse length when locked to GNSS time (TP1) in [us] */
	UBX_KEY_TP_LEN_LOCK_TP1 = 0x40050005,
	/** Time pulse duty cycle (TP1) in [%] */
	UBX_KEY_TP_DUTY_TP1 = 0x5005002a,
	/** Time pulse duty cycle when locked to GNSS time (TP1) in [%] */
	UBX_KEY_TP_DUTY_LOCK_TP1 = 0x5005002b,
	/** User-configurable time pulse delay (TP1) in [ns] */
	UBX_KEY_TP_USER_DELAY_TP1 = 0x40050006,
	/** Enable the time pulse (TP1) */
	UBX_KEY_TP_TP1_ENA = 0x10050007,
	/** Sync time pulse to GNSS time or local clock (TP1) */
	UBX_KEY_TP_SYNC_GNSS_TP1 = 0x10050008,
	/** Use locked parameters when possible (TP1) */
	UBX_KEY_TP_USE_LOCKED_TP1 = 0x10050009,
	/** Align time pulse to top of second (TP1) */
	UBX_KEY_TP_ALIGN_TO_TOW_TP1 = 0x1005000a,
	/** Set time pulse polarity (TP1) */
	UBX_KEY_TP_POL_TP1 = 0x1005000b,
	/** Time grid to use (TP1) */
	UBX_KEY_TP_TIMEGRID_TP1 = 0x2005000c,
	/** Time pulse period (TP2) in [us] */
	UBX_KEY_TP_PERIOD_TP2 = 0x4005000d,
	/** Time pulse period when locked to GNSS time (TP2) in [us] */
	UBX_KEY_TP_PERIOD_LOCK_TP2 = 0x4005000e,
	/** Time pulse frequency (TP2) */
	UBX_KEY_TP_FREQ_TP2 = 0x40050026,
	/** Time pulse frequency when locked to GNSS time (TP2) in [Hz] */
	UBX_KEY_TP_FREQ_LOCK_TP2 = 0x40050027,
	/** Time pulse length (TP2) in [us] */
	UBX_KEY_TP_LEN_TP2 = 0x4005000f,
	/** Time pulse length when locked to GNSS time (TP2) in [us] */
	UBX_KEY_TP_LEN_LOCK_TP2 = 0x40050010,
	/** Time pulse duty cycle (TP2) in [%] */
	UBX_KEY_TP_DUTY_TP2 = 0x5005002c,
	/** Time pulse duty cycle when locked to GNSS time (TP2) */
	UBX_KEY_TP_DUTY_LOCK_TP2 = 0x5005002d,
	/** User-configurable time pulse delay (TP2) in [ns] */
	UBX_KEY_TP_USER_DELAY_TP2 = 0x40050011,
	/** Enable the time pulse (TP2) */
	UBX_KEY_TP_TP2_ENA = 0x10050012,
	/** Sync time pulse to GNSS time or local clock (TP2) */
	UBX_KEY_TP_SYNC_GNSS_TP2 = 0x10050013,
	/** Use locked parameters when possible (TP2) */
	UBX_KEY_TP_USE_LOCKED_TP2 = 0x10050014,
	/** Align time pulse to top of second (TP2) */
	UBX_KEY_TP_ALIGN_TO_TOW_TP2 = 0x10050015,
	/** Set time pulse polarity (TP2) */
	UBX_KEY_TP_POL_TP2 = 0x10050016,
	/** Time grid to use (TP2) */
	UBX_KEY_TP_TIMEGRID_TP2 = 0x20050017,
	/** Set drive strength of TP1 */
	UBX_KEY_TP_DRSTR_TP1 = 0x20050035,
	/** Set drive strength of TP2 */
	UBX_KEY_TP_DRSTR_TP2 = 0x20050036,
	/** Set time pulse message behavior */
	UBX_KEY_TP_MSG_ALWAYS = 0x10050037,
};

/** TX ready configuration */
enum ubx_key_txready {
	/** Flag to indicate if TX ready pin mechanism should be enabled */
	UBX_KEY_TXREADY_ENABLED = 0x10a20001,
	/** The polarity of the TX ready pin: false:high-active, true:low-active */
	UBX_KEY_TXREADY_POLARITY = 0x10a20002,
	/** Pin number to use for the TX ready functionality */
	UBX_KEY_TXREADY_PIN = 0x20a20003,
	/** Amount of data that should be ready on the interface before triggering the TX ready pin
	 */
	UBX_KEY_TXREADY_THRESHOLD = 0x30a20004,
	/** Interface where the TX ready feature should be linked to */
	UBX_KEY_TXREADY_INTERFACE = 0x20a20005,
};

/** Configuration of the UART1 interface */
enum ubx_key_uart1 {
	/** The baud rate that should be configured on the UART1 */
	UBX_KEY_CFG_UART1_BAUDRATE = 0x40520001,
	/** Number of stopbits that should be used on UART1 */
	UBX_KEY_UART1_STOPBITS = 0x20520002,
	/** Number of databits that should be used on UART1 */
	UBX_KEY_UART1_DATABITS = 0x20520003,
	/** Parity mode that should be used on UART1 */
	UBX_KEY_UART1_PARITY = 0x20520004,
	/** Flag to indicate if the UART1 should be enabled */
	UBX_KEY_UART1_ENABLED = 0x10520005,
};

/** Configuration of the UART2 interface */
enum ubx_key_uart2 {
	/** The baud rate that should be configured on the UART2 */
	UBX_KEY_UART2_BAUDRATE = 0x40530001,
	/** Number of stopbits that should be used on UART2 */
	UBX_KEY_UART2_STOPBITS = 0x20530002,
	/** Number of databits that should be used on UART2 */
	UBX_KEY_UART2_DATABITS = 0x20530003,
	/** Parity mode that should be used on UART2 */
	UBX_KEY_UART2_PARITY = 0x20530004,
	/** Flag to indicate if the UART2 should be enabled */
	UBX_KEY_UART2_ENABLED = 0x10530005,
};

/** Configuration of the USB interface */
enum ubx_key_usb {
	/** Flag to indicate if the USB interface should be enabled */
	UBX_KEY_USB_ENABLED = 0x10650001,
	/** Self-powered device */
	UBX_KEY_USB_SELFPOW = 0x10650002,
	/** Vendor ID */
	UBX_KEY_USB_VENDOR_ID = 0x3065000a,
	/** Vendor ID */
	UBX_KEY_USB_PRODUCT_ID = 0x3065000b,
	/** Power consumption */
	UBX_KEY_USB_POWER = 0x3065000c,
	/** Vendor string characters 0-7 */
	UBX_KEY_USB_VENDOR_STR0 = 0x5065000d,
	/** Vendor string characters 8-15 */
	UBX_KEY_USB_VENDOR_STR1 = 0x5065000e,
	/** Vendor string characters 16-23 */
	UBX_KEY_USB_VENDOR_STR2 = 0x5065000f,
	/** Vendor string characters 24-31 */
	UBX_KEY_USB_VENDOR_STR3 = 0x50650010,
	/** Product string characters 0-7 */
	UBX_KEY_USB_PRODUCT_STR0 = 0x50650011,
	/** Product string characters 8-15 */
	UBX_KEY_USB_PRODUCT_STR1 = 0x50650012,
	/** Product string characters 16-23 */
	UBX_KEY_USB_PRODUCT_STR2 = 0x50650013,
	/** Product string characters 24-31 */
	UBX_KEY_USB_PRODUCT_STR3 = 0x50650014,
	/** Serial number string characters 0-7 */
	UBX_KEY_USB_SERIAL_NO_STR0 = 0x50650015,
	/** Serial number string characters 8-15 */
	UBX_KEY_USB_SERIAL_NO_STR1 = 0x50650016,
	/** Serial number string characters 16-23 */
	UBX_KEY_USB_SERIAL_NO_STR2 = 0x50650017,
	/** Serial number string characters 24-31 */
	UBX_KEY_USB_SERIAL_NO_STR3 = 0x50650018,
};

/** Input protocol configuration of the USB interface */
enum ubx_key_usbinprot {
	/** Flag to indicate if UBX should be an input protocol on USB */
	UBX_KEY_USBINPROT_UBX = 0x10770001,
	/** Flag to indicate if NMEA should be an input protocol on USB */
	UBX_KEY_USBINPROT_NMEA = 0x10770002,
	/** Flag to indicate if RTCM2X should be an input protocol on USB */
	UBX_KEY_USBINPROT_RTCM2X = 0x10770003,
	/** Flag to indicate if RTCM3X should be an input protocol on USB */
	UBX_KEY_USBINPROT_RTCM3X = 0x10770004,
	/** Flag to indicate if SPARTN should be an input protocol on USB */
	UBX_KEY_USBINPROT_SPARTN = 0x10770005,
};

/** Output protocol configuration of the USB interface */
enum ubx_key_usboutprot {
	/** Flag to indicate if UBX should be an output protocol on USB */
	UBX_KEY_USBOUTPROT_UBX = 0x10780001,
	/** Flag to indicate if NMEA should be an output protocol on USB */
	UBX_KEY_USBOUTPROT_NMEA = 0x10780002,
	/** Flag to indicate if RTCM3X should be an output protocol on USB */
	UBX_KEY_USBOUTPROT_RTCM3X = 0x10780004,
};

/** High precision navigation configuration */
enum ubx_nav_hp_dgnss_mode {
	UBX_NAV_HP_DGNSS_MODE_RTK_FLOAT = 2,
	UBX_NAV_HP_DGNSS_MODE_RTK_FIXED = 3,
	/** Conservative ambiguity resolution */
	UBX_NAV_HP_DGNSS_MODE_RTK_CAR = 5,
};

/** Dynamic platform model */
enum ubx_dyn_model {
	UBX_DYN_MODEL_PORTABLE = 0,
	UBX_DYN_MODEL_STATIONARY = 2,
	UBX_DYN_MODEL_PEDESTRIAN = 3,
	UBX_DYN_MODEL_AUTOMOTIVE = 4,
	UBX_DYN_MODEL_SEA = 5,
	UBX_DYN_MODEL_AIRBORNE_1G = 6,
	UBX_DYN_MODEL_AIRBORNE_2G = 7,
	UBX_DYN_MODEL_AIRBORNE_4G = 8,
	UBX_DYN_MODEL_WRIST = 9,
	/** Motorbike (not available in all products) */
	UBX_DYN_MODEL_BIKE = 10,
	/** Robotic lawn mower (not available in all products) */
	UBX_DYN_MODEL_MOWER = 11,
	/** E-scooter (not available in all products) */
	UBX_DYN_MODEL_ESCOOTER = 12,
	/** Rail vehicles (trains, trams) (not available in all products) */
	UBX_DYN_MODEL_RAIL = 13,
	/** Wrist-worn watch for swimming activity (not available in all products) */
	UBX_DYN_MODEL_SWIM = 14,
	/** Wrist-worn watch, optimized for low power (not available in all products) */
	UBX_DYN_MODEL_WRIST_LP = 15,
};

/** Position fix mode */
enum ubx_fix_mode {
	UBX_FIX_MODE_2D_ONLY = 1,
	UBX_FIX_MODE_3D_ONLY = 2,
	UBX_FIX_MODE_AUTO = 3,
};

/** UTC standard to be used */
enum ubx_utc_standard {
	UBX_UTC_STANDARD_AUTOMATIC = 0,
	UBX_UTC_STANDARD_GPS = 3,
	UBX_UTC_STANDARD_GALILEO = 5,
	UBX_UTC_STANDARD_GLONASS = 6,
	UBX_UTC_STANDARD_BEIDOU = 7,
	/** UTC as operated by the National Physics Laboratory, India (NPLI); derived from NavIC
	 *  time
	 */
	UBX_UTC_STANDARD_NPLI = 8,
	/** UTC as operated by the National Institute of Information and Communications Technology,
	 *  Japan (NICT); derived from QZSS time
	 */
	UBX_UTC_STANDARD_NICT = 9,
};

/** Time system to which measurements are aligned */
enum ubx_cfg_rate_time_ref {
	UBX_CFG_RATE_TIME_REF_UTC = 0,
	UBX_CFG_RATE_TIME_REF_GPS = 1,
	UBX_CFG_RATE_TIME_REF_GLONASS = 2,
	UBX_CFG_RATE_TIME_REF_BEIDOU = 3,
	UBX_CFG_RATE_TIME_REF_GALILEO = 4,
	UBX_CFG_RATE_TIME_REF_NAVIC = 5,
};

/** Enable only the given BDS D1/D2 navigation data streams, ignoring the others */
enum ubx_bds_d1d2_navdata {
	/** Enable all BDS D1D2 navigation data streams (default) */
	UBX_BDS_D1D2_NAVDATA_ALL = 0,
	/** Force B1I navigation data, ignoring B2I and B3I */
	UBX_BDS_D1D2_NAVDATA_B1I = 1,
};

/** Required confidence level for state evaluation */
enum ubx_geofence_conflvl {
	/** No confidence */
	UBX_GEOFENCE_CONFLVL_L000 = 0,
	/** 68% */
	UBX_GEOFENCE_CONFLVL_L680 = 1,
	/** 95% */
	UBX_GEOFENCE_CONFLVL_L950 = 2,
	/** 99.7% */
	UBX_GEOFENCE_CONFLVL_L997 = 3,
	/** 99.99% */
	UBX_GEOFENCE_CONFLVL_L9999 = 4,
	/** 99.9999% */
	UBX_GEOFENCE_CONFLVL_L999999 = 5,
};

/** PIO pin polarity */
enum ubx_geofence_pinpol {
	/** PIO low means inside geofence */
	UBX_GEOFENCE_PINPOL_LOW_IN = 0,
	/** PIO low means outside geofence */
	UBX_GEOFENCE_PINPOL_LOW_OUT = 1,
};

/** Select Wake-On-Motion mode */
enum ubx_hw_sens_wom_mode {
	/** Disable Wake-On-Motion feature. */
	UBX_HW_SENS_WOM_MODE_DISABLED = 0,
	/** Enable Wake-On-Motion feature on the host CPU. */
	UBX_HW_SENS_WOM_MODE_HOST = 1,
	/** Enable Wake-On-Motion feature on the receiver. */
	UBX_HW_SENS_WOM_MODE_RECEIVER = 2,
	/** Enable Wake-On-Motion feature on both host CPU and receiver. */
	UBX_HW_SENS_WOM_MODE_BOTH = 3,
};

/** Antenna supervisor engine selection */
enum ubx_hw_ant_sup_engine {
	/** Use the EXT engine (not available in all products) */
	UBX_HW_ANT_SUP_ENGINE_EXT = 0,
	/** Use the MADC engine (not available in all products) */
	UBX_HW_ANT_SUP_ENGINE_MADC = 1,
};

/** Internal LNA gain mode */
enum ubx_hw_rf_lna_mode {
	/** All RFs. Normal operation, internal LNA enabled at full gain */
	UBX_HW_RF_LNA_MODE_NORMAL = 0,
	/** All RFs. LNA enabled in low gain mode */
	UBX_HW_RF_LNA_MODE_LOWGAIN = 1,
	/** All RFs. Bypass LNA */
	UBX_HW_RF_LNA_MODE_BYPASS = 2,
};

/** Information message enable flags for the UBX protocol on the I2C interface */
enum ubx_infmsg_ubx_i2c {
	/** Enable ERROR information messages */
	UBX_INFMSG_UBX_I2C_ERROR = 0x1,
	/** Enable WARNING information messages */
	UBX_INFMSG_UBX_I2C_WARNING = 0x2,
	/** Enable NOTICE information messages */
	UBX_INFMSG_UBX_I2C_NOTICE = 0x4,
	/** Enable TEST information messages */
	UBX_INFMSG_UBX_I2C_TEST = 0x8,
	/** Enable DEBUG information messages */
	UBX_INFMSG_UBX_I2C_DEBUG = 0x10,
};

/** Information message enable flags for the UBX protocol on the UART1 interface */
enum ubx_infmsg_ubx_uart1 {
	/** Enable ERROR information messages */
	UBX_INFMSG_UBX_UART1_ERROR = 0x1,
	/** Enable WARNING information messages */
	UBX_INFMSG_UBX_UART1_WARNING = 0x2,
	/** Enable NOTICE information messages */
	UBX_INFMSG_UBX_UART1_NOTICE = 0x4,
	/** Enable TEST information messages */
	UBX_INFMSG_UBX_UART1_TEST = 0x8,
	/** Enable DEBUG information messages */
	UBX_INFMSG_UBX_UART1_DEBUG = 0x10,
};

/** Information message enable flags for the UBX protocol on the UART2 interface */
enum ubx_infmsg_ubx_uart2 {
	/** Enable ERROR information messages */
	UBX_INFMSG_UBX_UART2_ERROR = 0x1,
	/** Enable WARNING information messages */
	UBX_INFMSG_UBX_UART2_WARNING = 0x2,
	/** Enable NOTICE information messages */
	UBX_INFMSG_UBX_UART2_NOTICE = 0x4,
	/** Enable TEST information messages */
	UBX_INFMSG_UBX_UART2_TEST = 0x8,
	/** Enable DEBUG information messages */
	UBX_INFMSG_UBX_UART2_DEBUG = 0x10,
};

/** Information message enable flags for the UBX protocol on the USB interface */
enum ubx_infmsg_ubx_usb {
	/** Enable ERROR information messages */
	UBX_INFMSG_UBX_USB_ERROR = 0x1,
	/** Enable WARNING information messages */
	UBX_INFMSG_UBX_USB_WARNING = 0x2,
	/** Enable NOTICE information messages */
	UBX_INFMSG_UBX_USB_NOTICE = 0x4,
	/** Enable TEST information messages */
	UBX_INFMSG_UBX_USB_TEST = 0x8,
	/** Enable DEBUG information messages */
	UBX_INFMSG_UBX_USB_DEBUG = 0x10,
};

/** Information message enable flags for the UBX protocol on the SPI interface */
enum ubx_infmsg_ubx_spi {
	/** Enable ERROR information messages */
	UBX_INFMSG_UBX_SPI_ERROR = 0x1,
	/** Enable WARNING information messages */
	UBX_INFMSG_UBX_SPI_WARNING = 0x2,
	/** Enable NOTICE information messages */
	UBX_INFMSG_UBX_SPI_NOTICE = 0x4,
	/** Enable TEST information messages */
	UBX_INFMSG_UBX_SPI_TEST = 0x8,
	/** Enable DEBUG information messages */
	UBX_INFMSG_UBX_SPI_DEBUG = 0x10,
};

/** Information message enable flags for the NMEA protocol on the I2C interface */
enum ubx_infmsg_nmea_i2c {
	/** Enable ERROR information messages */
	UBX_INFMSG_NMEA_I2C_ERROR = 0x1,
	/** Enable WARNING information messages */
	UBX_INFMSG_NMEA_I2C_WARNING = 0x2,
	/** Enable NOTICE information messages */
	UBX_INFMSG_NMEA_I2C_NOTICE = 0x4,
	/** Enable TEST information messages */
	UBX_INFMSG_NMEA_I2C_TEST = 0x8,
	/** Enable DEBUG information messages */
	UBX_INFMSG_NMEA_I2C_DEBUG = 0x10,
};

/** Information message enable flags for the NMEA protocol on the UART1 interface */
enum ubx_infmsg_nmea_uart1 {
	/** Enable ERROR information messages */
	UBX_INFMSG_NMEA_UART1_ERROR = 0x1,
	/** Enable WARNING information messages */
	UBX_INFMSG_NMEA_UART1_WARNING = 0x2,
	/** Enable NOTICE information messages */
	UBX_INFMSG_NMEA_UART1_NOTICE = 0x4,
	/** Enable TEST information messages */
	UBX_INFMSG_NMEA_UART1_TEST = 0x8,
	/** Enable DEBUG information messages */
	UBX_INFMSG_NMEA_UART1_DEBUG = 0x10,
};

/** Information message enable flags for the NMEA protocol on the UART2 interface */
enum ubx_infmsg_nmea_uart2 {
	/** Enable ERROR information messages */
	UBX_INFMSG_NMEA_UART2_ERROR = 0x1,
	/** Enable WARNING information messages */
	UBX_INFMSG_NMEA_UART2_WARNING = 0x2,
	/** Enable NOTICE information messages */
	UBX_INFMSG_NMEA_UART2_NOTICE = 0x4,
	/** Enable TEST information messages */
	UBX_INFMSG_NMEA_UART2_TEST = 0x8,
	/** Enable DEBUG information messages */
	UBX_INFMSG_NMEA_UART2_DEBUG = 0x10,
};

/** Information message enable flags for the NMEA protocol on the USB interface */
enum ubx_infmsg_nmea_usb {
	/** Enable ERROR information messages */
	UBX_INFMSG_NMEA_USB_ERROR = 0x1,
	/** Enable WARNING information messages */
	UBX_INFMSG_NMEA_USB_WARNING = 0x2,
	/** Enable NOTICE information messages */
	UBX_INFMSG_NMEA_USB_NOTICE = 0x4,
	/** Enable TEST information messages */
	UBX_INFMSG_NMEA_USB_TEST = 0x8,
	/** Enable DEBUG information messages */
	UBX_INFMSG_NMEA_USB_DEBUG = 0x10,
};

/** Information message enable flags for the NMEA protocol on the SPI interface */
enum ubx_infmsg_nmea_spi {
	/** Enable ERROR information messages */
	UBX_INFMSG_NMEA_SPI_ERROR = 0x1,
	/** Enable WARNING information messages */
	UBX_INFMSG_NMEA_SPI_WARNING = 0x2,
	/** Enable NOTICE information messages */
	UBX_INFMSG_NMEA_SPI_NOTICE = 0x4,
	/** Enable TEST information messages */
	UBX_INFMSG_NMEA_SPI_TEST = 0x8,
	/** Enable DEBUG information messages */
	UBX_INFMSG_NMEA_SPI_DEBUG = 0x10,
};

/** ITFM antenna setting */
enum ubx_itfm_antsetting {
	/** Unknown */
	UBX_ITFM_ANTSETTING_UNKNOWN = 0,
	/** Passive */
	UBX_ITFM_ANTSETTING_PASSIVE = 1,
	/** Active */
	UBX_ITFM_ANTSETTING_ACTIVE = 2,
};

/** NMEA protocol version */
enum ubx_nmea_protver {
	/** NMEA protocol version 2.1 */
	UBX_NMEA_PROTVER_V21 = 21,
	/** NMEA protocol version 2.3 */
	UBX_NMEA_PROTVER_V23 = 23,
	/** NMEA protocol version 4.0 (not available in all products) */
	UBX_NMEA_PROTVER_V40 = 40,
	/** NMEA protocol version 4.10 (not available in all products) */
	UBX_NMEA_PROTVER_V41 = 41,
	/** NMEA protocol version 4.11 (not available in all products) */
	UBX_NMEA_PROTVER_V411 = 42,
};

/** Maximum number of SVs to report per Talker ID */
enum ubx_nmea_maxsvs {
	/** Unlimited */
	UBX_NMEA_MAXSVS_UNLIM = 0,
	/** 8 SVs */
	UBX_NMEA_MAXSVS_8SVS = 8,
	/** 12 SVs */
	UBX_NMEA_MAXSVS_12SVS = 12,
	/** 16 SVs */
	UBX_NMEA_MAXSVS_16SVS = 16,
};

/** Display configuration for SVs that do not have value defined in NMEA */
enum ubx_nmea_svnumbering {
	/** Strict - satellites are not output */
	UBX_NMEA_SVNUMBERING_STRICT = 0,
	/** Extended - use proprietary numbering */
	UBX_NMEA_SVNUMBERING_EXTENDED = 1,
};

/** Main Talker ID */
enum ubx_nmea_maintalkerid {
	/** Main Talker ID is not overridden */
	UBX_NMEA_MAINTALKERID_AUTO = 0,
	/** Set main Talker ID to 'GP' */
	UBX_NMEA_MAINTALKERID_GP = 1,
	/** Set main Talker ID to 'GL' */
	UBX_NMEA_MAINTALKERID_GL = 2,
	/** Set main Talker ID to 'GN' */
	UBX_NMEA_MAINTALKERID_GN = 3,
	/** Set main Talker ID to 'GA' (not available in all products) */
	UBX_NMEA_MAINTALKERID_GA = 4,
	/** Set main Talker ID to 'GB' (not available in all products) */
	UBX_NMEA_MAINTALKERID_GB = 5,
	/** Set main Talker ID to 'GQ' (not available in all products) */
	UBX_NMEA_MAINTALKERID_GQ = 7,
};

/** Talker ID for GSV NMEA messages */
enum ubx_nmea_gsvtalkerid {
	/** Use GNSS-specific Talker ID (as defined by NMEA) */
	UBX_NMEA_GSVTALKERID_GNSS = 0,
	/** Use the main Talker ID */
	UBX_NMEA_GSVTALKERID_MAIN = 1,
};

/** Odometer profile configuration */
enum ubx_odo_profile {
	/** Running */
	UBX_ODO_PROFILE_RUN = 0,
	/** Cycling */
	UBX_ODO_PROFILE_CYCL = 1,
	/** Swimming */
	UBX_ODO_PROFILE_SWIM = 2,
	/** Car */
	UBX_ODO_PROFILE_CAR = 3,
	/** Custom */
	UBX_ODO_PROFILE_CUSTOM = 4,
};

/** PM general mode of operation */
enum ubx_pm_operatemode {
	/** normal operation, no power save mode active */
	UBX_PM_OPERATEMODE_FULL = 0,
	/** Long Interval Tracking */
	UBX_PM_OPERATEMODE_PSMOO = 1,
	/** LEAP */
	UBX_PM_OPERATEMODE_PSMCT = 2,
};

/** EXTINT pin selection */
enum ubx_pm_extintsel {
	/** EXTINT0 Pin */
	UBX_PM_EXTINTSEL_EXTINT0 = 0,
	/** EXTINT1 Pin */
	UBX_PM_EXTINTSEL_EXTINT1 = 1,
};

/** PMP data rate configuration */
enum ubx_pmp_data_rate {
	/** 600 bps */
	UBX_PMP_DATA_RATE_B600 = 600,
	/** 1200 bps */
	UBX_PMP_DATA_RATE_B1200 = 1200,
	/** 2400 bps */
	UBX_PMP_DATA_RATE_B2400 = 2400,
	/** 4800 bps */
	UBX_PMP_DATA_RATE_B4800 = 4800,
};

/** QZSS L6 messages to be decoded by channel A */
enum ubx_qzss_l6_msga {
	/** L6D messages */
	UBX_QZSS_L6_MSGA_L6D = 0,
	/** L6E messages */
	UBX_QZSS_L6_MSGA_L6E = 1,
};

/** QZSS L6 messages to be decoded by channel B */
enum ubx_qzss_l6_msgb {
	/** L6D messages */
	UBX_QZSS_L6_MSGB_L6D = 0,
	/** L6E messages */
	UBX_QZSS_L6_MSGB_L6E = 1,
};

/** QZSS L6 message Reed-Solomon decoder mode */
enum ubx_qzss_l6_rsdecoder {
	/** Disabled, received messages are output with unknown bit error status */
	UBX_QZSS_L6_RSDECODER_DISABLED = 0,
	/** Error detection, RS-decoder detects bit errors in received messages */
	UBX_QZSS_L6_RSDECODER_ERRDETECT = 1,
	/** Error correction, RS-decoder detects and corrects bit errors in received messages */
	UBX_QZSS_L6_RSDECODER_ERRCORRECT = 2,
};

/** RTCM input filter configuration based on RTCM DF003 (Reference station ID) value */
enum ubx_rtcm_df003_in_filter {
	/** Disabled RTCM input filter; all input messages allowed */
	UBX_RTCM_DF003_IN_FILTER_DISABLED = 0,
	/** Relaxed RTCM input filter; input messages allowed must contain a DF003 data field
	 *  matching the CFG-RTCM-DF003_IN value or not contain by specification the DF003
	 *  data field
	 */
	UBX_RTCM_DF003_IN_FILTER_RELAXED = 1,
	/** Strict RTCM input filter; input messages allowed must contain a DF003 data field
	 *  matching the CFG-RTCM-DF003 value
	 */
	UBX_RTCM_DF003_IN_FILTER_STRICT = 2,
};

/** Accept corrections from SBAS SV, even if not self included in PRN MASK */
enum ubx_sbas_accept_not_in_prnmask {
	/** WAAS bit */
	UBX_SBAS_ACCEPT_NOT_IN_PRNMASK_WAAS = 0x1,
	/** EGNOS bit */
	UBX_SBAS_ACCEPT_NOT_IN_PRNMASK_EGNOS = 0x2,
	/** MSAS bit */
	UBX_SBAS_ACCEPT_NOT_IN_PRNMASK_MSAS = 0x4,
	/** GAGAN bit */
	UBX_SBAS_ACCEPT_NOT_IN_PRNMASK_GAGAN = 0x8,
	/** SDCM bit */
	UBX_SBAS_ACCEPT_NOT_IN_PRNMASK_SDCM = 0x10,
	/** BDSBAS bit */
	UBX_SBAS_ACCEPT_NOT_IN_PRNMASK_BDSBAS = 0x20,
	/** KASS bit */
	UBX_SBAS_ACCEPT_NOT_IN_PRNMASK_KASS = 0x40,
};

/** User-defined IMU mount alignment angles tolerance level */
enum ubx_sfimu_imu_mntalg_tolerance {
	/** Low tolerance to user-defined IMU alignment angles, error less than 2deg */
	UBX_SFIMU_IMU_MNTALG_TOLERANCE_LOW = 0,
	/** High tolerance to user-defined IMU alignment angles, error less than 10deg */
	UBX_SFIMU_IMU_MNTALG_TOLERANCE_HIGH = 1,
};

/** Active signal plan */
enum ubx_signal_plan {
	/** Signal plan 1 */
	UBX_SIGNAL_PLAN_SP1 = 0x1,
	/** Signal plan 2 */
	UBX_SIGNAL_PLAN_SP2 = 0x2,
	/** Signal plan 3 */
	UBX_SIGNAL_PLAN_SP3 = 0x3,
	/** Signal plan 4 */
	UBX_SIGNAL_PLAN_SP4 = 0x4,
	/** Signal plan 5 */
	UBX_SIGNAL_PLAN_SP5 = 0x5,
	/** Signal plan 6 */
	UBX_SIGNAL_PLAN_SP6 = 0x6,
	/** Signal plan 7 */
	UBX_SIGNAL_PLAN_SP7 = 0x7,
	/** Signal plan 8 */
	UBX_SIGNAL_PLAN_SP8 = 0x8,
	/** Signal plan 9 */
	UBX_SIGNAL_PLAN_SP9 = 0x9,
	/** Signal plan 10 */
	UBX_SIGNAL_PLAN_SP10 = 0xa,
};

/** Selector for source SPARTN stream */
enum ubx_spartn_use_source {
	/** IP source (default) */
	UBX_SPARTN_USE_SOURCE_IP = 0x0,
	/** L-Band source */
	UBX_SPARTN_USE_SOURCE_LBAND = 0x1,
};

/** Timing receiver mode */
enum ubx_tmode_mode {
	/** Disabled */
	UBX_TMODE_MODE_DISABLED = 0,
	/** Survey in */
	UBX_TMODE_MODE_SURVEY_IN = 1,
	/** Fixed mode (true ARP position information required) */
	UBX_TMODE_MODE_FIXED = 2,
};

/** Determines the ARP position type */
enum ubx_tmode_pos_type {
	/** Position is ECEF */
	UBX_TMODE_POS_TYPE_ECEF = 0,
	/** Position is Lat/Lon/Height */
	UBX_TMODE_POS_TYPE_LLH = 1,
};

/** Determines how the time pulse is interpreted */
enum ubx_tp_pulse_def {
	/** Time pulse period [us] */
	UBX_TP_PULSE_DEF_PERIOD = 0,
	/** Time pulse frequency [Hz] */
	UBX_TP_PULSE_DEF_FREQ = 1,
};

/** Determines how the time pulse length is interpreted */
enum ubx_tp_pulse_length_def {
	/** Time pulse ratio */
	UBX_TP_PULSE_LENGTH_DEF_RATIO = 0,
	/** Time pulse length */
	UBX_TP_PULSE_LENGTH_DEF_LENGTH = 1,
};

/** Time grid to use for TP1 */
enum ubx_tp_timegrid_tp1 {
	/** UTC time reference */
	UBX_TP_TIMEGRID_TP1_UTC = 0,
	/** GPS time reference */
	UBX_TP_TIMEGRID_TP1_GPS = 1,
	/** GLONASS time reference */
	UBX_TP_TIMEGRID_TP1_GLO = 2,
	/** BeiDou time reference */
	UBX_TP_TIMEGRID_TP1_BDS = 3,
	/** Galileo time reference */
	UBX_TP_TIMEGRID_TP1_GAL = 4,
	/** NavIC time reference */
	UBX_TP_TIMEGRID_TP1_NAVIC = 5,
};

/** Time grid to use for TP2 */
enum ubx_tp_timegrid_tp2 {
	/** UTC time reference */
	UBX_TP_TIMEGRID_TP2_UTC = 0,
	/** GPS time reference */
	UBX_TP_TIMEGRID_TP2_GPS = 1,
	/** GLONASS time reference */
	UBX_TP_TIMEGRID_TP2_GLO = 2,
	/** BeiDou time reference */
	UBX_TP_TIMEGRID_TP2_BDS = 3,
	/** Galileo time reference */
	UBX_TP_TIMEGRID_TP2_GAL = 4,
	/** NavIC time reference */
	UBX_TP_TIMEGRID_TP2_NAVIC = 5,
};

/** Set drive strength of TP1 */
enum ubx_tp_drstr_tp1 {
	/** 2 mA drive strength */
	UBX_TP_DRSTR_TP1_DRIVE_STRENGTH_2MA = 0,
	/** 4 mA drive strength */
	UBX_TP_DRSTR_TP1_DRIVE_STRENGTH_4MA = 1,
	/** 8 mA drive strength */
	UBX_TP_DRSTR_TP1_DRIVE_STRENGTH_8MA = 2,
	/** 12 mA drive strength */
	UBX_TP_DRSTR_TP1_DRIVE_STRENGTH_12MA = 3,
};

/** Set drive strength of TP2 */
enum ubx_tp_drstr_tp2 {
	/** 2 mA drive strength */
	UBX_TP_DRSTR_TP2_DRIVE_STRENGTH_2MA = 0,
	/** 4 mA drive strength */
	UBX_TP_DRSTR_TP2_DRIVE_STRENGTH_4MA = 1,
	/** 8 mA drive strength */
	UBX_TP_DRSTR_TP2_DRIVE_STRENGTH_8MA = 2,
	/** 12 mA drive strength */
	UBX_TP_DRSTR_TP2_DRIVE_STRENGTH_12MA = 3,
};

/** Interface where the TX ready feature should be linked to */
enum ubx_txready_interface {
	/** I2C interface */
	UBX_TXREADY_INTERFACE_I2C = 0,
	/** SPI interface */
	UBX_TXREADY_INTERFACE_SPI = 1,
};

/** Number of stopbits that should be used on UART1 */
enum ubx_uart1_stopbits {
	/** 0.5 stopbits */
	UBX_UART1_STOPBITS_HALF = 0,
	/** 1.0 stopbits */
	UBX_UART1_STOPBITS_ONE = 1,
	/** 1.5 stopbits */
	UBX_UART1_STOPBITS_ONEHALF = 2,
	/** 2.0 stopbits */
	UBX_UART1_STOPBITS_TWO = 3,
};

/** Number of databits that should be used on UART1 */
enum ubx_uart1_databits {
	/** 8 databits */
	UBX_UART1_DATABITS_EIGHT = 0,
	/** 7 databits */
	UBX_UART1_DATABITS_SEVEN = 1,
};

/** Parity mode that should be used on UART1 */
enum ubx_uart1_parity {
	/** No parity bit */
	UBX_UART1_PARITY_NONE = 0,
	/** Add an odd parity bit */
	UBX_UART1_PARITY_ODD = 1,
	/** Add an even parity bit */
	UBX_UART1_PARITY_EVEN = 2,
};

/** Number of stopbits that should be used on UART2 */
enum ubx_uart2_stopbits {
	/** 0.5 stopbits */
	UBX_UART2_STOPBITS_HALF = 0,
	/** 1.0 stopbits */
	UBX_UART2_STOPBITS_ONE = 1,
	/** 1.5 stopbits */
	UBX_UART2_STOPBITS_ONEHALF = 2,
	/** 2.0 stopbits */
	UBX_UART2_STOPBITS_TWO = 3,
};

/** Number of databits that should be used on UART2 */
enum ubx_uart2_databits {
	/** 8 databits */
	UBX_UART2_DATABITS_EIGHT = 0,
	/** 7 databits */
	UBX_UART2_DATABITS_SEVEN = 1,
};

/** Parity mode that should be used on UART2 */
enum ubx_uart2_parity {
	/** No parity bit */
	UBX_UART2_PARITY_NONE = 0,
	/** Add an odd parity bit */
	UBX_UART2_PARITY_ODD = 1,
	/** Add an even parity bit */
	UBX_UART2_PARITY_EVEN = 2,
};

#endif /**< ZEPHYR_MODEM_UBX_KEYS_ */
