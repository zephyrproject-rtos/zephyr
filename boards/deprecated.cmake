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
set(it51xxx_evb_DEPRECATED
    it515xx_evb/it51526aw
)
set(weact_stm32h5_core_DEPRECATED
    weact_stm32h562_core
)
set(ai_m62_12f_DEPRECATED
    ai_m62_12f_kit
)
set(ai_wb2_12f_DEPRECATED
    ai_wb2_12f_kit
)
set(bl54l15u_dvk/nrf54l15/cpuapp_DEPRECATED
    bl54l15_dvk/nrf54l15/cpuapp
)
set(bl54l15u_dvk/nrf54l15/cpuflpr_DEPRECATED
    bl54l15_dvk/nrf54l15/cpuflpr
)
set(elemrv/elemrv_n_DEPRECATED
    elemrv_flask_n
)
