.. _rfid_api:

RFID API
########

Overview
********

The RFID Subsystem provides APIs for RFID protocol standards.
Currently, only ISO 14443A is implemented. The protocol utilizes the RFID driver API.

Example (polling)
*****************

To read the UID of an RFID tag, the following steps are necessary:

1. Configure the device in the Device Tree
==========================================

The following example shows the DTS entry of an STM32L100 board with a CR95HF connected via SPI.

.. code-block:: dts

	&spi1 {
		pinctrl-0 = <&spi1_sck_pa5 &spi1_miso_pa6 &spi1_mosi_pa7>;
		pinctrl-names = "default";
		status = "okay";
		#address-cells = <1>;
		#size-cells = <0>;
		cs-gpios = <&gpioa 4 GPIO_ACTIVE_LOW>;

		cr95hf: spi-device@0 {
			reg = <0>;
			spi-max-frequency = <50000>;
			compatible = "st,cr95hf";
			status = "okay";
			irq-in-gpios = <&gpioa 2 GPIO_ACTIVE_LOW>;
			irq-out-gpios = <&gpioa 3 GPIO_ACTIVE_LOW>;
		};

		dmas = <&dma1 3 (STM32_DMA_PERIPH_TX | STM32_DMA_PRIORITY_HIGH)>,
			<&dma1 2 (STM32_DMA_PERIPH_RX | STM32_DMA_PRIORITY_HIGH)>;
		dma-names = "tx", "rx";
	};

2. Get the configuration
========================

.. code-block:: c

	#define RFID_NODE DT_NODELABEL(cr95hf)
	static const struct device *rfid_dev = DEVICE_DT_GET(RFID_NODE);


3. Load the protocol using the RFID Device API
==============================================

.. code-block:: c

	rfid_load_protocol(rfid_dev, RFID_PROTO_ISO14443A,
		RFID_MODE_INITIATOR | RFID_MODE_TX_106 | RFID_MODE_RX_106);


4. Request ATQA and single device detection using the ISO 14443A API
====================================================================

If there is a device, ``rfid_iso14443a_request()`` will return 0; otherwise, it returns -EPROTO.
Therefore, you can call ``rfid_iso14443a_sdd()`` if the return value of ``rfid_iso14443a_request()``
was 0. Furthermore, if the return value from ``rfid_iso14443a_sdd()`` is 0,
you can print out the UID. The following example demonstrates this in a while loop:

.. code-block:: c

	while (1) {
		memset(&info, 0, sizeof(info));
		if (rfid_iso14443a_request(rfid_dev, info.atqa, true) == 0) {
			if (rfid_iso14443a_sdd(rfid_dev, (struct rfid_iso14443a_info *)&info) == 0) {
				printk("UID: ");
				for (int i = 0; i < info.uid_len; i++) {
					printk("%X", info.uid[i]);
				}
			}
		}
	}

Example (sleep mode)
********************

If the RFID device support sleep mode you can use ``rfid_set_properties()`` to set the RFID device
into sleep mode and wait for a tag. Here is an example for a CR95HF:

.. code-block:: c

	static void rfid_main(void *p1, void *p2, void *p3)
	{
		struct rfid_property props[1];

		props[0].type = RFID_PROP_SLEEP;
		/* refid_set_properties shall block until a tag is detected*/
		props[0].timeout_us = UINT32_MAX;

		props[1].type = RFID_PROP_RESET;


		while(1) {
			rfid_set_properties(rfid_dev, &props[0], 1);

			if(props[0].status != 0)
			{
				/** Workaround for CR95HF:
				* If the Tag is removed while rfid_iso14443a_sdd is not finished, the
				* CR95HF seems to be in an internal state where Sleep Mode is not possible
				* any longer. To leave that state, you have to re-represent the tag and
				* wait for the end of communication or you have to reset the device.
				*/
				rfid_set_properties(rfid_dev, &props[1], 1);
				continue;
			}

			rfid_load_protocol(rfid_dev, RFID_PROTO_ISO14443A,
				RFID_MODE_INITIATOR | RFID_MODE_TX_106 | RFID_MODE_RX_106);

			memset(&info, 0, sizeof(info));

			if(rfid_iso14443a_request(rfid_dev,info.atqa,true) == 0) {
				if(rfid_iso14443a_sdd(rfid_dev,(struct rfid_iso14443a_info *)&info) == 0) {
					printk("UID: ");
					for(int i = 0; i < info.uid_len; i++)
					{
						printk("%X",info.uid[i]);
					}
				}
			}

		}
	}

API Reference
*************

.. doxygengroup:: rfid_interface

.. doxygengroup:: rfid_iso14443a_interface
