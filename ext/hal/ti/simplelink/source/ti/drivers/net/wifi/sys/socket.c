#include <ti/drivers/net/wifi/sys/socket.h>
#include <ti/drivers/net/wifi/sys/errno.h>
#include <ti/drivers/net/wifi/sys/netdb.h>
#include <ti/drivers/net/wifi/simplelink.h>

/*******************************************************************************/
/*  socket */
/*******************************************************************************/

#if _SL_INCLUDE_FUNC(sl_Socket)
int socket(int Domain, int Type, int Protocol)
{
    int RetVal = (int)sl_Socket(Domain, Type, Protocol);
    return _SlDrvSetErrno(RetVal);
}
#endif

/*******************************************************************************/
/*  bind */
/*******************************************************************************/

#if _SL_INCLUDE_FUNC(sl_Bind)
int bind(int sd, const sockaddr *addr, socklen_t addrlen)
{
    int RetVal = (int)sl_Bind(sd, addr, addrlen);
    return _SlDrvSetErrno(RetVal);
}
#endif

/*******************************************************************************/
/*  sendto */
/*******************************************************************************/

#if _SL_INCLUDE_FUNC(sl_SendTo)
ssize_t sendto(int sd, const void *pBuf, size_t Len, int flags, const sockaddr *to, socklen_t tolen)
{
    ssize_t RetVal = (ssize_t)sl_SendTo(sd, pBuf, (_i16)Len, flags, to, tolen);
    return (ssize_t)(_SlDrvSetErrno(RetVal));
}
#endif

/*******************************************************************************/
/*  recvfrom */
/*******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_RecvFrom)
ssize_t recvfrom(int sd, void *buf, _i16 Len, _i16 flags, sockaddr *from, socklen_t *fromlen)
{
    ssize_t RetVal = (ssize_t)sl_RecvFrom(sd, buf, Len, flags, from, fromlen);
    return (ssize_t)(_SlDrvSetErrno(RetVal));
}
#endif

/*******************************************************************************/
/*  connect */
/*******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_Connect)
int connect(int sd, const sockaddr *addr, socklen_t addrlen)
{
    int RetVal = (int)sl_Connect(sd, addr, addrlen);
    return _SlDrvSetErrno(RetVal);
}
#endif

/*******************************************************************************/
/*  send */
/*******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_Send)
ssize_t send(int sd, const void *pBuf, _i16 Len, _i16 flags)
{
    ssize_t RetVal = (ssize_t)sl_Send(sd, pBuf, Len, flags);
    return (ssize_t)(_SlDrvSetErrno(RetVal));
}
#endif

/*******************************************************************************/
/*  listen */
/*******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_Listen)
int listen(int sd, int backlog)
{
    int RetVal = (int)sl_Listen(sd, backlog);
    return _SlDrvSetErrno(RetVal);
}
#endif

/*******************************************************************************/
/*  accept */
/*******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_Accept)
int accept(int sd, sockaddr *addr, socklen_t *addrlen)
{
    int RetVal = (int)sl_Accept(sd, addr, addrlen);
    return _SlDrvSetErrno(RetVal);
}
#endif

/*******************************************************************************/
/*  recv */
/*******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_Recv)
ssize_t recv(int sd, void *pBuf, size_t Len, int flags)
{
    ssize_t RetVal = (ssize_t)sl_Recv(sd, pBuf, (_i16)Len, flags);
    return (ssize_t)(_SlDrvSetErrno(RetVal));
}
#endif

/*******************************************************************************/
/*  setsockopt */
/*******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_SetSockOpt)
int setsockopt(int sd, int level, int optname, const void *optval, socklen_t optlen)
{
    int RetVal = (int)sl_SetSockOpt(sd, level, optname, optval, optlen);
    return _SlDrvSetErrno(RetVal);
}
#endif

/*******************************************************************************/
/*  getsockopt */
/*******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_GetSockOpt)
int getsockopt(int sd, int level, int optname, void *optval, socklen_t *optlen)
{
    int RetVal = (int)sl_GetSockOpt(sd, level, optname, optval, optlen);
    return _SlDrvSetErrno(RetVal);
}
#endif

/*******************************************************************************/
/*  select */
/*******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_Select)
int select(int nfds, fd_set *readsds, fd_set *writesds, fd_set *exceptsds, struct timeval *timeout)
{
    int RetVal = (int)sl_Select(nfds, readsds, writesds, exceptsds, timeout);
    return _SlDrvSetErrno(RetVal);
}
#endif

/*******************************************************************************/
/*  gethostbyname */
/*******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_NetAppDnsGetHostByName)
struct hostent* gethostbyname(const char *name)
{
    static struct hostent Hostent;
    static char* AddrArray[2];
    static char Addr[16];

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
            /* if the request failed twice, there's an error, return NULL. */
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
