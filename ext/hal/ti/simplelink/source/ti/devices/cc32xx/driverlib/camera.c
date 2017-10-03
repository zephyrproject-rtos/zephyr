/*
 *  Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/ 
 *  
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *  
 */
//*****************************************************************************
//
//  camera.c
//
//  Driver for the camera controller module
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup camera_api
//! @{
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_camera.h"
#include "inc/hw_apps_config.h"
#include "interrupt.h"
#include "camera.h"

//******************************************************************************
//
//! Resets the Camera core
//!
//! \param ulBase is the base address of the camera module.
//!
//! This function resets the camera core
//!
//! \return None.
//
//******************************************************************************
void CameraReset(unsigned long ulBase)
{
  //
  // Reset the camera
  //
  HWREG(ulBase + CAMERA_O_CC_SYSCONFIG) = CAMERA_CC_SYSCONFIG_SOFT_RESET;

  //
  // Wait for reset completion
  //
  while(!(HWREG(ulBase + CAMERA_O_CC_SYSSTATUS)&
          CAMERA_CC_SYSSTATUS_RESET_DONE2))
  {

  }

}

//******************************************************************************
//
//! Configures camera parameters
//!
//! \param ulBase is the base address of the camera module.
//! \param ulHSPol sets the HSync polarity
//! \param ulVSPol sets the VSync polarity
//! \param ulFlags are configuration flags
//!
//! This function sets different camera parameters.
//!
//! The parameter \e ulHSPol should be on the follwoing:
//! - \b CAM_HS_POL_HI
//! - \b CAM_HS_POL_LO
//!
//! The parameter \e ulVSPol should be on the follwoing:
//! - \b CAM_VS_POL_HI
//! - \b CAM_VS_POL_LO
//!
//! The parameter \e ulFlags can be logical OR of one or more of the follwoing
//! or 0:
//! - \b CAM_PCLK_RISE_EDGE
//! - \b CAM_PCLK_FALL_EDGE
//! - \b CAM_ORDERCAM_SWAP
//! - \b CAM_NOBT_SYNCHRO
//! - \b CAM_IF_SYNCHRO
//!
//! \return None.
//
//******************************************************************************
void CameraParamsConfig(unsigned long ulBase, unsigned long ulHSPol,
                        unsigned long ulVSPol, unsigned long ulFlags)
{
  unsigned long ulReg;

  //
  // Read the register
  //
  ulReg = HWREG(ulBase + CAMERA_O_CC_CTRL);

  //
  // Set the requested parameter
  //
  ulFlags = (ulFlags|ulHSPol|ulVSPol);
  ulReg = ((ulReg & ~(CAMERA_CC_CTRL_NOBT_SYNCHRO |
                   CAMERA_CC_CTRL_NOBT_HS_POL |
                   CAMERA_CC_CTRL_NOBT_VS_POL |
                   CAMERA_CC_CTRL_BT_CORRECT  |
                   CAMERA_CC_CTRL_PAR_ORDERCAM |
                   CAMERA_CC_CTRL_PAR_CLK_POL )) | ulFlags);

  //
  // Write the configuration
  //
  HWREG(ulBase + CAMERA_O_CC_CTRL)=ulReg;
}


//******************************************************************************
//
//! Set the internal clock divider
//!
//! \param ulBase is the base address of the camera module.
//! \param ulCamClkIn is input to camera module
//! \param ulXClk defines the output required
//!
//! This function sets the internal clock divider based on \e ulCamClkIn to
//! generate XCLK as specified be \e ulXClk. Maximum suppoter division is 30
//!
//! \return None.
//
//******************************************************************************
void CameraXClkConfig(unsigned long ulBase, unsigned long ulCamClkIn,
                      unsigned long ulXClk)
{
  unsigned long ulReg;
  unsigned long ucDiv;

  //
  // Read the register
  //
  ulReg = HWREG(ulBase + CAMERA_O_CC_CTRL_XCLK);

  //
  // Mask XCLK divider value
  //
  ulReg &= ~(CAMERA_CC_CTRL_XCLK_XCLK_DIV_M);

  //
  // Compute the divider
  //
  ucDiv = ((ulCamClkIn)/ulXClk);

  //
  // Max supported division is 30
  //
  if(ucDiv > 30)
  {
      return;
  }

  //
  // Set and write back the configuration
  //
  ulReg |= ucDiv;
  HWREG(ulBase + CAMERA_O_CC_CTRL_XCLK) = ulReg;
}


