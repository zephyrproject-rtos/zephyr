zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_USB_DW}        usb_dc_dw.c
            IFDEF:${CONFIG_USB_DC_STM32}  usb_dc_stm32.c
            IFDEF:${CONFIG_USB_DC_SAM0}   usb_dc_sam0.c
            IFDEF:${CONFIG_USB_DC_SAM}    usb_dc_sam.c
            IFDEF:${CONFIG_USB_NRF52840}  usb_dc_nrfx.c
            IFDEF:${CONFIG_USB_KINETIS}   usb_dc_kinetis.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
