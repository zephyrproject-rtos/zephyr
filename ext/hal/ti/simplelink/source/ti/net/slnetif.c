/*
 * Copyright (c) 2017-2018, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/

/* Global includes                                                           */
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <ti/net/slnetif.h>
#include <ti/net/slneterr.h>

/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/

#define SLNETIF_NORMALIZE_NEEDED 0
#if SLNETIF_NORMALIZE_NEEDED
    #define SLNETIF_NORMALIZE_RET_VAL(retVal,err)   ((retVal < 0)?(retVal = err):(retVal))
#else
    #define SLNETIF_NORMALIZE_RET_VAL(retVal,err)
#endif

/* Interface maximum priority                                               */
#define SLNETIF_MAX_PRIORITY                        (15)

/* The 32bit interface flags structure:
   Bits  0-3 : interface priority
   Bit     4 : Interface state
   Bits 5-31 : Reserved
*/
#define IF_PRIORITY_BITS                            (0x000f)
#define IF_STATE_BIT                                (0x0010)

/* this macro returns the priority of the interface                          */
#define GET_IF_PRIORITY(netIf)                      ((netIf).flags & IF_PRIORITY_BITS)

/* this macro reset the priority of the interface                            */
#define RESET_IF_PRIORITY(netIf)                    ((netIf).flags &= ~IF_PRIORITY_BITS)

/* this macro set the priority of the interface                              */
#define SET_IF_PRIORITY(netIf, priority)            ((netIf).flags |= ((int32_t)priority))

/* this macro returns the state of the interface                             */
#define GET_IF_STATE(netIf)                         (((netIf).flags & IF_STATE_BIT)?SLNETIF_STATE_ENABLE:SLNETIF_STATE_DISABLE)

/* this macro set the interface to enable state                              */
#define SET_IF_STATE_ENABLE(netIf)                  ((netIf).flags |= IF_STATE_BIT)

/* this macro set the interface to disable state                             */
#define SET_IF_STATE_DISABLE(netIf)                 ((netIf).flags &= ~IF_STATE_BIT)

/* Check if the state bit is set                                             */
#define IS_STATE_BIT_SET(queryBitmap)               (0 != (queryBitmap & SLNETIF_QUERY_IF_STATE_BIT))

/* Check if the connection status bit is set                                 */
#define IS_CONNECTION_STATUS_BIT_SET(queryBitmap)   (0 != (queryBitmap & SLNETIF_QUERY_IF_CONNECTION_STATUS_BIT))

/* Return last found netIf, if none of the existing interfaces answers
   the query                                                                 */
#define IS_ALLOW_PARTIAL_MATCH_BIT_SET(queryBitmap) (0 != (queryBitmap & SLNETIF_QUERY_IF_ALLOW_PARTIAL_MATCH_BIT))

/*****************************************************************************/
/* Structure/Enum declarations                                               */
/*****************************************************************************/


/* Interface Node */
typedef struct SlNetIf_Node_t
{
    SlNetIf_t              netIf;
    struct SlNetIf_Node_t *next;
} SlNetIf_Node_t;

/*****************************************************************************/
/* Global declarations                                                       */
/*****************************************************************************/

static SlNetIf_Node_t * SlNetIf_listHead = NULL;

/*****************************************************************************/
/* Function prototypes                                                       */
/*****************************************************************************/

static int32_t SlNetIf_configCheck(const SlNetIf_Config_t *ifConf);

//*****************************************************************************
//
// SlNetIf_configCheck - Checks that all mandatory configuration exists
//
//*****************************************************************************

static int32_t SlNetIf_configCheck(const SlNetIf_Config_t *ifConf)
{
    /* Check if the mandatory configuration exists
       This configuration needs to be updated when new mandatory is added    */
    if ((NULL != ifConf) &&
        (NULL != ifConf->sockCreate)   &&
        (NULL != ifConf->sockClose)    &&
        (NULL != ifConf->sockSelect)   &&
        (NULL != ifConf->sockSetOpt)   &&
        (NULL != ifConf->sockGetOpt)   &&
        (NULL != ifConf->sockRecvFrom) &&
        (NULL != ifConf->sockSendTo)   &&
        (NULL != ifConf->ifGetIPAddr)  &&
        (NULL != ifConf->ifGetConnectionStatus) )
    {
        /* All mandatory configuration exists - return success               */
        return SLNETERR_RET_CODE_OK;
    }
    else
    {
        /* Not all mandatory configuration exists - return error code        */
        return SLNETERR_INVALPARAM;
    }

}

