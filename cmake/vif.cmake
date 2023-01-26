# Copyright (c) 2022 The Chromium OS Authors
# SPDX-License-Identifier: Apache-2.0

# Generates USB-C VIF policies in XML format from device tree.
set(gen_vif_script ${ZEPHYR_BASE}/scripts/generate_usb_vif/generate_vif.py)
set(dts_compatible usb-c-connector)
set(vif_xml ${PROJECT_BINARY_DIR}/vif.xml)
set(cmd_gen_vif ${PYTHON_EXECUTABLE} ${gen_vif_script}
        --edt-pickle ${EDT_PICKLE}
        --compatible ${dts_compatible}
        --vif-out ${vif_xml}
        --board ${BOARD}
        )

add_custom_command(
        OUTPUT ${vif_xml}
        DEPENDS ${EDT_PICKLE}
        COMMENT "Generating XML file at zephyr/vif.xml"
        COMMAND ${cmd_gen_vif}
)

add_custom_target(gen_vif ALL DEPENDS ${vif_xml})
