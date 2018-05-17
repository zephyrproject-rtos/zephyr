zephyr_sources_ifdef(CONFIG_ADC_DW            adc_dw.c)
zephyr_sources_ifdef(CONFIG_ADC_MCUX_ADC16 	  adc_mcux_adc16.c)
zephyr_sources_ifdef(CONFIG_ADC_QMSI   		  adc_qmsi.c)
zephyr_sources_ifdef(CONFIG_ADC_QMSI_SS		  adc_qmsi_ss.c)
zephyr_sources_ifdef(CONFIG_ADC_SAM_AFEC      adc_sam_afec.c)
zephyr_sources_ifdef(CONFIG_ADC_TI_ADC108S102 adc_ti_adc108s102.c)

zephyr_sources_ifdef(CONFIG_USERSPACE   adc_handlers.c)
