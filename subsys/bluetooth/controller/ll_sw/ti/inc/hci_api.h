/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/******************************************************************************

 @file  _hci_api.h

 @brief HCI layer interface APIs

 *****************************************************************************/

#ifndef HCI_API_H_
#define HCI_API_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "hal_types.h"


/*******************************************************************************
 * TYPEDEFS
 */

typedef struct hci_c2h_cbs_t
{
  int (*send)(uint8 *pHciPkt, uint16 pktLen);
}hci_c2h_cbs_t;



/*******************************************************************************
 * API FUNCTIONS
 */

/*******************************************************************************
 * @fn          HCI_ControllerToHostRegisterCb
 *
 * @brief       This function register callback function to HCI events
 *
 * input parameters
 *
 * @param       hci_c2h_cbs_t cb - callback function.
 *
 * output parameters
 *
 * @param       None.
 *
 * @return      SUCCESS / FAILURE.
 */
uint8 HCI_ControllerToHostRegisterCb( const hci_c2h_cbs_t *cbs );


#ifdef __cplusplus
}
#endif

#endif /* HCI_API_H_ */
