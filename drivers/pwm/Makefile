# Makefile for PWM drivers
ccflags-y +=-I$(srctree)/drivers
ccflags-$(CONFIG_PWM_QMSI) += -I$(CONFIG_QMSI_INSTALL_PATH)/include

obj-$(CONFIG_PWM_PCA9685)	+= pwm_pca9685.o
obj-$(CONFIG_PWM_DW)		+= pwm_dw.o
obj-$(CONFIG_PWM_QMSI)		+= pwm_qmsi.o
obj-$(CONFIG_PWM_STM32)		+= pwm_stm32.o
obj-$(CONFIG_PWM_NRF5_SW)       += pwm_nrf5_sw.o
obj-$(CONFIG_PWM_MCUX_FTM)	+= pwm_mcux_ftm.o