//*****************************************************************************
//
// SlNetIf_init - Initialize the SlNetIf module
//
//*****************************************************************************
int32_t SlNetIf_init(int32_t flags)
{
    return SLNETERR_RET_CODE_OK;
}


//*****************************************************************************
//
// SlNetIf_add - Adding new interface
//
//*****************************************************************************
int32_t SlNetIf_add(uint16_t ifID, char *ifName, const SlNetIf_Config_t *ifConf, uint8_t priority)
{
    SlNetIf_Node_t *ifNode    = SlNetIf_listHead;
    SlNetIf_Node_t *prvNode   = NULL;
    char           *allocName = NULL;
    int16_t         strLen;
    int32_t         retVal;

    /* Check if ifID is a valid input - Only one bit is set (Pow of 2) or if
       the priority isn't a valid input                                      */
    if ( (false == ONLY_ONE_BIT_IS_SET(ifID)) || (priority > SLNETIF_MAX_PRIORITY) )
    {
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }

    /* Run over the interface list until finding the required ifID or Until
       reaching the end of the list                                          */
    while (NULL != ifNode)
    {
        /* Check if the identifier of the interface is equal to the input    */
        if ((ifNode->netIf).ifID == ifID)
        {
            /* Required interface found, ifID cannot be used again           */
            return SLNETERR_RET_CODE_INVALID_INPUT;
        }
        /* Check if the identifier of the interface is equal to the input    */
        if ((GET_IF_PRIORITY(ifNode->netIf)) > priority)
        {
            /* Higher priority interface found, save location                */
            prvNode = ifNode;
        }
        else
        {
            break;
        }
        ifNode = ifNode->next;
    }


    /* Check if all required configuration exists                            */
    retVal = SlNetIf_configCheck(ifConf);

    /* Check retVal in order to continue with adding the interface           */
    if (SLNETERR_RET_CODE_OK == retVal)
    {
        /* Interface node memory allocation                                  */
        /* Allocate memory for new interface node for the interface list     */
        ifNode = malloc(sizeof(SlNetIf_Node_t));

        /* Check if the memory allocated successfully                        */
        if (NULL == ifNode)
        {
            /* Allocation failed, return error code                          */
            return SLNETERR_RET_CODE_MALLOC_ERROR;
        }

        /* Interface name memory allocation                                  */
        /* Check if memory allocation for the name of the string is required */
        if (NULL != ifName)
        {
            /* Store the length of the interface name                        */
            strLen = strlen(ifName);

            /* Allocate memory that will store the name of the interface     */
            allocName = malloc(strLen + 1);

            /* Check if the memory allocated successfully                    */
            if (NULL == allocName)
            {
                /* Allocation failed, free ifNode before returning from
                   function                                                  */
                free(ifNode);
                return SLNETERR_RET_CODE_MALLOC_ERROR;
            }
            /* Copy the input name into the allocated memory                 */
            strncpy(allocName, ifName, strLen + 1);
        }

        /* Fill the allocated interface node with the input parameters       */

        /* Copy the interface ID                                             */
        (ifNode->netIf).ifID   = ifID;
        /* Connect the string allocated memory into the allocated interface  */
        (ifNode->netIf).ifName = allocName;
        /* Copy the interface configuration                                  */
        (ifNode->netIf).ifConf = (SlNetIf_Config_t *)ifConf;
        /* Reset the flags, set state to ENABLE and set the priority         */
        (ifNode->netIf).flags  = 0;
        SET_IF_PRIORITY(ifNode->netIf, priority);
        SET_IF_STATE_ENABLE(ifNode->netIf);

        /* Check if CreateContext function exists                            */
        if (NULL != ifConf->ifCreateContext)
        {
            /* Function exists, run it and fill the context                  */
            retVal = ifConf->ifCreateContext(ifID, allocName, &((ifNode->netIf).ifContext));
        }

        /* Check retVal before continuing                                    */
        if (SLNETERR_RET_CODE_OK == retVal)
        {
            /* After creating and filling the interface, add it to the right
               place in the list according to its priority                   */

            /* Check if there isn't any higher priority node                 */
            if (NULL == prvNode)
            {
                /* there isn't any higher priority node so check if list is
                   empty                                                     */
                if (NULL != SlNetIf_listHead)
                {
                    /* List isn't empty, so use the new allocated interface
                       as the new head and connect the old list head as the
                       following node                                        */
                    ifNode->next = SlNetIf_listHead;
                }
                else
                {
                    /* List is empty, so the new allocated interface will be
                       the head list                                         */
                    ifNode->next = NULL;
                }
                SlNetIf_listHead = ifNode;
            }
            else
            {
                /* Higher priority exists, connect the allocated node after
                   the prvNode and the next node (if exists) of prvNode to
                   the next of the node                                      */
                ifNode->next  = prvNode->next;
                prvNode->next = ifNode;
            }
        }
    }

    return retVal;
}


