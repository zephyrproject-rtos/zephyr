# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_AUDIO_CODEC codec)
add_subdirectory_ifdef(CONFIG_AUDIO_DMIC dmic)
add_subdirectory_ifdef(CONFIG_INTEL_ADSP_MIC_PRIVACY mic_privacy)
