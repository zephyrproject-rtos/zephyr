/**
  ******************************************************************************
  * @file    stm32g0xx.h
  * @author  MCD Application Team
  * @brief   CMSIS STM32G0xx Device Peripheral Access Layer Header File.
  *
  *          The file is the unique include file that the application programmer
  *          is using in the C source code, usually in main.c. This file contains:
  *           - Configuration section that allows to select:
  *              - The STM32G0xx device used in the target application
  *              - To use or not the peripherals drivers in application code(i.e.
  *                code will be based on direct access to peripherals registers
  *                rather than drivers API), this option is controlled by
  *                "#define USE_HAL_DRIVER"
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics. 
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the 
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup stm32g0xx
  * @{
  */

#ifndef STM32G0xx_H
#define STM32G0xx_H

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/** @addtogroup Library_configuration_section
  * @{
  */

/**
  * @brief STM32 Family
  */
#if !defined (STM32G0)
#define STM32G0
#endif /* STM32G0 */

/* Uncomment the line below according to the target STM32G0 device used in your
   application
  */

#if !defined (STM32G071xx) && !defined (STM32G081xx) && !defined (STM32G070xx)
  /* #define STM32G070xx */   /*!< STM32G070xx Devices */
  /* #define STM32G071xx */   /*!< STM32G071xx Devices */
  /* #define STM32G081xx */   /*!< STM32G081xx Devices */
#endif

/*  Tip: To avoid modifying this file each time you need to switch between these
        devices, you can define the device in your toolchain compiler preprocessor.
  */
#if !defined  (USE_HAL_DRIVER)
/**
 * @brief Comment the line below if you will not use the peripherals drivers.
   In this case, these drivers will not be included and the application code will
   be based on direct access to peripherals registers
   */
  /*#define USE_HAL_DRIVER */
#endif /* USE_HAL_DRIVER */

/**
  * @brief CMSIS Device version number $VERSION$
  */
#define __STM32G0_CMSIS_VERSION_MAIN   (0x01U) /*!< [31:24] main version */
#define __STM32G0_CMSIS_VERSION_SUB1   (0x00U) /*!< [23:16] sub1 version */
#define __STM32G0_CMSIS_VERSION_SUB2   (0x00U) /*!< [15:8]  sub2 version */
#define __STM32G0_CMSIS_VERSION_RC     (0x00U) /*!< [7:0]  release candidate */
#define __STM32G0_CMSIS_VERSION        ((__STM32G0_CMSIS_VERSION_MAIN << 24)\
                                       |(__STM32G0_CMSIS_VERSION_SUB1 << 16)\
                                       |(__STM32G0_CMSIS_VERSION_SUB2 << 8 )\
                                       |(__STM32G0_CMSIS_VERSION_RC))

/**
  * @}
  */

/** @addtogroup Device_Included
  * @{
  */

#if defined(STM32G071xx)
  #include "stm32g071xx.h"
#elif defined(STM32G081xx)
  #include "stm32g081xx.h"
#elif defined(STM32G070xx)
  #include "stm32g070xx.h"
#else
 #error "Please select first the target STM32G0xx device used in your application (in stm32g0xx.h file)"
#endif

/**
  * @}
  */

/** @addtogroup Exported_types
  * @{
  */
typedef enum
{
  RESET = 0,
  SET = !RESET
} FlagStatus, ITStatus;

typedef enum
{
  DISABLE = 0,
  ENABLE = !DISABLE
} FunctionalState;
#define IS_FUNCTIONAL_STATE(STATE) (((STATE) == DISABLE) || ((STATE) == ENABLE))

typedef enum
{
  SUCCESS = 0,
  ERROR = !SUCCESS
} ErrorStatus;

/**
  * @}
  */


/** @addtogroup Exported_macros
  * @{
  */
#define SET_BIT(REG, BIT)     ((REG) |= (BIT))

#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))

#define READ_BIT(REG, BIT)    ((REG) & (BIT))

#define CLEAR_REG(REG)        ((REG) = (0x0))

#define WRITE_REG(REG, VAL)   ((REG) = (VAL))

#define READ_REG(REG)         ((REG))

#define MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))

#define POSITION_VAL(VAL)     (__CLZ(__RBIT(VAL)))
/**
  * @}
  */

#if defined (USE_HAL_DRIVER)
 #include "stm32g0xx_hal.h"
#endif /* USE_HAL_DRIVER */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* STM32G0xx_H */
/**
  * @}
  */

/**
  * @}
  */




/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
