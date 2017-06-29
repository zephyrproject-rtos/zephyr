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



/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/
#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/source/protocol.h>
#include <ti/drivers/net/wifi/source/driver.h>

#if (defined (SL_PLATFORM_MULTI_THREADED)) && (!defined (SL_PLATFORM_EXTERNAL_SPAWN))

#define _SL_MAX_INTERNAL_SPAWN_ENTRIES      10

typedef struct _SlInternalSpawnEntry_t
{
	_SlSpawnEntryFunc_t 		        pEntry;
	void* 						        pValue;
    struct _SlInternalSpawnEntry_t*     pNext;
}_SlInternalSpawnEntry_t;

typedef struct
{
	_SlInternalSpawnEntry_t     SpawnEntries[_SL_MAX_INTERNAL_SPAWN_ENTRIES];
    _SlInternalSpawnEntry_t*    pFree;
    _SlInternalSpawnEntry_t*    pWaitForExe;
    _SlInternalSpawnEntry_t*    pLastInWaitList;
    _SlSyncObj_t                SyncObj;
    _SlLockObj_t                LockObj;
	_u8							IrqWriteCnt;
	_u8							IrqReadCnt;
	void*						pIrqFuncValue;
}_SlInternalSpawnCB_t;

_SlInternalSpawnCB_t g_SlInternalSpawnCB;



void* _SlInternalSpawnTaskEntry()
{
    _i16                         i;
    _SlInternalSpawnEntry_t*    pEntry;
    _u8                         LastEntry;

    /* create and lock the locking object. lock in order to avoid race condition
        on the first creation */
    sl_LockObjCreate(&g_SlInternalSpawnCB.LockObj,"SlSpawnProtect");
    sl_LockObjLock(&g_SlInternalSpawnCB.LockObj,SL_OS_NO_WAIT);

    /* create and clear the sync object */
    sl_SyncObjCreate(&g_SlInternalSpawnCB.SyncObj,"SlSpawnSync");
    sl_SyncObjWait(&g_SlInternalSpawnCB.SyncObj,SL_OS_NO_WAIT);

    g_SlInternalSpawnCB.pFree = &g_SlInternalSpawnCB.SpawnEntries[0];
    g_SlInternalSpawnCB.pWaitForExe = NULL;
    g_SlInternalSpawnCB.pLastInWaitList = NULL;

    /* create the link list between the entries */
    for (i=0 ; i<_SL_MAX_INTERNAL_SPAWN_ENTRIES - 1 ; i++)
    {
        g_SlInternalSpawnCB.SpawnEntries[i].pNext = &g_SlInternalSpawnCB.SpawnEntries[i+1];
        g_SlInternalSpawnCB.SpawnEntries[i].pEntry = NULL;
    }
    g_SlInternalSpawnCB.SpawnEntries[i].pNext = NULL;

	g_SlInternalSpawnCB.IrqWriteCnt =0;
	g_SlInternalSpawnCB.IrqReadCnt = 0;
	g_SlInternalSpawnCB.pIrqFuncValue = NULL;

    SL_DRV_OBJ_UNLOCK(&g_SlInternalSpawnCB.LockObj);

    /* here we ready to execute entries */

    while (TRUE)
    {
        sl_SyncObjWait(&g_SlInternalSpawnCB.SyncObj,SL_OS_WAIT_FOREVER);

        /* handle IRQ requests */
        while (g_SlInternalSpawnCB.IrqWriteCnt != g_SlInternalSpawnCB.IrqReadCnt)
		{
			/* handle the ones that came from ISR context*/
			_SlDrvMsgReadSpawnCtx(g_SlInternalSpawnCB.pIrqFuncValue);
			g_SlInternalSpawnCB.IrqReadCnt++;
		}

        /* go over all entries that already waiting for execution */
        LastEntry = FALSE;

        do
        {
            /* get entry to execute */
            SL_DRV_OBJ_LOCK_FOREVER(&g_SlInternalSpawnCB.LockObj);

            pEntry = g_SlInternalSpawnCB.pWaitForExe;
            if ( NULL == pEntry )
            {
               SL_DRV_OBJ_UNLOCK(&g_SlInternalSpawnCB.LockObj);

               break;
            }

            g_SlInternalSpawnCB.pWaitForExe = pEntry->pNext;
            if (pEntry == g_SlInternalSpawnCB.pLastInWaitList)
            {
                g_SlInternalSpawnCB.pLastInWaitList = NULL;
                LastEntry = TRUE;
            }

            SL_DRV_OBJ_UNLOCK(&g_SlInternalSpawnCB.LockObj);

            /* pEntry could be null in case that the sync was already set by some
               of the entries during execution of earlier entry */
            if (NULL != pEntry)
            {
                pEntry->pEntry(pEntry->pValue);
                /* free the entry */

                SL_DRV_OBJ_LOCK_FOREVER(&g_SlInternalSpawnCB.LockObj);

                pEntry->pNext = g_SlInternalSpawnCB.pFree;
                g_SlInternalSpawnCB.pFree = pEntry;


                if (NULL != g_SlInternalSpawnCB.pWaitForExe)
                {
                    /* new entry received meanwhile */
                    LastEntry = FALSE;
                }

                SL_DRV_OBJ_UNLOCK(&g_SlInternalSpawnCB.LockObj);

            }

        }while (!LastEntry);
    }
}


_i16 _SlInternalSpawn(_SlSpawnEntryFunc_t pEntry , void* pValue , _u32 flags)
{
    _i16                         Res = 0;
    _SlInternalSpawnEntry_t*    pSpawnEntry;


	/*	Increment the counter that specifies that async event has recived
		from interrupt context and should be handled by the internal spawn task */
	if (flags & SL_SPAWN_FLAG_FROM_SL_IRQ_HANDLER)
	{
		g_SlInternalSpawnCB.IrqWriteCnt++;
		g_SlInternalSpawnCB.pIrqFuncValue = pValue;
		SL_DRV_SYNC_OBJ_SIGNAL(&g_SlInternalSpawnCB.SyncObj);
		return Res;
	}


    if (NULL == pEntry || (g_SlInternalSpawnCB.pFree == NULL))
    {
        Res = -1;
    }
    else
    {
		SL_DRV_OBJ_LOCK_FOREVER(&g_SlInternalSpawnCB.LockObj);

        pSpawnEntry = g_SlInternalSpawnCB.pFree;
        g_SlInternalSpawnCB.pFree = pSpawnEntry->pNext;

        pSpawnEntry->pEntry = pEntry;
        pSpawnEntry->pValue = pValue;
        pSpawnEntry->pNext = NULL;

        if (NULL == g_SlInternalSpawnCB.pWaitForExe)
        {
            g_SlInternalSpawnCB.pWaitForExe = pSpawnEntry;
            g_SlInternalSpawnCB.pLastInWaitList = pSpawnEntry;
        }
        else
        {
            g_SlInternalSpawnCB.pLastInWaitList->pNext = pSpawnEntry;
            g_SlInternalSpawnCB.pLastInWaitList = pSpawnEntry;
        }

        SL_DRV_OBJ_UNLOCK(&g_SlInternalSpawnCB.LockObj);

        /* this sync is called after releasing the lock object to avoid unnecessary context switches */
        SL_DRV_SYNC_OBJ_SIGNAL(&g_SlInternalSpawnCB.SyncObj);
    }

    return Res;
}





#endif
