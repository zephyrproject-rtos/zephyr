/*
 * Copyright (c) 2015-2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/** ============================================================================
 *  @file       Power.h
 *
 *  @brief      Power manager interface
 *
 *  The Power header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/Power.h>
 *  @endcode
 *
 *  # Operation #
 *  The Power manager facilitates the transition of the MCU from active state
 *  to one of the sleep states and vice versa.  It provides drivers the
 *  ability to set and release dependencies on hardware resources and keeps
 *  a reference count on each resource to know when to enable or disable the
 *  peripheral clock to the resource.  It provides drivers the ability to
 *  register a callback function upon a specific power event.  In addition,
 *  drivers and apps can set or release constraints to prevent the MCU from
 *  transitioning into a particular sleep state.
 *
 *  ============================================================================
 */

#ifndef ti_drivers_Power__include
#define ti_drivers_Power__include

#include <stdbool.h>
#include <stdint.h>

#include <ti/drivers/utils/List.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Power latency types */
#define Power_TOTAL               (1U)   /*!< total latency */
#define Power_RESUME              (2U)   /*!< resume latency */

/* Power notify responses */
#define Power_NOTIFYDONE          (0)   /*!< OK, notify completed */
#define Power_NOTIFYERROR         (-1)  /*!< an error occurred during notify */

/* Power status */
#define Power_SOK                 (0)    /*!< OK, operation succeeded */
#define Power_EFAIL               (-1)   /*!< general failure */
#define Power_EINVALIDINPUT       (-2)   /*!< invalid data value */
#define Power_EINVALIDPOINTER     (-3)   /*!< invalid pointer */
#define Power_ECHANGE_NOT_ALLOWED (-4)   /*!< change is not allowed */
#define Power_EBUSY               (-5)   /*!< busy with another transition */

/* Power transition states */
#define Power_ACTIVE              (1U)   /*!< normal active state */
#define Power_ENTERING_SLEEP      (2U)   /*!< entering a sleep state */
#define Power_EXITING_SLEEP       (3U)   /*!< exiting a sleep state */
#define Power_ENTERING_SHUTDOWN   (4U)   /*!< entering a shutdown state */
#define Power_CHANGING_PERF_LEVEL (5U)   /*!< moving to new performance level */


/*!
 *  @brief      Power policy initialization function pointer
 */
typedef void (*Power_PolicyInitFxn)(void);

/*!
 *  @brief      Power policy function pointer
 */
typedef void (*Power_PolicyFxn)(void);

/*!
 *  @brief      Power notify function pointer
 */
typedef int_fast16_t (*Power_NotifyFxn)(uint_fast16_t eventType,
     uintptr_t eventArg, uintptr_t clientArg);

/*!
 *  @brief      Power notify object structure.
 *
 *  This struct specification is for internal use.  Notification clients must
 *  pre-allocate a notify object when registering for a notification;
 *  Power_registerNotify() will take care initializing the internal elements
 *  appropriately.
 */
typedef struct Power_NotifyObj_ {
    List_Elem link;             /*!< for placing on the notify list */
    uint_fast16_t eventTypes;   /*!< the event type */
    Power_NotifyFxn notifyFxn;  /*!< notification function */
    uintptr_t clientArg;        /*!< argument provided by client */
} Power_NotifyObj;

/*!
 *  @brief  Disable the configured power policy from running when the CPU is
 *  idle
 *
 *  Calling this function clears the flag that controls whether the configured
 *  power policy function is invoked on each pass through the Idle loop.
 *  This function call will override both a 'true' setting of the
 *  "enablePolicy" setting in the Power manager configuration object, as well
 *  as a previous runtime call to the Power_enablePolicy() function.
 *
 *  @return The old value of "enablePolicy".
 *
 *  @sa     Power_enablePolicy
 */
bool Power_disablePolicy(void);

/*!
 *  @brief  Enable the configured power policy to run when the CPU is idle
 *
 *  Calling this function sets a flag that will cause the configured power
 *  policy function to be invoked on each pass through the Idle loop. This
 *  function call will override both a 'false' setting of the "enablePolicy"
 *  setting in the Power manager configuration object, as well as a previous
 *  runtime call to the Power_disablePolicy() function.
 *
 *  For some processor families, automatic power transitions can make initial
 *  application development more difficult, as well as being at odds with
 *  basic debugger operation.  This convenience function allows an application
 *  to be initially configured, built, and debugged, without automatic power
 *  transitions during idle time.  When the application is found to be working,
 *  this function can be called (typically in main()) to enable the policy
 *  to run, without having to change the application configuration.
 *
 *  @sa     Power_disablePolicy
 */
