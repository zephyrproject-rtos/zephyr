USB device stack
################

The USB device stack is split into three layers:
   * USB Device Controller drivers (hardware dependent)
   * USB device core driver (hardware independent)
   * USB device class drivers (hardware independent)

USB device controller drivers
*****************************

The Device Controller Driver Layer implements the low level control routines
to deal directly with the hardware. All device controller drivers should
implement the APIs described in file usb_dc.h. This allows the integration of
new USB device controllers to be done without changing the upper layers.
For now only Quark SE USB device controller (Designware IP) is supported.

Structures
==========

.. code-block:: c

   struct usb_dc_ep_cfg_data {
      uint8_t ep_addr;
      uint16_t ep_mps;
      enum usb_dc_ep_type ep_type;
   };

Structure containing the USB endpoint configuration.
   * ep_addr: endpoint address, the number associated with the EP in the device
     configuration structure.
     IN  EP = 0x80 | <endpoint number>. OUT EP = 0x00 | <endpoint number>
   * ep_mps: Endpoint max packet size.
   * ep_type: Endpoint type, may be Bulk, Interrupt or Control. Isochronous
     endpoints are not supported for now.

.. code-block:: c

   enum usb_dc_status_code {
      USB_DC_ERROR,
      USB_DC_RESET,
      USB_DC_CONNECTED,
      USB_DC_CONFIGURED,
      USB_DC_DISCONNECTED,
      USB_DC_SUSPEND,
      USB_DC_RESUME,
      USB_DC_UNKNOWN
   };

Status codes reported by the registered device status callback.
   * USB_DC_ERROR: USB error reported by the controller.
   * USB_DC_RESET: USB reset.
   * USB_DC_CONNECTED: USB connection established - hardware enumeration is completed.
   * USB_DC_CONFIGURED: USB configuration done.
   * USB_DC_DISCONNECTED: USB connection lost.
   * USB_DC_SUSPEND: USB connection suspended by the HOST.
   * USB_DC_RESUME: USB connection resumed by the HOST.
   * USB_DC_UNKNOWN: Initial USB connection status.

.. code-block:: c

   enum usb_dc_ep_cb_status_code {
      USB_DC_EP_SETUP,
      USB_DC_EP_DATA_OUT,
      USB_DC_EP_DATA_IN,
   };

Status Codes reported by the registered endpoint callback.
   * USB_DC_EP_SETUP: SETUP packet received.
   * USB_DC_EP_DATA_OUT: Out transaction on this endpoint. Data is available
     for read.
   * USB_DC_EP_DATA_IN: In transaction done on this endpoint.

APIs
====

The following APIs are provided by the device controller driver:

:c:func:`usb_dc_attach()`
   This function attaches USB for device connection. Upon success, the USB PLL
   is enabled, and the USB device is now capable of transmitting and receiving
   on the USB bus and of generating interrupts.

:c:func:`usb_dc_detach()`
   This function detaches the USB device. Upon success the USB hardware PLL is
   powered down and USB communication is disabled.

:c:func:`usb_dc_reset()`
   This function returns the USB device to it's initial state.

:c:func:`usb_dc_set_address()`
   This function sets USB device address.

:c:func:`usb_dc_set_status_callback()`
   This function sets USB device controller status callback. The registered
   callback is used to report changes in the status of the device controller.
   The status code are described by the usb_dc_status_code enumeration.

:c:func:`usb_dc_ep_configure()`
   This function configures an endpoint. usb_dc_ep_cfg_data structure provides
   the endpoint configuration parameters: endpoint address, endpoint maximum
   packet size and endpoint type.

:c:func:`usb_dc_ep_set_stall()`
   This function sets stall condition for the selected endpoint.

:c:func:`usb_dc_ep_clear_stall()`
   This functions clears stall condition for the selected endpoint

:c:func:`usb_dc_ep_is_stalled()`
   This function check if selected endpoint is stalled.

:c:func:`usb_dc_ep_halt()`
   This function halts the selected endpoint

