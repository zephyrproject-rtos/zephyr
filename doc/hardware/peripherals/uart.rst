.. _uart_api:

Universal Asynchronous Receiver-Transmitter (UART)
##################################################

Overview
********

The UART is a common peripheral on microcontrollers and microprocessors. To
find UART nodes in your SoC, locate its corresponding device tree source file
(dts). For example, for the nordic nrf54l15 SoC:


.. code-block:: console

   zephyr/dts/vendor/nordic/nrf54l_05_10_15.dtsi


From here, we can identify some of the **UART** ports in our SoC, their
corresponding labels, and some of their properties. It is also important to
identify the binding where we can find the remaining properties. Keep in mind
that some properties change according to each SoC vendor, and others are
inherited from different bindings.

.. code-block:: dts

    uart20: uart@c6000 {
        compatible = "nordic,nrf-uarte";
        reg = <0xc6000 0x1000>;
        interrupts = <198 NRF_DEFAULT_IRQ_PRIORITY>;
        status = "disabled";
        endtx-stoptx-supported;
        frame-timeout-supported;
    };
    ...
      
    uart21: uart@c7000 {
        compatible = "nordic,nrf-uarte";
        reg = <0xc7000 0x1000>;
        interrupts = <199 NRF_DEFAULT_IRQ_PRIORITY>;
        status = "disabled";
        endtx-stoptx-supported;
        frame-timeout-supported;
    };
    ...

The peripheral configuration can be performed directly in the device tree file
of your board or in an overlay using the parameters defined in their respective
bindings. For example, to enable and configure the UART20 peripheral in the
nrf54l15 SoC.

.. code-block:: dts

    &uart20 {
        status = "okay";
        current-speed = <115200>;
        stop-bits = "1";
        data-bits = <8>;
        parity = "none";
        pinctrl-0 = <&uart20_default>;
        pinctrl-names = "default", "sleep";
    };


You can also change its configuration at runtime by using the
:c:func:`uart_configure` function. Just keep in mind that the previous method
supports vendor-specific options, while configuration done this way only
supports standard parameters.

.. code-block:: c

    /* Get the device descriptor for the uart 20 */
    const struct device *uart_dev = DEVICE_DT_GET( DT_NODELABEL( uart20 ) );
    ...

    /* Configuration structure */
    struct uart_config uart_cfg;
        
    uart_cfg.baudrate  = 115200;
    uart_cfg.data_bits = UART_CFG_DATA_BITS_8;
    uart_cfg.stop_bits = UART_CFG_STOP_BITS_1;
    uart_cfg.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
    uart_cfg.parity    = UART_CFG_PARITY_NONE;    
    /* Set UART configuration */
    uart_configure( uart_dev, &uart_cfg );

Zephyr provides three different ways to access the UART peripheral. Depending
on the method, different API functions are used according to below sections:

1. :ref:`uart_polling_api`
2. :ref:`uart_interrupt_api`
3. :ref:`uart_async_api` using :ref:`dma_api`

.. _uart_polling_api:

Polling mode
============

Polling is the most basic method to access the UART peripheral. The reading
function, :c:func:`uart_poll_in`, is a non-blocking function and returns a
character or ``-1`` when no valid data is available. The only downside of this
method is that you need to call the function periodically enough to avoid
losing data, parameter that can determine the periodicity are the baudrate
and the size of the microcontroller's internal buffer.

The writing function, :c:func:`uart_poll_out`, is a blocking function and the
thread waits until the given character is sent. If you need to send more than
one character, then you need to call it multiple times in a loop.

.. code-block:: c

    const char *msg = "Hello, UART!\n";
	
    for ( int i = 0; msg[i] != '\0'; i++ ) {
        uart_poll_out( uart_dev, msg[i] );
    }

.. _uart_interrupt_api:

Interrupt driven
================

With the Interrupt-driven API, possibly slow communication can happen in the
background while the thread continues with other tasks.

First you need to declare a callback function to be called during any
Receiver-Transmitter event, then setup the function in your driver using
:c:func:`uart_irq_callback_user_data_set` within the data to transmit or any
other data you want to pass to the callback.

