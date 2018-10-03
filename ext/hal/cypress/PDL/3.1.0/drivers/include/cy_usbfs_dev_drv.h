/***************************************************************************//**
* \file cy_usbfs_dev_drv.h
* \version 1.0
*
* Provides API declarations of the USBFS driver.
*
********************************************************************************
* \copyright
* Copyright 2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_usbfs_dev_drv USB Full-Speed Device
* \{
*       \defgroup group_usbfs_dev_drv_mid    Middleware Requested Interface
*       \defgroup group_usbfs_dev_drv_common Driver Specific Interface
*       \defgroup group_usbfs_dev_drv_reg    Register Access (Internal)
* \} */

/**
* \addtogroup group_usbfs_dev_drv
* \{
* Driver API for Full-Speed Device.
*
* The description will be provided later.
*
* \section group_usbfs_dev_drv_configuration Configuration Considerations
*
* The description will be provided later.
*
* \section group_usbfs_drv_more_information More Information
*
* For more information on the USB Full-Speed Device peripheral, refer to the 
* technical reference manual (TRM).
*
* \section group_usbfs_drv_MISRA MISRA-C Compliance
* <table class="doxtable">
*   <tr>
*     <th>MISRA Rule</th>
*     <th>Rule Class (Required/Advisory)</th>
*     <th>Rule Description</th>
*     <th>Description of Deviation(s)</th>
*   </tr>
*   <tr>
*     <td>11.4</td>
*     <td>A</td>
*     <td>A cast should not be performed between a pointer to object type and
*         a different pointer to object type.</td>
*     <td>The description will be provided later.</td>
*   </tr>
* </table>
*
* \section group_usbfs_drv_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
* \} group_usbfs_dev_drv */


/**
* \addtogroup group_usbfs_dev_drv_common
* \{
*     \defgroup  group_usbfs_dev_hal_functions_common          Initialization Functions
*     \defgroup  group_usbfs_dev_hal_functions_ep0_service     Endpoint 0 Service Functions
*     \defgroup  group_usbfs_dev_hal_functions_endpoint_config Data Endpoint Configuration Functions
*     \defgroup  group_usbfs_dev_hal_functions_data_xfer       Data Transfer Functions
*     \defgroup  group_usbfs_dev_hal_functions_service         Service Functions

* \defgroup group_usbfs_dev_drv_macros Macros
* \{
*     \defgroup group_usbfs_dev_drv_macros_intr_cause Interrupt Cause
*     \defgroup group_usbfs_dev_drv_macros_ep_errors Endpoint Errors
* \}
* \defgroup group_usbfs_dev_drv_functions Functions
* \{
*     \defgroup group_usbfs_dev_drv_functions_interrupts Interrupt Functions
*     \defgroup group_usbfs_dev_drv_functions_hw_access  Hardware-Specific Functions
*     \defgroup group_usbfs_dev_drv_functions_lpm        LPM (Link Power Management) Functions
*     \defgroup group_usbfs_dev_drv_functions_service    Service Functions
* \}
* \defgroup group_usbfs_dev_drv_data_structures Data Structures
* \defgroup group_usbfs_dev_drv_enums           Enumerated Types
* \}
*/


#if !defined(CY_USBFS_DEV_DRV_H)
#define CY_USBFS_DEV_DRV_H

#include "cyip_usbfs.h"
#include "cy_dma.h"
#include "cy_usbfs_dev_drv_reg.h"

#ifdef CY_IP_MXUSBFS

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
*                           Driver version and ID
*******************************************************************************/

/** USBFS Driver major version */
#define CY_USBFS_VERSION_MAJOR      (1)

/** USBFS Driver minor version */
#define CY_USBFS_VERSION_MINOR      (0)

/** USBFS Driver identifier */
#define CY_USBFS_ID                 CY_PDL_DRV_ID(0x3B)

/** USBFS Driver mode position in STATUS CODE: 0 - Device, 1 - Host */
#define CY_USBFS_MODE_POS           (15UL)

/** USBFS Driver status code Device */
#define CY_USBFS_DEV_DRV_STATUS_CODE    (0U)


/*******************************************************************************
*                           Enumerated Types
*******************************************************************************/

/**
* \addtogroup group_usbfs_dev_drv_enums
* \{
*/

/** USBFS Device Driver return codes */
typedef enum
{
    CY_USB_DEV_EP_IDLE,       /**< Endpoint is in idle state after configuration is set */
    CY_USB_DEV_EP_PENDING,    /**< Transfer targeted to an endpoint is in progress */
    CY_USB_DEV_EP_COMPLETED,  /**< Transfer targeted to an endpoint is completed */
    CY_USB_DEV_EP_STALLED,    /**< Endpoint is stalled */
    CY_USB_DEV_EP_DISABLED,   /**< Endpoint is disabled (not used in this configuration) */
    CY_USB_DEV_EP_IVALID,     /**< Endpoint does not supported by the hardware */
}  cy_en_usb_dev_ep_state_t;

/** Callback Sources */
typedef enum
{
    CY_USB_DEV_BUS_RESET = 0U,   /**< Callback hooked to bus reset interrupt */
    CY_USB_DEV_EP0_SETUP = 1U,   /**< Callback hooked to endpoint 0 SETUP packet interrupt */
    CY_USB_DEV_EP0_IN    = 2U,   /**< Callback hooked to endpoint 0 IN packet interrupt */
    CY_USB_DEV_EP0_OUT   = 3U,   /**< Callback hooked to endpoint 0 OUT packet interrupt */
} cy_en_usb_dev_service_cb_t;

/** USBFS Device Driver return codes */
typedef enum
{
    /** Operation completed successfully */
    CY_USBFS_DEV_DRV_SUCCESS = 0U,
    
    /** One or more of input parameters are invalid */                                
    CY_USBFS_DEV_DRV_BAD_PARAM              = (CY_USBFS_ID | CY_PDL_STATUS_ERROR | CY_USBFS_DEV_DRV_STATUS_CODE | 1U), 
    
    /** There is not enough space in the buffer to be allocated for endpoint (hardware or ram) */ 
    CY_USBFS_DEV_DRV_BUF_ALLOC_FAILED       = (CY_USBFS_ID | CY_PDL_STATUS_ERROR | CY_USBFS_DEV_DRV_STATUS_CODE | 2U),        
    
    /** Failure during DMA configuration */                                                     
    /* When this type of error occurs there is no recovery and reasons must be investigated. */ 
    CY_USBFS_DEV_DRV_DMA_CFG_FAILED          = (CY_USBFS_ID | CY_PDL_STATUS_ERROR | CY_USBFS_DEV_DRV_STATUS_CODE | 3U),       
    
    /** Timeout during dynamic reconfiguration (5 ms) */                                         
    /* When this type of error occurs there is no recovery and reasons must be investigated. */    
    CY_USBFS_DEV_DRV_EP_DYN_RECONFIG_TIMEOUT = (CY_USBFS_ID | CY_PDL_STATUS_ERROR | CY_USBFS_DEV_DRV_STATUS_CODE | 4U), 
    
    /** Timeout during execution of the DMA read OUT endpoint request (5 ms) */
    /* When this type of error occurs there is no recovery and reasons must be investigated. */    
    CY_USBFS_DEV_DRV_EP_DMA_READ_TIMEOUT     = (CY_USBFS_ID | CY_PDL_STATUS_ERROR | CY_USBFS_DEV_DRV_STATUS_CODE | 5U), 

} cy_en_usbfs_dev_drv_status_t; 


