#ifndef __ALT_LICENSE_REMINDER_UCOSII_H__
#define __ALT_LICENSE_REMINDER_UCOSII_H__

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

/******************************************************************************
*                                                                             *
* THIS IS A LIBRARY READ-ONLY SOURCE FILE. DO NOT EDIT.                       *
*                                                                             *
******************************************************************************/

#include <stdio.h>

#define ALT_LICENSE_REMINDER_UCOSII_STRING                                     \
    "============== Software License Reminder ===============\n"               \
    "\n"                                                                       \
    "uC/OS-II is provided in source form for FREE evaluation,\n"               \
    "for educational use, or for peaceful research. If you\n"                  \
    "plan on using uC/OS-II in a commercial product you need\n"                \
    "to contact Micrium to properly license its use in your\n"                 \
    "product. Micrium provides ALL the source code on the\n"                   \
    "Altera distribution for your convenience and to help you\n"               \
    "experience uC/OS-II. The fact that the source is provided\n"              \
    "does NOT mean that you can use it without paying a\n"                     \
    "licensing fee. Please help us continue to provide the\n"                  \
    "Embedded community with the finest software available.\n"                 \
    "Your honesty is greatly appreciated.\n"                                   \
    "\n"                                                                       \
    "Please contact:\n"                                                        \
    "\n"                                                                       \
    "M I C R I U M\n"                                                          \
    "949 Crestview Circle\n"                                                   \
    "Weston,  FL 33327-1848\n"                                                 \
    "U.S.A.\n"                                                                 \
    "\n"                                                                       \
    "Phone : +1 954 217 2036\n"                                                \
    "FAX   : +1 954 217 2037\n"                                                \
    "WEB   : www.micrium.com\n"                                                \
    "E-mail: Sales@Micrium.com\n"                                              \
    "\n"                                                                       \
    "========================================================\n"

#define alt_license_reminder_ucosii() puts(ALT_LICENSE_REMINDER_UCOSII_STRING)


#endif /* __ALT_LICENSE_REMINDER_UCOSII_H__ */