.. code-block:: c

    /*declare the callback function to call from the ISR*/
    void uart_tx_callback( const struct device *dev, void* user_data )
    ...

    /*set data to transmit*/
    static uint8_t tx_buf[] = "Hello, UART Interrupt!\n";
    /*setup the callback function for tx events, and pass the data to transmit*/
    uart_irq_callback_user_data_set( uart_dev, uart_cb, (void*)tx_buf );
    /*enable tx interrupts, the ISR rutine will be triggered*/
    uart_irq_tx_enable( uart_dev );
    ...

Then, inside the callback function, you can place the logic to transmit the data,
the :c:func:`uart_fifo_fill` function. shall be called to place the data in the
microcontroller's buffer. This function accepts ``n`` number of bytes but will
only place in the microcontroller's buffer those for which it has capacity.
The function returns the number of bytes it was able to transmit, and you can
use this to know how many bytes remain to be placed in the buffer in the next
interrupt until the complete message has been transmitted.

.. code-block:: c

    void uart_tx_callback( const struct device *dev, void* user_data ) {
		
        /* acknowledge the interrupt */
        uart_irq_update(dev);
        ...

        if( count != 0 ){
            /* determine from which index we need to send the message*/
            index = message_len - count;
            /* send as many bytes as possible */
            sended = uart_fifo_fill( dev, &buffer[index], count );
            /* discount the actual bytes sended from the counter */
            count -= (uint8_t)sended;
        }
        ...

        if( count == 0 ){
            /* All bytes sent, disable TX interrupt */
            uart_irq_tx_disable(dev);
        }
    }

To receive data, you can use the :c:func:`uart_fifo_read` function inside the
callback function. This function reads as many bytes as are available in the
microcontroller's buffer, up to ``n`` bytes. It returns the number of bytes
actually read, which can be used to process the received data.

As same as with transmission you need call the
:c:func:`uart_irq_callback_user_data_set` as we did before, and enable Rx
interrupts using the :c:func:`uart_irq_tx_enable` function.

The Kernel's :ref:`kernel_data_passing_api` features can be used to communicate
between the thread and the UART driver, using for instance a queue. Or in some
cases you can also use the same buffer passed as user data to store the
received data.

.. code-block:: c

    void uart_rx_callback( const struct device *dev, void* user_data ) {
        uint8_t *rx_buf = user_data;
        int received;

        /* acknowledge the interrupt */	
        uart_irq_update(dev);

        /* read as many bytes as possible and place them into
        buffer pointed by rx_buf*/
        received = uart_fifo_read( dev, rx_buf, sizeof(rx_buf) );

        /* process the received data in someway */
        process_received_data( rx_buf, received );
        /* and/or send it to a queue to the main thread */
        k_msgq_put( &rx_msgq, rx_buf, K_NO_WAIT );
    }

Since there is only one callback we can set, you can use the functions
:c:func:`uart_irq_tx_ready` and :c:func:`uart_irq_rx_ready` to check which
event caused the interrupt and process each or both events accordingly.

.. code-block:: c

    void uart_irq_callback( const struct device *dev, void* user_data ) {
        /* acknowledge the interrupt */
        if(uart_irq_update(dev) && uart_irq_is_pending(dev)){
            if( uart_irq_tx_ready( dev ) ) {
                // Handle TX ready event
            }
            if( uart_irq_rx_ready( dev ) ) {
                // Handle RX ready event
            }
        }
    }

.. _uart_async_api:

Asynchronous API
================

The Asynchronous API allows to read and write data in the background using DMA
without interrupting the MCU at all. However, the setup is more complex
than the other methods.

To process data we need to use again a callback that will be called in an
interrupt context but with an extra parameters of type ``struct uart_event`` the
element ``type`` can be used to detect what causes the interrupts and to proceed
acordingly, such events can be found at **uart_event_type** enumeration.

.. code-block:: c

    static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
    {
        struct uart_rx *rx_buff = user_data;

        switch (evt->type) {
            case UART_TX_DONE:
                /*signal the message is completely sent*/
                ...
                k_sem_give( &tx_sem );
            break;
            case UART_TX_ABORTED:
                /*function uart_tx_abort was called*/
                ...
                k_sem_give( &tx_sem );
            break;
            case UART_RX_BUF_REQUEST:
                /*to avoid information overruns we switch between at least two buffers*/
                ...
                uart_rx_buf_rsp( dev, dma_buffers[index], sizeof(dma_buffers[0]) );
                index ^= 1;
            break;
            case UART_RX_RDY:
                /*process the data received, we got here because a timeout or
                the buffer is full*/
                ...
                process_received_data( evt->data.rx.buf[ evt->data.rx.offset ], evt->data.rx.len );
            case UART_RX_BUF_RELEASED:
                /*Buffer is no longer used by UART driver. */
                ...
            break;
            case UART_RX_DISABLED:
                /*uart has been disable and can be eneble again*/
                ...
                uart_rx_enable( dev );
            break;
            case UART_RX_STOPPED:
                /*for some reason receptiopn has stopped, could be an error*/
                ...
                process_stop_reason( evt->data.rx_stop.reason );
            break;
        }
    }