//*****************************************************************************
//
// SlNetIf_getIfByID - Get interface configuration from interface ID
//
//*****************************************************************************
SlNetIf_t * SlNetIf_getIfByID(uint16_t ifID)
{
    SlNetIf_Node_t *ifNode = SlNetIf_listHead;

    /* Check if ifID is a valid input - Only one bit is set (Pow of 2)       */
    if (false == ONLY_ONE_BIT_IS_SET(ifID))
    {
        return NULL;
    }

    /* Run over the interface list until finding the required ifID or Until
       reaching the end of the list                                          */
    while (NULL != ifNode)
    {
        /* Check if the identifier of the interface is equal to the input    */
        if ((ifNode->netIf).ifID == ifID)
        {
            /* Required interface found, return the interface pointer        */
            return &(ifNode->netIf);
        }
        ifNode = ifNode->next;
    }

    /* Interface identifier was not found                                    */
    return NULL;
}

//*****************************************************************************
//
// SlNetIf_queryIf - Get interface configuration with the highest priority
//                   from the provided interface bitmap
//                   Note: ifBitmap - 0 is not a valid input
//
//*****************************************************************************
SlNetIf_t * SlNetIf_queryIf(uint32_t ifBitmap, uint32_t queryBitmap)
{
    SlNetIf_Node_t *ifNode             = SlNetIf_listHead;
    SlNetIf_t      *bestPartialMatchIf = NULL;

    if (0 == ifBitmap)
    {
        return NULL;
    }

    /* Run over the interface list until finding the first ifID that is
       set in the ifBitmap or Until reaching the end of the list if no
       ifID were found                                                       */
    while (NULL != ifNode)
    {
        /* Check if the identifier of the interface is equal to the input    */
        if (((ifNode->netIf).ifID) & ifBitmap)
        {
            /* Save the netIf only at the first match                        */
            if (NULL == bestPartialMatchIf)
            {
                bestPartialMatchIf = &(ifNode->netIf);
            }
            /* Skip over Bitmap queries                                      */
            if ( 0 != queryBitmap)
            {
                /* Check if the state bit needs to be set and if it is       */
                if ( (true == IS_STATE_BIT_SET(queryBitmap)) &&
                     (SLNETIF_STATE_DISABLE == (GET_IF_STATE(ifNode->netIf))) )
                {
                    /* State is disabled when needed to be set, continue
                       to the next interface                                 */
                    ifNode = ifNode->next;
                    continue;
                }
                /* Check if the connection status bit needs to be set
                   and if it is                                              */
                if ( (true == IS_CONNECTION_STATUS_BIT_SET(queryBitmap)) &&
                     (SLNETIF_STATUS_DISCONNECTED == SlNetIf_getConnectionStatus((ifNode->netIf).ifID)) )
                {
                    /* Connection status is disconnected when needed to
                       be connected, continue to the next interface          */
                    ifNode = ifNode->next;
                    continue;
                }
            }
            /* Required interface found, return the interface pointer        */
            return &(ifNode->netIf);
        }
        ifNode = ifNode->next;
    }

    /* When bit is set, return the last interface found if there isn't
       an existing interface that answers the query return the last
       netIf that was found                                                  */
    if (true == IS_ALLOW_PARTIAL_MATCH_BIT_SET(queryBitmap))
    {
        return bestPartialMatchIf;
    }
    else
    {
        return NULL;
    }

}


