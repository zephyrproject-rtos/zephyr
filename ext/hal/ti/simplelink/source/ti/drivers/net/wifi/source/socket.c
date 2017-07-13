#include <ti/drivers/net/wifi/bsd/sys/socket.h>
#include <ti/drivers/net/wifi/bsd/errno.h>
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
    switch(optname)
    {
        /* This option (TCP_NODELAY) is always set by the NWP, setting it to true always success. */
        case TCP_NODELAY:
            if(optval)
            {
            /* if user wish to have TCP_NODELAY = FALSE, we return EINVAL and failure,
                in the cases below. */
                if(*(_u32*)optval){ return 0; }
            }
        /* These sock opts Aren't supported by the cc31xx\cc32xx network stack,
           hence, we silently ignore them and set errno to EINVAL (invalid argument).
           This is made in order to not break of "off-the-shelf" BSD code. */
        case SO_BROADCAST:
        case SO_REUSEADDR:
        case SO_SNDBUF:
            return _SlDrvSetErrno(EINVAL);
        default:
            break;
    }

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
    switch(optname)
    {
    /* This option (TCP_NODELAY) is always set by the NWP, hence we always return true */
        case TCP_NODELAY:
        if(optval)
        {
            (*(_u32*)optval) = TRUE;
            return 0;
        }
    /* These sock opts Aren't supported by the cc31xx\cc32xx network stack,
       hence, we silently ignore them and set errno to EINVAL (invalid argument).
       This is made in order to not break of "off-the-shelf" BSD code. */
        case SO_BROADCAST:
        case SO_REUSEADDR:
        case SO_SNDBUF:
            return _SlDrvSetErrno(EINVAL);
        default:
            break;
    }

    int RetVal = (int)sl_GetSockOpt(sd, level, optname, optval, optlen);
    return _SlDrvSetErrno(RetVal);
}
#endif
