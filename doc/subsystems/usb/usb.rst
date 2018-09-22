.. _usb_device_stack:

USB device stack
################

The USB device stack is split into three layers:
   * `USB device controller drivers`_ (hardware dependent)
   * `USB device core layer`_ (hardware independent)
   * `USB device class drivers`_ (hardware independent)

USB Vendor and Product identifiers
**********************************

The USB Vendor ID for the Zephyr project is 0x2FE3. The default USB Product
ID for the Zephyr project is 0x100. The USB bcdDevice Device Release Number
represents the Zephyr kernel major and minor versions as a binary coded
decimal value. When a vendor integrates the Zephyr USB subsystem into a
product, the vendor must use the USB Vendor and Product ID assigned to them.
A vendor integrating the Zephyr USB subsystem in a product must not use the
Vendor ID of the Zephyr project.

The USB maintainer, if one is assigned, or otherwise the Zephyr Technical
Steering Committee, may allocate other USB Product IDs based on well-motivated
and documented requests.

USB device controller drivers
*****************************

The Device Controller Driver Layer implements the low level control routines
to deal directly with the hardware. All device controller drivers should
implement the APIs described in file usb_dc.h. This allows the integration of
new USB device controllers to be done without changing the upper layers.

.. _usb_device_controller_api:

USB Device Controller API
=========================
.. doxygengroup:: _usb_device_controller_api
   :project: Zephyr

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

USB Device Core Layer API
=========================
.. doxygengroup:: _usb_device_core_api
   :project: Zephyr

USB device class drivers
************************

To initialize the device class driver instance the USB device class driver
should call usb_set_config() passing as parameter the instance's configuration
structure.

For example, for CDC_ACM sample application:

.. code-block:: c

   static const u8_t cdc_acm_usb_description[] = {
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
         s32_t *len, u8_t **data)
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
         dev_data->line_state = (u8_t)sys_le16_to_cpu(pSetup->wValue);
         DBG("CDC_SET_CONTROL_LINE_STATE 0x%x\n", dev_data->line_state);
            break;

      case CDC_GET_LINE_CODING:
         *data = (u8_t *)(&dev_data->line_coding);
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

The class driver should wait for the USB_DC_INTERFACE device status code
before transmitting any data.

There are two ways to transmit data, using the 'low' level read/write API or
the 'high' level transfer API.

low level API:

To transmit data to the host, the class driver should call usb_write().
Upon completion the registered endpoint callback will be called. Before
sending another packet the class driver should wait for the completion of
the previous write. When data is received, the registered endpoint callback
is called. usb_read() should be used for retrieving the received data.
For CDC ACM sample driver this happens via the OUT bulk endpoint handler
(cdc_acm_bulk_out) mentioned in the endpoint array (cdc_acm_ep_data).

high level API:

The usb_transfer method can be used to transfer data to/from the host. The
transfer API will automatically split the data transmission into one or more
USB transaction(s), depending endpoint max packet size. The class driver does
not have to implement endpoint callback and should set this callback to the
generic usb_transfer_ep_callback.


