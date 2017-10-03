/*
 * netdb.c - CC31xx/CC32xx Host Driver Implementation
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
#include <ti/drivers/net/wifi/bsd/netdb.h>
#include <ti/drivers/net/wifi/bsd/sys/socket.h>
#include <ti/drivers/net/wifi/simplelink.h>

/*******************************************************************************/
/*  gethostbyname */
/*******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_NetAppDnsGetHostByName)
struct hostent* gethostbyname(const char *name)
{
    static struct hostent Hostent;
    static char* AddrArray[2];
    static char Addr[IPv6_ADDR_LEN];

    int RetVal = 0;

    /* Clear the reused buffer */
    _SlDrvMemZero(&Hostent, sizeof(struct hostent));
    _SlDrvMemZero(&AddrArray, sizeof(AddrArray));
    _SlDrvMemZero(&Addr, sizeof(Addr));

    /* Set the host name */
    Hostent.h_name = (char*)name;
    /* Query DNS for IPv4 address. */
    RetVal = sl_NetAppDnsGetHostByName((signed char*)name , (_u16)strlen(name), (unsigned long*)(&Addr), AF_INET);

    if(RetVal < 0)
    {
        /* If call fails, try again for IPv6. */
        RetVal = sl_NetAppDnsGetHostByName((signed char*)name , (_u16)strlen(name), (unsigned long*)(&Addr), AF_INET6);
        if(RetVal < 0)
        {
            /* if the request failed twice, there's an error - return NULL. */
            return NULL;
        }
        else
        {
            /* fill the answer fields */
            Hostent.h_addrtype = AF_INET6 ;
            Hostent.h_length = IPv6_ADDR_LEN;
        }
    }
    else
    {
        /* fill the answer fields */
        Hostent.h_addrtype = AF_INET ;
        Hostent.h_length = IPv4_ADDR_LEN;
    }

    AddrArray[0] = &Addr[0];
    Hostent.h_addr_list = AddrArray;

    /* Return the address of the reused buffer */
    return (&Hostent);
}
#endif
