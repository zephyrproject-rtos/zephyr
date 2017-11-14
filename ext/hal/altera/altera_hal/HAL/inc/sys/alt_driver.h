#ifndef __ALT_DRIVER_H__
#define __ALT_DRIVER_H__

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2006 Altera Corporation, San Jose, California, USA.           *
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
* Altera does not recommend, suggest or require that this reference design    *
* file be used in conjunction or combination with any other product.          *
******************************************************************************/

/*
 * Macros used to access a driver without HAL file descriptors.
 */

/*
 * ALT_MODULE_CLASS
 *
 * This macro returns the module class name for the specified module instance.
 * It uses information in the system.h file.
 * Neither the instance name or class name are quoted (so that they can
 * be used with other pre-processor macros).
 *
 * Example:
 *   Assume the design has an instance of an altera_avalon_uart called uart1.
 *   Calling ALT_MODULE_CLASS(uart1) returns altera_avalon_uart.
 */

#define ALT_MODULE_CLASS(instance) ALT_MODULE_CLASS_ ## instance


/*
 * ALT_DRIVER_FUNC_NAME
 *
 *   --> instance               Instance name.
 *   --> func                   Function name.
 *
 * This macro returns the device driver function name of the specified
 * module instance for the specified function name.
 *
 * Example:
 *   Assume the design has an instance of an altera_avalon_uart called uart1.
 *   Calling ALT_DRIVER_FUNC_NAME(uart1, write) returns 
 *   altera_avalon_uart_write.
 */

#define ALT_DRIVER_FUNC_NAME(instance, func) \
  ALT_DRIVER_FUNC_NAME1(ALT_MODULE_CLASS(instance), func)
#define ALT_DRIVER_FUNC_NAME1(module_class, func) \
  ALT_DRIVER_FUNC_NAME2(module_class, func)
#define ALT_DRIVER_FUNC_NAME2(module_class, func) \
  module_class ## _ ## func

/*
 * ALT_DRIVER_STATE_STRUCT
 *
 *   --> instance               Instance name.
 *
 * This macro returns the device driver state type name of the specified
 * module instance.
 *
 * Example:
 *   Assume the design has an instance of an altera_avalon_uart called uart1.
 *   Calling ALT_DRIVER_STATE_STRUCT(uart1) returns:
 *     struct altera_avalon_uart_state_s
 *
 * Note that the ALT_DRIVER_FUNC_NAME macro is used even though "state" isn't
 * really a function but it does match the required naming convention.
 */
#define ALT_DRIVER_STATE_STRUCT(instance) \
    struct ALT_DRIVER_FUNC_NAME(instance, state_s)

/*
 * ALT_DRIVER_STATE
 *
 *   --> instance               Instance name.
 *
 * This macro returns the device driver state name of the specified
 * module instance.
 *
 * Example:
 *   Assume the design has an instance of an altera_avalon_uart called uart1.
 *   Calling ALT_DRIVER_STATE(uart1) returns uart1.
 */
#define ALT_DRIVER_STATE(instance) instance

/*
 * ALT_DRIVER_WRITE
 *
 *   --> instance               Instance name.
 *   --> buffer                 Write buffer.
 *   --> len                    Length of write buffer data.
 *   --> flags                  Control flags (e.g. O_NONBLOCK)
 *
 * This macro calls the "write" function of the specified driver instance.
 */

#define ALT_DRIVER_WRITE_EXTERNS(instance)                                  \
    extern ALT_DRIVER_STATE_STRUCT(instance) ALT_DRIVER_STATE(instance);    \
    extern int ALT_DRIVER_FUNC_NAME(instance, write)                        \
      (ALT_DRIVER_STATE_STRUCT(instance) *, const char *, int, int);

#define ALT_DRIVER_WRITE(instance, buffer, len, flags)                      \
    ALT_DRIVER_FUNC_NAME(instance, write)(&ALT_DRIVER_STATE(instance), buffer, len, flags)


/*
 * ALT_DRIVER_READ
 *
 *   --> instance               Instance name.
 *   <-- buffer                 Read buffer.
 *   --> len                    Length of read buffer.
 *   --> flags                  Control flags (e.g. O_NONBLOCK)
 *
 * This macro calls the "read" function of the specified driver instance.
 */

#define ALT_DRIVER_READ_EXTERNS(instance)                                   \
    extern ALT_DRIVER_STATE_STRUCT(instance) ALT_DRIVER_STATE(instance);    \
    extern int ALT_DRIVER_FUNC_NAME(instance, read)                         \
      (ALT_DRIVER_STATE_STRUCT(instance) *, const char *, int, int);

#define ALT_DRIVER_READ(instance, buffer, len, flags)                       \
    ALT_DRIVER_FUNC_NAME(instance, read)(&ALT_DRIVER_STATE(instance), buffer, len, flags)

/*
 * ALT_DRIVER_IOCTL
 *
 *   --> instance               Instance name.
 *   --> req                    ioctl request (e.g. TIOCSTIMEOUT)
 *   --> arg                    Optional argument (void*)
 *
 * This macro calls the "ioctl" function of the specified driver instance
 */

#define ALT_DRIVER_IOCTL_EXTERNS(instance)                                  \
    extern ALT_DRIVER_STATE_STRUCT(instance) ALT_DRIVER_STATE(instance);    \
    extern int ALT_DRIVER_FUNC_NAME(instance, ioctl)                        \
      (ALT_DRIVER_STATE_STRUCT(instance) *, int, void*);

#define ALT_DRIVER_IOCTL(instance, req, arg)                                \
    ALT_DRIVER_FUNC_NAME(instance, ioctl)(&ALT_DRIVER_STATE(instance), req, arg)

#endif /* __ALT_DRIVER_H__ */