//******************************************************************************
//
//! Sets the internal divide in specified mode
//!
//! \param ulBase is the base address of the camera module.
//! \param bXClkFlags decides the divide mode
//!
//! This function sets the internal divide in specified mode.
//!
//! The parameter \e bXClkFlags should be one of the following :
//!
//! - \b CAM_XCLK_STABLE_LO
//! - \b CAM_XCLK_STABLE_HI
//! - \b CAM_XCLK_DIV_BYPASS
//!
//! \return None.
//
//******************************************************************************
void CameraXClkSet(unsigned long ulBase, unsigned char bXClkFlags)
{
  unsigned long ulReg;

  //
  // Read and Mask XTAL Divider config.
  //
  ulReg = (HWREG(ulBase + CAMERA_O_CC_CTRL_XCLK) &
    ~(CAMERA_CC_CTRL_XCLK_XCLK_DIV_M));

  //
  // Set config. base on parameter flag
  //
  switch(bXClkFlags)
  {

  case CAM_XCLK_STABLE_HI : ulReg |= 0x00000001;
                            break;

  case CAM_XCLK_DIV_BYPASS: ulReg |= 0x0000001F;
                            break;
  }

  //
  // Write the config.
  //
  HWREG(ulBase + CAMERA_O_CC_CTRL_XCLK) = ulReg;
}


//******************************************************************************
//
//! Enable camera DMA
//!
//! \param ulBase is the base address of the camera module.
//!
//! This function enables transfer request to DMA from camera. DMA specific
//! configuration has to be done seperately.
//!
//! \return None.
//
//******************************************************************************
void CameraDMAEnable(unsigned long ulBase)
{
  //
  // Enable DMA
  //
  HWREG(ulBase + CAMERA_O_CC_CTRL_DMA) |= CAMERA_CC_CTRL_DMA_DMA_EN;
}


//******************************************************************************
//
//! Disable camera DMA
//!
//! \param ulBase is the base address of the camera module.
//!
//! This function masks transfer request to DMA from camera.
//!
//! \return None.
//
//******************************************************************************
void CameraDMADisable(unsigned long ulBase)
{
  //
  // Disable DMA
  //
  HWREG(ulBase + CAMERA_O_CC_CTRL_DMA) &= ~CAMERA_CC_CTRL_DMA_DMA_EN;
}



//******************************************************************************
//
//! Sets the FIFO threshold for DMA transfer request
//!
//! \param ulBase is the base address of the camera module.
//! \param ulThreshold specifies the FIFO threshold
//!
//! This function sets the FIFO threshold for DMA transfer request.
//! Parameter \e ulThreshold can range from 1 - 64
//!
//! \return None.
//
//******************************************************************************
void CameraThresholdSet(unsigned long ulBase, unsigned long ulThreshold)
{
  //
  // Read and Mask DMA threshold field
  //
  HWREG(ulBase + CAMERA_O_CC_CTRL_DMA) &= ~CAMERA_CC_CTRL_DMA_FIFO_THRESHOLD_M;
  //
  // Write the new threshold value
  //
  HWREG(ulBase + CAMERA_O_CC_CTRL_DMA) |= (ulThreshold -1);
}