/** Data Endpoints Buffer Management Mode */
typedef enum
{
    CY_USBFS_DEV_DRV_EP_MANAGEMENT_CPU = 0,         /**< CPU */
    CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA = 1,         /**< DMA manual trigger */
    CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA_AUTO = 2,    /**< DMA auto trigger */
} cy_en_usbfs_dev_drv_ep_management_mode_t;

/** Callback Sources */
typedef enum
{
    CY_USBFS_DEV_DRV_EP1 = 0U,   /**< Callback hooked to Data Endpoint 1 completion interrupt */
    CY_USBFS_DEV_DRV_EP2 = 1U,   /**< Callback hooked to Data Endpoint 2 completion interrupt */
    CY_USBFS_DEV_DRV_EP3 = 2U,   /**< Callback hooked to Data Endpoint 3 completion interrupt */
    CY_USBFS_DEV_DRV_EP4 = 3U,   /**< Callback hooked to Data Endpoint 4 completion interrupt */
    CY_USBFS_DEV_DRV_EP5 = 4U,   /**< Callback hooked to Data Endpoint 5 completion interrupt */
    CY_USBFS_DEV_DRV_EP6 = 5U,   /**< Callback hooked to Data Endpoint 6 completion interrupt */
    CY_USBFS_DEV_DRV_EP7 = 6U,   /**< Callback hooked to Data Endpoint 7 completion interrupt */
    CY_USBFS_DEV_DRV_EP8 = 7U,   /**< Callback hooked to Data Endpoint 8 completion interrupt */
    CY_USBFS_DEV_DRV_SOF = 8U,   /**< Callback hooked to SOF packet received interrupt */
    CY_USBFS_DEV_DRV_LPM = 9U,   /**< Callback hooked to LPM request received interrupt */
} cy_en_usbfs_dev_drv_cb_source_t;

/** Data Endpoint Register Access */
typedef enum
{
    CY_USBFS_DEV_DRV_USE_8_BITS_DR,     /**< Use 8-bits registers to access data endpoints */
    CY_USBFS_DEV_DRV_USE_16_BITS_DR,    /**< Use 16-bits registers to access data endpoints */
} cy_en_usbfs_dev_ep_access_t;

/** LPM (Link Power Management) Responses */
typedef enum
{
    CY_USBFS_DEV_DRV_LPM_REQ_ACK,   /**< The next LPM request will be responded with ACK */
    CY_USBFS_DEV_DRV_LPM_REQ_NACK,  /**< The next LPM request will be responded with NACK */
    CY_USBFS_DEV_DRV_LPM_REQ_NYET   /**< The next LPM request will be responded with NYET */
} cy_en_usbfs_dev_drv_lpm_req_t;

/** USB Line State Control */
typedef enum
{
    CY_USBFS_DEV_DRV_FORCE_STATE_J    = 0xA0U,  /**< Force a J State onto the USB lines */
    CY_USBFS_DEV_DRV_FORCE_STATE_K    = 0x80U,  /**< Force a K State onto the USB lines */
    CY_USBFS_DEV_DRV_FORCE_STATE_SE0  = 0xC0U,  /**< Force a Single Ended 0 onto the USB lines */
    CY_USBFS_DEV_DRV_FORCE_STATE_NONE = 0x00U   /**< Return bus to SIE control */
} cy_en_usbfs_dev_drv_force_bus_state_t;

/** \} group_usbfs_dev_drv_enums */


/*******************************************************************************
*                                  Type Definitions
*******************************************************************************/

/**
* \addtogroup group_usbfs_dev_drv_data_structures
* \{
*/

/**
* Endpoint configuration structure
*/
typedef struct
{
    bool     enableEndpoint;    /**< Defines if endpoint becomes active after configuration */
    bool     allocBuffer;       /**< Defines if buffer allocation is need for endpoint */
    uint8_t  endpointAddr;      /**< Endpoint address (number plus direction bit) */
    uint8_t  attributes;        /**< Endpoint attributes */
    uint16_t maxPacketSize;     /**< Endpoint max packet size */
    uint16_t bufferSize;        /**< Endpoint buffer size (the biggest max packet size
                                     across all alternate for this endpoint) */
    uint16_t outBufferSize;      /**< OUT endpoint buffer size (the biggest max packet size
                                      across all alternate for this endpoint) */
} cy_stc_usb_dev_ep_config_t;

/**
* Drive Context structure prototype.
* The driver must define this structure content using typedef.
*/
struct cy_stc_usbfs_dev_drv_context;

/**
* Provides the typedef for the callback function called in the
* \ref Cy_USBFS_Dev_Drv_Interrupt to notify the user interrupt events.
*/
typedef void (* cy_cb_usbfs_dev_drv_callback_t)(USBFS_Type *base, 
                                                struct cy_stc_usbfs_dev_drv_context *context);


/**
* Provides the typedef for the callback function called in the
* \ref Cy_USBFS_Dev_Drv_Interrupt to notify the user about transfer completion 
* event for endpoint.
*/
typedef void (* cy_cb_usbfs_dev_drv_ep_callback_t)(USBFS_Type *base, 
                                                   uint32_t errorType, 
                                                   struct cy_stc_usbfs_dev_drv_context *context);

/**
* Provides the typedef for the user defined memcpy.
*/
typedef uint8_t * (* cy_fn_usbfs_dev_drv_memcpy_ptr_t)(uint8_t *dest, 
                                                       uint8_t *src, 
                                                       uint32_t size);

/**
* Provides the typedef for the callback function called in the
* \ref Cy_USBFS_Dev_Drv_Interrupt to notify the user interrupt events.
*/
typedef void (* cy_cb_usbfs_dev_drv_service_t)(USBFS_Type *base, 
                                               struct cy_stc_usbfs_dev_drv_context *context);

/**
* Specifies the typedef for the pointer to the function which loads data into
* the data endpoint.
*/
typedef cy_en_usbfs_dev_drv_status_t (* cy_fn_usbfs_dev_drv_add_ep_ptr_t)
                                        (USBFS_Type *base,
                                         cy_stc_usb_dev_ep_config_t const *config,
                                         struct cy_stc_usbfs_dev_drv_context     *context);

/**
* Specifies the typedef for the pointer to the function which loads data into
* the data endpoint.
*/
typedef cy_en_usbfs_dev_drv_status_t (* cy_fn_usbfs_dev_drv_load_ep_ptr_t)
                                        (USBFS_Type    *base,
                                         uint32_t       endpoint,
                                         const uint8_t *buffer,
                                         uint32_t       size,
                                         struct cy_stc_usbfs_dev_drv_context *context);

/**
* Specifies the typedef for the pointer to the function which reads data from
* the data endpoint.
*/
typedef cy_en_usbfs_dev_drv_status_t (* cy_fn_usbfs_dev_drv_read_ep_ptr_t)
                                        (USBFS_Type *base,
                                         uint32_t    endpoint,
                                         uint8_t    *buffer,
                                         uint32_t    size,
                                         uint32_t   *actSize,
                                         struct cy_stc_usbfs_dev_drv_context *context);

