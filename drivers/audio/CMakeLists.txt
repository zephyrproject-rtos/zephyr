zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_AUDIO_TLV320DAC}  tlv320dac310x.c
            IFDEF:${CONFIG_AUDIO_INTEL_DMIC} intel_dmic.c
                                             decimation/pdm_decim_int32_02_4288_5100_010_095.c
                                             decimation/pdm_decim_int32_02_4375_5100_010_095.c
                                             decimation/pdm_decim_int32_03_3850_5100_010_095.c
                                             decimation/pdm_decim_int32_03_4375_5100_010_095.c
                                             decimation/pdm_decim_int32_04_4375_5100_010_095.c
                                             decimation/pdm_decim_int32_05_4331_5100_010_095.c
                                             decimation/pdm_decim_int32_06_4156_5100_010_095.c
                                             decimation/pdm_decim_int32_08_4156_5380_010_090.c
                                             decimation/pdm_decim_table.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
