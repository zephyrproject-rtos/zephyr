/*****************************************************************************
 *
 * Copyright (C) 2016 Atmel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * * Neither the name of the copyright holders nor the names of
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************/


#ifndef _COMPONENT_VERSION_H_INCLUDED
#define _COMPONENT_VERSION_H_INCLUDED

#define COMPONENT_VERSION_MAJOR 1
#define COMPONENT_VERSION_MINOR 0

//
// The COMPONENT_VERSION define is composed of the major and the minor version number.
//
// The last four digits of the COMPONENT_VERSION is the minor version with leading zeros.
// The rest of the COMPONENT_VERSION is the major version, with leading zeros. The COMPONENT_VERSION
// is at least 8 digits long.
//
#define COMPONENT_VERSION 00010000

//
// The build number does not refer to the component, but to the build number
// of the device pack that provides the component.
//
#define BUILD_NUMBER 56

//
// The COMPONENT_VERSION_STRING is a string (enclosed in ") that can be used for logging or embedding.
//
#define COMPONENT_VERSION_STRING "1.0"

//
// The COMPONENT_DATE_STRING contains a timestamp of when the pack was generated.
//
// The COMPONENT_DATE_STRING is written out using the following strftime pattern.
//
//     "%Y-%m-%d %H:%M:%S"
//
//
#define COMPONENT_DATE_STRING "2016-09-15 14:36:34"

#endif/* #ifndef _COMPONENT_VERSION_H_INCLUDED */

