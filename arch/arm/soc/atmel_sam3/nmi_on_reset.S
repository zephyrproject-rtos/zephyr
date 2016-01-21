/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#define _ASMLANGUAGE

#include <toolchain.h>
#include <sections.h>

_ASM_FILE_PROLOGUE

GTEXT(_SysNmiOnReset)

SECTION_FUNC(TEXT, _SysNmiOnReset)
    wfi
    b _SysNmiOnReset
