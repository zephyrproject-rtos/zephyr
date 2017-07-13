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

#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/eventreg.h>



typedef void (*_pSlDeviceFatalErrorEvtHdlr_t)(SlDeviceFatal_t *pSlFatalErrorEvent);
typedef void (*_pSlDeviceGeneralEvtHdlr_t)(SlDeviceEvent_t *pSlDeviceEvent);
typedef void (*_pSlWlanEvtHdlr)(SlWlanEvent_t* pSlWlanEvent);
typedef void (*_pSlNetAppEvtHdlr)(SlNetAppEvent_t* pSlNetAppEvent);
typedef void (*_pSlSockEvtHdlr)(SlSockEvent_t* pSlSockEvent);
typedef void (*_pSlNetAppHttpServerHdlr)(SlNetAppHttpServerEvent_t *pSlHttpServerEvent, SlNetAppHttpServerResponse_t *pSlHttpServerResponse);
typedef void (*_pSlNetAppRequestHdlr)(SlNetAppRequest_t *pNetAppRequest, SlNetAppResponse_t *pNetAppResponse);
typedef void (*_pSlNetAppRequestMemFree)(_u8 *buffer);
typedef void (*_pSlSocketTriggerEventHandler)(SlSockTriggerEvent_t* pSlSockTriggerEvent);


typedef _i32 (*_pSlPropogationDeviceFatalErrorEvtHdlr_t)(SlDeviceFatal_t *pSlFatalErrorEvent);
typedef _i32 (*_pSlPropogationDeviceGeneralEvtHdlr_t)(SlDeviceEvent_t *pSlDeviceEvent);
typedef _i32 (*_pSlPropogationWlanEvtHdlr)(SlWlanEvent_t* pSlWlanEvent);
typedef _i32 (*_pSlPropogationNetAppEvtHdlr)(SlNetAppEvent_t* pSlNetAppEvent);
typedef _i32 (*_pSlPropogationSockEvtHdlr)(SlSockEvent_t* pSlSockEvent);
typedef _i32 (*_pSlPropogationNetAppHttpServerHdlr)(SlNetAppHttpServerEvent_t *pSlHttpServerEvent, SlNetAppHttpServerResponse_t *pSlHttpServerResponse);
typedef _i32 (*_pSlPropogationNetAppRequestHdlr)(SlNetAppRequest_t *pNetAppRequest, SlNetAppResponse_t *pNetAppResponse);
typedef _i32 (*_pSlPropogationNetAppRequestMemFree)(_u8 *buffer);
typedef _i32 (*_pSlPropogationSocketTriggerEventHandler)(SlSockTriggerEvent_t* pSlSockTriggerEvent);

#ifdef SL_RUNTIME_EVENT_REGISTERATION

void* g_UserEvents[SL_NUM_OF_EVENT_TYPES] = {0};
SlEventsListNode_t* g_LibsEvents[SL_NUM_OF_EVENT_TYPES] = {0};

#endif


_i32 _SlIsEventRegistered(SlEventHandler_e EventHandlerType)
{
#ifdef SL_RUNTIME_EVENT_REGISTERATION
	if( (NULL != g_LibsEvents[EventHandlerType]) || (NULL != g_UserEvents[EventHandlerType]) )
	{
		return 1;
	}
#endif
	if(SL_EVENT_HDL_MEM_FREE == EventHandlerType)
	{
#ifdef slcb_NetAppRequestMemFree
		return 1;
#endif
	}
	if(SL_EVENT_HDL_SOCKET_TRIGGER == EventHandlerType)
	{
#ifdef slcb_SocketTriggerEventHandler
		return 1;
#endif
	}

	return 0;
}

#ifdef SL_RUNTIME_EVENT_REGISTERATION

_i32 sl_RegisterEventHandler(SlEventHandler_e EventHandlerType , void* EventHandler)
{
	g_UserEvents[EventHandlerType] = EventHandler;
	return 0;
}

_i32 sl_RegisterLibsEventHandler(SlEventHandler_e EventHandlerType , SlEventsListNode_t* EventHandlerNode)
{
	EventHandlerNode->next = NULL;

	if(g_LibsEvents[EventHandlerType] == NULL)
	{
		g_LibsEvents[EventHandlerType] = EventHandlerNode;
	}
	else
	{
		SlEventsListNode_t* currentNode = g_LibsEvents[EventHandlerType];
		while(currentNode->next != NULL)
		{
			currentNode = currentNode->next;
		}

		currentNode->next = EventHandlerNode;
	}
	return 0;
}

