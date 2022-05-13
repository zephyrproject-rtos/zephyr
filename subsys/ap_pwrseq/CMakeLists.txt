# SPDX-License-Identifier: Apache-2.0
zephyr_library_sources_ifdef(CONFIG_AP_PWRSEQ            ap_pwrseq.c)
zephyr_library_sources_ifdef(CONFIG_AP_PWRSEQ            ap_pwrseq_sm.c)
