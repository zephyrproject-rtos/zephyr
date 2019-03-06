/*
 * wlan.c - CC31xx/CC32xx Host Driver Implementation
 *
 * Copyright (C) 2017 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/


/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/
#include <ti/drivers/net/wifi/simplelink.h>


extern SlWifiCC32XXConfig_t SimpleLinkWifiCC32XX_config;

_i32 sl_WifiConfig()
{
     _u8                              ucConfigOpt;
     _u16                             uIPMode;
     _i32                             RetVal = -1;
     _i32                             RetVal_stop = -1;
     _i32                             Mode = -1;
     SlNetCfgIpV4Args_t               *ipV4 = NULL;
     SlNetCfgIpV4Args_t               localIpV4;
     _u16                             ipConfigSize = 0;


     /* Turn NWP on */
     Mode = sl_Start(NULL, NULL, NULL);
     if (Mode < 0)
     {
         return Mode;
     }

    while (1)
    {
        if ((SimpleLinkWifiCC32XX_config.Mode != SL_DEVICE_SYSCONFIG_AS_CONFIGURED) && (SimpleLinkWifiCC32XX_config.Mode != Mode))
        {
            /* Set NWP role */
            RetVal = sl_WlanSetMode(SimpleLinkWifiCC32XX_config.Mode);
            if (RetVal < 0)
            {
                break;
            }

            /* For changes to take affect, we restart the NWP */
            RetVal = sl_Stop(200);
            if (RetVal < 0)
            {
                break;
            }

            RetVal = sl_Start(NULL, NULL, NULL);
            if (RetVal < 0)
            {
                break;
            }

            if(SimpleLinkWifiCC32XX_config.Mode != RetVal)
            {
                RetVal = -1;
                break;
            }
        }

        /* Set connection policy */
        if (SimpleLinkWifiCC32XX_config.ConnectionPolicy != SL_DEVICE_SYSCONFIG_AS_CONFIGURED)
        {
            RetVal =
                sl_WlanPolicySet(SL_WLAN_POLICY_CONNECTION,
                                 SimpleLinkWifiCC32XX_config.ConnectionPolicy,
                                 NULL,0);
            if (RetVal < 0)
            {
                break;
            }
        }
        if (SimpleLinkWifiCC32XX_config.ProvisioningStop)
        {
            /* Stop Provisioning */
            RetVal = sl_WlanProvisioning(SL_WLAN_PROVISIONING_CMD_STOP,
                                         0xFF,
                                         0,
                                         NULL,
                                         0x0);
            if (RetVal < 0)
            {
                break;
            }
        }
        if (SimpleLinkWifiCC32XX_config.DeleteAllProfile)
        {
            /* Delete existing profiles */
            RetVal = sl_WlanProfileDel(SL_WLAN_DEL_ALL_PROFILES);
            if (RetVal < 0)
            {
                break;
            }
        }

        /* Configure ipv4/DHCP */
        if (SimpleLinkWifiCC32XX_config.Ipv4Config != SL_DEVICE_SYSCONFIG_AS_CONFIGURED)
        {
            if (SimpleLinkWifiCC32XX_config.Mode == ROLE_STA)
            {
                uIPMode = SL_NETCFG_IPV4_STA_ADDR_MODE;
            }
            else if (SimpleLinkWifiCC32XX_config.Mode == ROLE_AP)
            {
                uIPMode = SL_NETCFG_IPV4_AP_ADDR_MODE;
            }
            if (SimpleLinkWifiCC32XX_config.Ipv4Config == SL_NETCFG_ADDR_STATIC)
            {
                ipV4 = &localIpV4;
                ipConfigSize = sizeof(SlNetCfgIpV4Args_t);

                localIpV4.Ip          = (_u32)SimpleLinkWifiCC32XX_config.Ipv4;                // _u32 IP address
                localIpV4.IpMask      = (_u32)SimpleLinkWifiCC32XX_config.IpMask;              // _u32 Subnet mask for this AP/P2P
                localIpV4.IpGateway   = (_u32)SimpleLinkWifiCC32XX_config.IpGateway;           // _u32 Default gateway address
                localIpV4.IpDnsServer = (_u32)SimpleLinkWifiCC32XX_config.IpDnsServer;         // _u32 DNS server address
            }
            else if (SimpleLinkWifiCC32XX_config.Ipv4Config != SL_NETCFG_ADDR_DHCP)
            {
                RetVal = -1;
                if (RetVal < 0)
                {
                    break;
                }
            }

            RetVal = sl_NetCfgSet(uIPMode, SimpleLinkWifiCC32XX_config.Ipv4Config, ipConfigSize, (_u8 *)ipV4);
            if (RetVal < 0)
            {
                break;
            }
        }

        /* Set scan policy */
        if (SimpleLinkWifiCC32XX_config.ScanPolicy != SL_DEVICE_SYSCONFIG_AS_CONFIGURED)
        {
            _u32 intervalInSeconds = SimpleLinkWifiCC32XX_config.ScanIntervalInSeconds;
            ucConfigOpt = SimpleLinkWifiCC32XX_config.ScanPolicy;
            RetVal = sl_WlanPolicySet(SL_WLAN_POLICY_SCAN, ucConfigOpt, (_u8 *)&intervalInSeconds, sizeof(intervalInSeconds));
            if (RetVal < 0)
            {
                break;
            }
        }

        /* Set NWP Power Management Policy */
        if (SimpleLinkWifiCC32XX_config.PMPolicy != SL_DEVICE_SYSCONFIG_AS_CONFIGURED)
        {
            if (SimpleLinkWifiCC32XX_config.PMPolicy != SL_WLAN_LONG_SLEEP_INTERVAL_POLICY)
            {
                RetVal = sl_WlanPolicySet(SL_WLAN_POLICY_PM, SimpleLinkWifiCC32XX_config.PMPolicy, NULL,0);
                if (RetVal < 0)
                {
                    break;
                }
            }
            else
            {
                SlWlanPmPolicyParams_t PmPolicyParams;
                memset(&PmPolicyParams,0,sizeof(SlWlanPmPolicyParams_t));
                PmPolicyParams.MaxSleepTimeMs = SimpleLinkWifiCC32XX_config.MaxSleepTimeMS;  //max sleep time in mSec
                RetVal = sl_WlanPolicySet(SL_WLAN_POLICY_PM, SimpleLinkWifiCC32XX_config.PMPolicy, (_u8*)&PmPolicyParams, sizeof(PmPolicyParams));
                if (RetVal < 0)
                {
                    break;
                }
            }
        }
        
        /* Set DHCP Server Configuration */
        if (SimpleLinkWifiCC32XX_config.DHCPServer)
        {
            SlNetAppDhcpServerBasicOpt_t dhcpParams; 
            _u8 outLen = sizeof(SlNetAppDhcpServerBasicOpt_t); 
            dhcpParams.lease_time      = SimpleLinkWifiCC32XX_config.LeaseTime;           // lease time (in seconds) of the IP Address
            dhcpParams.ipv4_addr_start = (_u32)SimpleLinkWifiCC32XX_config.StartAddress;  // first IP Address for allocation. IP Address should be set as Hex number - i.e. 0A0B0C01 for (10.11.12.1)
            dhcpParams.ipv4_addr_last  = (_u32)SimpleLinkWifiCC32XX_config.LastAddress;   // last IP Address for allocation. IP Address should be set as Hex number - i.e. 0A0B0C01 for (10.11.12.1)
            RetVal = sl_NetAppStop(SL_NETAPP_DHCP_SERVER_ID);                             // Stop DHCP server before settings
            if (RetVal < 0)
            {
                break;
            }
            RetVal = sl_NetAppSet(SL_NETAPP_DHCP_SERVER_ID, SL_NETAPP_DHCP_SRV_BASIC_OPT, outLen, (_u8* )&dhcpParams);  // set parameters
            if (RetVal < 0)
            {
                break;
            }
            if (SimpleLinkWifiCC32XX_config.Mode == ROLE_AP)
            {
                RetVal = sl_NetAppStart(SL_NETAPP_DHCP_SERVER_ID);                  // Start DHCP server with new settings
                if (RetVal < 0)
                {
                    break;
                }
            }
        }
        else
        {
            sl_NetAppStop(SL_NETAPP_HTTP_SERVER_ID);
        }
        break;
    }
    /* Jump here if error occurred or after all the configurations was set successfully */  
    /* For changes to take affect, we restart the NWP - sl_start will be call by application */
    RetVal_stop = sl_Stop(200);
    if (RetVal_stop < 0)
    {
        return RetVal_stop;
    }
    return(RetVal);
}

