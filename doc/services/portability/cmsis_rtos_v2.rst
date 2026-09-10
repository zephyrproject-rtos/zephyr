.. _cmsis_rtos_v2:

CMSIS RTOS v2
##########################

Cortex-M Software Interface Standard (CMSIS) RTOS is a vendor-independent
hardware abstraction layer for the ARM Cortex-M processor series and defines
generic tool interfaces. Though it was originally defined for ARM Cortex-M
microcontrollers alone, it could be easily extended to other microcontrollers
making it generic. For more information on CMSIS RTOS v2, please refer to the
`CMSIS-RTOS2 Documentation <https://arm-software.github.io/CMSIS_6/latest/RTOS2/index.html>`_.

Features not supported in Zephyr implementation
***********************************************

Kernel
   ``osKernelGetState``, ``osKernelSuspend``, ``osKernelResume``, ``osKernelInitialize``
   and ``osKernelStart`` are not supported.

Mutex
   ``osMutexPrioInherit`` is supported by default and is not configurable,
   you cannot select/unselect this attribute.

   ``osMutexRecursive`` is also supported by default. If this attribute is
   not set, an error is thrown when the same thread tries to acquire
   it the second time.

   ``osMutexRobust`` is not supported in Zephyr.

Return values not supported in the Zephyr implementation
********************************************************

``osKernelUnlock``, ``osKernelLock``, ``osKernelRestoreLock``
   ``osError`` (Unspecified error) is not supported.

``osSemaphoreDelete``
   ``osErrorResource`` (the semaphore specified by parameter
   semaphore_id is in an invalid semaphore state) is not supported.

``osMutexDelete``
   ``osErrorResource`` (mutex specified by parameter mutex_id
   is in an invalid mutex state) is not supported.

``osTimerDelete``
   ``osErrorResource`` (the timer specified by parameter timer_id
   is in an invalid timer state) is not supported.

``osMessageQueueReset``
   ``osErrorResource`` (the message queue specified by
   parameter msgq_id is in an invalid message queue state)
   is not supported.

``osMessageQueueDelete``
   ``osErrorResource`` (the message queue specified by
   parameter msgq_id is in an invalid message queue state)
   is not supported.

``osMemoryPoolFree``
   ``osErrorResource`` (the memory pool specified by
   parameter mp_id is in an invalid memory pool state) is
   not supported.

``osMemoryPoolDelete``
   ``osErrorResource`` (the memory pool specified by
   parameter mp_id is in an invalid memory pool state) is
   not supported.

``osEventFlagsSet``, ``osEventFlagsClear``
   ``osFlagsErrorUnknown`` (Unspecified error)
   and osFlagsErrorResource (Event flags object specified by
   parameter ef_id is not ready to be used) are not supported.

``osEventFlagsDelete``
   ``osErrorParameter`` (the value of the parameter ef_id is
   incorrect) is not supported.

``osThreadFlagsSet``
   ``osFlagsErrorUnknown`` (Unspecified error) and
   ``osFlagsErrorResource`` (Thread specified by parameter
   thread_id is not active to receive flags) are not supported.

``osThreadFlagsClear``
   ``osFlagsErrorResource`` (Running thread is not active to
   receive flags) is not supported.

``osDelayUntil``
   ``osParameter`` (the time cannot be handled) is not supported.
