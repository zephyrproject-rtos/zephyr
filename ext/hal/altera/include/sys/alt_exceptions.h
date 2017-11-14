#ifndef __ALT_EXCEPTIONS_H__
#define __ALT_EXCEPTIONS_H__

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2008 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
******************************************************************************/
#include "alt_types.h"
#include "system.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * This file defines instruction-generated exception handling and registry
 * API, exception type enumeration, and handler return value enumeration for
 * Nios II.
 */

/*
 * The following enumeration describes the value in the CPU EXCEPTION
 * register CAUSE bit field.
 */
enum alt_exception_cause_e {
  /*
   * This value is passed to an exception handler's cause argument if
   * "extra exceptions" information (EXECPTION) register is not
   * present in the processor hardware configuration.
   */
  NIOS2_EXCEPTION_CAUSE_NOT_PRESENT            = -1,

  /*
   * Real values
   */
  NIOS2_EXCEPTION_RESET                        = 0,
  NIOS2_EXCEPTION_CPU_ONLY_RESET_REQUEST       = 1,
  NIOS2_EXCEPTION_INTERRUPT                    = 2,
  NIOS2_EXCEPTION_TRAP_INST                    = 3,
  NIOS2_EXCEPTION_UNIMPLEMENTED_INST           = 4,
  NIOS2_EXCEPTION_ILLEGAL_INST                 = 5,
  NIOS2_EXCEPTION_MISALIGNED_DATA_ADDR         = 6,
  NIOS2_EXCEPTION_MISALIGNED_TARGET_PC         = 7,
  NIOS2_EXCEPTION_DIVISION_ERROR               = 8,
  NIOS2_EXCEPTION_SUPERVISOR_ONLY_INST_ADDR    = 9,
  NIOS2_EXCEPTION_SUPERVISOR_ONLY_INST         = 10,
  NIOS2_EXCEPTION_SUPERVISOR_ONLY_DATA_ADDR    = 11,
  NIOS2_EXCEPTION_TLB_MISS                     = 12,
  NIOS2_EXCEPTION_TLB_EXECUTE_PERM_VIOLATION   = 13,
  NIOS2_EXCEPTION_TLB_READ_PERM_VIOLATION      = 14,
  NIOS2_EXCEPTION_TLB_WRITE_PERM_VIOLATION     = 15,
  NIOS2_EXCEPTION_MPU_INST_REGION_VIOLATION    = 16,
  NIOS2_EXCEPTION_MPU_DATA_REGION_VIOLATION    = 17,
  NIOS2_EXCEPTION_ECC_TLB_ERR                  = 18,
  NIOS2_EXCEPTION_ECC_FETCH_ERR                = 19,
  NIOS2_EXCEPTION_ECC_REGISTER_FILE_ERR        = 20,
  NIOS2_EXCEPTION_ECC_DATA_ERR                 = 21,
  NIOS2_EXCEPTION_ECC_DATA_CACHE_WRITEBACK_ERR = 22
};
typedef enum alt_exception_cause_e alt_exception_cause;

/*
 * These define valid return values for a user-defined instruction-generated
 * exception handler. The handler should return one of these to indicate
 * whether to re-issue the instruction that triggered the exception, or to
 * skip it.
 */
enum alt_exception_result_e {
  NIOS2_EXCEPTION_RETURN_REISSUE_INST = 0,
  NIOS2_EXCEPTION_RETURN_SKIP_INST    = 1
};
typedef enum alt_exception_result_e alt_exception_result;

/*
 * alt_instruction_exception_register() can be used to register an exception
 * handler for instruction-generated exceptions that are not handled by the
 * built-in exception handler (i.e. for interrupts).
 *
 * The registry API is optionally enabled through the "Enable
 * Instruction-related Exception API" HAL BSP setting, which will
 * define the macro below.
 */
#ifdef ALT_INCLUDE_INSTRUCTION_RELATED_EXCEPTION_API
void alt_instruction_exception_register (
  alt_exception_result (*exception_handler)(
    alt_exception_cause cause,
    alt_u32 exception_pc,
    alt_u32 bad_addr) );
#endif /*ALT_INCLUDE_INSTRUCTION_RELATED_EXCEPTION_API */

/*
 * alt_exception_cause_generated_bad_addr() indicates whether a particular
 * exception cause value was from an exception-type that generated a valid
 * address in the BADADDR register. The contents of BADADDR is passed to
 * a user-registered exception handler in all cases, whether valid or not.
 * This routine should be called to validate the bad_addr argument to
 * your exception handler.
 *
 * Note that this routine will return false (0) for causes 
 * NIOS2_EXCEPTION_TLB_MISS and NIOS2_EXCEPTION_ECC_TLB_ERR.
 * You must read the TLBMISC.D field to determine if BADADDR
 * is valid for these (valid if TLBMISC.D = 1).
 */
int alt_exception_cause_generated_bad_addr(alt_exception_cause cause);

/*
 * alt_ecc_fatal_exception_register() is called to register a handler to
 * service likely fatal ECC error exceptions. Likely the handler will
 * assume that correct execution of the running software is not possible
 * and re-initialize the processor (e.g. jump to reset address).
 * 
 * Passing null (0x0) in the handler argument will disable a previously-
 * registered handler.
 *
 * Note that if no handler is registered, just normal exception processing
 * occurs on a likely fatal ECC exception and the exception processing
 * code might trigger an infinite exception loop.
 *
 * Note that the handler isn't a C function: it must be written in 
 * assembly-code because it doesn't support C language calling conventions
 * and it can't return.
 *
 * The handler code must be carefully written to avoid triggering
 * another fatal ECC exception and creating an infinite exception loop.
 * The handler must avoid reading registers in case the fatal ECC
 * error is a register file ECC error.
 * If a data cache is present, the handler must avoid instructions that
 * access the data cache in case the fatal ECC error is a data cache 
 * related ECC error. This includes cacheable load, cacheable store, 
 * non-cacheable store (because it looks in the data cache to update the
 * data cache if it hits), and all data cache management instructions except
 * for INITD.
 */
void alt_ecc_fatal_exception_register(alt_u32 handler);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ALT_EXCEPTIONS_H__ */