/** DMA Channel Configuration Structure */
typedef struct
{
    DW_Type *base;          /**< Pointer to the base */
    uint32_t chNum;         /**< Channel number */
    uint32_t priority;      /**< Channel's priority */
    bool     preemptable;   /**< Specifies if the channel is preempt-able by another higher-priority channel */
    uint32_t outTrigMux;    /**< DMA out trigger mux */

    cy_stc_dma_descriptor_t *descr0;    /**< Pointer to descriptor 0 */
    cy_stc_dma_descriptor_t *descr1;    /**< Pointer to descriptor 1 */

} cy_stc_usbfs_dev_drv_dma_config_t;


/** Driver Configuration Structure */
typedef struct cy_stc_usbfs_dev_drv_config
{
    /** Operation mode: CPU, DMA, DMA auto */
    cy_en_usbfs_dev_drv_ep_management_mode_t mode;

    /** Dma channels configuration array */
    const cy_stc_usbfs_dev_drv_dma_config_t *dmaConfig[CY_USBFS_DEV_DRV_NUM_EPS_MAX];

    /** 
    * Pointer to the buffer for all output endpoints. 
    * Only required if \ref mode is CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA_AUTO. 
    */
    uint8_t  *epBuffer;     
    
    /** 
    * The size of buffer for all output endpoints. 
    * Only required if \ref mode is CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA_AUTO. 
    */
    uint16_t  epBufferSize;

    /** Mask which assigns interrupt to group LO, MED or HI */
    uint32_t intrLevelSel;

    /** Enables LMP response */
    uint32_t enableLmp;

    /** Data endpoints access type: 16-bits or 8-bits */
    cy_en_usbfs_dev_ep_access_t epAccess;

} cy_stc_usbfs_dev_drv_config_t;

/** Endpoint Structure */
typedef struct
{
    uint8_t  address; /**< Endpoint address (include direction bit) */
    uint8_t  toggle;  /**< Toggle bit in SIE_EPX_CNT1 register */
    uint8_t  sieMode; /**< SIE mode to arm endpoint on the bus */

    uint8_t  *buffer;  /**< Pointer to the buffer */  
    uint16_t  bufSize; /**< Endpoint buffer size */

    bool isPending;   /**< Save pending state before stall endpoint */
    volatile cy_en_usb_dev_ep_state_t state; /**< Endpoint state */

    /** Complete event notification callback */
    cy_cb_usbfs_dev_drv_ep_callback_t cbEpComplete;

    DW_Type *base;          /**< Pointer to the DMA base */
    uint32_t chNum;         /**< DMA Channel number */
    uint32_t outTrigMux;    /**< Out trigger mux for DMA channel number */

    cy_stc_dma_descriptor_t* descr0;    /**< Pointer to the descriptor 0 */
    cy_stc_dma_descriptor_t* descr1;    /**< Pointer to the descriptor 1 */
    
    cy_fn_usbfs_dev_drv_memcpy_ptr_t  userMemcpy;  /**< Pointer to user memcpy function */

} cy_stc_usbfs_dev_drv_endpoint_data_t;


/** USBFS Device context structure
* All fields for the context structure are internal. Firmware never reads or
* writes these values. Firmware allocates the structure and provides the
* address of the structure to the driver in function calls. Firmware must
* ensure that the defined instance of this structure remains in scope while
* the drive is in use.
 */
typedef struct cy_stc_usbfs_dev_drv_context
{
    /** Operation mode of driver: CPU, DMA, DMA auto */
    cy_en_usbfs_dev_drv_ep_management_mode_t mode;

    /** Defines which registers to use */
    bool useReg16;

    /** Bus reset callback notification */
    cy_cb_usbfs_dev_drv_callback_t busReset;

    /** Endpoint 0: Setup packet is received callback notification */
    cy_cb_usbfs_dev_drv_callback_t ep0Setup;

    /** Endpoint 0: IN data packet is received callback notification */
    cy_cb_usbfs_dev_drv_callback_t ep0In;

    /** Endpoint 0: OUT data packet is received callback notification */
    cy_cb_usbfs_dev_drv_callback_t ep0Out;

    /** SOF frame is received callback notification */
    cy_cb_usbfs_dev_drv_callback_t cbSof;

    /** LPM request is received callback notification */
    cy_cb_usbfs_dev_drv_callback_t cbLpm;

    /** Pointer to addEndpoint function: depends on operation mode */
    cy_fn_usbfs_dev_drv_add_ep_ptr_t addEndpoint;

    /** Pointer to LoadInEndpoint function: depends on operation mode */
    cy_fn_usbfs_dev_drv_load_ep_ptr_t loadInEndpoint;

    /** Pointer to ReadOutEndpoint function: depends on operation mode */
    cy_fn_usbfs_dev_drv_read_ep_ptr_t readOutEndpoint;

    uint32_t ep0ModeReg;  /**< Endpoint 0 Mode register */
    uint32_t ep0CntReg;   /**< Endpoint 0 Counter register */

    /** SRAM Buffer for output endpoints in
    * \ref CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA_AUTO mode
    */
    uint8_t  *epSharedBuf;
    uint16_t  epSharedBufSize; /**< SRAM buffer size */

    /** Current position in endpoint buffer (HW or SRAM) */
    uint16_t  curBufAddr;

    /** Stores endpoints information */
    cy_stc_usbfs_dev_drv_endpoint_data_t epPool[CY_USBFS_DEV_DRV_NUM_EPS_MAX];

    /** Pointer to the device context structure */
    void *devConext;

} cy_stc_usbfs_dev_drv_context_t;
/** \} group_usbfs_dev_drv_data_structures */


/*******************************************************************************
*                              Function Prototypes
*******************************************************************************/

/**
* \addtogroup group_usbfs_dev_hal_functions_common
* \{
*/
cy_en_usbfs_dev_drv_status_t Cy_USBFS_Dev_Drv_Init(USBFS_Type *base,
                                                   cy_stc_usbfs_dev_drv_config_t const *config,
                                                   cy_stc_usbfs_dev_drv_context_t      *context);

void Cy_USBFS_Dev_Drv_DeInit(USBFS_Type *base, 
                             cy_stc_usbfs_dev_drv_context_t *context);

void Cy_USBFS_Dev_Drv_Enable(USBFS_Type *base, 
                             cy_stc_usbfs_dev_drv_context_t *context);

void Cy_USBFS_Dev_Drv_Disable(USBFS_Type *base, 
                              cy_stc_usbfs_dev_drv_context_t *context);

__STATIC_INLINE void  Cy_USBFS_Dev_Drv_SetDevContext(void *devContext, 
                                                     cy_stc_usbfs_dev_drv_context_t *context);

__STATIC_INLINE void* Cy_USBFS_Dev_Drv_GetDevContext(cy_stc_usbfs_dev_drv_context_t *context);

