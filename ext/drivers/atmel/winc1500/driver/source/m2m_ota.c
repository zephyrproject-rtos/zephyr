/**
 *
 * \file
 *
 * \brief NMC1500 IoT OTA Interface.
 *
 * Copyright (c) 2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */



/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
INCLUDES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
#include "common/include/nm_common.h"
#include "driver/include/m2m_types.h"
#include "driver/include/m2m_ota.h"
#include "driver/source/m2m_hif.h"
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
MACROS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
DATA TYPES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
static tpfOtaUpdateCb gpfOtaUpdateCb = NULL;
static tpfOtaNotifCb  gpfOtaNotifCb = NULL;


/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
FUNCTION PROTOTYPES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
/**
*	@fn			m2m_wifi_cb(uint8 u8OpCode, uint16 u16DataSize, uint32 u32Addr, uint8 grp)
*	@brief		WiFi call back function
*	@param [in]	u8OpCode
*					HIF Opcode type.
*	@param [in]	u16DataSize
*					HIF data length.
*	@param [in]	u32Addr
*					HIF address.
*	@param [in]	grp
*					HIF group type.
*	@author
*	@date
*	@version	1.0
*/
static void m2m_ota_cb(uint8 u8OpCode, uint16 u16DataSize, uint32 u32Addr)
{
	sint8 ret = M2M_SUCCESS;
	if(u8OpCode == M2M_OTA_RESP_NOTIF_UPDATE_INFO)
	{
		tstrOtaUpdateInfo strOtaUpdateInfo;
		m2m_memset((uint8*)&strOtaUpdateInfo,0,sizeof(tstrOtaUpdateInfo));
		ret = hif_receive(u32Addr,(uint8*)&strOtaUpdateInfo,sizeof(tstrOtaUpdateInfo),0);
		if(ret == M2M_SUCCESS)
		{
			if(gpfOtaNotifCb)
				gpfOtaNotifCb(&strOtaUpdateInfo);
		}
	}
	else if (u8OpCode == M2M_OTA_RESP_UPDATE_STATUS)
	{
		tstrOtaUpdateStatusResp strOtaUpdateStatusResp;
		m2m_memset((uint8*)&strOtaUpdateStatusResp,0,sizeof(tstrOtaUpdateStatusResp));
		ret = hif_receive(u32Addr, (uint8*) &strOtaUpdateStatusResp,sizeof(tstrOtaUpdateStatusResp), 0);
		if(ret == M2M_SUCCESS)
		{
			if(gpfOtaUpdateCb)
				gpfOtaUpdateCb(strOtaUpdateStatusResp.u8OtaUpdateStatusType,strOtaUpdateStatusResp.u8OtaUpdateStatus);
		}
	}
	else
	{
		M2M_ERR("Invaild OTA resp %d ?\n",u8OpCode);
	}

}
/*!
@fn	\
	NMI_API sint8  m2m_ota_init(tpfOtaUpdateCb pfOtaUpdateCb, tpfOtaNotifCb pfOtaNotifCb);

@brief
	Initialize the OTA layer.

@param [in]	pfOtaUpdateCb
				OTA Update callback function

@param [in]	pfOtaNotifCb
				OTA notify callback function

@return
	The function SHALL return 0 for success and a negative value otherwise.
*/
NMI_API sint8  m2m_ota_init(tpfOtaUpdateCb pfOtaUpdateCb, tpfOtaNotifCb pfOtaNotifCb)
{
	sint8 ret = M2M_SUCCESS;

	if(pfOtaUpdateCb){
		gpfOtaUpdateCb = pfOtaUpdateCb;
	}else{
		M2M_ERR("Invaild Ota update cb\n");
	}
	if(pfOtaNotifCb){
		gpfOtaNotifCb = pfOtaNotifCb;
	}else{
		M2M_ERR("Invaild Ota notify cb\n");
	}

	hif_register_cb(M2M_REQ_GROUP_OTA,m2m_ota_cb);

	return ret;
}
/*!
@fn	\
	NMI_API sint8  m2m_ota_notif_set_url(uint8 * u8Url);

@brief
	Set the OTA url

@param [in]	u8Url
			 The url server address

@return
	The function SHALL return 0 for success and a negative value otherwise.
*/
NMI_API sint8  m2m_ota_notif_set_url(uint8 * u8Url)
{
	sint8 ret = M2M_SUCCESS;
	uint16 u16UrlSize = m2m_strlen(u8Url) + 1;
	/*Todo: we may change it to data pkt but we need to give it higer priority
			but the priorty is not implemnted yet in data pkt
	*/
	ret = hif_send(M2M_REQ_GROUP_OTA,M2M_OTA_REQ_START_UPDATE,u8Url,u16UrlSize,NULL,0,0);
	return ret;

}

