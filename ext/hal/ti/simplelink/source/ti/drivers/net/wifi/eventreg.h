/*
 * eventreg.h - CC31xx/CC32xx Host Driver Implementation
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

#ifndef EVENTREG_H_
#define EVENTREG_H_

#ifdef    __cplusplus
extern "C"
{
#endif

/*!
    \defgroup event_registration
    \short Allows user to register event handlers dynamically. 

*/
/*!

    \addtogroup event_registration
    @{

*/

typedef enum
{
    SL_EVENT_HDL_FATAL_ERROR,
    SL_EVENT_HDL_DEVICE_GENERAL,
    SL_EVENT_HDL_WLAN,
    SL_EVENT_HDL_NETAPP,
    SL_EVENT_HDL_SOCKET,
    SL_EVENT_HDL_HTTP_SERVER,
    SL_EVENT_HDL_NETAPP_REQUEST,
    SL_EVENT_HDL_MEM_FREE,
    SL_EVENT_HDL_SOCKET_TRIGGER,
    SL_NUM_OF_EVENT_TYPES
}SlEventHandler_e;

typedef struct SlEventsListNode_s
{
    void *event;
    struct SlEventsListNode_s *next;
}SlEventsListNode_t;

#ifdef SL_RUNTIME_EVENT_REGISTERATION

/*!
    \brief register events in runtime

    this api enables registration of the SimpleLink host driver in runtime.

    \param[in]  EventHandlerType event type - SlEventHandler_e - to register

    \param[in]  EventHandler pointer to the event handler

    \return 0 on success, error otherwise

    \sa     sl_RegisterEventHandler

    \note   registration of event with NULL, clears any registered event.
*/
_i32 sl_RegisterEventHandler(SlEventHandler_e EventHandlerType , void* EventHandler);



_i32 _SlIsEventRegistered(SlEventHandler_e EventHandlerType);

/******************************************************************************
    sl_RegisterLibsEventHandler

    \brief  this function registers event handlers from external libraries in runtime.

    the allocation and memory maintenance of the SlEventsListNode_t is on the library
    Responsibility.

    RETURNS:        success or error code.
******************************************************************************/

_i32 sl_RegisterLibsEventHandler(SlEventHandler_e EventHandlerType , SlEventsListNode_t* EventHandlerNode);

/******************************************************************************
    sl_UnregisterLibsEventHandler

    DESCRIPTION:
    this function unregisters event handlers from external libraries in runtime.
    the SlEventsListNode_t that was used for registration, must be used to unregister that event handler.

    the allocation and memory maintenance of the SlEventsListNode_t is on the library
    Responsibility.

    RETURNS:        success or error code.
******************************************************************************/
_i32 sl_UnregisterLibsEventHandler(SlEventHandler_e EventHandlerType , SlEventsListNode_t* EventHandlerNode);

/*!

 Close the Doxygen group.
 @}

 */


void _SlDeviceFatalErrorEvtHdlr(SlDeviceFatal_t *pSlFatalErrorEvent);
void _SlDeviceGeneralEvtHdlr(SlDeviceEvent_t *pSlDeviceEvent);
void _SlWlanEvtHdlr(SlWlanEvent_t* pSlWlanEvent);
void _SlNetAppEvtHdlr(SlNetAppEvent_t* pSlNetAppEvent);
void _SlSockEvtHdlr(SlSockEvent_t* pSlSockEvent);
void _SlNetAppHttpServerHdlr(SlNetAppHttpServerEvent_t *pSlHttpServerEvent, SlNetAppHttpServerResponse_t *pSlHttpServerResponse);
void _SlNetAppRequestHdlr(SlNetAppRequest_t *pNetAppRequest, SlNetAppResponse_t *pNetAppResponse);
void _SlNetAppRequestMemFree (_u8 *buffer);
void _SlSocketTriggerEventHandler(SlSockTriggerEvent_t* pSlSockTriggerEvent);

#endif /* SL_RUNTIME_EVENT_REGISTERATION */


#ifdef  __cplusplus
}
#endif /* __cplusplus */


#endif /* EVENTREG_H_ */
