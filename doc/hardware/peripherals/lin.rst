.. _lin:

Local Interconnect Network (LIN)
################################

Overview
********

LIN (Local Interconnect Network) is a serial bus system. Since 2016, it is standardized
internationally by ISO. In 2025, several parts of the ISO 17987 series have been updated.
The ISO 17987 document series covers the requirements of the seven OSI
(Open Systems Interconnection) layers and the corresponding conformance test plans.

LIN lower layers
================

The LIN data link layer, often called the LIN protocol, specifies a commander-responder protocol.
The LIN Commander uses one or more pre-programmed scheduling tables to start the sending and
receiving to the LIN frames. These scheduling tables contain at least the relative timing, when
the LIN frame sending is initiated. The LIN frame consists of two parts: header and response.
The LIN Commander sends the header, while either one dedicated LIN Responder or the LIN Commander
itself transmits the response.

The header comprises the 1-byte Break, the Inter-Byte Space, the 1-byte SYNC (5516),
the Identifier, and the Response Space. The responses consist of the payload-bytes and
the CRC byte.

Transmitted data within the LIN frame is transmitted serially as 8-bit data-bytes with
one additional start bit, one additional stop-bit, but no parity bit. Note that the break field in
the header has no start bit and no stop bit.

Bit rates may vary within the range of 1 kbit/s to 20 kbit/s. Bit values on the bus are recessive
(logical high) or dominant (logical low). The time normal is considered by the LIN Commanders
stable clock source, the smallest entity is one bit time (52 µs at a bitrate of 19,2 kbit/s).
Two bus states are defined: sleep mode and active mode. While data is on the bus, all LIN-nodes are
requested to be in active mode. After a specified timeout, the nodes enter the sleep mode and are
released back to active mode by a wake-up frame. This frame may be sent by any node requesting
activity on the bus, either the LIN Commander following its internal schedule, or one of the
attached LIN Responders being activated by its internal software application. After all nodes are
awakened, the LIN Commander continues to schedule the next LIN frame.

For more technical information on LIN you may visit
`LIN on Wikipedia <https://en.wikipedia.org/wiki/Local_Interconnect_Network>`_.

Zephyr LIN controller support following LIN features:

* Send and receive LIN frames in commander and responder mode.
* Filters with header masking (Responder mode), that allows a header ID to trigger responsder callback.
* Wake up pulse sending.

Sending
*******

The following code snippets demonstrates how to send a LIN frame:

This basic sample send a LIN frame from a commander node with the identifier 0x20 and a payload
of 4 bytes (0x01, 0x02, 0x03, 0x04).

.. code-block:: c

   uint8_t payload[4] = {0x01, 0x02, 0x03, 0x04};
   struct lin_msg msg = {
      .id = 0x20,
      .data = payload,
      .data_len = sizeof(payload),
      .checksum_type = LIN_CHECKSUM_CLASSIC
   };
   const struct device *lin_dev = DEVICE_DT_GET(DT_NODELABEL(lin0));
   int ret;

   ret = lin_send(lin_dev, &msg, K_FOREVER);
   if (ret < 0) {
      printk("Failed to send LIN frame: %d\n", ret);
   } else {
      printk("LIN frame sent successfully\n");
   }

From responder node, the LIN frame is sent in response to a header received from the commander.
When the header is received, the LIN controller will trigger a callback to the application,
which can then send the LIN frame as a response.

This basic sample send a LIN frame from a responder node with the identifier 0x20 and a payload of
4 bytes (0x01, 0x02, 0x03, 0x04).

.. code-block:: c

   struct k_poll_signal header_received_signal = K_POLL_SIGNAL_INITIALIZER(header_received_signal);
   struct k_poll_event evt = K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
                                                      K_POLL_MODE_NOTIFY_ONLY,
                                                      &header_received_signal);

   void responder_callback(const struct device *dev, const struct lin_event *event,
                           void *user_data)
   {
      switch (event->type) {
         case LIN_EVENT_HEADER_RECEIVED: {
            uint8_t id = lin_get_frame_id(event->header.pid);

            if (id == 0x20) {
               printk("Header received with PID: 0x%02X\n", event->header.pid);
               k_poll_signal(&header_received_signal);
            }
            break;
         }
         ...
         default:
            break;
      }
   }

   int main(void)
   {
      uint8_t payload[4] = {0x01, 0x02, 0x03, 0x04};
      struct lin_msg msg = {
         .id = 0x20,
         .data = payload,
         .data_len = sizeof(payload),
         .checksum_type = LIN_CHECKSUM_CLASSIC
      };
      const struct device *lin_dev = DEVICE_DT_GET(DT_NODELABEL(lin0));
      int ret;

      ret = lin_set_callback(lin_dev, responder_callback, NULL);
      if (ret < 0) {
         printk("Failed to set LIN callback: %d\n", ret);
      }

      while (1) {
         /* Wait for header received signal from the callback */
         ret = k_poll(&evt, 1, K_FOREVER);
         k_poll_signal_reset(&header_received_signal);

         /* Send response frame */
         ret = lin_send(lin_dev, &msg, K_FOREVER);
         if (ret < 0) {
            printk("Failed to send LIN frame: %d\n", ret);
         } else {
            printk("LIN frame sent successfully\n");
         }
      }
   }

