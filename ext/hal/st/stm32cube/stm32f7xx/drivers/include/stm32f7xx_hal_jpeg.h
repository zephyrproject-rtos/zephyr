/**
  ******************************************************************************
  * @file    stm32f7xx_hal_jpeg.h
  * @author  MCD Application Team
  * @brief   Header file of JPEG HAL module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F7xx_HAL_JPEG_H
#define __STM32F7xx_HAL_JPEG_H

#ifdef __cplusplus
 extern "C" {
#endif
#if defined (STM32F767xx) || defined (STM32F769xx) || defined (STM32F777xx) || defined (STM32F779xx)
/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal_def.h"

/** @addtogroup STM32F7xx_HAL_Driver
  * @{
  */

/** @addtogroup JPEG
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup JPEG_Exported_Types JPEG Exported Types
  * @{
  */

/** @defgroup JPEG_Configuration_Structure_definition JPEG Configuration for encoding Structure definition
  * @brief  JPEG encoding configuration Structure definition 
  * @{
  */
typedef struct
{
  uint8_t  ColorSpace;                /*!< Image Color space : gray-scale, YCBCR, RGB or CMYK
                                           This parameter can be a value of @ref JPEG_ColorSpace_Type */
  
  uint8_t  ChromaSubsampling;         /*!< Chroma Subsampling in case of YCBCR or CMYK color space, 0-> 4:4:4 , 1-> 4:2:2, 2 -> 4:1:1, 3 -> 4:2:0
                                           This parameter can be a value of @ref JPEG_ChromaSubsampling_Type */
  
  uint32_t ImageHeight;               /*!< Image height : number of lines */
  
  uint32_t ImageWidth;                /*!< Image width : number of pixels per line */
  
  uint8_t  ImageQuality;               /*!< Quality of the JPEG encoding : from 1 to 100 */

}JPEG_ConfTypeDef;
/** 
  * @}
  */

/** @defgroup HAL_JPEG_state_structure_definition HAL JPEG state structure definition
  * @brief  HAL JPEG State structure definition  
  * @{
  */
typedef enum
{
  HAL_JPEG_STATE_RESET              = 0x00U,  /*!< JPEG not yet initialized or disabled  */
  HAL_JPEG_STATE_READY              = 0x01U,  /*!< JPEG initialized and ready for use    */
  HAL_JPEG_STATE_BUSY               = 0x02U,  /*!< JPEG internal processing is ongoing   */
  HAL_JPEG_STATE_BUSY_ENCODING      = 0x03U,  /*!< JPEG encoding processing is ongoing   */
  HAL_JPEG_STATE_BUSY_DECODING      = 0x04U,  /*!< JPEG decoding processing is ongoing   */  
  HAL_JPEG_STATE_TIMEOUT            = 0x05U,  /*!< JPEG timeout state                    */
  HAL_JPEG_STATE_ERROR              = 0x06U   /*!< JPEG error state                      */
}HAL_JPEG_STATETypeDef;

/** 
  * @}
  */


/** @defgroup JPEG_handle_Structure_definition JPEG handle Structure definition 
  * @brief  JPEG handle Structure definition  
  * @{
  */
typedef struct
{
  JPEG_TypeDef             *Instance;        /*!< JPEG peripheral register base address */
            
  JPEG_ConfTypeDef         Conf;             /*!< Current JPEG encoding/decoding parameters */

  uint8_t                  *pJpegInBuffPtr;  /*!< Pointer to JPEG processing (encoding, decoding,...) input buffer */

  uint8_t                  *pJpegOutBuffPtr; /*!< Pointer to JPEG processing (encoding, decoding,...) output buffer */

  __IO uint32_t            JpegInCount;      /*!< Internal Counter of input data */

  __IO uint32_t            JpegOutCount;     /*!< Internal Counter of output data */
    
  uint32_t                 InDataLength;     /*!< Input Buffer Length in Bytes */

  uint32_t                 OutDataLength;    /*!< Output Buffer Length in Bytes */  

  DMA_HandleTypeDef        *hdmain;          /*!< JPEG In DMA handle parameters */

  DMA_HandleTypeDef        *hdmaout;         /*!< JPEG Out DMA handle parameters */

  uint8_t                  CustomQuanTable;  /*!< If set to 1 specify that user customized quantization tables are used */
      
  uint8_t                  *QuantTable0;     /*!< Basic Quantization Table for component 0 */

  uint8_t                  *QuantTable1;     /*!< Basic Quantization Table for component 1 */
      
  uint8_t                  *QuantTable2;     /*!< Basic Quantization Table for component 2 */
      
  uint8_t                  *QuantTable3;     /*!< Basic Quantization Table for component 3 */      
      
  HAL_LockTypeDef          Lock;             /*!< JPEG locking object */
      
  __IO  HAL_JPEG_STATETypeDef State;         /*!< JPEG peripheral state */
      
  __IO  uint32_t           ErrorCode;        /*!< JPEG Error code */
  
  __IO uint32_t Context;                     /*!< JPEG Internal context */

}JPEG_HandleTypeDef;