_i32 sl_UnregisterLibsEventHandler(SlEventHandler_e EventHandlerType , SlEventsListNode_t* EventHandlerNode)
{
	SlEventsListNode_t* currentNode = g_LibsEvents[EventHandlerType];
	SlEventsListNode_t* lastNode = g_LibsEvents[EventHandlerType];
	int count = 0;
	while(currentNode != NULL)
	{
		if(EventHandlerNode == currentNode)
		{
			if(count == 0)
			{
				g_LibsEvents[EventHandlerType] = g_LibsEvents[EventHandlerType]->next;
			}
			else
			{
				lastNode->next = currentNode->next;
			}
			return 0;
		}

		if(count != 0)
		{
			lastNode = lastNode->next;
		}
		count++;
		currentNode = currentNode->next;
	}

	return SL_RET_CODE_EVENT_LINK_NOT_FOUND;
}


/* Event handlers section */
void _SlDeviceFatalErrorEvtHdlr(SlDeviceFatal_t *pSlFatalErrorEvent)
{
	SlEventsListNode_t* currentNode = g_LibsEvents[SL_EVENT_HDL_FATAL_ERROR];
	while(currentNode != NULL)
	{
		if(EVENT_PROPAGATION_BLOCK == ((_pSlPropogationDeviceFatalErrorEvtHdlr_t)(currentNode->event))(pSlFatalErrorEvent))
		{
			return;
		}
		currentNode = currentNode->next;
	}

	if (NULL != g_UserEvents[SL_EVENT_HDL_FATAL_ERROR])
	{
		((_pSlDeviceFatalErrorEvtHdlr_t)g_UserEvents[SL_EVENT_HDL_FATAL_ERROR])(pSlFatalErrorEvent);
	}

#ifdef slcb_DeviceFatalErrorEvtHdlr
	else
	{
		slcb_DeviceFatalErrorEvtHdlr(pSlFatalErrorEvent);
	}
#endif
}


void _SlDeviceGeneralEvtHdlr(SlDeviceEvent_t *pSlDeviceEvent)
{
	SlEventsListNode_t* currentNode = g_LibsEvents[SL_EVENT_HDL_DEVICE_GENERAL];
	while(currentNode != NULL)
	{
		if(EVENT_PROPAGATION_BLOCK == ((_pSlPropogationDeviceGeneralEvtHdlr_t)(currentNode->event))(pSlDeviceEvent))
		{
			return;
		}
		currentNode = currentNode->next;
	}

	if (NULL != g_UserEvents[SL_EVENT_HDL_DEVICE_GENERAL])
	{
		((_pSlDeviceGeneralEvtHdlr_t)g_UserEvents[SL_EVENT_HDL_DEVICE_GENERAL])(pSlDeviceEvent);
	}
#ifdef slcb_DeviceGeneralEvtHdlr
	else
	{
		slcb_DeviceGeneralEvtHdlr(pSlDeviceEvent);
	}
#endif
}


void _SlWlanEvtHdlr(SlWlanEvent_t* pSlWlanEvent)
{
	SlEventsListNode_t* currentNode = g_LibsEvents[SL_EVENT_HDL_WLAN];
	while(currentNode != NULL)
	{
		if(EVENT_PROPAGATION_BLOCK == ((_pSlPropogationWlanEvtHdlr)(currentNode->event))(pSlWlanEvent))
		{
			return;
		}
		currentNode = currentNode->next;
	}

	if (NULL != g_UserEvents[SL_EVENT_HDL_WLAN])
	{
		((_pSlWlanEvtHdlr)g_UserEvents[SL_EVENT_HDL_WLAN])(pSlWlanEvent);
	}
#ifdef slcb_WlanEvtHdlr
	else
	{
		slcb_WlanEvtHdlr(pSlWlanEvent);
	}
#endif
}


void _SlNetAppEvtHdlr(SlNetAppEvent_t* pSlNetAppEvent)
{
	SlEventsListNode_t* currentNode = g_LibsEvents[SL_EVENT_HDL_NETAPP];
	while(currentNode != NULL)
	{
		if(EVENT_PROPAGATION_BLOCK == ((_pSlPropogationNetAppEvtHdlr)(currentNode->event))(pSlNetAppEvent))
		{
			return;
		}
		currentNode = currentNode->next;
	}
	if (NULL != g_UserEvents[SL_EVENT_HDL_NETAPP])
	{
		((_pSlNetAppEvtHdlr)g_UserEvents[SL_EVENT_HDL_NETAPP])(pSlNetAppEvent);
	}
#ifdef slcb_NetAppEvtHdlr
	else
	{
		slcb_NetAppEvtHdlr(pSlNetAppEvent);
	}
#endif
}