void Cy_USBFS_Dev_Drv_RegisterServiceCallback(cy_en_usb_dev_service_cb_t source,
                                              cy_cb_usbfs_dev_drv_service_t   callback,
                                              cy_stc_usbfs_dev_drv_context_t *context);

/** \} group_usbfs_dev_hal_functions_common */

/**
* \addtogroup group_usbfs_dev_hal_functions_ep0_service
* \{
*/
void Cy_USBFS_Dev_Drv_Ep0GetSetup(USBFS_Type *base,
                                  uint8_t    *buffer,
                                  cy_stc_usbfs_dev_drv_context_t *context);

uint32_t Cy_USBFS_Dev_Drv_Ep0Write(USBFS_Type *base,
                                   uint8_t    *buffer,
                                   uint32_t    size,
                                   cy_stc_usbfs_dev_drv_context_t *context);

uint32_t Cy_USBFS_Dev_Drv_Ep0Read(USBFS_Type *base,
                                  uint8_t    *buffer,
                                  uint32_t    size,
                                  cy_stc_usbfs_dev_drv_context_t *context);

__STATIC_INLINE void Cy_USBFS_Dev_Drv_Ep0Stall(USBFS_Type *base, 
                                               cy_stc_usbfs_dev_drv_context_t *context);
/** \} group_usbfs_dev_hal_functions_ep0_service */


/**
* \addtogroup group_usbfs_dev_hal_functions_service
* \{
*/

__STATIC_INLINE void Cy_USBFS_Dev_Drv_SetDeviceAddress(USBFS_Type *base, uint8_t address);

void Cy_USBFS_Dev_Drv_ConfigureDevice(USBFS_Type *base, 
                                     cy_stc_usbfs_dev_drv_context_t *context);

void Cy_USBFS_Dev_Drv_ConfigureDeviceComplete(USBFS_Type *base,
                                              cy_stc_usbfs_dev_drv_context_t *context);

void Cy_USBFS_Dev_Drv_UnConfigureDevice(USBFS_Type *base, 
                                        cy_stc_usbfs_dev_drv_context_t *context);
/** \} group_usbfs_dev_hal_functions_service */


/**
* \addtogroup group_usbfs_dev_hal_functions_endpoint_config
* \{
*/
__STATIC_INLINE  cy_en_usbfs_dev_drv_status_t Cy_USBFS_Dev_Drv_AddEndpoint(USBFS_Type *base,
                                                          cy_stc_usb_dev_ep_config_t const *config,
                                                          cy_stc_usbfs_dev_drv_context_t     *context);

cy_en_usbfs_dev_drv_status_t Cy_USBFS_Dev_Drv_RemoveEndpoint(USBFS_Type *base,
                                                             uint32_t    endpointAddr,
                                                             cy_stc_usbfs_dev_drv_context_t *context);

__STATIC_INLINE cy_en_usb_dev_ep_state_t Cy_USBFS_Dev_Drv_GetEndpointStallState(USBFS_Type *base,
                                                                               uint32_t    endpoint,
                                                                               cy_stc_usbfs_dev_drv_context_t *context);

cy_en_usbfs_dev_drv_status_t Cy_USBFS_Dev_Drv_StallEndpoint(USBFS_Type *base,
                                                            uint32_t    endpoint,
                                                            cy_stc_usbfs_dev_drv_context_t *context);

cy_en_usbfs_dev_drv_status_t Cy_USBFS_Dev_Drv_UnStallEndpoint(USBFS_Type *base,
                                                              uint32_t    endpoint,
                                                              cy_stc_usbfs_dev_drv_context_t *context);
/** \} group_usbfs_dev_hal_functions_endpoint_config */

/**
* \addtogroup group_usbfs_dev_hal_functions_data_xfer
* \{
*/
__STATIC_INLINE cy_en_usbfs_dev_drv_status_t Cy_USBFS_Dev_Drv_LoadInEndpoint(USBFS_Type    *base,
                                                             uint32_t       endpoint,
                                                             const uint8_t  buffer[],
                                                             uint32_t       size,
                                                             cy_stc_usbfs_dev_drv_context_t *context);

__STATIC_INLINE cy_en_usbfs_dev_drv_status_t Cy_USBFS_Dev_Drv_ReadOutEndpoint(USBFS_Type *base,
                                                              uint32_t    endpoint,
                                                              uint8_t     buffer[],
                                                              uint32_t    size,
                                                              uint32_t   *actSize,
                                                              cy_stc_usbfs_dev_drv_context_t *context);

void Cy_USBFS_Dev_Drv_EnableOutEndpoint(USBFS_Type *base,
                                        uint32_t    endpoint,
                                        cy_stc_usbfs_dev_drv_context_t *context);

__STATIC_INLINE cy_en_usb_dev_ep_state_t Cy_USBFS_Dev_Drv_GetEndpointState(USBFS_Type *base,
                                                                           uint32_t    endpoint,
                                                                           cy_stc_usbfs_dev_drv_context_t *context);
/** \} group_usbfs_dev_hal_functions_data_xfer */


/**
* \addtogroup group_usbfs_dev_drv_functions_interrupts
* \{
*/
/* Interrupt handler routine */
void Cy_USBFS_Dev_Drv_Interrupt(USBFS_Type *base, uint32_t intrCause, cy_stc_usbfs_dev_drv_context_t *context);

__STATIC_INLINE void Cy_USBFS_Dev_Drv_RegisterSofCallback(USBFS_Type *base, 
                                                          cy_cb_usbfs_dev_drv_callback_t  handle, 
                                                          cy_stc_usbfs_dev_drv_context_t *context);

__STATIC_INLINE void Cy_USBFS_Dev_Drv_RegisterLpmCallback(USBFS_Type *base, 
                                                          cy_cb_usbfs_dev_drv_callback_t  handle, 
                                                          cy_stc_usbfs_dev_drv_context_t *context);

__STATIC_INLINE void Cy_USBFS_Dev_Drv_RegisterEndpointCallback(USBFS_Type *base, 
                                                               uint32_t endpoint, 
                                                               cy_cb_usbfs_dev_drv_ep_callback_t callback,
                                                               cy_stc_usbfs_dev_drv_context_t    *context);
/* Get interrupt cause */
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetInterruptCauseHi (USBFS_Type const *base);
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetInterruptCauseMed(USBFS_Type const *base);
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetInterruptCauseLo (USBFS_Type const *base);

/* Configure to which group interrupt belongs */
__STATIC_INLINE     void Cy_USBFS_Dev_Drv_SetInterruptsLevel(USBFS_Type *base, uint32_t IntrLevel);
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetInterruptsLevel(USBFS_Type const *base);
/** \} group_usbfs_dev_drv_functions_interrupts */


/**
* \addtogroup group_usbfs_dev_drv_functions_lpm
* \{
*/
/** LPM kind of feature - user controlled */
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_Lpm_GetBeslValue       (USBFS_Type *base);
__STATIC_INLINE bool     Cy_USBFS_Dev_Drv_Lpm_RemoteWakeUpAllowed(USBFS_Type *base);
__STATIC_INLINE void     Cy_USBFS_Dev_Drv_Lpm_SetResponse        (USBFS_Type *base, cy_en_usbfs_dev_drv_lpm_req_t response);
__STATIC_INLINE cy_en_usbfs_dev_drv_lpm_req_t Cy_USBFS_Dev_Drv_Lpm_GetResponse(USBFS_Type *base);
/** \} group_usbfs_dev_drv_functions_lpm */


