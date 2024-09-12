/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************

 @file  hal_assert.h

  @brief This header file contains the Hardware Abstraction Layer (HAL) Assert
         defines, types, and API functions for how asserts are handled in system
         software. The assert handler, and its behavior, depend on the build
         time define:

         HALNODEBUG       - No action ever.
         RCN_APP_ASSERT   - Remote TI Application Callback.
         EXT_HAL_ASSERT   - Extended Application Callback.
         ICALL_HAL_ASSERT - Maps asserts to ICall_abort.
         Default          - Legacy HAL assert handler. Depends on following
                            additional build time define:
                            HAL_ASSERT_RESET  - Reset the device.
                            HAL_ASSERT_LIGHTS - Flash the LEDs.
                            HAL_ASSERT_SPIN   - Spinlock.
                            Otherwise:        - Just return.

 *****************************************************************************/

#ifndef HAL_ASSERT_H
#define HAL_ASSERT_H

/*******************************************************************************
 * INCLUDES
 */

#include "hal_types.h"
#include "hal_defs.h"

#ifdef CC33xx
#include "debug_assert.h"
#endif //CC33xx

#ifdef  RCN_APP_ASSERT
#include "hal_appasrt.h"
#endif /* RCN_APP_ASSERT */

/*******************************************************************************
 * MACROS
 */

// Set Subcause Macro
#define HAL_ASSERT_SET_SUBCAUSE(c) st( assertSubcause = (c); )

// Generic Spinlock Macro
#define HAL_ASSERT_SPINLOCK        st( volatile uint8 i=1; while(i); )

/*******************************************************************************
 * CONSTANTS
 */

// API Parameters

// Legacy Mode for FALSE
// Enabled  - A FALSE calls halAssertHandler.
// Disabled - A FALSE is passed to halAssertHandlerExt.
#define HAL_ASSERT_LEGACY_MODE_DISABLED         0
#define HAL_ASSERT_LEGACY_MODE_ENABLED          1

// Common Assert Causes (0x00 .. 0x1F)
#define HAL_ASSERT_CAUSE_FALSE                  0x00
#define HAL_ASSERT_CAUSE_TRUE                   0x01
#define HAL_ASSERT_CAUSE_INTERNAL_ERROR         0x02
#define HAL_ASSERT_CAUSE_HW_ERROR               0x03
#define HAL_ASSERT_CAUSE_OUT_OF_MEMORY          0x04
#define HAL_ASSERT_CAUSE_ICALL_ABORT            0x05
#define HAL_ASSERT_CAUSE_ICALL_TIMEOUT          0x06
#define HAL_ASSERT_CAUSE_WRONG_API_CALL         0x07
#define HAL_ASSERT_CAUSE_HARDWARE_ERROR         0x08
#define HAL_ASSERT_CAUSE_RF_DRIVER_ERROR        0x09
#define HAL_ASSERT_CAUSE_UNEXPECTED_ERROR       0x0A
#define HAL_ASSERT_CAUSE_STACK_OVERFLOW_ERROR   0x0B

// BLE Assert Causes (0x20 .. 0x3F)

// MAC Assert Causes (0x40 .. 0x5F)

// RCN Assert Causes (0x60 .. 0x7F)

// Reserved (0x80 .. 0xFF)

// Common Assert Subcauses (0x00 .. 0x1F)
// Note: Subcauses for project asserts are completely defined by the project.
#define HAL_ASSERT_SUBCAUSE_NONE                0x00
#define HAL_ASSERT_SUBCAUSE_FW_INERNAL_ERROR    0x01
#define HAL_ASSERT_OUT_OF_HEAP                  0x02
#define HAL_ASSERT_SUBCAUSE_                    0x03

// Project Specific Assert Subcauses (0x20 .. 0xFF)
// Note: Subcauses for project asserts are completely defined by the project.

////////////////////////////////////////////////////////////////////////////////
// Example Usage:
// In ble_user_config: halAssertInit( myAssertHandler,
//                                    HAL_ASSERT_LEGACY_MODE_DISABLED );
// In code:
// :
// HAL_ASSERT_SET_SUBCAUSE( HAL_ASSERT_SUBCAUSE_FW_INERNAL_ERROR );
// HAL_ASSERT( HAL_ASSERT_CAUSE_INTERNAL_ERROR );
// :
// HAL_ASSERT( a == b );
// :
// with legacyMode set to HAL_ASSERT_LEGACY_MODE_ENABLED
// if ( mPtr == NULL) HAL_ASSERT( HAL_ASSERT_CAUSE_OUT_OF_MEMORY );
// :
// with legacyMode set to HAL_ASSERT_LEGACY_MODE_DISABLED
// HAL_ASSERT_SET_SUBCAUSE( HAL_ASSERT_OUT_OF_HEAP );
// HAL_ASSERT( mPtr == NULL );
// :
////////////////////////////////////////////////////////////////////////////////