//*****************************************************************************
//
// SlNetIf_getNameByID - Get interface Name from interface ID
//
//*****************************************************************************
const char * SlNetIf_getNameByID(uint16_t ifID)
{
    SlNetIf_t *netIf;

    /* Run validity check and find the requested interface                   */
    netIf = SlNetIf_getIfByID(ifID);

    /* Check if the requested interface exists or the function returned NULL */
    if (NULL == netIf)
    {
        /* Interface doesn't exists or invalid input, return NULL            */
        return NULL;
    }
    else
    {
        /* Interface exists, return the interface name                       */
        return netIf->ifName;
    }
}


//*****************************************************************************
//
// SlNetIf_getIDByName - Get interface ID from interface name
//
//*****************************************************************************
int32_t SlNetIf_getIDByName(char *ifName)
{
    SlNetIf_Node_t *ifNode = SlNetIf_listHead;

    /* Check if ifName is a valid input                                      */
    if (NULL == ifName)
    {
        return(SLNETERR_RET_CODE_INVALID_INPUT);
    }

    /* Run over the interface list until finding the required ifID or Until
       reaching the end of the list                                          */
    while (NULL != ifNode)
    {
        /* Check if the identifier of the interface is equal to the input    */
        if (strcmp((ifNode->netIf).ifName, ifName) == 0)
        {
            /* Required interface found, return the interface identifier     */
            return ((ifNode->netIf).ifID);
        }
        ifNode = ifNode->next;
    }

    /* Interface identifier was not found, return error code                 */
    return(SLNETERR_RET_CODE_INVALID_INPUT);
}


//*****************************************************************************
//
// SlNetIf_getPriority - Get interface priority
//
//*****************************************************************************
int32_t SlNetIf_getPriority(uint16_t ifID)
{
    SlNetIf_t *netIf;

    /* Run validity check and find the requested interface                   */
    netIf = SlNetIf_getIfByID(ifID);

    /* Check if the requested interface exists or the function returned NULL */
    if (NULL == netIf)
    {
        /* Interface doesn't exists or invalid input, return error code      */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }
    else
    {
        /* Interface exists, return interface priority                       */
        return GET_IF_PRIORITY(*netIf);
    }
}


