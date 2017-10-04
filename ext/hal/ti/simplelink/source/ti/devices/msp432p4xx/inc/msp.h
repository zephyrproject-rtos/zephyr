/******************************************************************************
*
* Copyright (C) 2012 - 2017 Texas Instruments Incorporated - http://www.ti.com/
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
*  Redistributions of source code must retain the above copyright
*  notice, this list of conditions and the following disclaimer.
*
*  Redistributions in binary form must reproduce the above copyright
*  notice, this list of conditions and the following disclaimer in the
*  documentation and/or other materials provided with the
*  distribution.
*
*  Neither the name of Texas Instruments Incorporated nor the names of
*  its contributors may be used to endorse or promote products derived
*  from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* MSP432 Family Generic Include File
*
* File creation date: 08/03/17
*
******************************************************************************/
#ifndef __MSP432_H__
#define __MSP432_H__
/******************************************************************************
* MSP432 devices                                                              *
******************************************************************************/
#if defined (__MSP432P401R__)
#include "msp432p401r.h"
#elif defined (__MSP432P401M__)
#include "msp432p401m.h"
#elif defined (__MSP432P411V__)
#include "msp432p411v.h"
#elif defined (__MSP432P4111__)
#include "msp432p4111.h"
#elif defined (__MSP432P4XX__)
#include "msp432p4xx.h"
#elif defined (__MSP432P411Y__)
#include "msp432p411y.h"
/******************************************************************************
* Failed to match a default include file                                      *
******************************************************************************/
#else
#error "Failed to match a default include file"
#endif
#endif /* __MSP432_H__ */
