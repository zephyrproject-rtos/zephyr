/*
 * nonos.c - CC31xx/CC32xx Host Driver Implementation
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

