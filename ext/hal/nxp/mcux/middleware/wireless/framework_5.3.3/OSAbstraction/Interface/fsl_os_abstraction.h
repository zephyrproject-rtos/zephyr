/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
*
* \file
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* o Redistributions of source code must retain the above copyright notice, this list
*   of conditions and the following disclaimer.
*
* o Redistributions in binary form must reproduce the above copyright notice, this
*   list of conditions and the following disclaimer in the documentation and/or
*   other materials provided with the distribution.
*
* o Neither the name of Freescale Semiconductor, Inc. nor the names of its
*   contributors may be used to endorse or promote products derived from this
*   software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef _FSL_OS_ABSTRACTION_H_
#define _FSL_OS_ABSTRACTION_H_

#include "EmbeddedTypes.h"
#include "fsl_os_abstraction_config.h"

#ifdef  __cplusplus
extern "C"
{
#endif

    
/*! *********************************************************************************
*************************************************************************************
* Public type definitions
*************************************************************************************
********************************************************************************** */
/*! @brief Type for the Task Priority*/    
  typedef uint16_t osaTaskPriority_t;   
/*! @brief Type for the timer definition*/    
  typedef enum  {
    osaTimer_Once             =     0, /*!< one-shot timer*/
    osaTimer_Periodic         =     1  /*!< repeating timer*/ 
  } osaTimer_t; 
  /*! @brief Type for a task handler, returned by the OSA_TaskCreate function. */  
  typedef void* osaTaskId_t;
/*! @brief Type for the parameter to be passed to the task at its creation */    
  typedef void* osaTaskParam_t;
  /*! @brief Type for task pointer. Task prototype declaration */    
  typedef void (*osaTaskPtr_t) (osaTaskParam_t task_param); 
/*! @brief Type for the semaphore handler, returned by the OSA_SemaphoreCreate function. */    
  typedef void* osaSemaphoreId_t; 
/*! @brief Type for the mutex handler, returned by the OSA_MutexCreate function. */      
  typedef void* osaMutexId_t; 
/*! @brief Type for the event handler, returned by the OSA_EventCreate function. */        
  typedef void* osaEventId_t; 
/*! @brief Type for an event flags group, bit 32 is reserved. */  
  typedef uint32_t osaEventFlags_t; 
/*! @brief Message definition. */    
  typedef void* osaMsg_t; 
/*! @brief Type for the message queue handler, returned by the OSA_MsgQCreate function. */          
  typedef void* osaMsgQId_t; 
  /*! @brief Type for the Timer handler, returned by the OSA_TimerCreate function. */          
  typedef void *osaTimerId_t;
/*! @brief Type for the Timer callback function pointer. */          
  typedef void (*osaTimerFctPtr_t) (void const *argument); 
/*! @brief Thread Definition structure contains startup information of a thread.*/ 
typedef struct osaThreadDef_tag  {
  osaTaskPtr_t           pthread;    /*!< start address of thread function*/ 
  uint32_t             tpriority;    /*!< initial thread priority*/ 
  uint32_t             instances;    /*!< maximum number of instances of that thread function*/
  uint32_t             stacksize;    /*!< stack size requirements in bytes; 0 is default stack size*/
  uint32_t              *tstack;
  void                 *tlink;
  uint8_t               *tname;
  bool_t               useFloat;
} osaThreadDef_t;
/*! @brief Thread Link Definition structure .*/ 
typedef struct osaThreadLink_tag{
  uint8_t          link[12];
  osaTaskId_t      osThreadId;
  osaThreadDef_t    *osThreadDefHandle;
  uint32_t          *osThreadStackHandle;
}osaThreadLink_t, *osaThreadLinkHandle_t;

/*! @Timer Definition structure contains timer parameters.*/ 
typedef struct osaTimerDef_tag  {
  osaTimerFctPtr_t       pfCallback;    /* < start address of a timer function */
  void                   *argument;
} osaTimerDef_t;
/*! @brief Defines the return status of OSA's functions */
typedef enum osaStatus_tag
{
    osaStatus_Success = 0U, /*!< Success */
    osaStatus_Error   = 1U, /*!< Failed */
    osaStatus_Timeout = 2U, /*!< Timeout occurs while waiting */
    osaStatus_Idle    = 3U  /*!< Used for bare metal only, the wait object is not ready
                                and timeout still not occur */
}osaStatus_t;


/*! *********************************************************************************
*************************************************************************************
* Public macros
*************************************************************************************
********************************************************************************** */
#if defined (FSL_RTOS_MQX)
    #define USE_RTOS 1
#elif defined (FSL_RTOS_FREE_RTOS)
    #define USE_RTOS 1
#elif defined (FSL_RTOS_UCOSII)
    #define USE_RTOS 1
#elif defined (FSL_RTOS_UCOSIII)
    #define USE_RTOS 1
#else
    #define USE_RTOS 0
#endif

#define OSA_PRIORITY_IDLE          (6)
#define OSA_PRIORITY_LOW           (5)
#define OSA_PRIORITY_BELOW_NORMAL  (4)
#define OSA_PRIORITY_NORMAL        (3)
#define OSA_PRIORITY_ABOVE_NORMAL  (2)
#define OSA_PRIORITY_HIGH          (1)
#define OSA_PRIORITY_REAL_TIME     (0)
#define OSA_TASK_PRIORITY_MAX (0)
#define OSA_TASK_PRIORITY_MIN (15)  
#define SIZE_IN_UINT32_UNITS(size) (((size) + sizeof(uint32_t) - 1) / sizeof(uint32_t)) 

/*! @brief Constant to pass as timeout value in order to wait indefinitely. */
#define osaWaitForever_c   ((uint32_t)(-1))
#define osaEventFlagsAll_c  ((osaEventFlags_t)(0x00FFFFFF))
#define osThreadStackArray(name) osThread_##name##_stack
#define osThreadStackDef(name, stacksize, instances) \
        uint32_t osThreadStackArray(name)[SIZE_IN_UINT32_UNITS(stacksize)*(instances)];

/* ==== Thread Management ==== */

/* Create a Thread Definition with function, priority, and stack requirements. 
 * \param         name         name of the thread function. 
 * \param         priority     initial priority of the thread function.
 * \param         instances    number of possible thread instances.
 * \param         stackSz      stack size (in bytes) requirements for the thread function.
 * \param         useFloat             
 */ 
#if defined(FSL_RTOS_MQX)        
#define OSA_TASK_DEFINE(name, priority, instances, stackSz, useFloat)  \
osaThreadLink_t osThreadLink_##name[instances] = {0}; \
osThreadStackDef(name, stackSz, instances) \
osaThreadDef_t os_thread_def_##name = { (name), \
                                       (priority), \
                                       (instances), \
                                       (stackSz), \
                                       osThreadStackArray(name), \
                                       osThreadLink_##name, \
                                       (uint8_t*) #name,\
                                       (useFloat)}