:c:func:`usb_dc_ep_enable()`
   This function enables the selected endpoint. Upon success interrupts are
   enabled for the corresponding endpoint and the endpoint is ready for
   transmitting/receiving data.

:c:func:`usb_dc_ep_disable()`
   This function disables the selected endpoint. Upon success interrupts are
   disabled for the corresponding endpoint and the endpoint is no longer able
   for transmitting/receiving data.

:c:func:`usb_dc_ep_flush()`
   This function flushes the FIFOs for the selected endpoint.

:c:func:`usb_dc_ep_write()`
   This function writes data to the specified endpoint. The supplied
   usb_ep_callback function will be called when data is transmitted out.

:c:func:`usb_dc_ep_read()`
   This function is called by the Endpoint handler function, after an OUT
   interrupt has been received for that EP. The application must only call this
   function through the supplied usb_ep_callback function.

:c:func:`usb_dc_ep_set_callback()`
   This function sets callback function for notification of data received
   and available to application or transmit done on the selected endpoint.
   The callback status code is described by usb_dc_ep_cb_status_code.

USB device core layer
*********************

The USB Device core layer is a hardware independent interface between USB
device controller driver and USB device class drivers or customer applications.
It's a port of the LPCUSB device stack. It provides the following
functionalities:

   * Responds to standard device requests and returns standard descriptors,
     essentially handling 'Chapter 9' processing, specifically the standard
     device requests in table 9-3 from the universal serial bus specification
     revision 2.0.
   * Provides a programming interface to be used by USB device classes or
     customer applications. The APIs are described in the usb_device.h file.
   * Uses the APIs provided by the device controller drivers to interact with
     the USB device controller.

Structures
==========

.. code-block:: c

   typedef void (*usb_status_callback)(enum usb_dc_status_code status_code);

Callback function signature for the device status.

.. code-block:: c

   typedef void (*usb_ep_callback)(uint8_t ep,
      enum usb_dc_ep_cb_status_code cb_status);

Callback function signature for the USB Endpoint.

.. code-block:: c

   typedef int (*usb_request_handler) (struct usb_setup_packet *setup,
      int *transfer_len, uint8_t **payload_data);

Callback function signature for class specific requests. For host to device
direction the 'len' and 'payload_data' contain the length of the received data
and the pointer to the received data respectively. For device to host class
requests, 'len' and 'payload_data' should be set by the callback function
with the length and the address of the data to be transmitted buffer
respectively.

.. code-block:: c

   struct usb_ep_cfg_data {
      usb_ep_callback ep_cb;
      uint8_t ep_addr;
   };

This structure contains configuration for a certain endpoint.
   * ep_cb: callback function for notification of data received and available
     to application or transmit done, NULL if callback not required by
     application code.
   * ep_addr: endpoint address. The number associated with the EP in the device
     configuration structure.

.. code-block:: c

   struct usb_interface_cfg_data {
      usb_request_handler class_handler;
      usb_request_handler custom_handler;
      uint8_t *payload_data;
   };

This structure contains USB interface configuration.
   * class_handler: handler for USB Class specific Control (EP 0)
     communications.
   * custom_handler: the custom request handler gets a first
     chance at handling the request before it is handed over to the
     'chapter 9' request handler.
   * payload_data: this data area, allocated by the application, is used to
     store class specific command data and must be large enough to store the
     largest payload associated with the largest supported Class' command set.

.. code-block:: c

   struct usb_cfg_data {
      const uint8_t *usb_device_description;
      usb_status_callback cb_usb_status;
      struct usb_interface_cfg_data interface;
      uint8_t num_endpoints;
      struct usb_ep_cfg_data *endpoint;
   };

This structure contains USB device configuration.
   * usb_device_description: USB device description, see
     http://www.beyondlogic.org/usbnutshell/usb5.shtml#DeviceDescriptors
   * cb_usb_status: callback to be notified on USB connection status change
   * interface:  USB class handlers and storage space.
   * num_endpoints: number of individual endpoints in the device configuration
   * endpoint: pointer to an array of endpoint configuration structures
     (usb_cfg_data) of length equal to the number of EP associated with the
     device description, not including control endpoints.

