#include <zephyr.h>
#include <irq.h>
#include <soc.h>

#include "fsl_os_abstraction.h"

typedef struct osObjectInfo_tag
{
    void *pHeap;
    uint32_t objectStructSize;
    uint32_t objNo;
} osObjectInfo_t;

#if (osNumberOfEvents)
#define osObjectAlloc_c 1
#else
#define osObjectAlloc_c 0
#endif

/*! @brief Type for an event object */
/* NB: The FSL OSA expects the event object to be a group of bits.
 * Zephyr doesn't have this concept unlike FreeRTOS and ucosii. The
 * implementation is adding just the needed stuff and for now only
 * a single bit is needed but IF any additional IP is ported from NXP
 * that ends up wanting to use more bits then this simple implementation
 * will need to adjust or the port will have to change what comes from
 * NXP.
 */
typedef struct k_poll_event event_t;

#if osNumberOfEvents
typedef struct osEventStruct_tag
{
    uint32_t inUse;
    bool_t   autoClear;
    event_t  event;
    struct k_poll_signal signal;
}osEventStruct_t;

osEventStruct_t osEventHeap[osNumberOfEvents];
const osObjectInfo_t osEventInfo = {osEventHeap, sizeof(osEventStruct_t),osNumberOfEvents};
#endif

const uint8_t gUseRtos_c = 1;  // USE_RTOS = 0 for BareMetal and 1 for OS

unsigned int gIrqKey;

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_InterruptEnable
 * Description   : self explanatory.
 *
 *END**************************************************************************/
