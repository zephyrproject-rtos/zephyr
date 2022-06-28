.. _api_overview:

API Overview
############

The table lists Zephyr's APIs and information about them, including their
current :ref:`stability level <api_lifecycle>`.

.. Keep this list sorted by the name of the API as it appears
   in the HTML, *NOT* the :ref: label

.. list-table::
   :header-rows: 1

   * - API
     - Status
     - Version Introduced
     - Version Modified

   * - :ref:`adc_api`
     - Stable
     - 1.0
     - 3.1

   * - :ref:`audio_codec_api`
     - Experimental
     - 1.13
     - 1.13

   * - :ref:`audio_dmic_api`
     - Experimental
     - 1.13
     - 1.13

   * - :ref:`bluetooth_api`
     - Stable
     - 1.0
     - 3.1

   * - :ref:`clock_control_api`
     - Stable
     - 1.0
     - 3.1

   * - :ref:`coap_sock_interface`
     - Unstable
     - 1.10
     - 2.4

   * - :ref:`can_api`
     - Unstable
     - 1.14
     - 3.1

   * - :ref:`counter_api`
     - Unstable
     - 1.14
     - 2.6

   * - :ref:`crypto_api`
     - Stable
     - 1.7
     - 3.1

   * - :ref:`dac_api`
     - Experimental
     - 2.3
     - 2.3

   * - :ref:`dai_api`
     - Experimental
     - 3.1
     - 3.1

   * - :ref:`dma_api`
     - Stable
     - 1.5
     - 3.1

   * - :ref:`device_model_api`
     - Stable
     - 1.0
     - 3.1

   * - :ref:`devicetree_api`
     - Stable
     - 2.2
     - 3.1

   * - :ref:`disk_access_api`
     - Stable
     - 1.6
     - 3.1

   * - :ref:`display_api`
     - Unstable
     - 1.14
     - 2.2

   * - :ref:`ec_host_cmd_periph_api`
     - Experimental
     - 2.4
     - 2.4

   * - :ref:`edac_api`
     - Experimental
     - 2.5
     - 3.1

   * - :ref:`eeprom_api`
     - Stable
     - 2.1
     - 2.1

   * - :ref:`entropy_api`
     - Stable
     - 1.10
     - 1.12

   * - :ref:`file_system_api`
     - Stable
     - 1.5
     - 2.4

   * - :ref:`flash_api`
     - Stable
     - 1.2
     - 3.1

   * - :ref:`fcb_api`
     - Stable
     - 1.11
     - 2.1

   * - :ref:`flash_map_api`
     - Stable
     - 1.11
     - 2.6

   * - :ref:`gna_api`
     - Experimental
     - 1.14
     - 1.14

   * - :ref:`gpio_api`
     - Stable
     - 1.0
     - 3.1

   * - :ref:`hwinfo_api`
     - Stable
     - 1.14
     - 3.1

   * - :ref:`i2c_eeprom_target_api`
     - Stable
     - 1.13
     - 1.13

   * - :ref:`i2c_api`
     - Stable
     - 1.0
     - 3.2

   * - :ref:`i2c-target-api`
     - Experimental
     - 1.12
     - 3.2

   * - :ref:`i2s_api`
     - Stable
     - 1.9
     - 2.6

   * - :ref:`ipm_api`
     - Stable
     - 1.0
     - 3.1

   * - :ref:`kscan_api`
     - Stable
     - 2.1
     - 2.6

   * - :ref:`kernel_api`
     - Stable
     - 1.0
     - 3.1

   * - :ref:`led_api`
     - Stable
     - 1.12
     - 2.6

   * - :ref:`lwm2m_interface`
     - Unstable
     - 1.9
     - 3.1

   * - :ref:`logging_api`
     - Stable
     - 1.13
     - 3.1

   * - :ref:`lora_api`
     - Experimental
     - 2.2
     - 2.2

   * - :ref:`lorawan_api`
     - Experimental
     - 2.5
     - 3.1

   * - :ref:`mbox_api`
     - Experimental
     - 1.0
     - 3.1

   * - :ref:`mqtt_socket_interface`
     - Unstable
     - 1.14
     - 2.4

   * - :ref:`mipi_dsi_api`
     - Experimental
     - 3.1
     - 3.1

   * - :ref:`misc_api`
     - Stable
     - 1.0
     - 2.2

   * - :ref:`networking_api`
     - Stable
     - 1.0
     - 3.1

   * - :ref:`nvs_api`
     - Stable
     - 1.12
     - 3.1

   * - :ref:`peci_api`
     - Stable
     - 2.1
     - 2.6

   * - :ref:`ps2_api`
     - Stable
     - 2.1
     - 2.6

   * - :ref:`pwm_api`
     - Stable
     - 1.0
     - 3.1

   * - :ref:`pinctrl_api`
     - Experimental
     - 3.0
     - 3.1

   * - :ref:`pinmux_api`
     - Stable
     - 1.0
     - 1.11

   * - :ref:`pm_api`
     - Experimental
     - 1.2
     - 3.1

   * - :ref:`random_api`
     - Stable
     - 1.0
     - 2.1

   * - :ref:`regulator_api`
     - Experimental
     - 2.4
     - 2.4

   * - :ref:`reset_api`
     - Experimental
     - 3.1
     - 3.1

   * - :ref:`rtio_api`
     - Experimental
     - 3.2
     - 3.2

   * - :ref:`spi_api`
     - Stable
     - 1.0
     - 3.1

   * - :ref:`sensor_api`
     - Stable
     - 1.2
     - 2.6

   * - :ref:`settings_api`
     - Stable
     - 1.12
     - 2.1

   * - :ref:`shell_api`
     - Stable
     - 1.14
     - 3.1

   * - :ref:`stream_flash`
     - Experimental
     - 2.3
     - 2.3

   * - :ref:`sdhc_api`
     - Experimental
     - 3.1
     - 3.1

   * - :ref:`task_wdt_api`
     - Experimental
     - 2.5
     - 2.5

   * - :ref:`tcpc_api`
     - Experimental
     - 3.1
     - 3.1

   * - :ref:`uart_api`
     - Stable
     - 1.0
     - 3.1

   * - :ref:`UART async <uart_api>`
     - Unstable
     - 1.14
     - 2.2

   * - :ref:`usb_api`
     - Stable
     - 1.5
     - 3.1

   * - :ref:`usermode_api`
     - Stable
     - 1.11
     - 1.11

   * - :ref:`util_api`
     - Experimental
     - 2.4
     - 3.1

   * - :ref:`video_api`
     - Stable
     - 2.1
     - 2.6

   * - :ref:`watchdog_api`
     - Stable
     - 1.0
     - 2.0
