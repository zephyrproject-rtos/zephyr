#ifndef _FSL_XCVR_H_
#define _FSL_XCVR_H_

#include "MKW40Z4.h"
#include "KW4xXcvrDrv.h"

#define Radio_1_IRQn ZigBee_IRQn
#define ZIGBEE_MODE ZIGBEE
#define XCVR_TSM XCVR
#define ZLL_RX_FRAME_FILTER_FRM_VER_FILTER_MASK ZLL_RX_FRAME_FILTER_FRM_VER_MASK
#define ZLL_RX_FRAME_FILTER_FRM_VER_FILTER(x) ZLL_RX_FRAME_FILTER_FRM_VER(x)
#define XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_MASK XCVR_END_OF_SEQ_END_OF_RX_WU_MASK
#define XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_SHIFT XCVR_END_OF_SEQ_END_OF_RX_WU_SHIFT
#define XCVR_TSM_END_OF_SEQ_END_OF_TX_WU_MASK XCVR_END_OF_SEQ_END_OF_TX_WU_MASK
#define XCVR_TSM_END_OF_SEQ_END_OF_TX_WU_SHIFT XCVR_END_OF_SEQ_END_OF_TX_WU_SHIFT

/*! @brief  Data rate selections. Imported from MKW41Z4. */
typedef enum _data_rate
{
	DR_1MBPS = 0, /* Must match bit assignment in BITRATE field */
	DR_500KBPS = 1, /* Must match bit assignment in BITRATE field */
	DR_250KBPS = 2, /* Must match bit assignment in BITRATE field */
#if RADIO_IS_GEN_3P0
	DR_2MBPS = 3, /* Must match bit assignment in BITRATE field */
#endif /* RADIO_IS_GEN_3P0 */
	DR_UNASSIGNED = 4, /* Must match bit assignment in BITRATE field */
} data_rate_t;

xcvrStatus_t XCVR_Init(radio_mode_t radio_mode, data_rate_t data_rate)
{
	XcvrInit(radio_mode);

	return gXcvrSuccess_c;
}

#endif /* _FSL_XCVR_H_ */
