#
# Copyright (c) 2025 Nuvoton Technology Corporation.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
  scfg.c
  soc_common.c
)

set(NCT_BIN_NAME ${CONFIG_KERNEL_BIN_NAME}_signed.bin)
string(TOUPPER "${SOC_NAME}" soc_name_upper)

if(CONFIG_ESIO_EXT_SIGN)
  # 1. "NuveSIOBinGenTool" is called in bat, and exection tool and bat are in project folder
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${CMAKE_COMMAND} -E env cmd /c ${CMAKE_SOURCE_DIR}/esio_signing.bat ${KERNEL_BIN_NAME} ${NCT_BIN_NAME})
else()
   # 2. Signed tool is called "esiost.py" in zephyr kernel.
   set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
     COMMAND ${PYTHON_EXECUTABLE} ${SOC_${soc_name_upper}_DIR}/common/esiost/esiost.py
     -i ${KERNEL_BIN_NAME}
     -o ${NCT_BIN_NAME}
     -v
     )  
 endif()

# 3. "NuveSIOBinGenTool" in zephyr/soc/nuvoton/nct/common/esiost 

# set(INPUT_DIR ${SOC_${soc_name_upper}_DIR}/common/esiost/NuveSIOBinGenTool/Input)
# set(OUTPUT_DIR ${SOC_${soc_name_upper}_DIR}/common/esiost/NuveSIOBinGenTool/Output)

# set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
# COMMAND ${CMAKE_COMMAND} -E copy
# ${PROJECT_BINARY_DIR}/${KERNEL_BIN_NAME}
# ${INPUT_DIR}/${KERNEL_BIN_NAME})

# set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
# COMMAND ${SOC_${soc_name_upper}_DIR}/common/esiost/NuveSIOBinGenTool/NuveSIOBinGenTool.exe
# /path ${SOC_${soc_name_upper}_DIR}/common/esiost/NuveSIOBinGenTool/  
# /infile ${KERNEL_BIN_NAME} 
# /outfile ${NCT_BIN_NAME} 
# /chip nct6694 
# ${SOC_${soc_name_upper}_DIR}/common/esiost/NuveSIOBinGenTool/Xml/_NTC_Internal.xml)

# set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
# COMMAND ${CMAKE_COMMAND} -E copy
# ${OUTPUT_DIR}/${NCT_BIN_NAME}
# ${PROJECT_BINARY_DIR}/${NCT_BIN_NAME})