//*****************************************************************************
//
// SlNetIf_setPriority - Set interface priority
//
//*****************************************************************************
int32_t SlNetIf_setPriority(uint16_t ifID, uint8_t priority)
{
    SlNetIf_Node_t *ifListNode        = SlNetIf_listHead;
    SlNetIf_Node_t *prvIfListNode     = SlNetIf_listHead;
    SlNetIf_Node_t *reqIfListNode     = NULL;
    bool            connectAgain      = false;

    if (priority > SLNETIF_MAX_PRIORITY)
    {
        /* Run validity check and find the requested interface               */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }

    /* Find the location of required interface                               */
    while (NULL != ifListNode)
    {
        /* If the location of the required interface found, store the
           location                                                          */
        if ((ifListNode->netIf.ifID) == ifID)
        {
            reqIfListNode = ifListNode;
            /* Check if the required interface is the last node and needs
               lower priority than the previous node, only update of the
               priority is required for this node                            */
            if (NULL == ifListNode->next)
            {
                if (GET_IF_PRIORITY(prvIfListNode->netIf) >= priority)
                {
                    /* The required interface is the last node and needs
                       lower priority than the previous node, only update
                       of the priority is required for this node             */
                    break;
                }
                else
                {
                    /* The required interface is the last node but has
                       higher priority than the previous node, disconnect
                       the interface from the list. connect the previous node
                       to the end of the list
                       If this is not the only node in the list, find where 
                       it now belongs based on its new priority.            */
                    if (reqIfListNode != SlNetIf_listHead)
                    {
                        prvIfListNode->next = NULL;
                        connectAgain = true;
                        /* This is the end of the list, so stop looping      */
                        break;
                    }
                }
            }
            /* Check if the required interface is the first node in the list */
            else if (SlNetIf_listHead == ifListNode)
            {
                if ( (NULL == ifListNode->next) || ((GET_IF_PRIORITY(ifListNode->next->netIf)) <= priority) )
                {
                    /* The required interface is the first node and needs
                       higher priority than the following node, only update
                       of the priority is required for this node             */
                    break;
                }
                /* The required interface is the first node but doesn't need
                   higher priority than the following node so the following
                   node will be the first node of the list                   */
                else
                {
                    SlNetIf_listHead = ifListNode->next;
                    connectAgain = true;
                }
            }
            /* The required interface isn't the first or last node and needs
               priority change but is in the correct location, only update
               of the priority is required for this node                     */
            else if ( (GET_IF_PRIORITY(prvIfListNode->netIf) >= priority) && ((GET_IF_PRIORITY(ifListNode->next->netIf)) <= priority) )
            {
                break;
            }
            /* The required interface isn't the first or last node and needs
               priority change, disconnect it from the list. connect the
               previous node with the following node                         */
            else
            {
                prvIfListNode->next = prvIfListNode->next->next;
                connectAgain = true;
            }
        }
        else
        {
            prvIfListNode = ifListNode;
        }
        ifListNode = ifListNode->next;
    }

    /* When connectAgain is set, there's a need to find where to add back
       the required interface according to the priority                      */
    if (connectAgain == true)
    {
        ifListNode = SlNetIf_listHead;
        while (NULL != ifListNode)
        {
            /* If the incoming priority is higher than any present           */
            if ( (ifListNode == SlNetIf_listHead) )
            {
                if ((GET_IF_PRIORITY(ifListNode->netIf)) <= priority)
                {
                    reqIfListNode->next = ifListNode;
                    SlNetIf_listHead = reqIfListNode;
                    break;
                }
            }

            /* Check if the priority of the current interface is higher of
               the required priority, if so, store it as the previous
               interface and continue to search until finding interface with
               lower priority and than connect the required interface to the
               prior interface                                               */
            if ((GET_IF_PRIORITY(ifListNode->netIf)) > priority)
            {
                prvIfListNode = ifListNode;

            }
            else
            {
                reqIfListNode->next = prvIfListNode->next;
                prvIfListNode->next = reqIfListNode;
                break;
            }
            ifListNode = ifListNode->next;

            if (NULL == ifListNode)
            {
                /* All interfaces have higher priorities than reqIfListNode  */
                prvIfListNode->next = reqIfListNode;
                reqIfListNode->next = NULL;
                break;
            }
        }
    }

    if (NULL != reqIfListNode)
    {
        /* Interface exists, set the interface priority                          */
        RESET_IF_PRIORITY(reqIfListNode->netIf);
        SET_IF_PRIORITY(reqIfListNode->netIf, priority);
    }

    return SLNETERR_RET_CODE_OK;
}


//*****************************************************************************
//
// SlNetIf_setState - Set interface state
//
//*****************************************************************************
int32_t SlNetIf_setState(uint16_t ifID, SlNetIfState_e ifState)
{
    SlNetIf_t *netIf;

    /* Run validity check and find the requested interface                   */
    netIf = SlNetIf_getIfByID(ifID);

    /* Check if the requested interface exists or the function returned NULL */
    if (NULL == netIf)
    {
        /* Interface doesn't exists or invalid input, return error code      */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }
    else
    {
        /* Interface exists, set the interface state                         */
        if (ifState == SLNETIF_STATE_DISABLE)
        {
            /* Interface state - Disable                                     */
            SET_IF_STATE_DISABLE(*netIf);
        }
        else
        {
            /* Interface state - Enable                                      */
            SET_IF_STATE_ENABLE(*netIf);
        }
    }
    return SLNETERR_RET_CODE_OK;
}


//*****************************************************************************
//
// SlNetIf_getState - Get interface state
//
//*****************************************************************************
int32_t SlNetIf_getState(uint16_t ifID)
{
    SlNetIf_t *netIf;

    /* Run validity check and find the requested interface                   */
    netIf = SlNetIf_getIfByID(ifID);

    /* Check if the requested interface exists or the function returned NULL */
    if (NULL == netIf)
    {
        /* Interface doesn't exists or invalid input, return error code      */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }
    else
    {
        /* Interface exists, return interface state                          */
        return GET_IF_STATE(*netIf);
    }
}