To transmit data we first as before set a callback function and then use the
:c:func:`uart_tx` with the buffer to transmit and the number of bytes to send,
we can also indicate a timeout if necessary (only applicable with flow control
enable) or ignore using the ``SYS_FOREVER_US`` constant

.. code-block:: c

    /* This is the function we are going to call durint the uart ISR */
    static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data);
    ...
    
    /* message to be transmitted */
    const uint8_t tx_buf[ ] = "Hola DMA uart Tx from DMA\r\n";
    /* set the callback function to be used on interrupts */
    uart_callback_set( uart_dev, uart_cb, NULL);
    
    /*initiate the transmission*/
    uart_tx( uart_dev, tx_buf, sizeof(tx_buf), SYS_FOREVER_US);
    ...

To receive data besides setting the callback, we need to set the buffer to place
the receive bytes using :c:func:`uart_rx_enable`, plus a timeout in us necesary,
most of the time we receive less data than the buffer size, and this parameter
determines how long the driver should wait after receiving the first byte before
triggering the interrupt.

.. code-block:: c

    /*declare a double buffer for data reception*/
    uint8_t dma_buffer[2][BUFFER_SIZE];
    ...

    /* set the callback function to be used on interrupts */
    uart_callback_set( uart_dev, uart_cb, dma_buffer );
    /*enable reception and set the buffer, and wait up to 10ms to enter 
    the callback since the first character have been received*/
    uart_rx_enable( uart_dev, dma_buffer, sizeof( dma_buffer ), 10000 );
    ...

Reception is always more complex than transmition, first thing to notice is the
double buffer we need to declare, on ``UART_RX_BUF_REQUEST`` event we need to
alternate to avoid overruns in the buffer currently in use.

While in event ``UART_RX_RDY`` we need keep in mind we get here for several
reasons the two most common is our buffer is full, meaning
``evt->data.rx.len == BUFFER_SIZE`` or a timeout
``evt->data.rx.len < BUFFER_SIZE``, more than 10ms has passed (on the previous
snippet) has passed. You can calculate this timeout based on the baudrate
and your application buffer size.

For reception of data the following events are triggered in the next order

1. ``UART_RX_BUF_REQUEST``.- after calling uart_rx_enable function
2. ``UART_RX_RDY``.- after a timeout or a buffer is full, if it was due the
   buffer is full and there is more data to process, the event will be
   triggered again until a timeout.
3. ``UART_RX_BUF_REQUEST``.- called again after the last UART_RX_RDY event

.. warning::

   Interrupt-driven API and the Asynchronous API should NOT be used at
   the same time for the same hardware peripheral, since both APIs require
   hardware interrupts to function properly. Using the callbacks for both
   APIs would result in interference between each other.
   :kconfig:option:`CONFIG_UART_EXCLUSIVE_API_CALLBACKS` is enabled by default
   so that only the callbacks associated with one API is active at a time.


One more thing, you can look for more examples on how to use the uart API in
the following zephyr directory:

.. code-block:: console

    zephyr/tests/drivers/uart


Configuration Options
*********************

Most importantly, the Kconfig options define whether the polling API (default),
the interrupt-driven API or the asynchronous API can be used. Only enable the
features you need in order to minimize memory footprint.

Related configuration options:

* :kconfig:option:`CONFIG_SERIAL`
* :kconfig:option:`CONFIG_UART_INTERRUPT_DRIVEN`
* :kconfig:option:`CONFIG_UART_ASYNC_API`
* :kconfig:option:`CONFIG_UART_WIDE_DATA`
* :kconfig:option:`CONFIG_UART_USE_RUNTIME_CONFIGURE`
* :kconfig:option:`CONFIG_UART_LINE_CTRL`
* :kconfig:option:`CONFIG_UART_DRV_CMD`


API Reference
*************

.. doxygengroup:: uart_interface