void Power_enablePolicy(void);

/*!
 *  @brief  Get the constraints that have been declared with Power
 *
 *  This function returns a bitmask indicating the constraints that are
 *  currently declared to the Power manager (via previous calls to
 *  Power_setConstraint()).  For each constraint that is currently declared,
 *  the corresponding bit in the bitmask will be set.  For example, if two
 *  clients have independently declared two different constraints, the returned
 *  bitmask will have two bits set.
 *
 *  Constraint identifiers are device specific, and defined in the
 *  device-specific Power include file.  For example, the constraints for
 *  MSP432 are defined in PowerMSP432.h.  The corresponding bit in the
 *  bitmask returned by this function can be derived by a left-shift using
 *  the constraint identifier.  For example, for MSP432, for the corresponding
 *  bit for the PowerMSP432_DISALLOW_SLEEP constraint, the bit position is
 *  determined by the operation: (1 << PowerMSP432_DISALLOW_SLEEP)
 *
 *  @return A bitmask of the currently declared constraints.
 *
 *  @sa     Power_setConstraint
 */
uint_fast32_t Power_getConstraintMask(void);

/*!
 *  @brief  Get the current dependency count for a resource
 *
 *  This function returns the number of dependencies that are currently
 *  declared upon a resource.
 *
 *  Resource identifiers are device specific, and defined in the
 *  device-specific Power include file.  For example, the resources for
 *  CC32XX are defined in PowerCC32XX.h.
 *
 *  @param  resourceId  resource id
 *
 *  @return The number of dependencies declared for the resource.
 *          Power_EINVALIDINPUT if the resourceId is invalid.
 *
 *  @sa     Power_setDependency
 */
int_fast16_t Power_getDependencyCount(uint_fast16_t resourceId);

/*!
 *  @brief  Get the current performance level
 *
 *  This function returns the current device performance level in effect.
 *
 *  If performance scaling is not supported for the device, this function
 *  will always indicate a performance level of zero.
 *
 *  @return The current performance level.
 *
 *  @sa     Power_setPerformanceLevel
 */
uint_fast16_t Power_getPerformanceLevel(void);

/*!
 *  @brief  Get the hardware transition latency for a sleep state
 *
 *  This function reports the minimal hardware transition latency for a specific
 *  sleep state.  The reported latency is that for a direct transition, and does
 *  not include any additional latency that might occur due to software-based
 *  notifications.
 *
 *  Sleep states are device specific, and defined in the device-specific Power
 *  include file.  For example, the sleep states for CC32XX are defined in
 *  PowerCC32XX.h.
 *
 *  This function is typically called by the power policy function. The latency
 *  is reported in units of microseconds.
 *
 *  @param  sleepState  the sleep state
 *
 *  @param  type        the latency type (Power_TOTAL or Power_RESUME)
 *
 *  @return The latency value, in units of microseconds.
 */
uint_fast32_t Power_getTransitionLatency(uint_fast16_t sleepState,
    uint_fast16_t type);

/*!
 *  @brief  Get the current transition state of the Power manager
 *
 *  This function returns the current transition state for the Power manager.
 *  For example, when no transitions are in progress, a status of Power_ACTIVE
 *  is returned.  Power_ENTERING_SLEEP is returned during the transition to
 *  sleep, before sleep has occurred. Power_EXITING_SLEEP is returned
 *  after wakeup, as the device is being transitioned back to Power_ACTIVE.
 *  And Power_CHANGING_PERF_LEVEL is returned when a change is being made
 *  to the performance level.
 *
 *  @return The current Power manager transition state.
 */
uint_fast16_t Power_getTransitionState(void);

/*!
 *  @brief  Power function to be added to the application idle loop
 *
 *  This function should be added to the application idle loop. (The method to
 *  do this depends upon the operating system being used.)  This function
 *  will invoke the configured power policy function when appropriate. The
 *  specific policy function to be invoked is configured as the 'policyFxn'
 *  in the application-defined Power configuration object.
 *
 */
void Power_idleFunc(void);

/*!
 *  @brief  Power initialization function
 *
 *  This function initializes Power manager internal state.  It must be called
 *  prior to any other Power API.  This function is normally called as part
 *  of TI-RTOS board initialization, for example, from within the
 *  \<board name\>_initGeneral() function.
 *
 *  @return Power_SOK
 */
int_fast16_t Power_init(void);