/*!
@fn	\
	NMI_API sint8  m2m_ota_notif_check_for_update(void);

@brief
	check for ota update

@return
	The function SHALL return 0 for success and a negative value otherwise.
*/
NMI_API sint8  m2m_ota_notif_check_for_update(void)
{
	sint8 ret = M2M_SUCCESS;
	ret = hif_send(M2M_REQ_GROUP_OTA,M2M_OTA_REQ_NOTIF_CHECK_FOR_UPDATE,NULL,0,NULL,0,0);
	return ret;
}

/*!
@fn	\
	NMI_API sint8 m2m_ota_notif_sched(uint32 u32Period);

@brief
	Schedule OTA update

@param [in]	u32Period
	Period in days

@return
	The function SHALL return 0 for success and a negative value otherwise.
*/
NMI_API sint8 m2m_ota_notif_sched(uint32 u32Period)
{
	sint8 ret = M2M_SUCCESS;
	ret = hif_send(M2M_REQ_GROUP_OTA,M2M_OTA_REQ_NOTIF_CHECK_FOR_UPDATE,NULL,0,NULL,0,0);
	return ret;
}

/*!
@fn	\
	NMI_API sint8 m2m_ota_start_update(uint8 * u8DownloadUrl);

@brief
	Request OTA start update using the downloaded url

@param [in]	u8DownloadUrl
		The download firmware url, you get it from device info

@return
	The function SHALL return 0 for success and a negative value otherwise.

*/
NMI_API sint8 m2m_ota_start_update(uint8 * u8DownloadUrl)
{
	sint8 ret = M2M_SUCCESS;
	uint16 u16DurlSize = m2m_strlen(u8DownloadUrl) + 1;
	/*Todo: we may change it to data pkt but we need to give it higer priority
			but the priorty is not implemnted yet in data pkt
	*/
	ret = hif_send(M2M_REQ_GROUP_OTA,M2M_OTA_REQ_START_UPDATE,u8DownloadUrl,u16DurlSize,NULL,0,0);
	return ret;
}


/*!
@fn	\
	NMI_API sint8 m2m_ota_rollback(void);

@brief
	Request OTA Rollback image

@return
	The function SHALL return 0 for success and a negative value otherwise.
*/
NMI_API sint8 m2m_ota_rollback(void)
{
	sint8 ret = M2M_SUCCESS;
	ret = hif_send(M2M_REQ_GROUP_OTA,M2M_OTA_REQ_ROLLBACK,NULL,0,NULL,0,0);
	return ret;
}


/*!
@fn	\
	NMI_API sint8 m2m_ota_switch_firmware(void);

@brief
	Switch to the upgraded Firmware

@return
	The function SHALL return 0 for success and a negative value otherwise.
*/
NMI_API sint8 m2m_ota_switch_firmware(void)
{
	sint8 ret = M2M_SUCCESS;
	ret = hif_send(M2M_REQ_GROUP_OTA,M2M_OTA_REQ_SWITCH_FIRMWARE,NULL,0,NULL,0,0);
	return ret;
}
/*!
@fn	\
	NMI_API sint8 m2m_ota_get_firmware_version(tstrM2mRev * pstrRev);

@brief
	Get the OTA Firmware version.

@return
	The function SHALL return 0 for success and a negative value otherwise.
*/
NMI_API sint8 m2m_ota_get_firmware_version(tstrM2mRev * pstrRev)
{
	sint8 ret = M2M_SUCCESS;
	ret = hif_chip_wake();
	if(ret == M2M_SUCCESS)
	{
    	ret = nm_get_ota_firmware_info(pstrRev);
		hif_chip_sleep();
	}
	return ret;
}
#if 0
#define M2M_OTA_FILE	"../../../m2m_ota.dat"
NMI_API sint8 m2m_ota_test(void)
{
	uint32 page  = 0;
	uint8 buffer[1500];
	uint32 u32Sz = 0;
	sint8 ret = M2M_SUCCESS;
	FILE *fp =NULL;
	fp = fopen(M2M_OTA_FILE,"rb");
	if(fp)
	{
		fseek(fp, 0L, SEEK_END);
		u32Sz = ftell(fp);
		fseek(fp, 0L, SEEK_SET);

		while(u32Sz > 0)
		{
			{
				page = (rand()%1400);

				if((page<100)||(page>1400)) page  = 1400;
			}

			if(u32Sz>page)
			{
				u32Sz-=page;
			}
			else
			{
				page = u32Sz;
				u32Sz = 0;
			}
			printf("page %d\n", (int)page);
			fread(buffer,page,1,fp);
			ret = hif_send(M2M_REQ_GROUP_OTA,M2M_OTA_REQ_TEST|M2M_REQ_DATA_PKT,NULL,0,(uint8*)&buffer,page,0);
			if(ret != M2M_SUCCESS)
			{
				M2M_ERR("\n");
			}
			nm_bsp_sleep(1);
		}

	}
	else
	{
		M2M_ERR("nO err\n");
	}
	return ret;
}
#endif