//******************************************************************************
//
//! Register camera interrupt handler
//!
//! \param ulBase is the base address of the camera module.
//! \param pfnHandler hold pointer to interrupt handler
//!
//! This function registers and enables global camera interrupt from the
//! interrupt controller. Individual camera interrupts source
//! should be enabled using \sa CameraIntEnable().
//!
//! \return None.
//
//******************************************************************************
void CameraIntRegister(unsigned long ulBase, void (*pfnHandler)(void))
{
    //
    // Register the interrupt handler.
    //
    IntRegister(INT_CAMERA, pfnHandler);

    //
    // Enable the Camera interrupt.
    //
    IntEnable(INT_CAMERA);
}


//******************************************************************************
//
//! Un-Register camera interrupt handler
//!
//! \param ulBase is the base address of the camera module.
//!
//! This function unregisters and disables global camera interrupt from the
//! interrupt controller.
//!
//! \return None.
//
//******************************************************************************
void CameraIntUnregister(unsigned long ulBase)
{
    //
    // Disable the interrupt.
    //
    IntDisable(INT_CAMERA);

    //
    // Unregister the interrupt handler.
    //
    IntUnregister(INT_CAMERA);
}


//******************************************************************************
//! Enables individual camera interrupt sources.
//!
//! \param ulBase is the base address of the camera module.
//! \param ulIntFlags is the bit mask of the interrupt sources to be enabled.
//!
//! This function enables individual camera interrupt sources.
//!
//! the parameter \e ulIntFlags should be logical OR of one or more of the
//! following:
//!
//! - \b CAM_INT_DMA
//! - \b CAM_INT_FE
//! - \b CAM_INT_FSC_ERR
//! - \b CAM_INT_FIFO_NOEMPTY
//! - \b CAM_INT_FIFO_FULL
//! - \b CAM_INT_FIFO_THR
//! - \b CAM_INT_FIFO_OF
//! - \b CAN_INT_FIFO_UR
//!
//! \return None.
//
//******************************************************************************
void CameraIntEnable(unsigned long ulBase, unsigned long ulIntFlags)
{
  //
  // unmask Camera DMA done interrupt
  //
  if(ulIntFlags & CAM_INT_DMA)
  {
    HWREG(APPS_CONFIG_BASE + APPS_CONFIG_O_DMA_DONE_INT_MASK_CLR) = ((1<<8));
  }

  //
  // Enable specific camera interrupts
  //
  HWREG(ulBase + CAMERA_O_CC_IRQENABLE) |= ulIntFlags;
}


//******************************************************************************
//! Disables individual camera interrupt sources.
//!
//! \param ulBase is the base address of the camera module.
//! \param ulIntFlags is the bit mask of the interrupt sources to be disabled.
//!
//! This function disables individual camera interrupt sources.
//!
//! The parameter \e ulIntFlags should be logical OR of one or more of the
//! values as defined in CameraIntEnable().
//!
//! \return None.
//
//******************************************************************************
void CameraIntDisable(unsigned long ulBase, unsigned long ulIntFlags)
{
  //
  // Mask Camera DMA done interrupt
  //
  if(ulIntFlags & CAM_INT_DMA)
  {
    HWREG(APPS_CONFIG_BASE + APPS_CONFIG_O_DMA_DONE_INT_MASK_SET) = ((1<<8));
  }

  //
  // Disable specific camera interrupts
  //
  HWREG(ulBase + CAMERA_O_CC_IRQENABLE) &= ~ulIntFlags;
}

//******************************************************************************
//
//! Returns the current interrupt status,
//!
//! \param ulBase is the base address of the camera module.
//! \param ulBase is the base address of the camera module.
//!
//! This functions returns the current interrupt status for the camera.
//!
//! \return Returns the current interrupt status, enumerated as a bit field of
//! values described in CameraIntEnable().
//******************************************************************************
unsigned long CameraIntStatus(unsigned long ulBase)
{
  unsigned ulIntFlag;

  //
  // Read camera interrupt
  //
  ulIntFlag = HWREG(ulBase + CAMERA_O_CC_IRQSTATUS);

  //
  //
  // Read camera DMA doner interrupt
  //
  if(HWREG(APPS_CONFIG_BASE + APPS_CONFIG_O_DMA_DONE_INT_STS_MASKED) & (1<<8))
  {
    ulIntFlag |= CAM_INT_DMA;
  }

  //
  // Return status
  //
  return(ulIntFlag);
}