The class drivers instantiates this with given parameters using the
"usb_set_config" function.

APIs
====

:c:func:`usb_set_config()`
   This function configures USB device.

:c:func:`usb_deconfig()`
   This function returns the USB device back to it's initial state

:c:func:`usb_enable()`
   This function enable USB for host/device connection. Upon success, the USB
   module is no longer clock gated in hardware, it is now capable of
   transmitting and receiving on the USB bus and of generating interrupts.

:c:func:`usb_disable()`
   This function disables the USB device. Upon success, the USB module clock
   is gated in hardware and it is no longer capable of generating interrupts.

:c:func:`usb_write()`
   write data to the specified endpoint. The supplied usb_ep_callback will be
   called when transmission is done.

:c:func:`usb_read()`
   This function is called by the endpoint handler function after an OUT
   interrupt has been received for that EP. The application must only call
   this function through the supplied usb_ep_callback function.


USB device class drivers
************************

To initialize the device class driver instance the USB device class driver
should call usb_set_config() passing as parameter the instance's configuration
structure.

For example, for CDC_ACM sample application:

.. code-block:: c

   static const uint8_t cdc_acm_usb_description[] = {
      /* Device descriptor */
      USB_DEVICE_DESC_SIZE,           /* Descriptor size */
      USB_DEVICE_DESC,                /* Descriptor type */
      LOW_BYTE(USB_1_1),
      HIGH_BYTE(USB_1_1),             /* USB version in BCD format */
      COMMUNICATION_DEVICE_CLASS,     /* Class */
      0x00,                           /* SubClass - Interface specific */
      0x00,                           /* Protocol - Interface specific */
      MAX_PACKET_SIZE_EP0,            /* Max Packet Size */
      LOW_BYTE(VENDOR_ID),
      HIGH_BYTE(VENDOR_ID),           /* Vendor Id */
      LOW_BYTE(CDC_PRODUCT_ID),
      HIGH_BYTE(CDC_PRODUCT_ID),      /* Product Id */
      LOW_BYTE(BCDDEVICE_RELNUM),
      HIGH_BYTE(BCDDEVICE_RELNUM),    /* Device Release Number */
      0x01,                           /* Index of Manufacturer String Descriptor */
      0x02,                           /* Index of Product String Descriptor */
      0x03,                           /* Index of Serial Number String Descriptor */
      CDC_NUM_CONF,                   /* Number of Possible Configuration */

      /* Configuration descriptor */
      USB_CONFIGURATION_DESC_SIZE,    /* Descriptor size */
      USB_CONFIGURATION_DESC,         /* Descriptor type */
      LOW_BYTE(CDC_CONF_SIZE),
      HIGH_BYTE(CDC_CONF_SIZE),       /* Total length in bytes of data returned */
      CDC_NUM_ITF,                    /* Number of interfaces */
      0x01,                           /* Configuration value */
      0x00,                           /* Index of the Configuration string */
      USB_CONFIGURATION_ATTRIBUTES,   /* Attributes */
      MAX_LOW_POWER,                  /* Max power consumption */

      /* Interface descriptor */
      USB_INTERFACE_DESC_SIZE,        /* Descriptor size */
      USB_INTERFACE_DESC,             /* Descriptor type */
      0x00,                           /* Interface index */
      0x00,                           /* Alternate setting */
      CDC1_NUM_EP,                    /* Number of Endpoints */
      COMMUNICATION_DEVICE_CLASS,     /* Class */
      ACM_SUBCLASS,                   /* SubClass */
      V25TER_PROTOCOL,                /* Protocol */
      0x00,                           /* Index of the Interface String Descriptor */

      /* Header Functional Descriptor */
      USB_HFUNC_DESC_SIZE,            /* Descriptor size */
      CS_INTERFACE,                   /* Descriptor type */
      USB_HFUNC_SUBDESC,              /* Descriptor SubType */
      LOW_BYTE(USB_1_1),
      HIGH_BYTE(USB_1_1),             /* CDC Device Release Number */

      /* Call Management Functional Descriptor */
      USB_CMFUNC_DESC_SIZE,           /* Descriptor size */
      CS_INTERFACE,                   /* Descriptor type */
      USB_CMFUNC_SUBDESC,             /* Descriptor SubType */
      0x00,                           /* Capabilities */
      0x01,                           /* Data Interface */

      /* ACM Functional Descriptor */
      USB_ACMFUNC_DESC_SIZE,          /* Descriptor size */
      CS_INTERFACE,                   /* Descriptor type */
      USB_ACMFUNC_SUBDESC,            /* Descriptor SubType */
      /* Capabilities - Device supports the request combination of:
       *	Set_Line_Coding,
       *	Set_Control_Line_State,
       *	Get_Line_Coding
       *	and the notification Serial_State
       */
      0x02,

      /* Union Functional Descriptor */
      USB_UFUNC_DESC_SIZE,            /* Descriptor size */
      CS_INTERFACE,                   /* Descriptor type */
      USB_UFUNC_SUBDESC,              /* Descriptor SubType */
      0x00,                           /* Master Interface */
      0x01,                           /* Slave Interface */

      /* Endpoint INT */
      USB_ENDPOINT_DESC_SIZE,         /* Descriptor size */
      USB_ENDPOINT_DESC,              /* Descriptor type */
      CDC_ENDP_INT,                   /* Endpoint address */
      USB_DC_EP_INTERRUPT,            /* Attributes */
      LOW_BYTE(CDC_INTERRUPT_EP_MPS),
      HIGH_BYTE(CDC_INTERRUPT_EP_MPS),/* Max packet size */
      0x0A,                           /* Interval */

      /* Interface descriptor */
      USB_INTERFACE_DESC_SIZE,        /* Descriptor size */
      USB_INTERFACE_DESC,             /* Descriptor type */
      0x01,                           /* Interface index */
      0x00,                           /* Alternate setting */
      CDC2_NUM_EP,                    /* Number of Endpoints */
      COMMUNICATION_DEVICE_CLASS_DATA,/* Class */
      0x00,                           /* SubClass */
      0x00,                           /* Protocol */
      0x00,                           /* Index of the Interface String Descriptor */

      /* First Endpoint IN */
      USB_ENDPOINT_DESC_SIZE,         /* Descriptor size */
      USB_ENDPOINT_DESC,              /* Descriptor type */
      CDC_ENDP_IN,                    /* Endpoint address */
      USB_DC_EP_BULK,                 /* Attributes */
      LOW_BYTE(CDC_BULK_EP_MPS),
      HIGH_BYTE(CDC_BULK_EP_MPS),     /* Max packet size */
      0x00,                           /* Interval */

      /* Second Endpoint OUT */
      USB_ENDPOINT_DESC_SIZE,         /* Descriptor size */
      USB_ENDPOINT_DESC,              /* Descriptor type */
      CDC_ENDP_OUT,                   /* Endpoint address */
      USB_DC_EP_BULK,                 /* Attributes */
      LOW_BYTE(CDC_BULK_EP_MPS),
      HIGH_BYTE(CDC_BULK_EP_MPS),     /* Max packet size */
      0x00,                           /* Interval */

      /* String descriptor language, only one, so min size 4 bytes.
       * 0x0409 English(US) language code used
       */
      USB_STRING_DESC_SIZE,           /* Descriptor size */
      USB_STRING_DESC,                /* Descriptor type */
      0x09,
      0x04,
      /* Manufacturer String Descriptor "Intel" */
      0x0C,
      USB_STRING_DESC,
      'I', 0, 'n', 0, 't', 0, 'e', 0, 'l', 0,
      /* Product String Descriptor "CDC-ACM" */
      0x10,
      USB_STRING_DESC,
      'C', 0, 'D', 0, 'C', 0, '-', 0, 'A', 0, 'C', 0, 'M', 0,
      /* Serial Number String Descriptor "00.01" */
      0x0C,
      USB_STRING_DESC,
      '0', 0, '0', 0, '.', 0, '0', 0, '1', 0,
   };

