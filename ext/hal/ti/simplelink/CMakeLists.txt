add_subdirectory(source/ti/devices)

if(CONFIG_SIMPLELINK_HOST_DRIVER)
  zephyr_include_directories(
    source
    kernel/zephyr/dpl
    )
  zephyr_compile_definitions(
    SL_SUPPORT_IPV6
    SL_PLATFORM_MULTI_THREADED
    )

  zephyr_library()
  zephyr_library_compile_definitions(${COMPILER})
  zephyr_library_sources(
    source/ti/drivers/SPI.c
    source/ti/drivers/spi/SPICC32XXDMA.c
    source/ti/drivers/dma/UDMACC32XX.c
    source/ti/drivers/power/PowerCC32XX.c
    source/ti/drivers/utils/List.c
    source/ti/drivers/net/wifi/source/device.c
    source/ti/drivers/net/wifi/source/flowcont.c
    source/ti/drivers/net/wifi/source/fs.c
    source/ti/drivers/net/wifi/source/netapp.c
    source/ti/drivers/net/wifi/source/netcfg.c
    source/ti/drivers/net/wifi/source/netutil.c
    source/ti/drivers/net/wifi/source/nonos.c
    source/ti/drivers/net/wifi/source/spawn.c
    source/ti/drivers/net/wifi/source/wlan.c
    source/ti/drivers/net/wifi/porting/CC3220SF_LAUNCHXL.c
    source/ti/drivers/net/wifi/porting/cc_pal.c
    source/ti/drivers/net/wifi/eventreg.c
    source/ti/drivers/net/wifi/source/socket.c

    source/ti/devices/cc32xx/driverlib/timer.c
    source/ti/devices/cc32xx/driverlib/udma.c

    kernel/zephyr/dpl/dpl.c
    kernel/zephyr/dpl/MutexP_zephyr.c
    kernel/zephyr/dpl/SemaphoreP_zephyr.c
    kernel/zephyr/dpl/ClockP_zephyr.c
    kernel/zephyr/dpl/HwiP_zephyr.c
    )


  # Create library for these two source files because they need to be
  # built slightly differently. TODO: Use set_source_files_properties
  # instead?
  zephyr_library_named(simplelink_wifi_driver)
  zephyr_library_sources(source/ti/drivers/net/wifi/source/driver.c)
  zephyr_library_compile_definitions(__LINUX_ERRNO_EXTENSIONS__)
  zephyr_library_compile_definitions(${COMPILER})
  zephyr_library_compile_options(-Wno-strict-aliasing) # driver.c warns on strict-aliasing

  zephyr_library_named(simplelink_wifi_sl_socket)
  zephyr_library_sources(source/ti/drivers/net/wifi/source/sl_socket.c)
  zephyr_library_compile_definitions(${COMPILER})
  zephyr_library_compile_options(-Wno-missing-braces) # sl_socket warns on missing braces
endif()
