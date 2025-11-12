# SPDX-License-Identifier: Apache-2.0

# This file contains boards in Zephyr which have been replaced with a new board
# name.
# This allows the system to automatically change the board while at the same
# time prints a warning to the user, that the board name is deprecated.
#
# To add a board rename, add a line in following format:
# set(<old_board_name>_DEPRECATED <new_board_name>)
#
# When adding board aliases here, remember to add a mention in the corresponding GitHub issue
# tracking the removal of API/options
# https://docs.zephyrproject.org/latest/develop/api/api_lifecycle.html#deprecated,
# so these aliases are eventually removed

set(arduino_uno_r4_minima_DEPRECATED
    arduino_uno_r4@minima
)
set(arduino_uno_r4_wifi_DEPRECATED
    arduino_uno_r4@wifi
)
set(esp32c6_devkitc_DEPRECATED
    esp32c6_devkitc/esp32c6/hpcore
)
set(neorv32_DEPRECATED
    neorv32/neorv32/up5kdemo
)
set(panb511evb_DEPRECATED
    panb611evb
)
set(xiao_esp32c6_DEPRECATED
    xiao_esp32c6/esp32c6/hpcore
)
set(esp32_devkitc_wroom/esp32/procpu_DEPRECATED
    esp32_devkitc/esp32/procpu
)
set(esp32_devkitc_wrover/esp32/procpu_DEPRECATED
    esp32_devkitc/esp32/procpu
)
set(esp32_devkitc_wroom/esp32/appcpu_DEPRECATED
    esp32_devkitc/esp32/appcpu
)
set(esp32_devkitc_wrover/esp32/appcpu_DEPRECATED
    esp32_devkitc/esp32/appcpu
)
set(scobc_module1_DEPRECATED
    scobc_a1
)
set(raytac_an54l15q_db/nrf54l15/cpuapp_DEPRECATED
    raytac_an54lq_db_15/nrf54l15/cpuapp
)
set(fvp_base_revc_2xaemv8a_DEPRECATED
    fvp_base_revc_2xaem/v8a
)
set(fvp_base_revc_2xaemv8a/fvp_base_revc_2xaemv8a/smp_DEPRECATED
    fvp_base_revc_2xaem/v8a/smp
)
set(fvp_base_revc_2xaemv8a/fvp_base_revc_2xaemv8a/smp/ns_DEPRECATED
    fvp_base_revc_2xaem/v8a/smp/ns
)
set(esp32s3_devkitm/esp32s3/procpu_DEPRECATED
    esp32s3_devkitc/esp32s3/procpu
)
set(esp32s3_devkitm/esp32s3/appcpu_DEPRECATED
    esp32s3_devkitc/esp32s3/appcpu
)
set(ubx_evk_iris_w1_fidelex/rw612_DEPRECATED
    ubx_evk_iris_w1@fidelix/rw612
)