/** 
  * @}
  */

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup JPEG_Exported_Constants JPEG Exported Constants
  * @{
  */

/** @defgroup JPEG_Error_Code_definition JPEG Error Code definition
  * @brief  JPEG Error Code definition  
  * @{
  */ 

#define  HAL_JPEG_ERROR_NONE        ((uint32_t)0x00000000U)    /*!< No error             */
#define  HAL_JPEG_ERROR_HUFF_TABLE  ((uint32_t)0x00000001U)    /*!< HUffman Table programming error */
#define  HAL_JPEG_ERROR_QUANT_TABLE ((uint32_t)0x00000002U)    /*!< Quantization Table programming error */
#define  HAL_JPEG_ERROR_DMA         ((uint32_t)0x00000004U)    /*!< DMA transfer error   */
#define  HAL_JPEG_ERROR_TIMEOUT     ((uint32_t)0x00000008U)    /*!< Timeout error        */

/** 
  * @}
  */

/** @defgroup JPEG_Quantization_Table_Size JPEG Quantization Table Size
  * @brief  JPEG Quantization Table Size  
  * @{
  */
#define JPEG_QUANT_TABLE_SIZE  ((uint32_t)64U)
/**
  * @}
  */

  
/** @defgroup JPEG_ColorSpace_Type JPEG ColorSpace
  * @brief  JPEG Color Space  
  * @{
  */
#define JPEG_GRAYSCALE_COLORSPACE     ((uint32_t)0x00000000U)
#define JPEG_YCBCR_COLORSPACE         JPEG_CONFR1_COLORSPACE_0
#define JPEG_CMYK_COLORSPACE          JPEG_CONFR1_COLORSPACE


/**
  * @}
  */


/** @defgroup JPEG_ChromaSubsampling_Type JPEG Chrominance Sampling
  * @brief  JPEG Chrominance Sampling  
  * @{
  */
#define JPEG_444_SUBSAMPLING     ((uint32_t)0x00000000U)   /*!< Chroma Subsampling 4:4:4 */
#define JPEG_420_SUBSAMPLING     ((uint32_t)0x00000001U)   /*!< Chroma Subsampling 4:2:0 */
#define JPEG_422_SUBSAMPLING     ((uint32_t)0x00000002U)   /*!< Chroma Subsampling 4:2:2 */

/**
  * @}
  */ 

/** @defgroup JPEG_ImageQuality JPEG Image Quality
  * @brief  JPEG Min and Max Image Quality  
  * @{
  */
#define JPEG_IMAGE_QUALITY_MIN     ((uint32_t)1U)     /*!< Minimum JPEG quality */
#define JPEG_IMAGE_QUALITY_MAX     ((uint32_t)100U)   /*!< Maximum JPEG quality */

/**
  * @}
  */     
  
/** @defgroup JPEG_Interrupt_configuration_definition JPEG Interrupt configuration definition
  * @brief JPEG Interrupt definition
  * @{
  */
#define JPEG_IT_IFT     ((uint32_t)JPEG_CR_IFTIE)   /*!< Input FIFO Threshold Interrupt */
#define JPEG_IT_IFNF    ((uint32_t)JPEG_CR_IFNFIE)  /*!< Input FIFO Not Full Interrupt */
#define JPEG_IT_OFT     ((uint32_t)JPEG_CR_OFTIE)   /*!< Output FIFO Threshold Interrupt */
#define JPEG_IT_OFNE    ((uint32_t)JPEG_CR_OFTIE)   /*!< Output FIFO Not Empty Interrupt */
#define JPEG_IT_EOC     ((uint32_t)JPEG_CR_EOCIE)   /*!< End of Conversion Interrupt */
#define JPEG_IT_HPD     ((uint32_t)JPEG_CR_HPDIE)   /*!< Header Parsing Done Interrupt */ 
/**
  * @}
  */  

/** @defgroup JPEG_Flag_definition JPEG Flag definition
  * @brief JPEG Flags definition
  * @{
  */ 
#define JPEG_FLAG_IFTF     ((uint32_t)JPEG_SR_IFTF)   /*!< Input FIFO is not full and is bellow its threshold flag */
#define JPEG_FLAG_IFNFF    ((uint32_t)JPEG_SR_IFNFF)  /*!< Input FIFO Not Full Flag, a data can be written */
#define JPEG_FLAG_OFTF     ((uint32_t)JPEG_SR_OFTF)   /*!< Output FIFO is not empty and has reach its threshold */
#define JPEG_FLAG_OFNEF    ((uint32_t)JPEG_SR_OFNEF)  /*!< Output FIFO is not empty, a data is available  */
#define JPEG_FLAG_EOCF     ((uint32_t)JPEG_SR_EOCF)   /*!< JPEG Codec core has finished the encoding or the decoding process and than last data has been sent to the output FIFO  */
#define JPEG_FLAG_HPDF     ((uint32_t)JPEG_SR_HPDF)   /*!< JPEG Codec has finished the parsing of the headers and the internal registers have been updated  */
#define JPEG_FLAG_COF      ((uint32_t)JPEG_SR_COF)    /*!< JPEG Codec operation on going  flag*/

#define JPEG_FLAG_ALL      ((uint32_t)0x000000FEU)     /*!< JPEG Codec All previous flag*/
/**
  * @}
  */

/** @defgroup JPEG_PROCESS_PAUSE_RESUME_definition JPEG Process Pause Resume definition
  * @brief JPEG process pause, resume definition
  * @{
  */  
#define JPEG_PAUSE_RESUME_INPUT          ((uint32_t)0x00000001U)     /*!< Pause/Resume Input FIFO Xfer*/
#define JPEG_PAUSE_RESUME_OUTPUT         ((uint32_t)0x00000002U)     /*!< Pause/Resume Output FIFO Xfer*/
#define JPEG_PAUSE_RESUME_INPUT_OUTPUT   ((uint32_t)0x00000003U)     /*!< Pause/Resume Input and Output FIFO Xfer*/
/**
  * @}
  */

/**
  * @}
  */
/* Exported macro ------------------------------------------------------------*/

/** @defgroup JPEG_Exported_Macros JPEG Exported Macros
  * @{
  */

/** @brief Reset JPEG handle state
  * @param  __HANDLE__ specifies the JPEG handle.
  * @retval None
  */
#define __HAL_JPEG_RESET_HANDLE_STATE(__HANDLE__) ( (__HANDLE__)->State = HAL_JPEG_STATE_RESET)


/**
  * @brief  Enable the JPEG peripheral.
  * @param  __HANDLE__ specifies the JPEG handle.
  * @retval None
  */
#define __HAL_JPEG_ENABLE(__HANDLE__)  ((__HANDLE__)->Instance->CR |=  JPEG_CR_JCEN)

/**
  * @brief Disable the JPEG peripheral.
  * @param  __HANDLE__ specifies the JPEG handle.
  * @retval None
  */
#define __HAL_JPEG_DISABLE(__HANDLE__) ((__HANDLE__)->Instance->CR &=  ~JPEG_CR_JCEN)


/**
  * @brief  Check the specified JPEG status flag.
  * @param  __HANDLE__ specifies the JPEG handle. 
  * @param  __FLAG__  specifies the flag to check
  *         This parameter can be one of the following values:
  *         @arg JPEG_FLAG_IFTF  : The input FIFO is not full and is bellow its threshold flag
  *         @arg JPEG_FLAG_IFNFF : The input FIFO Not Full Flag, a data can be written
  *         @arg JPEG_FLAG_OFTF  : The output FIFO is not empty and has reach its threshold
  *         @arg JPEG_FLAG_OFNEF : The output FIFO is not empty, a data is available
  *         @arg JPEG_FLAG_EOCF  : JPEG Codec core has finished the encoding or the decoding process 
  *                                and than last data has been sent to the output FIFO
  *         @arg JPEG_FLAG_HPDF  : JPEG Codec has finished the parsing of the headers 
  *                                and the internal registers have been updated
  *         @arg JPEG_FLAG_COF   : JPEG Codec operation on going  flag
  *                        
  * @retval : __HAL_JPEG_GET_FLAG : returns The new state of __FLAG__ (TRUE or FALSE)  
  */

#define __HAL_JPEG_GET_FLAG(__HANDLE__,__FLAG__)  (((__HANDLE__)->Instance->SR & (__FLAG__)))

/**
  * @brief  Clear the specified JPEG status flag.
  * @param  __HANDLE__ specifies the JPEG handle. 
  * @param  __FLAG__  specifies the flag to clear
  *         This parameter can be one of the following values:
  *         @arg JPEG_FLAG_EOCF  : JPEG Codec core has finished the encoding or the decoding process 
  *                                and than last data has been sent to the output FIFO
  *         @arg JPEG_FLAG_HPDF  : JPEG Codec has finished the parsing of the headers 
  * @retval : None    
  */

#define __HAL_JPEG_CLEAR_FLAG(__HANDLE__,__FLAG__)  (((__HANDLE__)->Instance->CFR |= ((__FLAG__) & (JPEG_FLAG_EOCF | JPEG_FLAG_HPDF))))


/**
  * @brief  Enable Interrupt.
  * @param   __HANDLE__ specifies the JPEG handle.
  * @param  __INTERRUPT__  specifies the interrupt to enable
  *         This parameter can be one of the following values:
  *         @arg JPEG_IT_IFT   : Input FIFO Threshold Interrupt
  *         @arg JPEG_IT_IFNF  : Input FIFO Not Full Interrupt
  *         @arg JPEG_IT_OFT   : Output FIFO Threshold Interrupt
  *         @arg JPEG_IT_OFNE  : Output FIFO Not empty Interrupt
  *         @arg JPEG_IT_EOC   : End of Conversion Interrupt
  *         @arg JPEG_IT_HPD   : Header Parsing Done Interrupt       
  *           
  * @retval : No retrun 
  */
#define __HAL_JPEG_ENABLE_IT(__HANDLE__,__INTERRUPT__)  ((__HANDLE__)->Instance->CR |= (__INTERRUPT__) )

/**
  * @brief  Disable Interrupt.
  * @param   __HANDLE__ specifies the JPEG handle.
  * @param  __INTERRUPT__  specifies the interrupt to disable
  *         This parameter can be one of the following values:
  *         @arg JPEG_IT_IFT   : Input FIFO Threshold Interrupt
  *         @arg JPEG_IT_IFNF  : Input FIFO Not Full Interrupt
  *         @arg JPEG_IT_OFT   : Output FIFO Threshold Interrupt
  *         @arg JPEG_IT_OFNE  : Output FIFO Not empty Interrupt
  *         @arg JPEG_IT_EOC   : End of Conversion Interrupt
  *         @arg JPEG_IT_HPD   : Header Parsing Done Interrupt       
  *           
  * @note  : To disable an IT we must use MODIFY_REG macro to avoid writing "1" to the FIFO flush bits 
  *          located in the same IT enable register (CR register).  
  * @retval : No retrun 
  */
#define __HAL_JPEG_DISABLE_IT(__HANDLE__,__INTERRUPT__) MODIFY_REG((__HANDLE__)->Instance->CR, (__INTERRUPT__), 0)


/**
  * @brief  Get Interrupt state.
  * @param   __HANDLE__ specifies the JPEG handle.
  * @param  __INTERRUPT__  specifies the interrupt to check
  *         This parameter can be one of the following values:
  *         @arg JPEG_IT_IFT   : Input FIFO Threshold Interrupt
  *         @arg JPEG_IT_IFNF  : Input FIFO Not Full Interrupt
  *         @arg JPEG_IT_OFT   : Output FIFO Threshold Interrupt
  *         @arg JPEG_IT_OFNE  : Output FIFO Not empty Interrupt
  *         @arg JPEG_IT_EOC   : End of Conversion Interrupt
  *         @arg JPEG_IT_HPD   : Header Parsing Done Interrupt       
  *           
  * @retval : returns The new state of __INTERRUPT__ (Enabled or disabled)
  */
#define __HAL_JPEG_GET_IT_SOURCE(__HANDLE__,__INTERRUPT__)     ((__HANDLE__)->Instance->CR & (__INTERRUPT__))

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup JPEG_Exported_Functions
  * @{
  */

/** @addtogroup JPEG_Exported_Functions_Group1
  * @{
  */    
/* Initialization/de-initialization functions  ********************************/
HAL_StatusTypeDef HAL_JPEG_Init(JPEG_HandleTypeDef *hjpeg);
HAL_StatusTypeDef HAL_JPEG_DeInit(JPEG_HandleTypeDef *hjpeg);
void HAL_JPEG_MspInit(JPEG_HandleTypeDef *hjpeg);
void HAL_JPEG_MspDeInit(JPEG_HandleTypeDef *hjpeg);

/**
  * @}
  */

/** @addtogroup JPEG_Exported_Functions_Group2
  * @{
  */ 
/* Encoding/Decoding Configuration functions  ********************************/
HAL_StatusTypeDef HAL_JPEG_ConfigEncoding(JPEG_HandleTypeDef *hjpeg, JPEG_ConfTypeDef *pConf);
HAL_StatusTypeDef HAL_JPEG_GetInfo(JPEG_HandleTypeDef *hjpeg, JPEG_ConfTypeDef *pInfo);
HAL_StatusTypeDef HAL_JPEG_EnableHeaderParsing(JPEG_HandleTypeDef *hjpeg);
HAL_StatusTypeDef HAL_JPEG_DisableHeaderParsing(JPEG_HandleTypeDef *hjpeg);
HAL_StatusTypeDef HAL_JPEG_SetUserQuantTables(JPEG_HandleTypeDef *hjpeg, uint8_t *QTable0, uint8_t *QTable1, uint8_t *QTable2, uint8_t *QTable3);

/**
  * @}
  */

/** @addtogroup JPEG_Exported_Functions_Group3
  * @{
  */ 
/* JPEG processing functions  **************************************/
HAL_StatusTypeDef  HAL_JPEG_Encode(JPEG_HandleTypeDef *hjpeg, uint8_t *pDataInMCU, uint32_t InDataLength, uint8_t *pDataOut, uint32_t OutDataLength, uint32_t Timeout);
HAL_StatusTypeDef  HAL_JPEG_Decode(JPEG_HandleTypeDef *hjpeg ,uint8_t *pDataIn ,uint32_t InDataLength ,uint8_t *pDataOutMCU ,uint32_t OutDataLength, uint32_t Timeout);
HAL_StatusTypeDef  HAL_JPEG_Encode_IT(JPEG_HandleTypeDef *hjpeg, uint8_t *pDataInMCU, uint32_t InDataLength, uint8_t *pDataOut, uint32_t OutDataLength);
HAL_StatusTypeDef  HAL_JPEG_Decode_IT(JPEG_HandleTypeDef *hjpeg ,uint8_t *pDataIn ,uint32_t InDataLength ,uint8_t *pDataOutMCU ,uint32_t OutDataLength);
HAL_StatusTypeDef  HAL_JPEG_Encode_DMA(JPEG_HandleTypeDef *hjpeg, uint8_t *pDataInMCU, uint32_t InDataLength, uint8_t *pDataOut, uint32_t OutDataLength);
HAL_StatusTypeDef  HAL_JPEG_Decode_DMA(JPEG_HandleTypeDef *hjpeg ,uint8_t *pDataIn ,uint32_t InDataLength ,uint8_t *pDataOutMCU ,uint32_t OutDataLength);
HAL_StatusTypeDef  HAL_JPEG_Pause(JPEG_HandleTypeDef *hjpeg, uint32_t XferSelection);
HAL_StatusTypeDef  HAL_JPEG_Resume(JPEG_HandleTypeDef *hjpeg, uint32_t XferSelection);
void HAL_JPEG_ConfigInputBuffer(JPEG_HandleTypeDef *hjpeg, uint8_t *pNewInputBuffer, uint32_t InDataLength);
void HAL_JPEG_ConfigOutputBuffer(JPEG_HandleTypeDef *hjpeg, uint8_t *pNewOutputBuffer, uint32_t OutDataLength);
HAL_StatusTypeDef HAL_JPEG_Abort(JPEG_HandleTypeDef *hjpeg);

/**
  * @}
  */

/** @addtogroup JPEG_Exported_Functions_Group4
  * @{
  */ 
/* JPEG Decode/Encode callback functions  ********************************************************/
void HAL_JPEG_InfoReadyCallback(JPEG_HandleTypeDef *hjpeg,JPEG_ConfTypeDef *pInfo);
void HAL_JPEG_EncodeCpltCallback(JPEG_HandleTypeDef *hjpeg);
void HAL_JPEG_DecodeCpltCallback(JPEG_HandleTypeDef *hjpeg);
void HAL_JPEG_ErrorCallback(JPEG_HandleTypeDef *hjpeg);
void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbDecodedData);
void HAL_JPEG_DataReadyCallback (JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut, uint32_t OutDataLength);

/**
  * @}
  */

/** @addtogroup JPEG_Exported_Functions_Group5
  * @{
  */ 
/* JPEG IRQ handler management  ******************************************************/
void HAL_JPEG_IRQHandler(JPEG_HandleTypeDef *hjpeg);

/**
  * @}
  */

/** @addtogroup JPEG_Exported_Functions_Group6
  * @{
  */ 
/* Peripheral State and Error functions  ************************************************/
HAL_JPEG_STATETypeDef  HAL_JPEG_GetState(JPEG_HandleTypeDef *hjpeg);
uint32_t               HAL_JPEG_GetError(JPEG_HandleTypeDef *hjpeg);

/**
  * @}
  */

/**
  * @}
  */ 

/* Private types -------------------------------------------------------------*/
/** @defgroup JPEG_Private_Types JPEG Private Types
  * @{
  */

/**
  * @}
  */ 

/* Private defines -----------------------------------------------------------*/
/** @defgroup JPEG_Private_Defines JPEG Private Defines
  * @{
  */

/**
  * @}
  */ 
          
/* Private variables ---------------------------------------------------------*/
/** @defgroup JPEG_Private_Variables JPEG Private Variables
  * @{
  */

/**
  * @}
  */ 

/* Private constants ---------------------------------------------------------*/
/** @defgroup JPEG_Private_Constants JPEG Private Constants
  * @{
  */

/**
  * @}
  */ 

/* Private macros ------------------------------------------------------------*/
/** @defgroup JPEG_Private_Macros JPEG Private Macros
  * @{
  */

/** @defgroup JPEG_IS_Definitions JPEG Private macros to check input parameters
  * @{
  */

#define IS_JPEG_CHROMASUBSAMPLING(SUBSAMPLING) (((SUBSAMPLING) == JPEG_444_SUBSAMPLING) || \
                                                ((SUBSAMPLING) == JPEG_420_SUBSAMPLING) || \
                                                ((SUBSAMPLING) == JPEG_422_SUBSAMPLING))

