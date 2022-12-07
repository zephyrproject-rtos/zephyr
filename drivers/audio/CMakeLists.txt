# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_AUDIO_TLV320DAC	tlv320dac310x.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_MPXXDTYY	mpxxdtyy.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_MPXXDTYY	mpxxdtyy-i2s.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_DMIC_NRFX_PDM	dmic_nrfx_pdm.c)