/**
* \addtogroup group_usbfs_dev_drv_functions_hw_access
* \{
*/
__STATIC_INLINE bool Cy_USBFS_Dev_Drv_CheckActivity(USBFS_Type *base);
__STATIC_INLINE void Cy_USBFS_Dev_Drv_Force        (USBFS_Type *base, cy_en_usbfs_dev_drv_force_bus_state_t state);
                void Cy_USBFS_Dev_Drv_Suspend(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context);
                void Cy_USBFS_Dev_Drv_Resume (USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context);

__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetDeviceAddress   (USBFS_Type *base);
__STATIC_INLINE bool     Cy_USBFS_Dev_Drv_GetEndpointAckState(USBFS_Type *base, uint32_t endpoint);
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetEndpointCount   (USBFS_Type *base, uint32_t endpoint);
/** \} group_usbfs_dev_drv_functions_hw_access */


/**
* \addtogroup group_usbfs_dev_drv_functions_service
* \{
*/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_OverwriteMemcpy(USBFS_Type *base, 
                                                      uint32_t    endpoint, 
                                                      cy_fn_usbfs_dev_drv_memcpy_ptr_t  func, 
                                                      cy_stc_usbfs_dev_drv_context_t *context);
/** \} group_usbfs_dev_drv_functions_service */


/*******************************************************************************
*                                 Driver Constants
*******************************************************************************/

/**
* \addtogroup group_usbfs_dev_drv_macros_intr_cause
* \{
*/
#define CY_USBFS_DEV_DRV_LPM_INTR       USBFS_USBLPM_INTR_CAUSE_HI_LPM_INTR_Msk         /**< Link Power Management request interrupt */
#define CY_USBFS_DEV_DRV_ARBITER_INTR   USBFS_USBLPM_INTR_CAUSE_HI_ARB_EP_INTR_Msk      /**< Arbiter interrupt */
#define CY_USBFS_DEV_DRV_EP0_INTR       USBFS_USBLPM_INTR_CAUSE_HI_EP0_INTR_Msk         /**< Endpoint 0 interrupt */
#define CY_USBFS_DEV_DRV_SOF_INTR       USBFS_USBLPM_INTR_CAUSE_HI_SOF_INTR_Msk         /**< SOF interrupt */
#define CY_USBFS_DEV_DRV_BUS_RESET_INTR USBFS_USBLPM_INTR_CAUSE_HI_BUS_RESET_INTR_Msk   /**< Bus Reset interrupt */
#define CY_USBFS_DEV_DRV_EP1_INTR       USBFS_USBLPM_INTR_CAUSE_HI_EP1_INTR_Msk         /**< Data endpoint 1 interrupt */
#define CY_USBFS_DEV_DRV_EP2_INTR       USBFS_USBLPM_INTR_CAUSE_HI_EP2_INTR_Msk         /**< Data endpoint 2 interrupt */
#define CY_USBFS_DEV_DRV_EP3_INTR       USBFS_USBLPM_INTR_CAUSE_HI_EP3_INTR_Msk         /**< Data endpoint 3 interrupt */
#define CY_USBFS_DEV_DRV_EP4_INTR       USBFS_USBLPM_INTR_CAUSE_HI_EP4_INTR_Msk         /**< Data endpoint 4 interrupt */
#define CY_USBFS_DEV_DRV_EP5_INTR       USBFS_USBLPM_INTR_CAUSE_HI_EP5_INTR_Msk         /**< Data endpoint 5 interrupt */
#define CY_USBFS_DEV_DRV_EP6_INTR       USBFS_USBLPM_INTR_CAUSE_HI_EP6_INTR_Msk         /**< Data endpoint 6 interrupt */
#define CY_USBFS_DEV_DRV_EP7_INTR       USBFS_USBLPM_INTR_CAUSE_HI_EP7_INTR_Msk         /**< Data endpoint 7 interrupt */
#define CY_USBFS_DEV_DRV_EP8_INTR       USBFS_USBLPM_INTR_CAUSE_HI_EP8_INTR_Msk         /**< Data endpoint 8 interrupt */
/** \} group_usbfs_dev_drv_macros_intr_cause */

/**
* \addtogroup group_usbfs_dev_drv_macros_ep_errors
* \{
*/
/** Error occurred during USB transfer. 
* For an IN transaction, this indicates a no response from HOST scenario. 
* For an OUT transaction, this represents an RxErr (PID
* error/ CRC error/ bit-stuff error scenario).  
*/
#define CY_USBFS_DEV_ENDPOINT_TRANSFER_ERROR    (1U)

/** The received OUT packet has the same data toggle that previous packet has. 
* This indicates that host retransmit packet.
*/
#define CY_USBFS_DEV_ENDPOINT_SAME_DATA_TOGGLE  (2U)
/** \} group_usbfs_dev_drv_macros_intr_cause */


/*******************************************************************************
*                              Internal Constants
*******************************************************************************/

/** \cond INTERNAL */

/* Start position of data endpoints SEI interrupt sources */
#define USBFS_USBLPM_INTR_CAUSE_LPM_INTR_Msk        USBFS_USBLPM_INTR_CAUSE_HI_LPM_INTR_Msk
#define USBFS_USBLPM_INTR_CAUSE_ARB_EP_INTR_Msk     USBFS_USBLPM_INTR_CAUSE_HI_ARB_EP_INTR_Msk
#define USBFS_USBLPM_INTR_CAUSE_EP0_INTR_Msk        USBFS_USBLPM_INTR_CAUSE_HI_EP0_INTR_Msk
#define USBFS_USBLPM_INTR_CAUSE_SOF_INTR_Msk        USBFS_USBLPM_INTR_CAUSE_HI_SOF_INTR_Msk
#define USBFS_USBLPM_INTR_CAUSE_BUS_RESET_INTR_Msk  USBFS_USBLPM_INTR_CAUSE_HI_BUS_RESET_INTR_Msk
#define USBFS_USBLPM_INTR_CAUSE_EP1_INTR_Pos        USBFS_USBLPM_INTR_CAUSE_HI_EP1_INTR_Pos

/* Validation macros */
#define CY_USBFS_DEV_DRV_IS_EP_VALID(endpoint)  (((endpoint) > 0U) && ((endpoint) <= CY_USBFS_DEV_DRV_NUM_EPS_MAX))
#define CY_USBFS_DEV_DRV_EP2PHY(endpoint)       ((endpoint) - 1U)
#define CY_USBFS_DEV_DRV_EP2MASK(endpont)       (0x1UL << endpoint)

#define CY_USBFS_DEV_DRV_EPADDR2EP(endpointAddr)        ((endpointAddr) & 0x0FU)
#define CY_USBFS_DEV_DRV_IS_EP_DIR_IN(endpointAddr)     (0U != ((endpointAddr) & 0x80U))
#define CY_USBFS_DEV_DRV_IS_EP_DIR_OUT(endpointAddr)    (0U == ((endpointAddr) & 0x80U))
#define CY_USBFS_DEV_DRV_EPADDR2PHY(endpointAddr)       CY_USBFS_DEV_DRV_EP2PHY(CY_USBFS_DEV_DRV_EPADDR2EP(endpointAddr))