#elif defined (FSL_RTOS_UCOSII)         
  #if gTaskMultipleInstancesManagement_c                                       
#define OSA_TASK_DEFINE(name, priority, instances, stackSz, useFloat)  \
osaThreadLink_t osThreadLink_##name[instances] = {0}; \
osThreadStackDef(name, stackSz, instances) \
osaThreadDef_t os_thread_def_##name = { (name), \
                                        (priority), \
                                        (instances), \
                                        (stackSz), \
                                        osThreadStackArray(name), \
                                        osThreadLink_##name, \
                                        (uint8_t*) #name,\
                                        (useFloat)}                                       
#else
#define OSA_TASK_DEFINE(name, priority, instances, stackSz, useFloat)  \
osThreadStackDef(name, stackSz, instances) \
osaThreadDef_t os_thread_def_##name = { (name), \
                                        (priority), \
                                        (instances), \
                                        (stackSz), \
                                        osThreadStackArray(name), \
                                        NULL, \
                                        (uint8_t*) #name,\
                                        (useFloat)}                                                                               
#endif
#else
#define OSA_TASK_DEFINE(name, priority, instances, stackSz, useFloat)  \
osaThreadDef_t os_thread_def_##name = { (name), \
                                       (priority), \
                                       (instances), \
                                       (stackSz), \
                                       NULL, \
                                       NULL, \
                                       (uint8_t*) #name,\
                                       (useFloat)}                                       
