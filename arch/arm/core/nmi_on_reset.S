/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Default basic NMI handler before the kernel is up
 *
 * Provide a default handler for NMI before the system is up. The default action
 * is to hard hang, sleeping.
 *
 * This might be preferable than rebooting to help debugging, or because
 * rebooting might trigger the exact same problem over and over.
 */

#include <toolchain.h>
#include <linker/sections.h>

_ASM_FILE_PROLOGUE

GTEXT(z_SysNmiOnReset)

SECTION_FUNC(TEXT, z_SysNmiOnReset)
    wfi
    b z_SysNmiOnReset
