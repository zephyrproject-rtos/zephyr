/***************************************************************************//**
* \file cy_dmac.c
* \version 1.0
*
* \brief
* The source code file for the DMA driver.
*
********************************************************************************
* \copyright
* Copyright 2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_dmac.h"

#if defined(CY_IP_M4CPUSS_DMAC)

#if defined(__cplusplus)
extern "C" {
#endif


/*******************************************************************************
* Function Name: Cy_DMAC_Descriptor_Init
****************************************************************************//**
*
* Initializes the descriptor structure in SRAM from a pre-initialized
* configuration structure.
* This function initializes only the descriptor and not the channel.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param config
* This is a configuration structure that has all initialization information for
* the descriptor.
*
* \return
* The status /ref cy_en_dmac_status_t.
*
* \funcusage 
* \snippet dmac\1.0\snippet\main.c snippet_Cy_DMAC_Enable
*
*******************************************************************************/
cy_en_dmac_status_t Cy_DMAC_Descriptor_Init(cy_stc_dmac_descriptor_t * descriptor, const cy_stc_dmac_descriptor_config_t * config)
{
    cy_en_dmac_status_t ret = CY_DMAC_BAD_PARAM;

    if ((NULL != descriptor) && (NULL != config))
    {
        CY_ASSERT_L3(CY_DMAC_IS_RETRIGGER_VALID(config->retrigger));
        CY_ASSERT_L3(CY_DMAC_IS_TRIG_TYPE_VALID(config->interruptType));
        CY_ASSERT_L3(CY_DMAC_IS_TRIG_TYPE_VALID(config->triggerOutType));
        CY_ASSERT_L3(CY_DMAC_IS_TRIG_TYPE_VALID(config->triggerInType));
        CY_ASSERT_L3(CY_DMAC_IS_XFER_SIZE_VALID(config->srcTransferSize));
        CY_ASSERT_L3(CY_DMAC_IS_XFER_SIZE_VALID(config->dstTransferSize));
        CY_ASSERT_L3(CY_DMAC_IS_CHANNEL_STATE_VALID(config->channelState));
        CY_ASSERT_L3(CY_DMAC_IS_DATA_SIZE_VALID(config->dataSize));
        CY_ASSERT_L3(CY_DMAC_IS_TYPE_VALID(config->descriptorType));
        
        descriptor->ctl =
            _VAL2FLD(DMAC_CH_V2_DESCR_CTL_WAIT_FOR_DEACT, config->retrigger) |
            _VAL2FLD(DMAC_CH_V2_DESCR_CTL_INTR_TYPE, config->interruptType) |
            _VAL2FLD(DMAC_CH_V2_DESCR_CTL_TR_OUT_TYPE, config->triggerOutType) |
            _VAL2FLD(DMAC_CH_V2_DESCR_CTL_TR_IN_TYPE, config->triggerInType) |
           _BOOL2FLD(DMAC_CH_V2_DESCR_CTL_DATA_PREFETCH, config->dataPrefetch) |
            _VAL2FLD(DMAC_CH_V2_DESCR_CTL_SRC_TRANSFER_SIZE, config->srcTransferSize) |
            _VAL2FLD(DMAC_CH_V2_DESCR_CTL_DST_TRANSFER_SIZE, config->dstTransferSize) |
            _VAL2FLD(DMAC_CH_V2_DESCR_CTL_CH_DISABLE, config->channelState) |
            _VAL2FLD(DMAC_CH_V2_DESCR_CTL_DATA_SIZE, config->dataSize) |
            _VAL2FLD(DMAC_CH_V2_DESCR_CTL_DESCR_TYPE, config->descriptorType);

        descriptor->src = (uint32_t)config->srcAddress;

        switch(config->descriptorType)
        {
            case CY_DMAC_SINGLE_TRANSFER:
            {
                descriptor->dst = (uint32_t)config->dstAddress;
                descriptor->xSize = (uint32_t)config->nextDescriptor;
                break;
            }
            
            case CY_DMAC_1D_TRANSFER:
            {
                CY_ASSERT_L2(CY_DMAC_IS_LOOP_INCR_VALID(config->srcXincrement));
                CY_ASSERT_L2(CY_DMAC_IS_LOOP_INCR_VALID(config->dstXincrement));
                CY_ASSERT_L2(CY_DMAC_IS_LOOP_COUNT_VALID(config->xCount));
                
                descriptor->dst = (uint32_t)config->dstAddress;
    /* Convert the data count from the user's range (1-65536) into the machine range (0-65535). */
                descriptor->xSize = _VAL2FLD(DMAC_CH_V2_DESCR_X_SIZE_X_COUNT, config->xCount - 1UL);
                
                descriptor->xIncr = _VAL2FLD(DMAC_CH_V2_DESCR_X_INCR_SRC_X, config->srcXincrement) |
                                    _VAL2FLD(DMAC_CH_V2_DESCR_X_INCR_DST_X, config->dstXincrement);

                descriptor->ySize = (uint32_t)config->nextDescriptor;
                break;
            }
            
            case CY_DMAC_2D_TRANSFER:
            {
                CY_ASSERT_L2(CY_DMAC_IS_LOOP_INCR_VALID(config->srcXincrement));
                CY_ASSERT_L2(CY_DMAC_IS_LOOP_INCR_VALID(config->dstXincrement));
                CY_ASSERT_L2(CY_DMAC_IS_LOOP_COUNT_VALID(config->xCount));
                CY_ASSERT_L2(CY_DMAC_IS_LOOP_INCR_VALID(config->srcYincrement));
                CY_ASSERT_L2(CY_DMAC_IS_LOOP_INCR_VALID(config->dstYincrement));
                CY_ASSERT_L2(CY_DMAC_IS_LOOP_COUNT_VALID(config->yCount));

                descriptor->dst = (uint32_t)config->dstAddress;
    /* Convert the data count from the user's range (1-65536) into the machine range (0-65535). */
                descriptor->xSize = _VAL2FLD(DMAC_CH_V2_DESCR_X_SIZE_X_COUNT, config->xCount - 1UL);

                descriptor->xIncr = _VAL2FLD(DMAC_CH_V2_DESCR_X_INCR_SRC_X, config->srcXincrement) |
                                    _VAL2FLD(DMAC_CH_V2_DESCR_X_INCR_DST_X, config->dstXincrement);

    /* Convert the data count from the user's range (1-65536) into the machine range (0-65535). */
                descriptor->ySize = _VAL2FLD(DMAC_CH_V2_DESCR_Y_SIZE_Y_COUNT, config->yCount - 1UL);

                descriptor->yIncr = _VAL2FLD(DMAC_CH_V2_DESCR_Y_INCR_SRC_Y, config->srcYincrement) |
                                    _VAL2FLD(DMAC_CH_V2_DESCR_Y_INCR_DST_Y, config->dstYincrement);

                descriptor->nextPtr = (uint32_t)config->nextDescriptor;
                break;
            }
            
            case CY_DMAC_MEMORY_COPY:
            {
                CY_ASSERT_L2(CY_DMAC_IS_LOOP_INCR_VALID(config->srcXincrement));

                descriptor->dst = (uint32_t)config->dstAddress;
    /* Convert the data count from the user's range (1-65536) into the machine range (0-65535). */
                descriptor->xSize = _VAL2FLD(DMAC_CH_V2_DESCR_X_SIZE_X_COUNT, config->xCount - 1UL);
                descriptor->xIncr = (uint32_t)config->nextDescriptor;
                break;
            }
            
            case CY_DMAC_SCATTER_TRANSFER:
            {
    /* Convert the data count from the user's range (1-65536) into the machine range (0-65535). */
                descriptor->dst = _VAL2FLD(DMAC_CH_V2_DESCR_X_SIZE_X_COUNT, config->xCount - 1UL);
                descriptor->xSize = (uint32_t)config->nextDescriptor;
                break;
            }
            
            default:
            {
                /* An unsupported type of a descriptor */
                break;
            }
        }

        ret = CY_DMAC_SUCCESS;
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_DMAC_Descriptor_DeInit
****************************************************************************//**
*
* Clears the content of the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \funcusage 
* \snippet dmac\1.0\snippet\main.c snippet_Cy_DMAC_Descriptor_Deinit
*
*******************************************************************************/
void Cy_DMAC_Descriptor_DeInit(cy_stc_dmac_descriptor_t * descriptor)
{
    descriptor->ctl = 0UL;
    descriptor->src = 0UL;
    descriptor->dst = 0UL;
    descriptor->xSize = 0UL;
    descriptor->xIncr = 0UL;
    descriptor->ySize = 0UL;
    descriptor->yIncr = 0UL;
    descriptor->nextPtr = 0UL;
}


/*******************************************************************************
* Function Name: Cy_DMAC_Channel_Init
****************************************************************************//**
*
* Initializes the DMA channel with a descriptor and other parameters.
*
* \param base
* The pointer to the hardware DMAC block.
*
* \param channel
* A channel number.
*
* \param channelConfig
* The structure that has the initialization information for the
* channel.
*
* \return
* The status /ref cy_en_dmac_status_t.
*
* \funcusage 
* \snippet dmac\1.0\snippet\main.c snippet_Cy_DMAC_Enable
*
*******************************************************************************/
cy_en_dmac_status_t Cy_DMAC_Channel_Init(DMAC_Type * base, uint32_t channel, cy_stc_dmac_channel_config_t const * channelConfig)
{
    cy_en_dmac_status_t ret = CY_DMAC_BAD_PARAM;

    if ((NULL != base) && (CY_DMAC_IS_CH_NR_VALID(channel)) && (NULL != channelConfig) && (NULL != channelConfig->descriptor))
    {
        CY_ASSERT_L2(CY_DMAC_IS_PRIORITY_VALID(channelConfig->priority));
        
        /* Set the current descriptor */
        DMAC_CH_CURR(base, channel) = (uint32_t)channelConfig->descriptor;

        /* Set the channel configuration */
        DMAC_CH_CTL(base, channel) = _VAL2FLD(DMAC_CH_V2_CTL_PRIO,    channelConfig->priority) |
                                    _BOOL2FLD(DMAC_CH_V2_CTL_ENABLED, channelConfig->enable);
        ret = CY_DMAC_SUCCESS;
    }

    return (ret);
}


/*******************************************************************************
* Function Name: Cy_DMAC_Channel_DeInit
****************************************************************************//**
*
* Clears the content of registers corresponding to the channel.
*
* \param base
* The pointer to the hardware DMAC block.
*
* \param channel
* A channel number.
*
* \funcusage 
* \snippet dmac\1.0\snippet\main.c snippet_Cy_DMAC_Disable
*
*******************************************************************************/
void Cy_DMAC_Channel_DeInit(DMAC_Type * base, uint32_t channel)
{
    CY_ASSERT_L1(CY_DMAC_IS_CH_NR_VALID(channel));
    
    DMAC_CH_CTL(base, channel) = 0UL;
    DMAC_CH_CURR(base, channel) = 0UL;
    DMAC_CH_INTR_MASK(base, channel) = 0UL;
}


/*******************************************************************************
* Function Name: Cy_DMAC_Descriptor_SetNextDescriptor
****************************************************************************//**
*
* Sets a Next Descriptor parameter for the specified descriptor.
*
* Based on the descriptor type, the offset of the address for the next descriptor may
* vary. For the single-transfer descriptor type, this register is at offset 0x0c.
* For the 1D-transfer descriptor type, this register is at offset 0x10. 
* For the 2D-transfer descriptor type, this register is at offset 0x14.  
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param nextDescriptor
* The pointer to the next descriptor.
*
* \funcusage 
* \snippet dmac\1.0\snippet\main.c snippet_Cy_DMAC_Descriptor_SetNextDescriptor
*
*******************************************************************************/
void Cy_DMAC_Descriptor_SetNextDescriptor(cy_stc_dmac_descriptor_t * descriptor, cy_stc_dmac_descriptor_t const * nextDescriptor)
{
    CY_ASSERT_L1(NULL != descriptor);
    
    switch((cy_en_dmac_descriptor_type_t) _FLD2VAL(DMAC_CH_V2_DESCR_CTL_DESCR_TYPE, descriptor->ctl))
    {
        case CY_DMAC_SINGLE_TRANSFER:
        case CY_DMAC_SCATTER_TRANSFER:
            descriptor->xSize = (uint32_t)nextDescriptor;
            break;
            
        case CY_DMAC_1D_TRANSFER:
            descriptor->ySize = (uint32_t)nextDescriptor;
            break;
            
        case CY_DMAC_2D_TRANSFER:
            descriptor->nextPtr = (uint32_t)nextDescriptor;
            break;
            
        case CY_DMAC_MEMORY_COPY:
            descriptor->xIncr = (uint32_t)nextDescriptor;
            break;
            
        default:
            /* Unsupported type of descriptor */
            break;
    }
}


/*******************************************************************************
* Function Name: Cy_DMAC_Descriptor_GetNextDescriptor
****************************************************************************//**
*
* Returns a next descriptor address of the specified descriptor.
*
* Based on the descriptor type, the offset of the address for the next descriptor may
* vary. For a single-transfer descriptor type, this register is at offset 0x0c.
* For the 1D-transfer descriptor type, this register is at offset 0x10. 
* For the 2D-transfer descriptor type, this register is at offset 0x14.  
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The pointer to the next descriptor.
*
* \funcusage 
* \snippet dmac\1.0\snippet\main.c snippet_Cy_DMAC_Descriptor_GetNextDescriptor
*
*******************************************************************************/
cy_stc_dmac_descriptor_t * Cy_DMAC_Descriptor_GetNextDescriptor(cy_stc_dmac_descriptor_t const * descriptor)
{
    cy_stc_dmac_descriptor_t * retVal = NULL;
    
    CY_ASSERT_L1(NULL != descriptor);
    
    switch((cy_en_dmac_descriptor_type_t) _FLD2VAL(DMAC_CH_V2_DESCR_CTL_DESCR_TYPE, descriptor->ctl))
    {
        case CY_DMAC_SINGLE_TRANSFER:
        case CY_DMAC_SCATTER_TRANSFER:
            retVal = (cy_stc_dmac_descriptor_t*) descriptor->xSize;
            break;
            
        case CY_DMAC_1D_TRANSFER:
            retVal = (cy_stc_dmac_descriptor_t*) descriptor->ySize;
            break;
            
        case CY_DMAC_2D_TRANSFER:
            retVal = (cy_stc_dmac_descriptor_t*) descriptor->nextPtr;
            break;
            
        case CY_DMAC_MEMORY_COPY:
            retVal = (cy_stc_dmac_descriptor_t*) descriptor->xIncr;
            break;
            
        default:
            /* An unsupported type of the descriptor */
            break;
    }
    
    return (retVal);
}


/*******************************************************************************
* Function Name: Cy_DMAC_Descriptor_SetDescriptorType
****************************************************************************//**
*
* Sets the descriptor's type for the specified descriptor.
* Moves the next descriptor pointer and X data count values into the proper 
* place in accordance to the actual descriptor type.
* During the descriptor's type changing, the Xloop and Yloop settings, such as 
* data count and source/destination increment (i.e. the content of the 
* xIncr, ySize and yIncr descriptor registers) might be lost (overriden by the 
* next descriptor/X data count values) because of the different descriptor
* registers structures for different descriptor types. 
* Set up carefully the Xloop (and Yloop, if used) data count and 
* source/destination increment if the descriptor type is changed from a simpler 
* to a more complicated type ("single transfer" -> "1D", "1D" -> "2D", etc.).
* 
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param descriptorType 
* The descriptor type \ref cy_en_dmac_descriptor_type_t.
*
* \funcusage 
* \snippet dmac\1.0\snippet\main.c snippet_Cy_DMAC_Descriptor_SetNextDescriptor
*
*******************************************************************************/
void Cy_DMAC_Descriptor_SetDescriptorType(cy_stc_dmac_descriptor_t * descriptor, cy_en_dmac_descriptor_type_t descriptorType)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L3(CY_DMAC_IS_TYPE_VALID(descriptorType));
    
    if (descriptorType != Cy_DMAC_Descriptor_GetDescriptorType(descriptor)) /* Don't perform if the type is not changed */
    {
        /* Store the current nextDescriptor pointer. */
        cy_stc_dmac_descriptor_t * locNextDescriptor = Cy_DMAC_Descriptor_GetNextDescriptor(descriptor);
        /* Store the current X data counter. */
        uint32_t locXcount = Cy_DMAC_Descriptor_GetXloopDataCount(descriptor);
        
        /* Change the descriptor type. */
        CY_REG32_CLR_SET(descriptor->ctl, DMAC_CH_V2_DESCR_CTL_DESCR_TYPE, descriptorType);
        
        Cy_DMAC_Descriptor_SetXloopDataCount(descriptor, locXcount);
        
        /* Restore the nextDescriptor pointer into the proper place. */
        Cy_DMAC_Descriptor_SetNextDescriptor(descriptor, locNextDescriptor);
    }
}


/*******************************************************************************
* Function Name: Cy_DMAC_Descriptor_GetXloopDataCount
****************************************************************************//**
*
* Returns the number of data elements for the X loop of the specified
* descriptor (for 1D or 2D descriptors only).
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The number of data elements to transfer in the X loop.
*
* \funcusage 
* \snippet dmac\1.0\snippet\main.c snippet_Cy_DMAC_Descriptor_GetNextDescriptor
*  
*******************************************************************************/
uint32_t Cy_DMAC_Descriptor_GetXloopDataCount(cy_stc_dmac_descriptor_t const * descriptor)
{
    CY_ASSERT_L1(NULL != descriptor);
    
    uint32_t retVal = 0UL;
    cy_en_dmac_descriptor_type_t locDescriptorType = Cy_DMAC_Descriptor_GetDescriptorType(descriptor);
    
    CY_ASSERT_L1(CY_DMAC_SINGLE_TRANSFER != locDescriptorType);
    
    /* Convert the data count from the machine range (0-65535) into the user's range (1-65536). */
    if (CY_DMAC_SCATTER_TRANSFER == locDescriptorType)
    {
        retVal = _FLD2VAL(DMAC_CH_V2_DESCR_X_SIZE_X_COUNT, descriptor->dst) + 1UL;
    }
    else
    {
        retVal = _FLD2VAL(DMAC_CH_V2_DESCR_X_SIZE_X_COUNT, descriptor->xSize) + 1UL;
    }
    
    return (retVal);
}


/*******************************************************************************
* Function Name: Cy_DMAC_Descriptor_SetXloopDataCount
****************************************************************************//**
*
* Sets the number of data elements to transfer in the X loop
* for the specified descriptor (for 1D or 2D descriptors only).
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param xCount
* The number of data elements to transfer in the X loop.
*
* \funcusage 
* \snippet dmac\1.0\snippet\main.c snippet_Cy_DMAC_Descriptor_SetNextDescriptor
*  
*******************************************************************************/
void Cy_DMAC_Descriptor_SetXloopDataCount(cy_stc_dmac_descriptor_t * descriptor, uint32_t xCount)
{
    CY_ASSERT_L1(NULL != descriptor);
    
    cy_en_dmac_descriptor_type_t locDescriptorType = Cy_DMAC_Descriptor_GetDescriptorType(descriptor);
    
    CY_ASSERT_L1(CY_DMAC_SINGLE_TRANSFER != locDescriptorType);
    CY_ASSERT_L2(CY_DMAC_IS_LOOP_COUNT_VALID(xCount));
    
    /* Convert the data count from the user's range (1-65536) into the machine range (0-65535). */
    if (CY_DMAC_SCATTER_TRANSFER == locDescriptorType)
    {
        descriptor->dst = _VAL2FLD(DMAC_CH_V2_DESCR_X_SIZE_X_COUNT, xCount);
    }
    else
    {
        descriptor->xSize = _VAL2FLD(DMAC_CH_V2_DESCR_X_SIZE_X_COUNT, xCount);
    }
}


#if defined(__cplusplus)
}
#endif

#endif /* defined(CY_IP_M4CPUSS_DMAC) */

/* [] END OF FILE */