#endif                                       
/* Access a Thread defintion. 
 * \param         name          name of the thread definition object. 
 */
#define OSA_TASK(name)  \
&os_thread_def_##name

#define OSA_TASK_PROTO(name)  \
extern osaThreadDef_t os_thread_def_##name
/*  ==== Timer Management  ====
 * Define a Timer object.
 * \param         name          name of the timer object.
 * \param         function      name of the timer call back function.
 */

#define OSA_TIMER_DEF(name, function)  \
osaTimerDef_t os_timer_def_##name = \
{ (function), NULL }

/* Access a Timer definition. 
 * \param         name          name of the timer object.
 */
#define OSA_TIMER(name) \
&os_timer_def_##name


/*****************************************************************************
******************************************************************************
* Public memory declarations
******************************************************************************
*****************************************************************************/
extern const uint8_t gUseRtos_c;


/*! *********************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
********************************************************************************** */
/*!
 * @name Task management
 * @{
 */

/*!
 * @brief Creates a task.
 *
 * This function is used to create task based on the resources defined
 * by the macro OSA_TASK_DEFINE.
 *
 * @param thread_def      pointer to the osaThreadDef_t structure which defines the task.
 * @param task_param     Pointer to be passed to the task when it is created.
 *
 * @retval taskId The task is successfully created.
 * @retval NULL   The task can not be created..
 *
 * Example:
   @code
   osaTaskId_t taskId;
   OSA_TASK_DEFINE( Job1, OSA_PRIORITY_HIGH,  1,   800, 0);;
   taskId = OSA__TaskCreate(OSA__TASK(Job1), (osaTaskParam_t)NULL);
   @endcode
 */
osaTaskId_t OSA_TaskCreate(osaThreadDef_t *thread_def, osaTaskParam_t task_param);

/*!
 * @brief Gets the handler of active task.
 *
 * @return Handler to current active task.
 */
osaTaskId_t OSA_TaskGetId(void);

/*!
 * @brief Puts the active task to the end of scheduler's queue.
 *
 * When a task calls this function, it gives up the CPU and puts itself to the
 * end of a task ready list.
 *
 * @retval osaStatus_Success The function is called successfully.
 * @retval osaStatus_Error   Error occurs with this function.
  */
osaStatus_t OSA_TaskYield(void);

/*!
 * @brief Gets the priority of a task.
 *
 * @param taskId The handler of the task whose priority is received.
 *
 * @return Task's priority.
 */
osaTaskPriority_t OSA_TaskGetPriority(osaTaskId_t taskId);

/*!
 * @brief Sets the priority of a task.
 *
 * @param taskId  The handler of the task whose priority is set.
 * @param taskPriority The priority to set.
 *
 * @retval osaStatus_Success Task's priority is set successfully.
 * @retval osaStatus_Error   Task's priority can not be set.
 */
osaStatus_t OSA_TaskSetPriority(osaTaskId_t taskId, osaTaskPriority_t taskPriority);
/*!
 * @brief Destroys a previously created task.
 *
 * @param taskId The handler of the task to destroy. Returned by the OSA_TaskCreate function.
 *
 * @retval osaStatus_Success The task was successfully destroyed.
 * @retval osaStatus_Error   Task destruction failed or invalid parameter.
  */
osaStatus_t OSA_TaskDestroy(osaTaskId_t taskId);

/*!
 * @brief Creates a semaphore with a given value.
 *
 * This function creates a semaphore and sets the value to the parameter
 * initValue.
 *
 * @param initValue Initial value the semaphore will be set to.
 *
 * @retval handler to the new semaphore if the semaphore is created successfully.
 * @retval NULL   if the semaphore can not be created.
 *
 *
 */
