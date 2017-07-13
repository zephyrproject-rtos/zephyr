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
