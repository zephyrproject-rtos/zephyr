# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_AUDIO_DMIC_AMBIQ_PDM dmic_ambiq_pdm.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_DMIC_MCUX dmic_mcux.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_DMIC_NRFX_PDM dmic_nrfx_pdm.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_DMIC_NXP_MICFIL dmic_nxp_micfil.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_MPXXDTYY mpxxdtyy-i2s.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_MPXXDTYY mpxxdtyy.c)
# zephyr-keep-sorted-stop