/*!
 *  @brief  Register a function to be called upon a specific power event
 *
 *  This function registers a function to be called when a Power event occurs.
 *  Registrations and the corresponding notifications are processed in
 *  first-in-first-out (FIFO) order. The function registered must behave as
 *  described later, below.
 *
 *  The pNotifyObj parameter is a pointer to a pre-allocated, opaque object
 *  that will be used by Power to support the notification.  This object could
 *  be dynamically allocated, or declared as a global object. This function
 *  will properly initialized the object's fields as appropriate; the caller
 *  just needs to provide a pointer to this pre-existing object.
 *
 *  The eventTypes parameter identifies the type of power event(s) for which
 *  the notify function being registered is to be called. (Event identifiers are
 *  device specific, and defined in the device-specific Power include file.
 *  For example, the events for MSP432 are defined in PowerMSP432.h.)  The
 *  eventTypes parameter for this function call is treated as a bitmask, so
 *  multiple event types can be registered at once, using a common callback
 *  function.  For example, to call the specified notifyFxn when both
 *  the entering deepsleep and awake from deepsleep events occur, eventTypes
 *  should be specified as: PowerMSP432_ENTERING_DEEPSLEEP |
 *  PowerMSP432_AWAKE_DEEPSLEEP
 *
 *  The notifyFxn parameter specifies a callback function to be called when the
 *  specified Power event occurs. The notifyFxn must implement the following
 *  signature:
 *       status = notifyFxn(eventType, eventArg, clientArg);
 *
 *  Where: eventType identifies the event being signalled, eventArg is an
 *  optional event-specific argument, and clientArg is an abitrary argument
 *  specified by the client at registration.  Note that multipe types of events
 *  can be specified when registering the notification callback function,
 *  but when the callback function is actually called by Power, only a
 *  single eventType will be specified for the callback (i.e., the current
 *  event).  The status returned by the client notification function must
 *  be one of the following constants: Power_NOTIFYDONE if the client processed
 *  the notification successfully, or Power_NOTIFYERROR if an error occurred
 *  during notification.
 *
 *  The clientArg parameter is an arbitrary, client-defined argument to be
 *  passed back to the client upon notification. This argument may allow one
 *  notify function to be used by multiple instances of a driver (that is, the
 *  clientArg can be used to identify the instance of the driver that is being
 *  notified).
 *
 *  @param  pNotifyObj      notification object (preallocated by caller)
 *
 *  @param  eventTypes      event type or types
 *
 *  @param  notifyFxn       client's callback function
 *
 *  @param  clientArg       client-specified argument to pass with notification
 *
 *  @return  Power_SOK on success.
 *           Power_EINVALIDPOINTER if either pNotifyObj or notifyFxn are NULL.
 *
 *  @sa     Power_unregisterNotify
 */
int_fast16_t Power_registerNotify(Power_NotifyObj *pNotifyObj,
                                  uint_fast16_t eventTypes,
                                  Power_NotifyFxn notifyFxn,
                                  uintptr_t clientArg);

/*!
 *  @brief  Release a previously declared constraint
 *
 *  This function releases a constraint that was previously declared with
 *  Power_setConstraint().  For example, if a device driver is starting an I/O
 *  transaction and wants to prohibit activation of a sleep state during the
 *  transaction, it uses Power_setConstraint() to declare the constraint,
 *  before starting the transaction.  When the transaction completes, the
 *  driver calls this function to release the constraint, to allow the Power
 *  manager to once again allow transitions to sleep.
 *
 *  Constraint identifiers are device specific, and defined in the
 *  device-specific Power include file.  For example, the constraints for
 *  MSP432 are defined in PowerMSP432.h.
 *
 *  Only one constraint can be specified with each call to this function; to
 *  release multiple constraints this function must be called multiple times.
 *
 *  It is critical that clients call Power_releaseConstraint() when operational
 *  constraints no longer exists. Otherwise, Power may be left unnecessarily
 *  restricted from activating power savings.
 *
 *  @param  constraintId      constraint id
 *
 *  @return <b>CC26XX/CC13XX only</b>: Power_SOK. To minimize code size
 *          asserts are used internally to check that the constraintId is valid,
 *          and that the constraint count is not already zero;
 *          the function always returns Power_SOK.
 *
 *  @return <b>All other devices</b>: Power_SOK on success,
 *          Power_EINVALIDINPUT if the constraintId is invalid, and Power_EFAIL
 *          if the constraint count is already zero.
 *
 *  @sa     Power_setConstraint
 */
int_fast16_t Power_releaseConstraint(uint_fast16_t constraintId);

