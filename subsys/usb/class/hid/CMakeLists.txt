target_sources(subsys_usb PRIVATE ${CMAKE_CURRENT_LIST_DIR}/core.c)
target_include_directories(subsys_usb PRIVATE ${ZEPHYR_BASE}/include/usb)
