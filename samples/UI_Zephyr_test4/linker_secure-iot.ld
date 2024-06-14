
/***************************************************************************
 * Project           		    :  Mindgrove Silicon Secure IoT
 * Name of the file	     	    :  linker_secure-iot.ld
 * Brief Description of file    :  Linker Script for Secure Iot XIP
 * Name of Author    	        :  Kapil Shyam. M
 * Email ID                     :  kapil@mindgrovetech.in

 Copyright (C) 2023  Mindgrove Technologies Private Limited.

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.

 ***************************************************************************/
#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

MEMORY
{
     EXTFLASH (wx) : ORIGIN = 0x90000000, LENGTH = 0x10000000
}

#include <zephyr/arch/riscv/common/linker.ld>