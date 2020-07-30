# SPDX-License-Identifier: Apache-2.0

zephyr_library()

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

zephyr_library_include_directories(
  $ENV{BSIM_COMPONENTS_PATH}/libUtilv1/src/
  $ENV{BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  $ENV{BSIM_COMPONENTS_PATH}/libRandv2/src/
  $ENV{BSIM_COMPONENTS_PATH}/ext_2G4_libPhyComv1/src/
)

zephyr_library_sources(bs_radio.c
  					   bs_radio_argparse.c)

set(bsim_lib_path $ENV{BSIM_OUT_PATH}/lib)
zephyr_library_import(bsim_libUtilv1	  ${bsim_lib_path}/libUtilv1.32.a)
zephyr_library_import(bsim_libPhyComv1    ${bsim_lib_path}/libPhyComv1.32.a)
zephyr_library_import(bsim_lib2G4PhyComv1 ${bsim_lib_path}/lib2G4PhyComv1.32.a)
zephyr_library_import(bsim_libRandv2      ${bsim_lib_path}/libRandv2.32.a)