void OSA_InterruptEnable(void)
{
	irq_unlock(gIrqKey);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_InterruptDisable
 * Description   : self explanatory.
 *
 *END**************************************************************************/
void OSA_InterruptDisable(void)
{
	gIrqKey = irq_lock();
}


/*! *********************************************************************************
* \brief     Allocates a osObjectStruct_t block in the osObjectHeap array.
* \param[in] pointer to the object info struct.
* Object can be semaphore, mutex, message Queue, event
* \return Pointer to the allocated osObjectStruct_t, NULL if failed.
*
* \pre 
*
* \post
*
* \remarks Function is unprotected from interrupts. 
*
********************************************************************************** */
#if osObjectAlloc_c
static void* osObjectAlloc(const osObjectInfo_t* pOsObjectInfo)
{
    uint32_t i;
    uint8_t* pObj = (uint8_t*)pOsObjectInfo->pHeap;
    for( i=0 ; i < pOsObjectInfo->objNo ; i++, pObj += pOsObjectInfo->objectStructSize)
    {
        if(((osEventStruct_t*)pObj)->inUse == 0)
        {
            ((osEventStruct_t*)pObj)->inUse = 1;
            return (void*)pObj;
        }
    }
    return NULL;
}
#endif

/*! *********************************************************************************
* \brief     Verifies the object is valid and allocated in the osObjectHeap array.
* \param[in] the pointer to the object info struct.
* \param[in] the pointer to the object struct.
* Object can be semaphore, mutex,  message Queue, event
* \return TRUE if the object is valid and allocated, FALSE otherwise
*
* \pre 
*
* \post
*
* \remarks Function is unprotected from interrupts. 
*
********************************************************************************** */
#if osObjectAlloc_c
static bool_t osObjectIsAllocated(const osObjectInfo_t* pOsObjectInfo, void* pObjectStruct)
{
    uint32_t i;
    uint8_t* pObj = (uint8_t*)pOsObjectInfo->pHeap;
    for( i=0 ; i < pOsObjectInfo->objNo ; i++ , pObj += pOsObjectInfo->objectStructSize)
    {
        if(pObj == pObjectStruct)
        {
            if(((osEventStruct_t*)pObj)->inUse)
            {
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}
#endif

/*! *********************************************************************************
* \brief     Frees an osObjectStruct_t block from the osObjectHeap array.
* \param[in] pointer to the object info struct.
* \param[in] Pointer to the allocated osObjectStruct_t to free.
* Object can be semaphore, mutex, message Queue, event
* \return none.
*
* \pre 
*
* \post
*
* \remarks Function is unprotected from interrupts. 
*
********************************************************************************** */
#if osObjectAlloc_c
static void osObjectFree(const osObjectInfo_t* pOsObjectInfo, void* pObjectStruct)
{
    uint32_t i;
    uint8_t* pObj = (uint8_t*)pOsObjectInfo->pHeap;
    for( i=0; i < pOsObjectInfo->objNo; i++, pObj += pOsObjectInfo->objectStructSize )
    {
        if(pObj == pObjectStruct)
        {
            ((osEventStruct_t*)pObj)->inUse = 0;
            break;
        }
    }
}
#endif

/*!
 * @brief Initializes an event object with all flags cleared.
 *
 * This function creates an event object and set its clear mode. If autoClear
 * is TRUE, when a task gets the event flags, these flags will be
 * cleared automatically. Otherwise these flags must
 * be cleared manually.
 *
 * @param autoClear TRUE The event is auto-clear.
 *                  FALSE The event manual-clear
 * @retval handler to the new event if the event is created successfully.
 * @retval NULL   if the event can not be created.
 */
osaEventId_t OSA_EventCreate(bool_t autoClear)
{
#if osNumberOfEvents  
  osaEventId_t eventId;
  osEventStruct_t* pEventStruct;

  
  OSA_InterruptDisable();
  eventId = pEventStruct = osObjectAlloc(&osEventInfo);
  OSA_InterruptEnable();

  if(eventId == NULL)
  {
    return NULL;
  }
  
  k_poll_signal_init(&pEventStruct->signal);
  k_poll_event_init(&pEventStruct->event, K_POLL_TYPE_SIGNAL,
    K_POLL_MODE_NOTIFY_ONLY, &pEventStruct->signal);
  pEventStruct->autoClear = autoClear;

  return eventId;
#else
  (void)autoClear;
  return NULL;
#endif
}

/*!
 * @brief Sets one or more event flags.
 *
 * Sets specified flags of an event object.
 *
 * @param eventId     Pointer to the event.
 * @param flagsToSet  Flags to be set.
 *
 * @retval osaStatus_Success The flags were successfully set.
 * @retval osaStatus_Error   An incorrect parameter was passed.
 */
osaStatus_t OSA_EventSet(osaEventId_t eventId, osaEventFlags_t flagsToSet)
{
#if osNumberOfEvents
    osEventStruct_t *pEventStruct;

    if (osObjectIsAllocated(&osEventInfo, eventId) == FALSE)
    {
	    printk("OSA_EventSet: ERROR\n");
        return osaStatus_Error;
    }
    pEventStruct = (osEventStruct_t *)eventId;
    
    k_poll_signal(&pEventStruct->signal, flagsToSet);

    return osaStatus_Success;
#else
    (void)eventId;
    (void)flagsToSet;
    return osaStatus_Error;
#endif
}

/*!
 * @brief Clears one or more flags.
 *
 * Clears specified flags of an event object.
 *
 * @param eventId       Pointer to the event.
 * @param flagsToClear  Flags to be clear.
 *
 * @retval osaStatus_Success The flags were successfully cleared.
 * @retval osaStatus_Error   An incorrect parameter was passed.
 */
osaStatus_t OSA_EventClear(osaEventId_t eventId, osaEventFlags_t flagsToClear)
{
#if osNumberOfEvents

    osEventStruct_t *pEventStruct;
    if (osObjectIsAllocated(&osEventInfo, eventId) == FALSE)
    {
	    printk("OSA_EventClear: ERROR\n");
        return osaStatus_Error;
    }
    pEventStruct = (osEventStruct_t *)eventId;

    k_poll_signal_reset(&pEventStruct->signal);

    return osaStatus_Success;
#else
    (void)eventId;
    (void)flagsToClear;
    return osaStatus_Error;
#endif
}

/*!
 * @brief Waits for specified event flags to be set.
 *
 * This function waits for a combination of flags to be set in an event object.
 * Applications can wait for any/all bits to be set. Also this function could
 * obtain the flags who wakeup the waiting task.
 *
 * @param eventId     Pointer to the event.
 * @param flagsToWait Flags that to wait.
 * @param waitAll     Wait all flags or any flag to be set.
 * @param millisec    The maximum number of milliseconds to wait for the event.
 *                    If the wait condition is not met, pass osaWaitForever_c will
 *                    wait indefinitely, pass 0 will return osaStatus_Timeout
 *                    immediately.
 * @param setFlags    Flags that wakeup the waiting task are obtained by this parameter.
 *
 * @retval osaStatus_Success The wait condition met and function returns successfully.
 * @retval osaStatus_Timeout Has not met wait condition within timeout.
 * @retval osaStatus_Error   An incorrect parameter was passed.
 
 *
 * @note    Please pay attention to the flags bit width, FreeRTOS uses the most
 *          significant 8 bis as control bits, so do not wait these bits while using
 *          FreeRTOS.
 *
 */
osaStatus_t OSA_EventWait(osaEventId_t eventId, osaEventFlags_t flagsToWait, bool_t waitAll, uint32_t millisec, osaEventFlags_t *pSetFlags)
{
#if osNumberOfEvents
    osEventStruct_t *pEventStruct;
    int rslt;
    
    if (osObjectIsAllocated(&osEventInfo, eventId) == FALSE)
    {
	    printk("OSA_EventWait: ERROR\n");
        return osaStatus_Error;
    }

    flagsToWait = flagsToWait & 0x00FFFFFF;

    pEventStruct = (osEventStruct_t *)eventId;

    rslt = k_poll(&pEventStruct->event, 1, millisec);

    if (rslt == 0)
    {
	    *pSetFlags = pEventStruct->signal.signaled;
	    if (pEventStruct->autoClear)
	    {
		    k_poll_signal_reset(&pEventStruct->signal);
	    }
        return osaStatus_Success;
    }
    else
    {
	printk("OSA_EventWait TIMEOUT **************************\n");
        return osaStatus_Timeout;
    }

#else
    (void)eventId;
    (void)flagsToWait;
    (void)waitAll;
    (void)millisec;
    (void)pSetFlags;
    return osaStatus_Error;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_EventDestroy
 * Description   : This function is used to destroy a event object. Return
 * osaStatus_Success if the event object is destroyed successfully, otherwise
 * return osaStatus_Error.
 *
 *END**************************************************************************/
osaStatus_t OSA_EventDestroy(osaEventId_t eventId)
{
#if osNumberOfEvents
    osEventStruct_t *pEventStruct;
    printk("OSA_EventDestroy(eventId: %p)\n", eventId);
    if (osObjectIsAllocated(&osEventInfo, eventId) == FALSE)
    {
	    printk("OSA_EventDestroy: ERROR\n");
        return osaStatus_Error;
    }
    pEventStruct = (osEventStruct_t *)eventId;

    pEventStruct->event.state = K_POLL_STATE_NOT_READY;

    OSA_InterruptDisable();
    osObjectFree(&osEventInfo, eventId);
    OSA_InterruptEnable();

    return osaStatus_Success;
#else
    (void)eventId;
    return osaStatus_Error;
#endif
}

