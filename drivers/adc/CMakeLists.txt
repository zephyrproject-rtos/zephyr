zephyr_library()
target_link_libraries(                      drivers__adc drivers)

target_sources_ifdef(CONFIG_ADC_DW              drivers__adc PRIVATE adc_dw.c)
target_sources_ifdef(CONFIG_ADC_MCUX_ADC16 	drivers__adc PRIVATE adc_mcux_adc16.c)
target_sources_ifdef(CONFIG_ADC_TI_ADC108S102   drivers__adc PRIVATE adc_ti_adc108s102.c)
target_sources_ifdef(CONFIG_ADC_QMSI   		drivers__adc PRIVATE adc_qmsi.c)
target_sources_ifdef(CONFIG_ADC_QMSI_SS		drivers__adc PRIVATE adc_qmsi_ss.c)
