/* Copyright 2024 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTIONS
{
    .app_header :
    {
	    KEEP(*(.app_header))
    } > APP_HEADER_FLASH

   /* Cortex-M33 bootstrap code area */
    .bootstrapText :
    {
        . = ALIGN(4);
        __bootstrapText_begin = .;

        /* Located in generated directory. This file is populated by calling
         * zephyr_linker_sources(ROM_START ...). This typically contains the vector
         * table and debug information.
         */
        #include <snippets-rom-start.ld>

        /* startup code */
        *(.text._reset_section)

        *startup_cat1b_cm33.*(.text*)
        *system_cyw20829.*(.text*)

        /* drivers */
        *cy_device.*(.text*)
        *cy_btss.*(.text*)
        *cy_sysclk_v2.*(.text*)
        *cy_syspm_v2.*(.text*)
        *cy_sysint_v2.*(.text*)
        *cy_syslib*.*(.text*)
        *ppu_v1.*(.text*)
        *cy_mpc.*(.text*)
        *cy_syspm_ppu.*(.text*)

        *memcpy*.* (.text*)   /* add memcpy from the NewLib library here*/
        *memset*.* (.text*)   /* add memcpy from the NewLib library here*/
        *memmove*.* (.text*)  /* add memcpy from the NewLib library here*/
        *s_fabs.* (.text*)

        KEEP(*(.cy_l1func*))

        . = ALIGN(4);
        __bootstrapText_end = .;
    } > BOOTSTRAP_RAM AT>BOOTSTRAP_FLASH

    .bootstrapzero.table :
    {
        . = ALIGN(4);
        __bootstrapzero_table_start__ = .;
        LONG (__bootstrap_bss_start__)
        LONG ((__bootstrap_bss_end__ - __bootstrap_bss_start__)/4)
        . = ALIGN(4);
        __bootstrapzero_table_end__ = .;
    } > BOOTSTRAP_RAM AT>BOOTSTRAP_FLASH

    .bootstrapData :
    {
        __bootstrapData_start__ = .;
        . = ALIGN(4);

        /* startup code */
        *startup_cat1b_cm33.*(.data* .rodata .rodata.* .constdata .constdata.* .conststring .conststring.*)
        *system_cyw20829.*(.data* .rodata .rodata.* .constdata .constdata.* .conststring .conststring.*)

        /* drivers */
        *cy_device.*(.data* .rodata .rodata.* .constdata .constdata.* .conststring .conststring.*)
        *cy_btss.*(.data* .rodata .rodata.* .constdata .constdata.* .conststring .conststring.*)
        *cy_sysclk_v2.*(.data* .rodata .rodata.* .constdata .constdata.* .conststring .conststring.*)
        *cy_syspm_v2.*(.data* .rodata .rodata.* .constdata .constdata.* .conststring .conststring.*)
        *cy_sysint_v2.*(.data* .rodata .rodata.* .constdata .constdata.* .conststring .conststring.*)
        *cy_syslib.*(.data* .rodata .rodata.* .constdata .constdata.* .conststring .conststring.*)
        *ppu_v1.*(.data* .rodata .rodata.* .constdata .constdata.* .conststring .conststring.*)
        *cy_mpc.*(.data* .rodata .rodata.* .constdata .constdata.* .conststring .conststring.*)
        *cy_pd_ppu.*(.data* .rodata .rodata.* .constdata .constdata.* .conststring .conststring.*)

        KEEP(*(.cy_l1data*))

        . = ALIGN(4);
        __bootstrapData_end__ = .;
    } > BOOTSTRAP_RAM AT>BOOTSTRAP_FLASH

    .bootstrapBss (NOLOAD):
    {
        . = ALIGN(4);
        __bootstrap_bss_start__ = .;

        /* startup code */
        *startup_cat1b_cm33.*(.bss* COMMON)
        *system_cyw20829.*(.bss* COMMON)

        /* drivers */
        *cy_device.*(.bss* COMMON)
        *cy_btss.*(.bss* COMMON)
        *cy_sysclk_v2.*(.bss* COMMON)
        *cy_syspm_v2.*(.bss* COMMON)
        *cy_sysint_v2.*(.bss* COMMON)
        *cy_syslib.*(.bss* COMMON)
        *ppu_v1.*(.bss* COMMON)
        *cy_mpc.*(.bss* COMMON)
        *cy_pd_ppu.*(.bss* COMMON)
        KEEP(*(.cy_l1bss*))

        . = ALIGN(4);
        __bootstrap_bss_end__ = .;
    } > BOOTSTRAP_RAM
}
