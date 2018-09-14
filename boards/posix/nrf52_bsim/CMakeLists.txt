if (NOT DEFINED ENV{BSIM_COMPONENTS_PATH})
	message(FATAL_ERROR "This board requires the BabbleSim simulator. Please set\
 the  enviroment variable BSIM_COMPONENTS_PATH to point to its components \
 folder. More information can be found in\
 https://babblesim.github.io/folder_structure_and_env.html")
endif()
if (NOT DEFINED ENV{BSIM_OUT_PATH})
	message(FATAL_ERROR "This board requires the BabbleSim simulator. Please set\
 the enviroment variable BSIM_OUT_PATH to point to the folder where the\
 simulator is compiled to. More information can be found in\
 https://babblesim.github.io/folder_structure_and_env.html")
endif()

zephyr_library()
zephyr_library_compile_definitions(NO_POSIX_CHEATS)

zephyr_include_directories(
  fake
  $ENV{BSIM_COMPONENTS_PATH}/ext_NRF52_hw_models/src/nrfx_hal/
  $ENV{BSIM_COMPONENTS_PATH}/ext_NRF52_hw_models/src/HW_models/
)

#Due to the BLE controller assumption about enum size
zephyr_compile_options(
  -fshort-enums
)

zephyr_library_sources(
	irq_handler.c
	k_busy_wait.c
	bstests_entry.c
	argparse.c
	main.c
	time_machine.c
	trace_hook.c
)

zephyr_library_include_directories(
  fake
  $ENV{BSIM_COMPONENTS_PATH}/libUtilv1/src/
  $ENV{BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  $ENV{BSIM_COMPONENTS_PATH}/ext_NRF52_hw_models/src/HW_models/
  $ENV{BSIM_COMPONENTS_PATH}/ext_NRF52_hw_models/src/nrfx_hal/
  $ENV{BSIM_COMPONENTS_PATH}/libRandv2/src/
)

zephyr_ld_options(
  -lm
)

add_library(bsim_libUtilv1      STATIC IMPORTED GLOBAL)
add_library(bsim_libPhyComv1    STATIC IMPORTED GLOBAL)
add_library(bsim_lib2G4PhyComv1 STATIC IMPORTED GLOBAL)
add_library(bsim_libRandv2      STATIC IMPORTED GLOBAL)
add_library(bsim_libNRF52_hw    STATIC IMPORTED GLOBAL)
set_target_properties(bsim_libUtilv1      PROPERTIES IMPORTED_LOCATION $ENV{BSIM_OUT_PATH}/lib/libUtilv1.32.a)
set_target_properties(bsim_libPhyComv1    PROPERTIES IMPORTED_LOCATION $ENV{BSIM_OUT_PATH}/lib/libPhyComv1.32.a)
set_target_properties(bsim_lib2G4PhyComv1 PROPERTIES IMPORTED_LOCATION $ENV{BSIM_OUT_PATH}/lib/lib2G4PhyComv1.32.a)
set_target_properties(bsim_libRandv2      PROPERTIES IMPORTED_LOCATION $ENV{BSIM_OUT_PATH}/lib/libRandv2.32.a)
set_target_properties(bsim_libNRF52_hw    PROPERTIES IMPORTED_LOCATION $ENV{BSIM_OUT_PATH}/lib/libNRF52_hw_models.32.a)
zephyr_append_cmake_library(bsim_libUtilv1)
zephyr_append_cmake_library(bsim_libPhyComv1)
zephyr_append_cmake_library(bsim_lib2G4PhyComv1)
zephyr_append_cmake_library(bsim_libRandv2)
zephyr_append_cmake_library(bsim_libNRF52_hw)

# This is due to some tests using _Static_assert which is a 2011 feature, but
# otherwise relying on compilers supporting it also when set to C99.
# This was in general ok, but with some host compilers and C library versions
# it led to problems. So we override it to 2011 for native applications.
set_property(GLOBAL PROPERTY CSTD c11)
