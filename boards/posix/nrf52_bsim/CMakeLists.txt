# SPDX-License-Identifier: Apache-2.0

if (NOT DEFINED ENV{BSIM_COMPONENTS_PATH})
	message(FATAL_ERROR "This board requires the BabbleSim simulator. Please set\
 the  environment variable BSIM_COMPONENTS_PATH to point to its components \
 folder. More information can be found in\
 https://babblesim.github.io/folder_structure_and_env.html")
endif()
if (NOT DEFINED ENV{BSIM_OUT_PATH})
	message(FATAL_ERROR "This board requires the BabbleSim simulator. Please set\
 the environment variable BSIM_OUT_PATH to point to the folder where the\
 simulator is compiled to. More information can be found in\
 https://babblesim.github.io/folder_structure_and_env.html")
endif()

#Let's check that the HW models are the needed version or newer
#That is, that the needed tag is somewhere in the past of its branch
if(NOT DEFINED ENV{NO_NRF52_BSIM_VERSION_WARNING})
  if(GIT_FOUND) #boilerplate.cmake searches for git
    file(STRINGS "hw_models_version" NRF52_HW_MODELS_TAG) #desired version
    execute_process(COMMAND ${GIT_EXECUTABLE} merge-base --is-ancestor
      ${NRF52_HW_MODELS_TAG} HEAD
      WORKING_DIRECTORY $ENV{BSIM_COMPONENTS_PATH}/ext_NRF52_hw_models/
      OUTPUT_VARIABLE NRF52_HW_MODELS_TAG_FOUND
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE
      ERROR_VARIABLE stderr
      RESULT_VARIABLE return_code
    )
    if(return_code)
      message(WARNING "The NRF52 HW models are out of date\
 (${NRF52_HW_MODELS_TAG} needed at least); Please update them or expect\
 problems!\nReported error while trying to find the tag: \"${stderr}\"")

      #let's print the latest actual tag for the available models:
      execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags
        WORKING_DIRECTORY $ENV{BSIM_COMPONENTS_PATH}/ext_NRF52_hw_models/
        OUTPUT_VARIABLE NRF52_HW_MODELS_TAG_FOUND
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
        ERROR_VARIABLE stderr
        RESULT_VARIABLE return_code
      )
      message(STATUS "Found NRF52 models version ${NRF52_HW_MODELS_TAG_FOUND}")

      message(FATAL_ERROR "To disable this check set the environment variable\
 NO_NRF52_BSIM_VERSION_WARNING")

    elseif(CMAKE_VERBOSE_MAKEFILE)
      message(STATUS "nrf52_bsim: git merge-base --is-ancestor\
 ${NRF52_HW_MODELS_TAG} HEAD,
 stdout: ${NRF52_HW_MODELS_TAG_FOUND}
 stderr: ${stderr}")
    endif()
  endif()
endif()

zephyr_library()
zephyr_library_compile_definitions(NO_POSIX_CHEATS)

zephyr_include_directories(
  fake
  $ENV{BSIM_COMPONENTS_PATH}/ext_NRF52_hw_models/src/
  $ENV{BSIM_COMPONENTS_PATH}/ext_NRF52_hw_models/src/nrfx/hal/
  $ENV{BSIM_COMPONENTS_PATH}/ext_NRF52_hw_models/src/nrfx/
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

set(libpath $ENV{BSIM_OUT_PATH}/lib)
zephyr_library_import(bsim_libUtilv1      ${libpath}/libUtilv1.32.a)
zephyr_library_import(bsim_libPhyComv1    ${libpath}/libPhyComv1.32.a)
zephyr_library_import(bsim_lib2G4PhyComv1 ${libpath}/lib2G4PhyComv1.32.a)
zephyr_library_import(bsim_libRandv2      ${libpath}/libRandv2.32.a)
zephyr_library_import(bsim_libNRF52_hw    ${libpath}/libNRF52_hw_models.32.a)

# This is due to some tests using _Static_assert which is a 2011 feature, but
# otherwise relying on compilers supporting it also when set to C99.
# This was in general ok, but with some host compilers and C library versions
# it led to problems. So we override it to 2011 for native applications.
set_property(GLOBAL PROPERTY CSTD c11)