/*
 *  HAL_ASSERT( expression ) - The given expression must evaluate as "true" or
 *  else the assert handler is called.  From here, the call stack feature of
 *  the debugger can pinpoint where the problem occurred.
 *
 *  HAL_ASSERT_FORCED( ) - If asserts are in use, immediately calls the
 *  assert handler.
 *
 *  HAL_ASSERT_STATEMENT( statement ) - Inserts the given C statement but only
 *  when asserts are in use.  This macros allows debug code that is not part
 *  of an expression.
 *
 *  HAL_ASSERT_DECLARATION( declaration ) - Inserts the given C declaration
 *  but only when asserts are in use.  This macros allows debug code that is
 *  not part of an expression.
 *
 *  Asserts can be disabled for optimum performance and minimum code size
 *  (ideal for finalized, debugged production code).  To disable, define the
 *  preprocessor symbol HALNODEBUG at the project level.
 */

#ifdef HALNODEBUG
#define HAL_ASSERT(expr)
#define HAL_ASSERT_FORCED()
#define HAL_ASSERT_STATEMENT(statement)
#define HAL_ASSERT_DECLARATION(declaration)
#elif  defined(RCN_APP_ASSERT)
#define HAL_ASSERT(expr)                     st( if (!(expr)) HAL_APPASRT_callAssrtHanlder(); )
#define HAL_ASSERT_FORCED()                  HAL_APPASRT_callAssrtHanlder()
#define HAL_ASSERT_STATEMENT(statement)      st( statement )
#define HAL_ASSERT_DECLARATION(declaration)  declaration
#elif  defined(EXT_HAL_ASSERT)
#define HAL_ASSERT(cause)                    st( if ((cause)!=TRUE) halAssertHandlerExt((cause)); )
#define HAL_ASSERT_FORCED()                  halAssertHandlerExt( FALSE );
#define HAL_ASSERT_STATEMENT(statement)      st( statement )
#define HAL_ASSERT_DECLARATION(declaration)  declaration
#elif  defined(ICALL_HAL_ASSERT)
#define HAL_ASSERT(expr)                     st( if (!(expr)) ICall_abort(); )
#define HAL_ASSERT_FORCED()                  ICall_abort();
#define HAL_ASSERT_STATEMENT(statement)      st( statement )
#define HAL_ASSERT_DECLARATION(declaration)  declaration
#elif  defined(CC33xx)
#define HAL_ASSERT(expr)                     ASSERT_BLE(expr)
#define HAL_ASSERT_FORCED()                  ASSERT_BLE(FALSE);
#define HAL_ASSERT_STATEMENT(statement)      // st( statement )
#define HAL_ASSERT_DECLARATION(declaration)  // declaration
#else // default handler LEGACY_HAL_ASSERT
#define HAL_ASSERT(expr)                     st( if (!(expr)) halAssertHandler(); )
#define HAL_ASSERT_FORCED()                  halAssertHandler();
#define HAL_ASSERT_STATEMENT(statement)      st( statement )
#define HAL_ASSERT_DECLARATION(declaration)  declaration
#endif

/*
 *  This macro compares the size of the first parameter to the integer value
 *  of the second parameter.  If they do not match, a compile time error for
 *  negative array size occurs (even gnu chokes on negative array size).
 *
 *  This compare is done by creating a typedef for an array.  No variables are
 *  created and no memory is consumed with this check.  The created type is
 *  used for checking only and is not for use by any other code.  The value
 *  of 10 in this macro is arbitrary, it just needs to be a value larger
 *  than one to result in a positive number for the array size.
 */
#define HAL_ASSERT_SIZE(x,y) typedef char x ## _assert_size_t[-1+10*(sizeof(x) == (y))]

/*******************************************************************************
 * TYPEDEFS
 */

// User Defined HAL Assert Callback
typedef void (*assertCback_t)( uint8 assertCause, uint8 assertSubcause );

/*******************************************************************************
 * EXTERNS
 */

extern uint8 assertSubcause;

// API
extern void halAssertInit( assertCback_t assertCback, uint8 legacyMode );
extern void halAssertHandler( void );
extern void halAssertHandlerExt( uint8 assertCause );
extern void halAssertSpinlock( void );
extern void halAssertHazardLights( void );

#endif /* HAL_ASSERT_H */
