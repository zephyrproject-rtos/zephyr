/***************************************************************************//**
* \file cyip_headers.h
*
* \brief
* Common header file to be included by all IP definition headers
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef _CYIP_HEADERS_H_
#define _CYIP_HEADERS_H_

#include <stdint.h>

/* These are CMSIS-CORE defines used for structure members definitions */
#ifndef     __IM
#define     __IM     volatile const      /*! Defines 'read only' structure member permissions */
#endif
#ifndef     __OM
#define     __OM     volatile            /*! Defines 'write only' structure member permissions */
#endif
#ifndef     __IOM
#define     __IOM    volatile            /*! Defines 'read / write' structure member permissions */
#endif

#endif /* _CYIP_HEADERS_H_ */


/* [] END OF FILE */
