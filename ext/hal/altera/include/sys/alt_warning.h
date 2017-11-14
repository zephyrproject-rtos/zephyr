#ifndef __WARNING_H__
#define __WARNING_H__

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2003 Altera Corporation, San Jose, California, USA.           *
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

/*
 * alt_warning.h provides macro definitions that can be used to generate link 
 * time warnings.
 */ 

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * The symbol "__alt_invalid" is used to force a link error. There should be 
 * no corresponding implementation of this function.
 */  

extern void __alt_invalid (void);

#define ALT_LINK_WARNING(symbol, msg)                                                      \
  __asm__(".ifndef __evoke_link_warning_" #symbol                                          \
          "\n\t .section .gnu.warning." #symbol                                            \
          "\n__evoke_link_warning_" #symbol ":\n\t .string \x22" msg "\x22 \n\t .previous" \
          "\n .endif");

/* A canned warning for sysdeps/stub functions.  */

#define ALT_STUB_WARNING(name) \
  ALT_LINK_WARNING (name, \
    "warning: " #name " is not implemented and will always fail")

#define ALT_OBSOLETE_FUNCTION_WARNING(name) \
  ALT_LINK_WARNING (name, \
    "warning: " #name " is a deprecated function")

#define ALT_LINK_ERROR(msg) \
  ALT_LINK_WARNING (__alt_invalid, msg); \
  __alt_invalid()

#ifdef __cplusplus
}
#endif

#endif /* __WARNING_H__ */
