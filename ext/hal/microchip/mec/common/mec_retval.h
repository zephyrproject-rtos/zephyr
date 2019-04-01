/**
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

/** @file mec_retval.h
 *MEC Peripheral library return values
 */
/** @defgroup MEC Peripherals Return values
 */

#ifndef _MEC_RETVAL_H
#define _MEC_RETVAL_H

#define MCHP_RET_OK              0
#define MCHP_RET_ERR             1
#define MCHP_RET_ERR_INVAL       2 /* bad parameter */
#define MCHP_RET_ERR_BUSY        3
#define MCHP_RET_ERR_NOP         4
#define MCHP_RET_ERR_XFR         5
#define MCHP_RET_ERR_TIMEOUT     6
#define MCHP_RET_ERR_NACK        7 /* a device did not respond */
#define MCHP_RET_ERR_HW          8

#define MCHP_FALSE   0
#define MCHP_TRUE    1

#define MCHP_OFF     0
#define MCHP_ON      1

#endif // #ifndef _MEC_RETVAL_H
/* end mec_retval.h */
/**   @}
 */
