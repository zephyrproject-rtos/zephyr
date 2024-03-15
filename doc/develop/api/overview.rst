.. _api_overview:

API Overview
############

The table lists Zephyr's APIs and information about them, including their
current :ref:`stability level <api_lifecycle>`.  More details about API changes
between major releases are available in the :ref:`zephyr_release_notes`.

.. Keep this list sorted by the name of the API as it appears
   in the HTML, *NOT* the :ref: label

.. list-table::
   :header-rows: 1

   * - API
     - Status
     - Version Introduced

   * - :ref:`adc_api`
     - Stable
     - 1.0

   * - :ref:`audio_codec_api`
     - Experimental
     - 1.13

   * - :ref:`audio_dmic_api`
     - Experimental
     - 1.13

   * - :ref:`auxdisplay_api`
     - Experimental
     - 3.4

   * - :ref:`barriers_api`
     - Experimental
     - 3.4

   * - :ref:`blinfo_api`
     - Experimental
     - 3.5

   * - :ref:`bluetooth_api`
     - Stable
     - 1.0

   * - :ref:`clock_control_api`
     - Stable
     - 1.0

   * - :ref:`coap_sock_interface`
     - Unstable
     - 1.10

   * - :ref:`conn_mgr_docs`
     - Experimental
     - 3.4.0

   * - :ref:`can_api`
     - Stable
     - 1.14

   * - :ref:`can_transceiver_api`
     - Experimental
     - 3.1

   * - :ref:`charger_api`
     - Experimental
     - 3.5

   * - :ref:`counter_api`
     - Unstable
     - 1.14

   * - :ref:`crypto_api`
     - Stable
     - 1.7

   * - :ref:`dac_api`
     - Unstable
     - 2.3

   * - :ref:`dai_api`
     - Experimental
     - 3.1

   * - :ref:`dma_api`
     - Stable
     - 1.5

   * - :ref:`device_model_api`
     - Stable
     - 1.0

   * - :ref:`devicetree_api`
     - Stable
     - 2.2

   * - :ref:`disk_access_api`
     - Stable
     - 1.6

   * - :ref:`display_api`
     - Unstable
     - 1.14

   * - :ref:`ec_host_cmd_backend_api`
     - Experimental
     - 2.4

   * - :ref:`edac_api`
     - Unstable
     - 2.5

   * - :ref:`eeprom_api`
     - Stable
     - 2.1

   * - :ref:`entropy_api`
     - Stable
     - 1.10

   * - :ref:`file_system_api`
     - Stable
     - 1.5

   * - :ref:`flash_api`
     - Stable
     - 1.2

   * - :ref:`fcb_api`
     - Stable
     - 1.11

   * - :ref:`fuel_gauge_api`
     - Experimental
     - 3.3

   * - :ref:`flash_map_api`
     - Stable
     - 1.11

   * - :ref:`gnss_api`
     - Experimental
     - 3.6

   * - :ref:`gpio_api`
     - Stable
     - 1.0

   * - :ref:`hwinfo_api`
     - Stable
     - 1.14

   * - :ref:`i2c_eeprom_target_api`
     - Stable
     - 1.13

   * - :ref:`i2c_api`
     - Stable
     - 1.0

   * - :ref:`i2c-target-api`
     - Experimental
     - 1.12

   * - :ref:`i2s_api`
     - Stable
     - 1.9

   * - :ref:`i3c_api`
     - Experimental
     - 3.2

   * - :ref:`ieee802154_driver_api`
     - Unstable
     - 1.0

   * - :ref:`ieee802154_l2_api`
     - Unstable
     - 1.0

   * - :ref:`ieee802154_mgmt_api`
     - Unstable
     - 1.0

   * - :ref:`input`
     - Experimental
     - 3.4

   * - :ref:`ipm_api`
     - Stable
     - 1.0

   * - :ref:`kscan_api`
     - Stable
     - 2.1

   * - :ref:`kernel_api`
     - Stable
     - 1.0

   * - :ref:`led_api`
     - Stable
     - 1.12

   * - :ref:`lwm2m_interface`
     - Unstable
     - 1.9

   * - :ref:`llext`
     - Experimental
     - 3.5

   * - :ref:`logging_api`
     - Stable
     - 1.13

   * - :ref:`lora_api`
     - Experimental
     - 2.2

   * - :ref:`lorawan_api`
     - Experimental
     - 2.5

   * - :ref:`mbox_api`
     - Experimental
     - 1.0

   * - :ref:`mcu_mgr`
     - Stable
     - 1.11

   * - :ref:`modem`
     - Experimental
     - 3.5

   * - :ref:`mqtt_socket_interface`
     - Unstable
     - 1.14

   * - :ref:`mipi_dbi_api`
     - Experimental
     - 3.6

   * - :ref:`mipi_dsi_api`
     - Experimental
     - 3.1

   * - :ref:`misc_api`
     - Stable
     - 1.0

   * - :ref:`networking_api`
     - Stable
     - 1.0

   * - :ref:`nvs_api`
     - Stable
     - 1.12

   * - :ref:`peci_api`
     - Stable
     - 2.1

   * - :ref:`ps2_api`
     - Stable
     - 2.1

   * - :ref:`pwm_api`
     - Stable
     - 1.0

   * - :ref:`pinctrl_api`
     - Experimental
     - 3.0

   * - :ref:`pm_api`
     - Experimental
     - 1.2

   * - :ref:`random_api`
     - Stable
     - 1.0

   * - :ref:`regulator_api`
     - Experimental
     - 2.4

   * - :ref:`reset_api`
     - Experimental
     - 3.1

   * - :ref:`retained_mem_api`
     - Unstable
     - 3.4

   * - :ref:`retention_api`
     - Experimental
     - 3.4

   * - :ref:`rtc_api`
     - Experimental
     - 3.4

   * - :ref:`rtio_api`
     - Experimental
     - 3.2

   * - :ref:`smbus_api`
     - Experimental
     - 3.4

   * - :ref:`spi_api`
     - Stable
     - 1.0

   * - :ref:`sensor_api`
     - Stable
     - 1.2

   * - :ref:`settings_api`
     - Stable
     - 1.12

   * - :ref:`shell_api`
     - Stable
     - 1.14

   * - :ref:`stream_flash`
     - Experimental
     - 2.3

   * - :ref:`sdhc_api`
     - Experimental
     - 3.1

   * - :ref:`task_wdt_api`
     - Unstable
     - 2.5

   * - :ref:`tcpc_api`
     - Experimental
     - 3.1

   * - :ref:`tgpio_api`
     - Experimental
     - 3.5

   * - :ref:`uart_api`
     - Stable
     - 1.0

   * - :ref:`UART async <uart_api>`
     - Unstable
     - 1.14

   * - :ref:`usb_api`
     - Stable
     - 1.5

   * - :ref:`usbc_api`
     - Experimental
     - 3.3

   * - :ref:`usermode_api`
     - Stable
     - 1.11

   * - :ref:`usbc_vbus_api`
     - Experimental
     - 3.3

   * - :ref:`util_api`
     - Experimental
     - 2.4

   * - :ref:`video_api`
     - Stable
     - 2.1

   * - :ref:`w1_api`
     - Experimental
     - 3.2

   * - :ref:`watchdog_api`
     - Stable
     - 1.0

   * - :ref:`zdsp_api`
     - Experimental
     - 3.3
