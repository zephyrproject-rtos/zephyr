# SPDX-License-Identifier: Apache-2.0
zephyr_library_sources_ifdef(CONFIG_AP_PWRSEQ            ap_pwrseq.c)
zephyr_library_sources_ifdef(CONFIG_AP_PWRSEQ            ap_pwrseq_sm.c)
zephyr_library_sources_ifdef(CONFIG_AP_PWRSEQ_X86        x86/ap_pwrseq_x86.c)
zephyr_library_sources_ifdef(CONFIG_AP_PWRSEQ_X86_ADL    x86/ap_pwrseq_x86_adl.c)
zephyr_library_sources_ifdef(CONFIG_AP_PWRSEQ_X86_ADL    x86/application.c)
