/**
 * ile
 *
 * rief Component version header file
 *
 * Copyright (c) 2016 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \license_stop
 *
 */

#ifndef _COMPONENT_VERSION_H_INCLUDED
#define _COMPONENT_VERSION_H_INCLUDED

#define COMPONENT_VERSION_MAJOR 2
#define COMPONENT_VERSION_MINOR 0

//
// The COMPONENT_VERSION define is composed of the major and the minor version number.
//
// The last four digits of the COMPONENT_VERSION is the minor version with leading zeros.
// The rest of the COMPONENT_VERSION is the major version, with leading zeros. The COMPONENT_VERSION
// is at least 8 digits long.
//
#define COMPONENT_VERSION 00020000

//
// The build number does not refer to the component, but to the build number
// of the device pack that provides the component.
//
#define BUILD_NUMBER 78

//
// The COMPONENT_VERSION_STRING is a string (enclosed in ") that can be used for logging or embedding.
//
#define COMPONENT_VERSION_STRING "2.0"

//
// The COMPONENT_DATE_STRING contains a timestamp of when the pack was generated.
//
// The COMPONENT_DATE_STRING is written out using the following strftime pattern.
//
//     "%Y-%m-%d %H:%M:%S"
//
//
#define COMPONENT_DATE_STRING "2016-10-24 16:50:28"

#endif/* #ifndef _COMPONENT_VERSION_H_INCLUDED */

