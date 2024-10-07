# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_ACP_6_0)
    board_set_flasher_ifnset(misc-flasher)
    board_finalize_runner_args(misc-flasher)
    
    board_set_rimage_target(acp_6_0)
endif()