Receiving
*********

The following code snippets demonstrates how to receive a LIN frame:

This basic sample use a commander node to trigger a response from a responder node with
the frame identifier 0x20 and a payload of 4 bytes (0x01, 0x02, 0x03, 0x04).

.. code-block:: c

   struct k_poll_signal response_received_signal = K_POLL_SIGNAL_INITIALIZER(response_received_signal);
   struct k_poll_event evt = K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
                                                      K_POLL_MODE_NOTIFY_ONLY,
                                                      &response_received_signal);

   void commander_callback(const struct device *dev, const struct lin_event *event,
                           void *user_data)
   {
      switch (event->type) {
         case LIN_EVT_RX_DATA: {
            uint8_t id = lin_get_frame_id(event->data.pid);

            if (id == 0x20) {
               printk("Response received with PID: 0x%02X\n", event->data.pid);
            }
            break;
         }
         ...
         default:
            break;
      }
   }

   int main(void)
   {
      const struct device *lin_dev = DEVICE_DT_GET(DT_NODELABEL(lin0));
      uint8_t payload[4];
      struct lin_msg msg = {
         .id = 0x20,
         .data = payload,
         .data_len = sizeof(payload),
         .checksum_type = LIN_CHECKSUM_CLASSIC
      };
      int ret;

      /* Do any configuration set and start the LIN bus here */

      /* Register event handler */
      ret = lin_set_callback(lin_dev, commander_callback, NULL);
      if (ret < 0) {
         printk("Failed to set LIN callback: %d\n", ret);
      }

      /* Send the header and trigger the response */
      ret = lin_receive(lin_dev, &msg, K_FOREVER);
      if (ret < 0) {
         printk("Failed to receive LIN frame: %d\n", ret);
      }

      /* Wait for response received signal from the callback */
      ret = k_poll(&evt, 1, K_FOREVER);
      k_poll_signal_reset(&response_received_signal);

      /* Process the received response frame as needed */
   }

This basic sample use a responder node to receive a payload of 4 bytes (0x01, 0x02, 0x03, 0x04)
from a commander node with the frame identifier 0x20.

.. code-block:: c

   struct k_poll_signal header_received_signal = K_POLL_SIGNAL_INITIALIZER(header_received_signal);
   struct k_poll_signal data_received_signal = K_POLL_SIGNAL_INITIALIZER(data_received_signal);
   struct k_poll_event evt[] = {
      K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &header_received_signal),
      K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &data_received_signal)
   };

   void responder_callback(const struct device *dev, const struct lin_event *event,
                           void *user_data)
   {
      switch (event->type) {
         case LIN_VT_RX_HEADER: {
            uint8_t id = lin_get_frame_id(event->header.pid);

            if (id == 0x20) {
               printk("Header received with PID: 0x%02X\n", event->header.pid);
               k_poll_signal(&header_received_signal);
            }
            break;
         }
         case LIN_EVT_RX_DATA: {
            uint8_t id = lin_get_frame_id(event->data.pid);

            if (id == 0x20) {
               k_poll_signal(&data_received_signal);
               printk("Response received with PID: 0x%02X\n", event->data.pid);
            }
            break;
         }
         ...
         default:
            break;
      }
   }

   int main(void){
      const struct device *lin_dev = DEVICE_DT_GET(DT_NODELABEL(lin0));
      unsigned int header_poll_signaled, data_poll_signaled;
      int header_received = 0, data_received = 0;
      uint8_t payload[4];
      struct lin_msg msg = {
         .id = 0x20,
         .data = payload,
         .data_len = sizeof(payload),
         .checksum_type = LIN_CHECKSUM_CLASSIC
      };
      int ret;

      /* Do any configuration set and start the LIN bus here */

      /* Register event handler */
      ret = lin_set_callback(lin_dev, responder_callback, NULL);
      if (ret < 0) {
         printk("Failed to set LIN callback: %d\n", ret);
      }

      /* Wait for header received signal from the callback */
      k_poll(evt, ARRAY_SIZE(evt), K_FOREVER);
      if (evt[0].state == K_POLL_STATE_SIGNALED) {
         k_poll_signal_reset(&header_received_signal);
      }

      /* Trigger data reception */
      ret = lin_receive(lin_dev, &msg, K_FOREVER);
      if (ret < 0) {
         printk("Failed to receive LIN frame: %d\n", ret);
      }

      /* Wait for data received signal from the callback */
      k_poll(evt, ARRAY_SIZE(evt), K_FOREVER);
      if (evt[1].state == K_POLL_STATE_SIGNALED) {
         k_poll_signal_reset(&data_received_signal);
      }

      /* Process the received header and data frame as needed */
   }

API Reference
*************

.. doxygengroup:: lin_controller