/*!
 *  @brief  Release a previously declared dependency
 *
 *  This function releases a dependency that had been previously declared upon
 *  a resource (by a call to Power_setDependency()).
 *
 *  Resource identifiers are device specific, and defined in the
 *  device-specific Power include file.  For example, the resources for
 *  CC32XX are defined in PowerCC32XX.h.
 *
 *  @param  resourceId      resource id
 *
 *  @return <b>CC26XX/CC13XX only</b>: Power_SOK. To minimize code size
 *          asserts are used internally to check that the resourceId is valid,
 *          and that the resource reference count is not already zero;
 *          the function always returns Power_SOK.
 *
 *  @return <b>All other devices</b>: Power_SOK on success,
 *          Power_EINVALIDINPUT if the resourceId is invalid, and Power_EFAIL
 *          if the resource reference count is already zero.
 *
 *  @sa     Power_setDependency
 */
int_fast16_t Power_releaseDependency(uint_fast16_t resourceId);

/*!
 *  @brief  Declare an operational constraint
 *
 *  Before taking certain actions, the Power manager checks to see if the
 *  requested action would conflict with a client-declared constraint. If the
 *  action does conflict, Power will not proceed with the request.  This is the
 *  function that allows clients to declare their constraints with Power.
 *
 *  Constraint identifiers are device specific, and defined in the
 *  device-specific Power include file.  For example, the constraints for
 *  MSP432 are defined in PowerMSP432.h.
 *
 *  Only one constraint can be specified with each call to this function; to
 *  declare multiple constraints this function must be called multiple times.
 *
 *  @param  constraintId      constraint id
 *
 *  @return <b>CC26XX/CC13XX only</b>: Power_SOK. To minimize code size an
 *          assert is used internally to check that the constraintId is valid;
 *          the function always returns Power_SOK.
 *
 *  @return <b>All other devices</b>: Power_SOK on success,
 *          Power_EINVALIDINPUT if the constraintId is invalid.
 *
 *  @sa     Power_releaseConstraint
 */
int_fast16_t Power_setConstraint(uint_fast16_t constraintId);

/*!
 *  @brief  Declare a dependency upon a resource
 *
 *  This function declares a dependency upon a resource. For example, if a
 *  UART driver needs a specific UART peripheral, it uses this function to
 *  declare this to the Power manager.  If the resource had been inactive,
 *  then Power will activate the peripheral during this function call.
 *
 *  What is needed to make a peripheral resource 'active' will vary by device
 *  family. For some devices this may be a simple enable of a clock to the
 *  specified peripheral.  For others it may also require a power on of a
 *  power domain.  In either case, the Power manager will take care of these
 *  details, and will also implement reference counting for resources and their
 *  interdependencies.  For example, if multiple UART peripherals reside in
 *  a shared serial power domain, the Power manager will power up the serial
 *  domain when it is first needed, and then automatically power the domain off
 *  later, when all related dependencies for the relevant peripherals are
 *  released.
 *
 *  Resource identifiers are device specific, and defined in the
 *  device-specific Power include file.  For example, the resources for
 *  CC32XX are defined in PowerCC32XX.h.
 *
 *  @param  resourceId      resource id
 *
 *  @return <b>CC26XX/CC13XX only</b>: Power_SOK. To minimize code size an
 *          assert is used internally to check that the resourceId is valid;
 *          the function always returns Power_SOK.
 *
 *  @return <b>All other devices</b>: Power_SOK on success,
 *          Power_EINVALIDINPUT if the reseourceId is invalid.
 *
 *  @sa     Power_releaseDependency
 */
int_fast16_t Power_setDependency(uint_fast16_t resourceId);

/*!
 *  @brief  Set the MCU performance level
 *
 *  This function manages a transition to a new device performance level.
 *  Before the actual transition is initiated, notifications will be sent to
 *  any clients who've registered (with Power_registerNotify()) for a
 *  'start change performance level' notification.  The event name is device
 *  specific, and defined in the device-specific Power include file.  For
 *  example, for MSP432, the event is "PowerMSP432_START_CHANGE_PERF_LEVEL",
 *  which is defined in PowerMSP432.h.  Once notifications have been completed,
 *  the change to the performance level is initiated.  After the level change
 *  is completed, there is a comparable event that can be used to signal a
 *  client that the change has completed.  For example, on MSP432 the
 *  "PowerMSP432_DONE_CHANGE_PERF_LEVEL" event can be used to signal
 *  completion.
 *
 *  This function will not return until the new performance level is in effect.
 *  If performance scaling is not supported for the device, or is prohibited
 *  by an active constraint, or if the specified level is invalid, then an
 *  error status will be returned.
 *
 *  @param  level      the new performance level
 *
 *  @return Power_SOK on success.
 *          Power_EINVALIDINPUT if the specified performance level is out of
 *          range of valid levels.
 *          Power_EBUSY if another transition is already in progress, or if
 *          a single constraint is set to prohibit any change to the
 *          performance level.
 *          Power_ECHANGE_NOT_ALLOWED if a level-specific constraint prohibits
 *          a change to the requested level.
 *          Power_EFAIL if performance scaling is not supported, if an
 *          error occurred during initialization, or if an error occurred
 *          during client notifications.
 *
 *  @sa     Power_getPerformanceLevel
 */