osaSemaphoreId_t OSA_SemaphoreCreate(uint32_t initValue);

/*!
 * @brief Destroys a previously created semaphore.
 *
 * @param semId Pointer to the semaphore to destroy.
 *
 * @retval osaStatus_Success The semaphore is successfully destroyed.
 * @retval osaStatus_Error   The semaphore can not be destroyed.
 */
osaStatus_t OSA_SemaphoreDestroy(osaSemaphoreId_t semId);

/*!
 * @brief Pending a semaphore with timeout.
 *
 * This function checks the semaphore's counting value. If it is positive,
 * decreases it and returns osaStatus_Success. Otherwise, a timeout is used
 * to wait.
 *
 * @param semId    Pointer to the semaphore.
 * @param millisec The maximum number of milliseconds to wait if semaphore is not
 *                 positive. Pass osaWaitForever_c to wait indefinitely, pass 0
 *                 will return osaStatus_Timeout immediately.
 *
 * @retval osaStatus_Success  The semaphore is received.
 * @retval osaStatus_Timeout  The semaphore is not received within the specified 'timeout'.
 * @retval osaStatus_Error    An incorrect parameter was passed.
 */
osaStatus_t OSA_SemaphoreWait(osaSemaphoreId_t semId, uint32_t millisec);

/*!
 * @brief Signals for someone waiting on the semaphore to wake up.
 *
 * Wakes up one task that is waiting on the semaphore. If no task is waiting, increases
 * the semaphore's counting value.
 *
 * @param semId Pointer to the semaphore to signal.
 *
 * @retval osaStatus_Success The semaphore is successfully signaled.
 * @retval osaStatus_Error   The object can not be signaled or invalid parameter.
 *
 */
osaStatus_t OSA_SemaphorePost(osaSemaphoreId_t semId);

/*!
 * @brief Create an unlocked mutex.
 *
 * This function creates a non-recursive mutex and sets it to unlocked status.
 *
 * @param none.
 *
 * @retval handler to the new mutex if the mutex is created successfully.
 * @retval NULL   if the mutex can not be created.
 */
osaMutexId_t OSA_MutexCreate(void);

/*!
 * @brief Waits for a mutex and locks it.
 *
 * This function checks the mutex's status. If it is unlocked, locks it and returns the
 * osaStatus_Success. Otherwise, waits for a timeout in milliseconds to lock.
 *
 * @param mutexId Pointer to the Mutex.
 * @param millisec The maximum number of milliseconds to wait for the mutex.
 *                 If the mutex is locked, Pass the value osaWaitForever_c will
 *                 wait indefinitely, pass 0 will return osaStatus_Timeout
 *                 immediately.
 *
 * @retval osaStatus_Success The mutex is locked successfully.
 * @retval osaStatus_Timeout Timeout occurred.
 * @retval osaStatus_Error   Incorrect parameter was passed.
 *
 * @note This is non-recursive mutex, a task can not try to lock the mutex it has locked.
 */
osaStatus_t OSA_MutexLock(osaMutexId_t mutexId, uint32_t millisec);

/*!
 * @brief Unlocks a previously locked mutex.
 *
 * @param mutexId Pointer to the Mutex.
 *
 * @retval osaStatus_Success The mutex is successfully unlocked.
 * @retval osaStatus_Error   The mutex can not be unlocked or invalid parameter.
 */
osaStatus_t OSA_MutexUnlock(osaMutexId_t mutexId);

/*!
 * @brief Destroys a previously created mutex.
 *
 * @param mutexId Pointer to the Mutex.
 *
 * @retval osaStatus_Success The mutex is successfully destroyed.
 * @retval osaStatus_Error   The mutex can not be destroyed.
 *
 */
osaStatus_t OSA_MutexDestroy(osaMutexId_t mutexId);

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
osaEventId_t OSA_EventCreate(bool_t autoClear);

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
osaStatus_t OSA_EventSet(osaEventId_t eventId, osaEventFlags_t flagsToSet);

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
osaStatus_t OSA_EventClear(osaEventId_t eventId, osaEventFlags_t flagsToClear);

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
osaStatus_t OSA_EventWait(osaEventId_t eventId, osaEventFlags_t flagsToWait, bool_t waitAll, uint32_t millisec, osaEventFlags_t *pSetFlags);

