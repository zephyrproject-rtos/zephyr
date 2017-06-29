/*
 *   Copyright (C) 2016 Texas Instruments Incorporated
 *
 *   All rights reserved. Property of Texas Instruments Incorporated.
 *   Restricted rights to use, duplicate or disclose this code are
 *   granted through contract.
 *
 *   The program may not be used without the written permission of
 *   Texas Instruments Incorporated or against the terms and conditions
 *   stipulated in the agreement under which this program has been supplied,
 *   and under no circumstances can it be used with non-TI connectivity device.
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




#ifdef  __cplusplus
}
#endif /* __cplusplus */


#endif /* EVENTREG_H_ */
