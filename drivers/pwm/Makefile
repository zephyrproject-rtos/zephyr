# Makefile for PWM drivers
ccflags-y +=-I$(srctree)/drivers
ccflags-$(CONFIG_PWM_QMSI) += -I$(CONFIG_QMSI_INSTALL_PATH)/include

obj-$(CONFIG_PWM_PCA9685)	+= pwm_pca9685.o
obj-$(CONFIG_PWM_DW)		+= pwm_dw.o
obj-$(CONFIG_PWM_QMSI)		+= pwm_qmsi.o
obj-$(CONFIG_PWM_K64_FTM)	+= pwm_k64_ftm.o

