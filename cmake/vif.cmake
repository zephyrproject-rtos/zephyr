# Copyright (c) 2022 The Chromium OS Authors
# SPDX-License-Identifier: Apache-2.0

# Generates USB-C VIF policies in XML format from device tree.
set(gen_vif_script ${ZEPHYR_BASE}/scripts/generate_usb_vif/generate_vif.py)
set(dts_compatible usb-c-connector)
set(vif_xml vif.xml)
set(board_vif_xml ${BOARD}_${vif_xml})
set(vif_out ${PROJECT_BINARY_DIR}/${vif_xml})

set(cmd_gen_vif ${PYTHON_EXECUTABLE} ${gen_vif_script}
        --edt-pickle ${EDT_PICKLE}
        --compatible ${dts_compatible}
        --vif-out ${vif_out}
        )

if (CONFIG_GENVIF_INPUT_VIF_XML_PATH)
    if (IS_ABSOLUTE ${CONFIG_GENVIF_INPUT_VIF_XML_PATH})
        if (EXISTS ${CONFIG_GENVIF_INPUT_VIF_XML_PATH})
            set(vif_source_xml ${CONFIG_GENVIF_INPUT_VIF_XML_PATH})
        endif ()
    elseif (EXISTS ${APPLICATION_CONFIG_DIR}/${CONFIG_GENVIF_INPUT_VIF_XML_PATH})
        set(vif_source_xml ${APPLICATION_CONFIG_DIR}/${CONFIG_GENVIF_INPUT_VIF_XML_PATH})
    endif ()
else ()
    if (EXISTS ${APPLICATION_CONFIG_DIR}/boards/${board_vif_xml})
        set(vif_source_xml ${APPLICATION_CONFIG_DIR}/boards/${board_vif_xml})
    elseif (EXISTS ${APPLICATION_CONFIG_DIR}/${vif_xml})
        set(vif_source_xml ${APPLICATION_CONFIG_DIR}/${vif_xml})
    elseif (EXISTS ${BOARD_DIR}/${vif_xml})
        set(vif_source_xml ${BOARD_DIR}/${vif_xml})
    endif ()
endif ()

if (DEFINED vif_source_xml)
    list(APPEND cmd_gen_vif --vif-source-xml ${vif_source_xml})
else ()
    if (CONFIG_GENVIF_INPUT_VIF_XML_PATH)
        message(FATAL_ERROR "Incorrect VIF source XML file path. To fix specify"
                " correct XML file path at 'CONFIG_GENVIF_INPUT_VIF_XML_PATH'.")
    else ()
        message(FATAL_ERROR "No VIF source XML file found. To fix, create"
                " '${board_vif_xml}' in 'boards' directory of application"
                " directory, or create '${vif_xml}' file in application"
                " directory or board directory, or supply a custom XML VIF path"
                " using 'CONFIG_GENVIF_INPUT_VIF_XML_PATH'.")
    endif ()
endif ()

add_custom_command(
        OUTPUT ${vif_xml}
        DEPENDS ${EDT_PICKLE}
        COMMENT "Generating XML file at zephyr/vif.xml"
        COMMAND ${cmd_gen_vif}
)

add_custom_target(gen_vif ALL DEPENDS ${vif_xml})