#define IS_JPEG_IMAGE_QUALITY(NUMBER) (((NUMBER) >= JPEG_IMAGE_QUALITY_MIN) && ((NUMBER) <= JPEG_IMAGE_QUALITY_MAX))

#define IS_JPEG_COLORSPACE(COLORSPACE) (((COLORSPACE) == JPEG_GRAYSCALE_COLORSPACE) || \
                                        ((COLORSPACE) == JPEG_YCBCR_COLORSPACE)     || \
                                        ((COLORSPACE) == JPEG_CMYK_COLORSPACE))

#define IS_JPEG_PAUSE_RESUME_STATE(VALUE) (((VALUE) == JPEG_PAUSE_RESUME_INPUT) || \
                                           ((VALUE) == JPEG_PAUSE_RESUME_OUTPUT)|| \
                                           ((VALUE) == JPEG_PAUSE_RESUME_INPUT_OUTPUT))

/**
  * @}
  */ 

/**
  * @}
  */ 

/* Private functions prototypes ----------------------------------------------*/
/** @defgroup JPEG_Private_Functions_Prototypes JPEG Private Functions Prototypes
  * @{
  */

/**
  * @}
  */

/* Private functions ---------------------------------------------------------*/
/** @defgroup JPEG_Private_Functions JPEG Private Functions
  * @{
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#endif /* STM32F767xx ||  STM32F769xx ||  STM32F777xx ||  STM32F779xx */ 
#ifdef __cplusplus
}
#endif

#endif /* __STM32F7xx_HAL_JPEG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