//******************************************************************************
//! Clears individual camera interrupt sources.
//!
//! \param ulBase is the base address of the camera module.
//! \param ulIntFlags is the bit mask of the interrupt sources to be Clears.
//!
//! This function Clears individual camera interrupt sources.
//!
//! The parameter \e ulIntFlags should be logical OR of one or more of the
//! values as defined in CameraIntEnable().
//!
//! \return None.
//
//******************************************************************************
void CameraIntClear(unsigned long ulBase, unsigned long ulIntFlags)
{
  //
  // Clear DMA done int  status
  //
  if(ulIntFlags & CAM_INT_DMA)
  {
    HWREG(APPS_CONFIG_BASE + APPS_CONFIG_O_DMA_DONE_INT_ACK) = ((1<<8));
  }

  //
  // Clear the interrupts
  //
  HWREG(ulBase + CAMERA_O_CC_IRQSTATUS) = ulIntFlags;
}

//******************************************************************************
//
//! Starts image capture
//!
//! \param ulBase is the base address of the camera module.
//!
//! This function starts the image capture over the configured camera interface
//! This function should be called after configuring the camera module
//! completele
//!
//! \return None.
//
//******************************************************************************
void CameraCaptureStart(unsigned long ulBase)
{
  //
  // Set the mode
  //
  HWREG(ulBase + CAMERA_O_CC_CTRL) &= ~0xF;

  //
  // Enable image capture
  //
  HWREG(ulBase + CAMERA_O_CC_CTRL) |= CAMERA_CC_CTRL_CC_EN;
}

//******************************************************************************
//
//! Stops image capture
//!
//! \param ulBase is the base address of the camera module.
//! \param bImmediate is \b true to stop capture imeediately else \b flase.
//!
//! This function stops the image capture over the camera interface.
//! The capture is stopped either immediatelt or at the end of current frame
//! based on \e bImmediate parameter.
//!
//! \return None.
//
//******************************************************************************
void CameraCaptureStop(unsigned long ulBase, tBoolean bImmediate)
{
  if(bImmediate)
  {
    //
    // Stop capture immediately
    //
    HWREG(ulBase + CAMERA_O_CC_CTRL) &= ~CAMERA_CC_CTRL_CC_FRAME_TRIG;
  }
  else
  {
    //
    // Stop capture at the end of frame
    //
    HWREG(ulBase + CAMERA_O_CC_CTRL) |= CAMERA_CC_CTRL_CC_FRAME_TRIG;
  }

  //
  // Request camera to stop capture
  //
  HWREG(ulBase + CAMERA_O_CC_CTRL) &= ~CAMERA_CC_CTRL_CC_EN;
}


//******************************************************************************
//
//! Reads the camera buffer (FIFO)
//!
//! \param ulBase is the base address of the camera module.
//! \param pBuffer is the pointer to the read buffer
//! \param ucSize specifies the size to data to be read
//!
//! This function reads the camera buffer (FIFO).
//!
//! \return None.
//
//******************************************************************************
void CameraBufferRead(unsigned long ulBase, unsigned long *pBuffer,
                   unsigned char ucSize)
{
  unsigned char *pCamBuff;
  unsigned char i;

  //
  // Initilize a pointer to ecamera buffer
  //
  pCamBuff = (unsigned char *)CAM_BUFFER_ADDR;

  //
  // Read out requested data
  //
  for(i=0; i < ucSize; i++)
  {
    *(pBuffer+i) = *(pCamBuff + i);
  }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