.. code-block:: c

   static struct usb_ep_cfg_data cdc_acm_ep_data[] = {
      {
         .ep_cb = cdc_acm_int_in,
         .ep_addr = CDC_ENDP_INT
      },
      {
         .ep_cb = cdc_acm_bulk_out,
         .ep_addr = CDC_ENDP_OUT
      },
      {
         .ep_cb = cdc_acm_bulk_in,
         .ep_addr = CDC_ENDP_IN
      }
   };

.. code-block:: c

   static struct usb_cfg_data cdc_acm_config = {
      .usb_device_description = cdc_acm_usb_description,
      .cb_usb_status = cdc_acm_dev_status_cb,
      .interface = {
      .class_handler = cdc_acm_class_handle_req,
      .custom_handler = NULL,
      .payload_data = NULL,
      },
      .num_endpoints = CDC1_NUM_EP + CDC2_NUM_EP,
      .endpoint = cdc_acm_ep_data
   };

.. code-block:: c

   ret = usb_set_config(&cdc_acm_config);
   if (ret < 0) {
      DBG("Failed to config USB\n");
      return ret;
   }

To enable the USB device for host/device connection:

.. code-block:: c

   ret = usb_enable(&cdc_acm_config);
   if (ret < 0) {
      DBG("Failed to enable USB\n");
      return ret;
   }

