# '-mcmse' enables the generation of code for the Secure state of the ARMv8-M
# Security Extensions. This option is required when building a Secure firmware.
zephyr_compile_options(-mcmse)

zephyr_sources(arm_core_tz.c)