//*****************************************************************************
//
// SlNetIf_getIPAddr - Get IP Address of specific interface
//
//*****************************************************************************
int32_t SlNetIf_getIPAddr(uint16_t ifID, SlNetIfAddressType_e addrType, uint16_t *addrConfig, uint32_t *ipAddr)
{
    SlNetIf_t *netIf;
    int32_t    retVal;

    /* Run validity check and find the requested interface                   */
    netIf = SlNetIf_getIfByID(ifID);

    /* Check if the requested interface exists or the function returned NULL */
    if (NULL == netIf)
    {
        /* Interface doesn't exists or invalid input, return NULL            */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }
    else
    {
        /* Interface exists, return interface IP address                     */
        retVal = (netIf->ifConf)->ifGetIPAddr(netIf->ifContext, addrType, addrConfig, ipAddr);
        SLNETIF_NORMALIZE_RET_VAL(retVal,SLNETIF_ERR_IFGETIPADDR_FAILED);

        /* Check retVal for error codes                                      */
        if (retVal < SLNETERR_RET_CODE_OK)
        {
            /* Return retVal, function error                                 */
            return retVal;
        }
        else
        {
            /* Return success                                                */
            return SLNETERR_RET_CODE_OK;
        }
    }
}


//*****************************************************************************
//
// SlNetIf_getConnectionStatus - Get interface connection status
//
//*****************************************************************************
int32_t SlNetIf_getConnectionStatus(uint16_t ifID)
{
    SlNetIf_t *netIf;
    int16_t    connectionStatus;

    /* Run validity check and find the requested interface                   */
    netIf = SlNetIf_getIfByID(ifID);

    /* Check if the requested interface exists or the function returned NULL */
    if (NULL == netIf)
    {
        /* Interface doesn't exists or invalid input, return NULL            */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }
    else
    {
        /* Interface exists, return interface connection status              */
        connectionStatus = (netIf->ifConf)->ifGetConnectionStatus(netIf->ifContext);

        /* Interface exists, set the interface state                         */
        if (connectionStatus == SLNETIF_STATUS_DISCONNECTED)
        {
            /* Interface is disconnected                                     */
            return SLNETIF_STATUS_DISCONNECTED;
        }
        else if (connectionStatus > SLNETIF_STATUS_DISCONNECTED)
        {
            SLNETIF_NORMALIZE_RET_VAL(connectionStatus,SLNETIF_STATUS_CONNECTED);
        }
        else
        {
            SLNETIF_NORMALIZE_RET_VAL(connectionStatus,SLNETIF_ERR_IFGETCONNECTIONSTATUS_FAILED);
        }
        return connectionStatus;
    }
}


//*****************************************************************************
//
// SlNetIf_loadSecObj - Load secured buffer to the network stack
//
//*****************************************************************************
int32_t SlNetIf_loadSecObj(uint16_t objType, char *objName, int16_t objNameLen, uint8_t *objBuff, int16_t objBuffLen, uint32_t ifBitmap)
{
    SlNetIf_t *netIf;
    int32_t    retVal;
    uint32_t   ifIDIndex = 1;  /* Set value to highest bit in uint32_t       */
    uint32_t   maxIDIndex = (uint32_t)1 << SLNETIF_MAX_IF;

    if ((NULL == objName) || (NULL == objBuff))
    {
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }

    /* bitmap 0 entered, load sec obj to all available interfaces            */
    if (0 == ifBitmap)
    {
        ifBitmap = ~ifBitmap;
    }

    while ( ifIDIndex < maxIDIndex )
    {
        /* Check if ifIDIndex is a required ifID from the ifBitmap           */
        if ( ifIDIndex & ifBitmap )
        {
            /* Run validity check and find the requested interface           */
            netIf = SlNetIf_getIfByID(ifIDIndex & ifBitmap);

            /* Check if the requested interface exists or the function
               returned NULL                                                 */
            if ( (NULL != netIf) && (NULL != (netIf->ifConf)->ifLoadSecObj) )
            {
                /* Interface exists, return interface IP address             */
                retVal = (netIf->ifConf)->ifLoadSecObj(netIf->ifContext, objType, objName, objNameLen, objBuff, objBuffLen);
                SLNETIF_NORMALIZE_RET_VAL(retVal,SLNETIF_ERR_IFLOADSECOBJ_FAILED);

                /* Check retVal for error codes                              */
                if (retVal < SLNETERR_RET_CODE_OK)
                {
                    /* Return retVal, function error                         */
                    return retVal;
                }
                else
                {
                    /* Return success                                        */
                    return SLNETERR_RET_CODE_OK;
                }
            }
        }
        ifIDIndex <<= 1;
    }
    /* Interface doesn't exists or invalid input, return error code          */
    return SLNETERR_RET_CODE_INVALID_INPUT;
}