The class device requests are forwarded by the USB stack core driver to the
class driver through the registered class handler.
For the CDC ACM sample class driver, 'cdc_acm_class_handle_req' processes
the SET_LINE_CODING, CDC_SET_CONTROL_LINE_STATE and CDC_GET_LINE_CODING
class requests:

.. code-block:: c

   int cdc_acm_class_handle_req(struct usb_setup_packet *pSetup,
         int32_t *len, uint8_t **data)
   {
      struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(cdc_acm_dev);

      switch (pSetup->bRequest) {
      case CDC_SET_LINE_CODING:
         memcpy(&dev_data->line_coding, *data, sizeof(dev_data->line_coding));
         DBG("\nCDC_SET_LINE_CODING %d %d %d %d\n",
            sys_le32_to_cpu(dev_data->line_coding.dwDTERate),
            dev_data->line_coding.bCharFormat,
            dev_data->line_coding.bParityType,
            dev_data->line_coding.bDataBits);
      break;

      case CDC_SET_CONTROL_LINE_STATE:
         dev_data->line_state = (uint8_t)sys_le16_to_cpu(pSetup->wValue);
         DBG("CDC_SET_CONTROL_LINE_STATE 0x%x\n", dev_data->line_state);
            break;

      case CDC_GET_LINE_CODING:
         *data = (uint8_t *)(&dev_data->line_coding);
         *len = sizeof(dev_data->line_coding);
         DBG("\nCDC_GET_LINE_CODING %d %d %d %d\n",
         sys_le32_to_cpu(dev_data->line_coding.dwDTERate),
            dev_data->line_coding.bCharFormat,
            dev_data->line_coding.bParityType,
            dev_data->line_coding.bDataBits);
            break;

      default:
         DBG("CDC ACM request 0x%x, value 0x%x\n",
            pSetup->bRequest, pSetup->wValue);
            return -EINVAL;
      }

      return 0;
   }

The class driver should wait for the USB_DC_CONFIGURED device status code
before transmitting any data.

To transmit data to the host, the class driver should call usb_write().
Upon completion the registered endpoint callback will be called. Before
sending another packet the class driver should wait for the completion of
the previous transfer.

When data is received, the registered endpoint callback is called.
usb_read() should be used for retrieving the received data. It must
always be called through the registered endpoint callback. For CDC ACM
sample driver this happens via the OUT bulk endpoint handler (cdc_acm_bulk_out)
mentioned in the endpoint array (cdc_acm_ep_data).

Only CDC ACM and DFU class driver examples are provided for now.
