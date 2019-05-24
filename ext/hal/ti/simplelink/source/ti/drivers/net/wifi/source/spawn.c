/*
 * spawn.c - CC31xx/CC32xx Host Driver Implementation
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
#include <ti/drivers/net/wifi/source/protocol.h>
#include <ti/drivers/net/wifi/source/driver.h>

#if (defined (SL_PLATFORM_MULTI_THREADED)) && (!defined (SL_PLATFORM_EXTERNAL_SPAWN))


typedef struct
{
    _SlSyncObj_t                SyncObj;  
    _u8                         IrqWriteCnt;
    _u8                         IrqReadCnt;
    void*                       pIrqFuncValue;

#ifdef SL_PLATFORM_MULTI_THREADED
    _u32 						ThreadId;
#endif
}_SlInternalSpawnCB_t;

_SlInternalSpawnCB_t g_SlInternalSpawnCB;

_u8 _SlInternalIsItSpawnThread(_u32 ThreadId)
{
	return (ThreadId == g_SlInternalSpawnCB.ThreadId);
}

void _SlInternalSpawnWaitForEvent(void)
{

	sl_SyncObjWait(&g_SlInternalSpawnCB.SyncObj, SL_OS_WAIT_FOREVER);

	/*
	* call the processQ function will handle the pending async
	* events already read from NWP, and only wait for handling
	* the events that have been read only during command execution. */
	_SlSpawnMsgListProcess();

	/* handle IRQ requests */
	while (g_SlInternalSpawnCB.IrqWriteCnt != g_SlInternalSpawnCB.IrqReadCnt)
	{
		/* handle the ones that came from ISR context*/
		_SlDrvMsgReadSpawnCtx(g_SlInternalSpawnCB.pIrqFuncValue);
		g_SlInternalSpawnCB.IrqReadCnt++;
	}

}

void* _SlInternalSpawnTaskEntry()
{

    /* create and clear the sync object */
    sl_SyncObjCreate(&g_SlInternalSpawnCB.SyncObj,"SlSpawnSync");
    sl_SyncObjWait(&g_SlInternalSpawnCB.SyncObj,SL_OS_NO_WAIT);

    g_SlInternalSpawnCB.ThreadId = 0xFFFFFFFF;

#ifdef SL_PLATFORM_MULTI_THREADED
	g_SlInternalSpawnCB.ThreadId = (_i32)pthread_self();
#endif

    g_SlInternalSpawnCB.IrqWriteCnt = 0;
    g_SlInternalSpawnCB.IrqReadCnt = 0;
    g_SlInternalSpawnCB.pIrqFuncValue = NULL;

    /* here we ready to execute entries */
    while (TRUE)
    {
	    /* wait for event */
	    _SlInternalSpawnWaitForEvent();
    }
}

_i16 _SlInternalSpawn(_SlSpawnEntryFunc_t pEntry , void* pValue , _u32 flags)
{
    _i16 Res = 0;

    /*  Increment the counter that specifies that async event has recived 
        from interrupt context and should be handled by the internal spawn task */
    if ((flags & SL_SPAWN_FLAG_FROM_SL_IRQ_HANDLER) || (flags & SL_SPAWN_FLAG_FROM_CMD_CTX))
    {
        g_SlInternalSpawnCB.IrqWriteCnt++;
        g_SlInternalSpawnCB.pIrqFuncValue = pValue;
        SL_DRV_SYNC_OBJ_SIGNAL(&g_SlInternalSpawnCB.SyncObj);
        return Res;
    }
    else if (flags & SL_SPAWN_FLAG_FROM_CMD_PROCESS)
    {
        SL_DRV_SYNC_OBJ_SIGNAL(&g_SlInternalSpawnCB.SyncObj);
    }

    return Res;
}

#endif
