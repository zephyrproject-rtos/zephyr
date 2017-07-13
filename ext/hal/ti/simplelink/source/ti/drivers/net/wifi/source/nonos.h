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

#ifndef __NONOS_H__
#define	__NONOS_H__

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef SL_PLATFORM_MULTI_THREADED

/* This function call the user defined function, if defined, from the sync wait loop  */
/* The use case of this function is to allow nonos system to call a user function to put the device into sleep */
/* The wake up should be activated after getting an interrupt from the device to Host */
/* The user function must return without blocking to prevent a delay on the event handling */
/*
#define _SlSyncWaitLoopCallback  UserSleepFunction
*/

#ifndef SL_TINY_EXT
#define NONOS_MAX_SPAWN_ENTRIES                 (5)
#else
#define NONOS_MAX_SPAWN_ENTRIES                 (1)
#endif

#define NONOS_WAIT_FOREVER   					~(0UL)
#define NONOS_NO_WAIT        				    (0x0)

#define NONOS_RET_OK                            (0)
#define NONOS_RET_ERR                           (0xFF)
#define OSI_OK                                  (NONOS_RET_OK)

typedef struct
{
    _SlSpawnEntryFunc_t 		pEntry;
    void* 						pValue;
	_u8                         IsAllocated;
}_SlNonOsSpawnEntry_t;

typedef struct
{
    _SlNonOsSpawnEntry_t	SpawnEntries[NONOS_MAX_SPAWN_ENTRIES];
}_SlNonOsCB_t;


/*!
	\brief type definition for the return values of this adaptation layer
*/
typedef _u32 _SlNonOsRetVal_t;

/*!
	\brief type definition for a time value
*/
typedef _u32 _SlNonOsTime_t;


#define _SlTime_t               _SlNonOsTime_t

#define SL_OS_WAIT_FOREVER      NONOS_WAIT_FOREVER

#define SL_OS_RET_CODE_OK       NONOS_RET_OK

#define SL_OS_NO_WAIT           NONOS_NO_WAIT


/*!
	\brief 	This function call the pEntry callback from a different context

	\param	pEntry		-	pointer to the entry callback function

	\param	pValue		- 	pointer to any type of memory structure that would be
							passed to pEntry callback from the execution thread.

	\param	flags		- 	execution flags - reserved for future usage

	\return upon successful registration of the spawn the function return 0
			(the function is not blocked till the end of the execution of the function
			and could be returned before the execution is actually completed)
			Otherwise, a negative value indicating the error code shall be returned
	\note
	\warning
*/
_SlNonOsRetVal_t _SlNonOsSpawn(_SlSpawnEntryFunc_t pEntry , void* pValue , _u32 flags);


/*!
	\brief 	This function is called form the main context, while waiting on a sync object.

	\param	None

	\return None

	\note
	\warning
*/
void tiDriverSpawnCallback(void);


/*!
	\brief 	This function is called directly the main context, while in main task loop.

	\param	None

	\return None

	\note
	\warning
*/
_SlNonOsRetVal_t _SlNonOsHandleSpawnTask(void);

extern _SlNonOsRetVal_t _SlNonOsSpawn(_SlSpawnEntryFunc_t pEntry , void* pValue , _u32 flags);

/*****************************************************************************

    Overwrite SimpleLink driver OS adaptation functions

 *****************************************************************************/

#undef sl_Spawn
#define sl_Spawn(pEntry,pValue,flags)               _SlNonOsSpawn(pEntry,pValue,flags)

#undef _SlTaskEntry
#define _SlTaskEntry                                _SlNonOsHandleSpawnTask

#endif /* !SL_PLATFORM_MULTI_THREADED */

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif
