zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_ADC_DW}                adc_dw.c
            IFDEF:${CONFIG_ADC_MCUX_ADC16}        adc_mcux_adc16.c
            IFDEF:${CONFIG_ADC_SAM_AFEC}          adc_sam_afec.c
            IFDEF:${CONFIG_ADC_NRFX_ADC}          adc_nrfx_adc.c
            IFDEF:${CONFIG_ADC_NRFX_SAADC}        adc_nrfx_saadc.c
            IFDEF:${CONFIG_ADC_INTEL_QUARK_D2000} adc_intel_quark_d2000.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
