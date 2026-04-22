# Copyright (c) 2025 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_STM32_USB_COMMON)
  zephyr_library()

  zephyr_include_directories(.)

  zephyr_library_sources(stm32_usb_pwr.c)

  # PHY interface drivers
  #
  # These drivers don't control a particular PHY; instead,
  # they merely configure clocks/etc such that a specific
  # PHY interface can be used by the USB controller.
  #
  # These drivers must be added to all builds where they
  # MIGHT be useful since there is no (easy) way to detect
  # whether or not they are required; however, the linker
  # should ultimately discard their code if not needed.
  # The drivers are written carefully such that they don't
  # trigger build errors if built when not actually needed.
  if(CONFIG_STM32_HAS_EMBEDDED_FS_ULPI_OFF_QUIRK)
    zephyr_library_sources(phy_embeddedfs_ulpi_off.c)
  endif()
  zephyr_library_sources_ifdef(CONFIG_STM32_PHY_EXTERNAL_ULPI phy_ulpi_itf.c)

  # Embedded HS PHY drivers
  if(CONFIG_STM32_PHY_USBPHYC)
    zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_STM32F7X phy_usbphyc_f7.c)
    zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_STM32N6X phy_usbphyc_n6.c)
  endif()
  zephyr_library_sources_ifdef(CONFIG_STM32_PHY_U5OTGHSPHY phy_u5otghs.c)
endif()