int_fast16_t Power_setPerformanceLevel(uint_fast16_t level);

/*!
 *  @brief  Set a new Power policy
 *
 *  This function allows a new Power policy function to be selected at runtime.
 *
 *  @param  policy      the new Power policy function
 */
void Power_setPolicy(Power_PolicyFxn policy);

/*!
 *  @brief  Put the device into a shutdown state
 *
 *  This function will transition the device into a shutdown state.
 *  Before the actual transition is initiated, notifications will be sent to
 *  any clients who've registered (with Power_registerNotify()) for an
 *  'entering shutdown' event.  The event name is device specific, and defined
 *  in the device-specific Power include file.  For example, for CC32XX, the
 *  event is "PowerCC32XX_ENTERING_SHUTDOWN", which is defined in
 *  PowerCC32XX.h.  Once notifications have been completed, the device shutdown
 *  will commence.
 *
 *  If the device is successfully transitioned to shutdown, this function
 *  call will never return.  Upon wakeup, the device and application will
 *  be rebooted (through a device reset).  If the transition is not
 *  successful, one of the error codes listed below will be returned.
 *
 *  On some devices a timed wakeup from shutdown can be specified, using
 *  the shutdownTime parameter.  This enables an autonomous application reboot
 *  at a future time.  For example, an application can go to shutdown, and then
 *  automatically reboot at a future time to do some work. And once that work
 *  is done, the application can shutdown again, for another timed interval.
 *  The time interval is specified via the shutdownTime parameter. (On devices
 *  that do not support this feature, any value specified for shutdownTime will
 *  be ignored.)  If the specified shutdownTime is less than the total
 *  shutdown latency for the device, then shutdownTime will be ignored.  The
 *  shutdown latency for the device can be found in the device-specific Power
 *  include file.  For example, for the CC32XX, this latency is defined in
 *  PowerCC32XX.h, as "PowerCC32XX_TOTALTIMESHUTDOWN".)
 *
 *  @param  shutdownState  the device-specific shutdown state
 *
 *  @param  shutdownTime   the amount of time (in milliseconds) to keep the
 *                         the device in the shutdown state; this parameter
 *                         is not supported on all device families
 *
 *  @return Power_ECHANGE_NOT_ALLOWED if a constraint is prohibiting shutdown.
 *          Power_EFAIL if an error occurred during client notifications.
 *          Power_EINVALIDINPUT if the shutdownState is invalid.
 *          Power_EBUSY if another transition is already in progress.
 */
int_fast16_t Power_shutdown(uint_fast16_t shutdownState,
    uint_fast32_t shutdownTime);

/*!
 *  @brief  Transition the device into a sleep state
 *
 *  This function is called from the power policy when it has made a decision
 *  to put the device in a specific sleep state.  This function returns to the
 *  caller (the policy function) once the device has awoken from sleep.
 *
 *  This function must be called with interrupts disabled, and should not be
 *  called directly by the application, or by any drivers.
 *  This function does not check declared constraints; the policy function
 *  must check constraints before calling this function to initiate sleep.
 *
 *  @param  sleepState      the sleep state
 *
 *  @return Power_SOK on success, the device has slept and is awake again.
 *          Power_EFAIL if an error occurred during client notifications, or
 *          if a general failure occurred.
 *          Power_EINVALIDINPUT if the sleepState is invalid.
 *          Power_EBUSY if another transition is already in progress.
 */
int_fast16_t Power_sleep(uint_fast16_t sleepState);

/*!
 *  @brief  Unregister previously registered notifications
 *
 *  This function unregisters for event notifications that were previously
 *  registered with Power_registerNotify().  The caller must specify a pointer
 *  to the same notification object used during registration.
 *
 *  @param  pNotifyObj    notify object
 *
 *  @sa     Power_registerNotify
 */
void Power_unregisterNotify(Power_NotifyObj *pNotifyObj);

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_Power__include */
