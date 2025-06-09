# SPDX-License-Identifier: MIT
zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/rf.h)

zephyr_library()
add_subdirectory_ifdef(CONFIG_BEAMFORMER beamformer)
add_subdirectory_ifdef(CONFIG_BASEBAND baseband)
add_subdirectory_ifdef(CONFIG_CLOCK_SOURCE clk_source)

zephyr_library_sources_ifdef(CONFIG_XLNX_RFDC xlnx_rfdc.c)
zephyr_library_sources_ifdef(CONFIG_XLNX_RFDC murata_att_pe43712.c)



if (CONFIG_XLNX_RFDC) 
# # Add this section to find and include libmetal in your project
# set(LIBMETAL_DIR "$(CMAKE_CURRENT_SOURCE_DIR)/libmetal")

# include_directories(${LIBMETAL_DIR}/lib/system)
# include_directories(${LIBMETAL_DIR}/lib/include)

# add_library(libmetal STATIC IMPORTED)
# set_target_properties(libmetal PROPERTIES
#     IMPORTED_LOCATION ${LIBMETAL_DIR}/lib/libmetal.a
# )

# target_link_libraries(app PRIVATE libmetal)

endif()