/*!
 * @brief Destroys a previously created event object.
 *
 * @param eventId Pointer to the event.
 *
 * @retval osaStatus_Success The event is successfully destroyed.
 * @retval osaStatus_Error   Event destruction failed.
 */
osaStatus_t OSA_EventDestroy(osaEventId_t eventId);

/*!
 * @brief Initializes a message queue.
 *
 * This function  allocates memory for and initializes a message queue. Message queue elements are hardcoded as void*.
 *
 * @param msgNo :number of messages the message queue should accommodate.
 *               This parameter should not exceed osNumberOfMessages defined in OSAbstractionConfig.h. 
 *
* @return:  Handler to access the queue for put and get operations. If message queue
 *         creation failed, return NULL.
 */
osaMsgQId_t OSA_MsgQCreate(uint32_t  msgNo);

/*!
 * @brief Puts a message at the end of the queue.
 *
 * This function puts a message to the end of the message queue. If the queue
 * is full, this function returns the osaStatus_Error;
 *
 * @param msgQId  pointer to queue returned by the OSA_MsgQCreate function.
 * @param pMessage Pointer to the message to be put into the queue.
 *
 * @retval osaStatus_Success Message successfully put into the queue.
 * @retval osaStatus_Error   The queue was full or an invalid parameter was passed.
 */
osaStatus_t OSA_MsgQPut(osaMsgQId_t msgQId, osaMsg_t pMessage);

/*!
 * @brief Reads and remove a message at the head of the queue.
 *
 * This function gets a message from the head of the message queue. If the
 * queue is empty, timeout is used to wait.
 *
 * @param msgQId   Queue handler returned by the OSA_MsgQCreate function.
 * @param pMessage Pointer to a memory to save the message.
 * @param millisec The number of milliseconds to wait for a message. If the
 *                 queue is empty, pass osaWaitForever_c will wait indefinitely,
 *                 pass 0 will return osaStatus_Timeout immediately.
 *
 * @retval osaStatus_Success   Message successfully obtained from the queue.
 * @retval osaStatus_Timeout   The queue remains empty after timeout.
 * @retval osaStatus_Error     Invalid parameter.
 */
osaStatus_t OSA_MsgQGet(osaMsgQId_t msgQId, osaMsg_t pMessage, uint32_t millisec);

/*!
 * @brief Destroys a previously created queue.
 *
 * @param msgQId queue handler returned by the OSA_MsgQCreate function.
 *
 * @retval osaStatus_Success The queue was successfully destroyed.
 * @retval osaStatus_Error   Message queue destruction failed.
*/
osaStatus_t OSA_MsgQDestroy(osaMsgQId_t msgQId);

/*!
 * @brief Enable all interrupts.
*/
void OSA_InterruptEnable(void);

/*!
 * @brief Disable all interrupts.
*/
void OSA_InterruptDisable(void);

/*!
 * @brief Enable all interrupts using PRIMASK.
*/
void OSA_EnableIRQGlobal(void);

/*!
 * @brief Disable all interrupts using PRIMASK.
*/
void OSA_DisableIRQGlobal(void);

/*!
 * @brief Delays execution for a number of milliseconds.
 *
 * @param millisec The time in milliseconds to wait.
 */
void OSA_TimeDelay(uint32_t millisec);

/*!
 * @brief This function gets current time in milliseconds.
 *
 * @retval current time in milliseconds
 */
uint32_t OSA_TimeGetMsec(void);

/*!
 * @brief Installs the interrupt handler.
 *
 * @param IRQNumber IRQ number of the interrupt.
 * @param handler The interrupt handler to install.
 */
void OSA_InstallIntHandler(uint32_t IRQNumber, void (*handler)(void));

#ifdef  __cplusplus
}
#endif

#endif  