#define CY_USBFS_DEV_DRV_IS_MODE_VALID(mode)    (((mode) == CY_USBFS_DEV_DRV_EP_MANAGEMENT_CPU) || \
                                                 ((mode) == CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA) || \
                                                 ((mode) == CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA_AUTO))
/** \endcond */

/** \} group_usbfs_drv_macros */


/*******************************************************************************
*                         In-line Function Implementation
*******************************************************************************/

/**
* \addtogroup group_usbfs_dev_drv_functions_interrupts
* \{
*/

/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_RegisterSofCallback
****************************************************************************//**
*
* Registers a callback function that notifies that SOF event occurred. The SOF 
* interrupt source is enabled after registration. When callback removed the 
* interrupt is disabled.
*
* \param base
* The pointer to the USBFS instance.
*
* \param callback
* The pointer to a callback function.
* See \ref cy_cb_usbfs_dev_drv_callback_t for the function prototype.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.
*
* \note
* To remove the callback, pass NULL as the pointer to a callback function.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_RegisterSofCallback(USBFS_Type *base, 
                                                          cy_cb_usbfs_dev_drv_callback_t  callback, 
                                                          cy_stc_usbfs_dev_drv_context_t *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) base;

    uint32_t mask;
    
    context->cbSof = callback;
    
    /* Enable/Disable SOF interrupt */
    mask = Cy_USBFS_Dev_Drv_GetSieInterruptMask(base);
    
    if (NULL != callback)
    {
        mask |= CY_USBFS_DEV_DRV_INTR_SIE_SOF;
    }
    else
    {
        mask &= ~CY_USBFS_DEV_DRV_INTR_SIE_SOF;
    }
    
    Cy_USBFS_Dev_Drv_SetSieInterruptMask(base, mask);    
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_RegisterLpmCallback
****************************************************************************//**
*
* Registers a callback function that notifies that LPM event occurred. The LPM 
* interrupt source is enabled after registration. When callback is removed the 
* interrupt source is disabled.
*
* \param base
* The pointer to the USBFS instance.
*
* \param callback
* The pointer to a callback function.
* See \ref cy_cb_usbfs_dev_drv_callback_t for the function prototype.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.
*
* \note
* To remove the callback, pass NULL as the pointer to a callback function.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_RegisterLpmCallback(USBFS_Type *base, 
                                                          cy_cb_usbfs_dev_drv_callback_t  callback, 
                                                          cy_stc_usbfs_dev_drv_context_t *context)