void _SlSockEvtHdlr(SlSockEvent_t* pSlSockEvent)
{
	SlEventsListNode_t* currentNode = g_LibsEvents[SL_EVENT_HDL_SOCKET];
	while(currentNode != NULL)
	{
		if(EVENT_PROPAGATION_BLOCK == ((_pSlPropogationSockEvtHdlr)(currentNode->event))(pSlSockEvent))
		{
			return;
		}
		currentNode = currentNode->next;
	}
	if (NULL != g_UserEvents[SL_EVENT_HDL_SOCKET])
	{
		((_pSlSockEvtHdlr)g_UserEvents[SL_EVENT_HDL_SOCKET])(pSlSockEvent);
	}

#ifdef slcb_SockEvtHdlr
	else
	{
		slcb_SockEvtHdlr(pSlSockEvent);
	}
#endif
}


void _SlNetAppHttpServerHdlr(SlNetAppHttpServerEvent_t *pSlHttpServerEvent, SlNetAppHttpServerResponse_t *pSlHttpServerResponse)
{
	SlEventsListNode_t* currentNode = g_LibsEvents[SL_EVENT_HDL_HTTP_SERVER];
	while(currentNode != NULL)
	{
		if(EVENT_PROPAGATION_BLOCK == ((_pSlPropogationNetAppHttpServerHdlr)(currentNode->event))(pSlHttpServerEvent,pSlHttpServerResponse))
		{
			return;
		}
		currentNode = currentNode->next;
	}
	if (NULL != g_UserEvents[SL_EVENT_HDL_HTTP_SERVER])
	{
		((_pSlNetAppHttpServerHdlr)g_UserEvents[SL_EVENT_HDL_HTTP_SERVER])(pSlHttpServerEvent,pSlHttpServerResponse);
	}
#ifdef slcb_NetAppHttpServerHdlr
	else
	{
		slcb_NetAppHttpServerHdlr(pSlHttpServerEvent,pSlHttpServerResponse);
	}
#endif
}



void _SlNetAppRequestHdlr(SlNetAppRequest_t *pNetAppRequest, SlNetAppResponse_t *pNetAppResponse)
{
	SlEventsListNode_t* currentNode = g_LibsEvents[SL_EVENT_HDL_NETAPP_REQUEST];
	while(currentNode != NULL)
	{
		if(EVENT_PROPAGATION_BLOCK == ((_pSlPropogationNetAppRequestHdlr)(currentNode->event))(pNetAppRequest,pNetAppResponse))
		{
			return;
		}
		currentNode = currentNode->next;
	}
	if (NULL != g_UserEvents[SL_EVENT_HDL_NETAPP_REQUEST])
	{
		((_pSlNetAppRequestHdlr)g_UserEvents[SL_EVENT_HDL_NETAPP_REQUEST])(pNetAppRequest,pNetAppResponse);
	}
#ifdef slcb_NetAppRequestHdlr
	else
	{
		slcb_NetAppRequestHdlr(pNetAppRequest,pNetAppResponse);
	}
#endif
}



void _SlNetAppRequestMemFree (_u8 *buffer)
{
	SlEventsListNode_t* currentNode = g_LibsEvents[SL_EVENT_HDL_MEM_FREE];
	while(currentNode != NULL)
	{
		if(EVENT_PROPAGATION_BLOCK == ((_pSlPropogationNetAppRequestMemFree)(currentNode->event))(buffer))
		{
			return;
		}
		currentNode = currentNode->next;
	}
	if (NULL != g_UserEvents[SL_EVENT_HDL_MEM_FREE])
	{
		((_pSlNetAppRequestMemFree)g_UserEvents[SL_EVENT_HDL_MEM_FREE])(buffer);
	}
#ifdef slcb_NetAppRequestMemFree
	else
	{
		slcb_NetAppRequestMemFree(buffer);
	}
#endif
}


void _SlSocketTriggerEventHandler(SlSockTriggerEvent_t* pSlSockTriggerEvent)
{
	SlEventsListNode_t* currentNode = g_LibsEvents[SL_EVENT_HDL_SOCKET_TRIGGER];
	while(currentNode != NULL)
	{
		if(EVENT_PROPAGATION_BLOCK == ((_pSlPropogationSocketTriggerEventHandler)(currentNode->event))(pSlSockTriggerEvent))
		{
			return;
		}
		currentNode = currentNode->next;
	}
	if (NULL != g_UserEvents[SL_EVENT_HDL_SOCKET_TRIGGER])
	{
		((_pSlSocketTriggerEventHandler)g_UserEvents[SL_EVENT_HDL_SOCKET_TRIGGER])(pSlSockTriggerEvent);
	}
#ifdef slcb_SocketTriggerEventHandler
	else
	{
		slcb_SocketTriggerEventHandler(pSlSockTriggerEvent);
	}
#endif
}

#endif /* SL_RUNTIME_EVENT_REGISTERATION */
