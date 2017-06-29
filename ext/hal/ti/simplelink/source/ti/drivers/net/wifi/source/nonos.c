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

#ifndef SL_PLATFORM_MULTI_THREADED

#include "nonos.h"

_SlNonOsCB_t g__SlNonOsCB;


_SlNonOsRetVal_t _SlNonOsSpawn(_SlSpawnEntryFunc_t pEntry , void* pValue , _u32 flags)
{
	 _i8 i = 0;

     /* The parameter is currently not in use */
     (void)flags;

#ifndef SL_TINY
	for (i=0 ; i<NONOS_MAX_SPAWN_ENTRIES ; i++)
#endif
	{
		_SlNonOsSpawnEntry_t* pE = &g__SlNonOsCB.SpawnEntries[i];

		if (pE->IsAllocated == FALSE)
		{
			pE->pValue = pValue;
			pE->pEntry = pEntry;
			pE->IsAllocated = TRUE;
#ifndef SL_TINY
			break;
#endif
		}
	}

    return NONOS_RET_OK;
}


_SlNonOsRetVal_t _SlNonOsHandleSpawnTask(void)
{
	_i8    i=0;
	void*  pValue;

#ifndef SL_TINY
	for (i=0 ; i<NONOS_MAX_SPAWN_ENTRIES ; i++)
#endif
	{
		_SlNonOsSpawnEntry_t* pE = &g__SlNonOsCB.SpawnEntries[i];

		if (pE->IsAllocated == TRUE)
		{
			_SlSpawnEntryFunc_t  pF = pE->pEntry;
			pValue = pE->pValue;

			/* Clear the entry */
			pE->pEntry = NULL;
			pE->pValue = NULL;
			pE->IsAllocated = FALSE;

			/* execute the spawn function */
            pF(pValue);
		}
	}
        return NONOS_RET_OK;
}


void tiDriverSpawnCallback(void)
{
    /* If we are in cmd context and waiting for its cmd response
     * do not handle async events from spawn, as the global lock was already taken when sending the command,
     * and the Async event would be handled in read cmd context, so we do nothing.
     */
    if (FALSE == g_pCB->WaitForCmdResp)
    {
	(void)_SlNonOsHandleSpawnTask();
    }
}

#endif
