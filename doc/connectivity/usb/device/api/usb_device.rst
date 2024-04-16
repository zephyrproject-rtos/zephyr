.. _usb_device_stack_api:

USB device stack API
####################

API reference
*************

There are two ways to transmit data, using the 'low' level read/write API or
the 'high' level transfer API.

Low level API
  To transmit data to the host, the class driver should call usb_write().
  Upon completion the registered endpoint callback will be called. Before
  sending another packet the class driver should wait for the completion of
  the previous write. When data is received, the registered endpoint callback
  is called. usb_read() should be used for retrieving the received data.
  For CDC ACM sample driver this happens via the OUT bulk endpoint handler
  (cdc_acm_bulk_out) mentioned in the endpoint array (cdc_acm_ep_data).

High level API
  The usb_transfer method can be used to transfer data to/from the host. The
  transfer API will automatically split the data transmission into one or more
  USB transaction(s), depending endpoint max packet size. The class driver does
  not have to implement endpoint callback and should set this callback to the
  generic usb_transfer_ep_callback.

.. doxygengroup:: _usb_device_core_api
