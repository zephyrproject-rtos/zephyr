# Copyright (c) 2022 The Chromium OS Authors
# SPDX-License-Identifier: Apache-2.0

# Generates VIF policies in XML format from zephyr.dts.pre file.

set(GEN_VIF_SCRIPT ${ZEPHYR_BASE}/scripts/utils/generate_vif/generate_vif.py)
set(ZEPHYR_DTS_PRE ${PROJECT_BINARY_DIR}/zephyr.dts.pre)
set(DTS_BINDINGS ${ZEPHYR_BASE}/dts/bindings)
set(DTS_COMPATIBLE usb-c-connector)
set(VIF_XML ${PROJECT_BINARY_DIR}/vif.xml)

message(STATUS "executing gen vif, reading file from loc : ${ZEPHYR_DTS_PRE}")

# Executes generate_vif.py to create vif.xml.

set(CMD_GEN_VIF ${PYTHON_EXECUTABLE} ${GEN_VIF_SCRIPT}
        --dts ${ZEPHYR_DTS_PRE}
        --vif-out ${VIF_XML}.new
        --bindings-dirs ${DTS_BINDINGS}
        --compatible ${DTS_COMPATIBLE}
        --board ${BOARD}
        )

execute_process(
        COMMAND ${CMD_GEN_VIF}
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        RESULT_VARIABLE ret
)

if (NOT "${ret}" STREQUAL "0")
    message(STATUS "In: ${PROJECT_BINARY_DIR}, command: ${CMD_GEN_VIF}")
    message(FATAL_ERROR "generate_vif.py failed with return code: ${ret}")
else ()
    zephyr_file_copy(${VIF_XML}.new ${VIF_XML} ONLY_IF_DIFFERENT)
    file(REMOVE ${VIF_XML}.new)
    message(STATUS "Generated vif.xml: ${VIF_XML}")
endif ()