{
    /* Suppress a compiler warning about unused variables */
    (void) base;
    
    uint32_t mask;
    
    context->cbLpm = callback;
    
    /* Enable/Disable LPM interrupt source */
    mask = Cy_USBFS_Dev_Drv_GetSieInterruptMask(base);
    
    if (NULL != callback)
    {
        mask |= CY_USBFS_DEV_DRV_INTR_SIE_LPM;
    }
    else
    {
        mask &= ~CY_USBFS_DEV_DRV_INTR_SIE_LPM;
    }
    
    Cy_USBFS_Dev_Drv_SetSieInterruptMask(base, mask);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_RegisterEndpointCallback
****************************************************************************//**
*
* Registers a callback function that notifies endpoint transfer completed 
* event occurred. 
* For IN endpoint it means that host read the data. 
* For OUT endpoint it means that data ready for processing.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
*
* \param callback
* The pointer to a callback function.
* See \ref cy_cb_usbfs_dev_drv_ep_callback_t for the function prototype.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.
*
* \note
* To remove the callback, pass NULL as the pointer to a callback function.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_RegisterEndpointCallback(USBFS_Type *base, 
                                                               uint32_t endpoint, 
                                                               cy_cb_usbfs_dev_drv_ep_callback_t  callback, 
                                                               cy_stc_usbfs_dev_drv_context_t    *context)

{
    /* Suppress a compiler warning about unused variables */
    (void) base;
    
    CY_ASSERT_L1(CY_USBFS_DEV_DRV_IS_EP_VALID(endpoint));
    
    endpoint = CY_USBFS_DEV_DRV_EP2PHY(endpoint);
    context->epPool[endpoint].cbEpComplete = callback;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetInterruptCauseHi
****************************************************************************//**
*
* Returns the mask of bits showing the source of the current triggered
* interrupt. This is useful for modes of operation where an interrupt can
* be generated by conditions in multiple interrupt source registers.
*
* \param base
* The pointer to the USBFS instance.
*
* \return
* The mask with the OR of the following conditions that have been triggered.
* See \ref group_usbfs_dev_drv_macros_intr_cause for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetInterruptCauseHi(USBFS_Type const *base)
{
    return base->USBLPM.INTR_CAUSE_HI;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetInterruptCauseMed
****************************************************************************//**
*
* Returns the mask of bits showing the source of the current triggered
* interrupt. This is useful for modes of operation where an interrupt can
* be generated by conditions in multiple interrupt source registers.
*
* \param base
* The pointer to the USBFS instance.
*
* \return
* The mask with the OR of the following conditions that have been triggered.
* See \ref group_usbfs_dev_drv_macros_intr_cause for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetInterruptCauseMed(USBFS_Type const *base)
{
    return base->USBLPM.INTR_CAUSE_MED;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetInterruptCauseLo
****************************************************************************//**
*
* Returns the mask of bits showing the source of the current triggered
* interrupt. This is useful for modes of operation where an interrupt can
* be generated by conditions in multiple interrupt source registers.
*
* \param base
* The pointer to the USBFS instance.
*
* \return
* The mask with the OR of the following conditions that have been triggered.
* See \ref group_usbfs_dev_drv_macros_intr_cause for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetInterruptCauseLo(USBFS_Type const *base)
{
    return base->USBLPM.INTR_CAUSE_LO;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_SetInterruptsLevel
****************************************************************************//**
*
* Writes INTR_LVL_SEL register which contains groups for all interrupt sources.
*
* \param base
* The pointer to the USBFS instance.
*
* \param intrLevel
* INTR_LVL_SEL register value.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_SetInterruptsLevel(USBFS_Type *base, uint32_t intrLevel)
{
    base->USBLPM.INTR_LVL_SEL = intrLevel;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetInterruptsLevel
****************************************************************************//**
*
* Returns INTR_LVL_SEL register which contains groups for all interrupt sources.
*
* \param base
* The pointer to the USBFS instance.
*
* \return
* Returns INTR_LVL_SEL register which contains groups for all interrupt sources.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetInterruptsLevel(USBFS_Type const *base)
{
    return base->USBLPM.INTR_LVL_SEL;
}
/** \} group_usbfs_dev_drv_functions_interrupts */

/**
* \addtogroup group_usbfs_dev_drv_functions_hw_access
* \{
*/
/*******************************************************************************
* Function Name: USBFS_CheckActivity
****************************************************************************//**
*
* This function returns the activity status of the bus. It clears the hardware
* status to provide updated status on the next call of this function. It
* provides a way to determine whether any USB bus activity occurred. The
* application should use this function to determine if the USB suspend
* conditions are met.
*
* \param base
* The pointer to the USBFS instance.
*
* \return
* Returns true if Bus activity was detected since the last call to this function
* Otherwise false.
*
*******************************************************************************/
__STATIC_INLINE bool Cy_USBFS_Dev_Drv_CheckActivity(USBFS_Type *base)
{
    uint32_t tmpReg = base->USBDEV.CR1;

    /* Clear hardware status */
    base->USBDEV.CR1 &= (tmpReg & ~USBFS_USBDEV_CR1_BUS_ACTIVITY_Msk);

    /* Push bufferable write to execute */
    (void) base->USBDEV.CR1;

    return (0U != (tmpReg & USBFS_USBDEV_CR1_BUS_ACTIVITY_Msk));
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_Force
****************************************************************************//**
*
* This function forces a USB J, K, or SE0 state on the USB lines. It provides
* the necessary mechanism for a USB device application to perform a USB Remote
* Wakeup. For more information, see the USB 2.0 Specification for details on
* Suspend and Resume.
*
* \param base
* The pointer to the USBFS instance.

* \param state
* Desired bus state.
* See \ref cy_en_usbfs_dev_drv_force_bus_state_t for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_Force(USBFS_Type *base, cy_en_usbfs_dev_drv_force_bus_state_t state)
{
    base->USBDEV.USBIO_CR0 = (uint32_t) state;

    /* Push bufferable write to execute */
    (void) base->USBDEV.USBIO_CR0;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetDeviceAddress
****************************************************************************//**
*
* This function returns the currently assigned address for the USB device.
*
* \param base
* The pointer to the USBFS instance.
*
* \return
* Returns the currently assigned address.
* Returns 0 if the device has not yet been assigned an address.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetDeviceAddress(USBFS_Type *base)
{
    return _FLD2VAL(USBFS_USBDEV_CR0_DEVICE_ADDRESS, base->USBDEV.CR0);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetEndpointAckState
****************************************************************************//**
*
* This function determines whether an ACK transaction occurred on this endpoint
* by reading the ACK bit in the control register of the endpoint. It does not
* clear the ACK bit.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Contains the data endpoint number. Valid values are between 1 and 8.
*
* \return
* If an ACKed transaction occurred, this function returns a TRUE value.
* Otherwise, it returns FALSE.
*
*******************************************************************************/
__STATIC_INLINE bool Cy_USBFS_Dev_Drv_GetEndpointAckState(USBFS_Type *base, uint32_t endpoint)
{
    CY_ASSERT_L1(CY_USBFS_DEV_DRV_IS_EP_VALID(endpoint));
    
    cy_stc_usbfs_dev_drv_acces_sie_t *sieRegs = (cy_stc_usbfs_dev_drv_acces_sie_t *) base;

    endpoint = CY_USBFS_DEV_DRV_EP2PHY(endpoint);

    return _FLD2BOOL(USBFS_USBDEV_SIE_EP1_CR0_ACKED_TXN, sieRegs->EP[endpoint].SIE_EP_CR0);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetEndpointCount
****************************************************************************//**
*
* This function supports Data Endpoints only(EP1-EP8).
* Returns the transfer count for the requested endpoint.  The value from
* the count registers includes 2 counts for the two byte checksum of the
* packet. This function subtracts the two counts.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data Endpoint Number. Valid values are between 1 and 8.
*
* \return
* Returns the current byte count from the specified endpoint or 0 for an
* invalid endpoint.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetEndpointCount(USBFS_Type *base, uint32_t endpoint)
{
    CY_ASSERT_L1(CY_USBFS_DEV_DRV_IS_EP_VALID(endpoint));

    return Cy_USBFS_Dev_Drv_GetSieEpCount(base, CY_USBFS_DEV_DRV_EP2PHY(endpoint));
}
/** \} group_usbfs_dev_drv_functions_hw_access */

/**
* \addtogroup group_usbfs_dev_drv_functions_lpm
* \{
*/
/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_Lpm_GetBeslValue
****************************************************************************//**
*
* This function returns the Best Effort Service Latency (BESL) value
* sent by the host as part of the LPM token transaction.
*
* \param base
* The pointer to the USBFS instance.
*
* \return
* 4-bit BESL value received in the LPM token packet from the host.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_Lpm_GetBeslValue(USBFS_Type *base)
{
    return _FLD2VAL(USBFS_USBLPM_LPM_STAT_LPM_BESL, base->USBLPM.LPM_STAT);
}


/*******************************************************************************
* Function Name: USBFS_Lpm_RemoteWakeUpAllowed
****************************************************************************//**
*
* This function returns the remote wakeup permission set for the device by
* the host as part of the LPM token transaction.
*
* \param base
* The pointer to the USBFS instance.
*
* \return
*  True - remote wakeup not allowed, False - remote wakeup allowed
*
*******************************************************************************/
__STATIC_INLINE bool Cy_USBFS_Dev_Drv_Lpm_RemoteWakeUpAllowed(USBFS_Type *base)
{
    return _FLD2BOOL(USBFS_USBLPM_LPM_STAT_LPM_REMOTEWAKE, base->USBLPM.LPM_STAT);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_Lpm_SetResponse
****************************************************************************//**
*
* This function configures the response in the handshake packet the device
* has to send when an LPM token packet is received.
*
* \param base
* The pointer to the USBFS instance.
*
* \param response
* The type of response to return for an LPM token packet.
* See \ref cy_en_usbfs_dev_drv_lpm_req_t.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_Lpm_SetResponse(USBFS_Type *base, cy_en_usbfs_dev_drv_lpm_req_t response)
{
    base->USBLPM.LPM_CTL = _CLR_SET_FLD32U(base->USBLPM.LPM_CTL, USBFS_USBLPM_LPM_CTL_LPM_RESP, response);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_Lpm_GetResponse
****************************************************************************//**
*
* This function returns the currently configured response value that the
* device will send as part of the handshake packet when an LPM token
* packet is received.
*
* \param base
* The pointer to the USBFS instance.
*
* \return
* The type of handshake response that will be returned by the device
* for an LPM token packet. See \ref cy_en_usbfs_dev_drv_lpm_req_t.
*
*******************************************************************************/
__STATIC_INLINE cy_en_usbfs_dev_drv_lpm_req_t Cy_USBFS_Dev_Drv_Lpm_GetResponse(USBFS_Type *base)
{
    return _FLD2VAL(USBFS_USBLPM_LPM_CTL_LPM_RESP, base->USBLPM.LPM_CTL);
}
/** \} group_usbfs_dev_drv_functions_lpm */


/**
* \addtogroup group_usbfs_dev_drv_functions_service
* \{
*/
/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_OverwriteMemcpy
****************************************************************************//**
*
* Overwrite the memory copy function used internally when moving data from the 
* internal buffer to the application for OUT endpoints in 
* \ref CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA_AUTO endpoint management mode.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint 
* Endpoint to be updated.
*
* \param func
* The pointer to function to be executed to copying data from driver buffer into 
* the user provided buffer in the \ref Cy_USBFS_Dev_Drv_ReadOutEndpoint.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_OverwriteMemcpy(USBFS_Type *base, 
                                                      uint32_t    endpoint, 
                                cy_fn_usbfs_dev_drv_memcpy_ptr_t  func, 
                                  cy_stc_usbfs_dev_drv_context_t *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) base;
    
    CY_ASSERT_L1(CY_USBFS_DEV_DRV_IS_EP_VALID(endpoint));
    
    endpoint = CY_USBFS_DEV_DRV_EP2PHY(endpoint);
    context->epPool[endpoint].userMemcpy = func;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_SetDeviceAddress
****************************************************************************//**
*
* Sets device address.
*
* \param base
* The pointer to the USBFS instance.
*
* \param address
* Device address.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_SetDeviceAddress(USBFS_Type *base, uint8_t address)
{
    base->USBDEV.CR0 = _CLR_SET_FLD32U(base->USBDEV.CR0, USBFS_USBDEV_CR0_DEVICE_ADDRESS, address);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_SetDevContext
****************************************************************************//**
*
* Stores pointer to the device context in the driver.
*
* \param devContext
* The pointer to the device context.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_SetDevContext(void *devContext, 
                                                    cy_stc_usbfs_dev_drv_context_t *context)
{
    context->devConext = devContext;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetDevContext
****************************************************************************//**
*
* Returns pointer to the device context.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.
*
* \return
* The pointer to the device context.
*
*******************************************************************************/
__STATIC_INLINE void* Cy_USBFS_Dev_Drv_GetDevContext(cy_stc_usbfs_dev_drv_context_t *context)
{
    return (context->devConext);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_Ep0Stall
****************************************************************************//**
*
* Stalls endpoint 0.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* The data endpoint number from where the data should be read.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_Ep0Stall(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context)
{
    (void) base;
    context->ep0ModeReg = CY_USBFS_DEV_DRV_EP_CR_STALL_INOUT;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_AddEndpoint
****************************************************************************//**
*
* Configures data endpoint for the following operation.
*
* \param base
* The pointer to the USBFS instance.
*
* \param config
* Configuration of the endpoint.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
* \return
* Status of executed operation \ref cy_en_usbfs_dev_drv_status_t.
*
*******************************************************************************/
__STATIC_INLINE cy_en_usbfs_dev_drv_status_t Cy_USBFS_Dev_Drv_AddEndpoint(USBFS_Type *base,
                                                      cy_stc_usb_dev_ep_config_t  const *config,
                                                      cy_stc_usbfs_dev_drv_context_t      *context)
{
    return context->addEndpoint(base, config, context);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetEndpointState
****************************************************************************//**
*
* This function returns the state of the requested endpoint.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* The data endpoint number to which the data should be transferred.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
* \return
* Status of executed operation \ref cy_en_usbfs_dev_drv_ep_state.
*
*******************************************************************************/
__STATIC_INLINE cy_en_usb_dev_ep_state_t Cy_USBFS_Dev_Drv_GetEndpointState(
                                                             USBFS_Type *base,
                                                             uint32_t    endpoint,
                                                             cy_stc_usbfs_dev_drv_context_t *context)

{
    /* Suppress a compiler warning about unused variables */
    (void) base;
    
    CY_ASSERT_L1(CY_USBFS_DEV_DRV_IS_EP_VALID(endpoint));

    return context->epPool[CY_USBFS_DEV_DRV_EP2PHY(endpoint)].state;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_LoadInEndpoint
****************************************************************************//**
*
* Loads data into the endpoint buffer.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* The data endpoint number to which the data should be transferred.
*
* \param buffer
* Pointer to a data array from which the data for the endpoint space is loaded.
*
* \param size
* The number of bytes to transfer from the array and then send as a result of an
* IN request. Valid values are between 0 and 512 (1023 for Automatic DMA mode).
* The value 512 is applicable if only one endpoint is used.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
* \return
* Status of executed operation \ref cy_en_usbfs_dev_drv_status_t.
*
*******************************************************************************/
__STATIC_INLINE cy_en_usbfs_dev_drv_status_t Cy_USBFS_Dev_Drv_LoadInEndpoint(
                                                         USBFS_Type   *base,
                                                         uint32_t      endpoint,
                                                         const uint8_t buffer[],
                                                         uint32_t      size,
                                                         cy_stc_usbfs_dev_drv_context_t *context)
{
    CY_ASSERT_L1(CY_USBFS_DEV_DRV_IS_EP_VALID(endpoint));
    CY_ASSERT_L1(CY_USBFS_DEV_DRV_IS_EP_DIR_IN(context->epPool[CY_USBFS_DEV_DRV_EP2PHY(endpoint)].address));

    return context->loadInEndpoint(base, CY_USBFS_DEV_DRV_EP2PHY(endpoint), buffer, size, context);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_ReadOutEndpoint
****************************************************************************//**
*
* Reads data from OUT endpoint.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* The data endpoint number from where the data should be read.
*
* \param buffer
* Pointer to a data array to which the data from the endpoint space is loaded.
*
* \param size
* The number of bytes to transfer from the USB OUT endpoint and load into data
* array. Valid values are between 0 and 512 (1023 for Automatic DMA mode).
* The function moves fewer than the requested number of bytes if the host sends
* fewer bytes than requested.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
* \return
* Status of executed operation \ref cy_en_usbfs_dev_drv_status_t.
*
*******************************************************************************/
__STATIC_INLINE cy_en_usbfs_dev_drv_status_t Cy_USBFS_Dev_Drv_ReadOutEndpoint(
                                                          USBFS_Type *base,
                                                          uint32_t    endpoint,
                                                          uint8_t     buffer[],
                                                          uint32_t    size,
                                                          uint32_t   *actSize,
                                                          cy_stc_usbfs_dev_drv_context_t *context)
{
    CY_ASSERT_L1(CY_USBFS_DEV_DRV_IS_EP_VALID(endpoint));
    CY_ASSERT_L1(CY_USBFS_DEV_DRV_IS_EP_DIR_OUT(context->epPool[CY_USBFS_DEV_DRV_EP2PHY(endpoint)].address));

    return context->readOutEndpoint(base, CY_USBFS_DEV_DRV_EP2PHY(endpoint), buffer, size, actSize, context);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetEndpointStallState
****************************************************************************//**
*
* This function returns the state of the requested endpoint.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* The data endpoint number to which the data should be transferred.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
* \return
* Status of executed operation \ref cy_en_usbfs_dev_drv_ep_state.
*
*******************************************************************************/
__STATIC_INLINE cy_en_usb_dev_ep_state_t Cy_USBFS_Dev_Drv_GetEndpointStallState(
                                                              USBFS_Type *base,
                                                              uint32_t    endpoint,
                                                              cy_stc_usbfs_dev_drv_context_t *context)
{    
    if (CY_USBFS_DEV_DRV_IS_EP_VALID(endpoint))
    {
        return Cy_USBFS_Dev_Drv_GetEndpointState(base, endpoint, context);   
    }
    else
    {
        return CY_USB_DEV_EP_IVALID;
    }
}

/** \} group_usbfs_dev_drv_functions_service */

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXUSBFS */

#endif /* (CY_USBFS_DEV_DRV_H) */


/* [] END OF FILE */
