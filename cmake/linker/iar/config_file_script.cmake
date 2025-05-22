# Copyright (c) 2025 IAR Systems AB
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.17)

set(SORT_TYPE_NAME Lexical)

set_property(GLOBAL PROPERTY ILINK_REGION_SYMBOL_ICF)

# This function post process the region for easier use.
#
# Tasks:
# - Symbol translation using a steering file is configured.
function(process_region)
  cmake_parse_arguments(REGION "" "OBJECT" "" ${ARGN})

  process_region_common(${ARGN})

  get_property(empty GLOBAL PROPERTY ${REGION_OBJECT}_EMPTY)
  if(NOT empty)
    # For scatter files we move any system symbols into first non-empty load section.
    get_parent(OBJECT ${REGION_OBJECT} PARENT parent TYPE SYSTEM)
    get_property(symbols GLOBAL PROPERTY ${parent}_SYMBOLS)
    set_property(GLOBAL APPEND PROPERTY ${REGION_OBJECT}_SYMBOLS ${symbols})
    set_property(GLOBAL PROPERTY ${parent}_SYMBOLS)
  endif()

  get_property(sections GLOBAL PROPERTY ${REGION_OBJECT}_SECTION_LIST_ORDERED)
  foreach(section ${sections})

    get_property(name       GLOBAL PROPERTY ${section}_NAME)
    get_property(name_clean GLOBAL PROPERTY ${section}_NAME_CLEAN)
    get_property(noinput    GLOBAL PROPERTY ${section}_NOINPUT)
    get_property(type       GLOBAL PROPERTY ${section}_TYPE)
    get_property(nosymbols  GLOBAL PROPERTY ${section}_NOSYMBOLS)

    if(NOT nosymbols)
      if(${name} STREQUAL .ramfunc)
        create_symbol(OBJECT ${REGION_OBJECT} SYMBOL __${name_clean}_load_start
          EXPR "@ADDR(.ramfunc_init)@"
          )
      else()
        create_symbol(OBJECT ${REGION_OBJECT} SYMBOL __${name_clean}_load_start
          EXPR "@LOADADDR(${name_clean})@"
          )
      endif()
    endif()

    get_property(indicies GLOBAL PROPERTY ${section}_SETTINGS_INDICIES)
    list(LENGTH indicies length)
    foreach(idx ${indicies})
      set(steering_postfixes Base Limit)
      get_property(symbols GLOBAL PROPERTY ${section}_SETTING_${idx}_SYMBOLS)
      get_property(sort    GLOBAL PROPERTY ${section}_SETTING_${idx}_SORT)
      get_property(offset  GLOBAL PROPERTY ${section}_SETTING_${idx}_OFFSET)
      if(DEFINED offset AND NOT offset EQUAL 0 )
        # Same behavior as in section_to_string
      elseif(DEFINED offset AND offset STREQUAL 0 )
        # Same behavior as in section_to_string
      elseif(sort)
        # Treated by labels in the icf or image symbols.
      elseif(DEFINED symbols AND ${length} EQUAL 1 AND noinput)
      endif()
    endforeach()

    # Symbols translation here.

    get_property(symbol_val GLOBAL PROPERTY SYMBOL_TABLE___${name_clean}_end)

    if("${symbol_val}" STREQUAL "${name_clean}")
      create_symbol(OBJECT ${REGION_OBJECT} SYMBOL __${name_clean}_size
        EXPR "@SIZE(${name_clean})@"
        )
    else()
      # These seem to be thing that can't be transformed to $$Length
      set_property(GLOBAL APPEND PROPERTY ILINK_REGION_SYMBOL_ICF
        "define image symbol __${name_clean}_size = (__${symbol_val} - ADDR(${name_clean}))")
    endif()
    set(ZI)

    if(${name_clean} STREQUAL last_ram_section)
      # A trick to add the symbol for the nxp devices
      # _flash_used = LOADADDR(.last_section) + SIZEOF(.last_section) - __rom_region_start;
      create_symbol(OBJECT ${REGION_OBJECT} SYMBOL _flash_used
        EXPR "(@LOADADDR(last_section)@ + @SIZE(last_section)@ - @__rom_region_start@)"
        )
    endif()

    if(${name_clean} STREQUAL rom_start)
      # The below two symbols is meant to make aliases to the _vector_table symbol.
      list(GET symbols 0 symbol_start)
      create_symbol(OBJECT ${REGION_OBJECT} SYMBOL __Vectors
        EXPR "@ADDR(${symbol_start})@"
        )
      create_symbol(OBJECT ${REGION_OBJECT} SYMBOL __vector_table
        EXPR "@ADDR(${symbol_start})@"
        )
    endif()

  endforeach()

  get_property(groups GLOBAL PROPERTY ${REGION_OBJECT}_GROUP_LIST_ORDERED)
  foreach(group ${groups})
    get_property(name GLOBAL PROPERTY ${group}_NAME)
    string(TOLOWER ${name} name)

    get_property(group_type  GLOBAL PROPERTY ${group}_OBJ_TYPE)
    get_property(parent      GLOBAL PROPERTY ${group}_PARENT)
    get_property(parent_type GLOBAL PROPERTY ${parent}_OBJ_TYPE)
    # Need to find the init manually group or parent
    if(${parent_type} STREQUAL GROUP)
      get_property(vma GLOBAL PROPERTY ${parent}_VMA)
      get_property(lma GLOBAL PROPERTY ${parent}_LMA)
    else()
      get_property(vma GLOBAL PROPERTY ${group}_VMA)
      get_property(lma GLOBAL PROPERTY ${group}_LMA)
    endif()

    get_objects(LIST sections OBJECT ${group} TYPE SECTION)
    list(GET sections 0 section)
    get_property(first_section_name GLOBAL PROPERTY ${section}_NAME_CLEAN)
    list(POP_BACK sections section)
    get_property(last_section_name GLOBAL PROPERTY ${section}_NAME_CLEAN)

    if(DEFINED vma AND DEFINED lma)
      # Something to init
      create_symbol(OBJECT ${REGION_OBJECT} SYMBOL __${name}_load_start
        EXPR "@ADDR(${first_section_name}_init)@"
        )
    else()
      create_symbol(OBJECT ${REGION_OBJECT} SYMBOL __${name}_load_start
        EXPR "@LOADADDR(${first_section_name})@"
        )
    endif()

    create_symbol(OBJECT ${REGION_OBJECT} SYMBOL __${name}_start
      EXPR "@ADDR(${first_section_name})@"
      )
    create_symbol(OBJECT ${REGION_OBJECT} SYMBOL __${name}_end
      EXPR "@END(${last_section_name})@"
      )
    create_symbol(OBJECT ${REGION_OBJECT} SYMBOL __${name}_size
      EXPR "(@(__${name}_end)@ - @(__${name}_start)@)"
      )

  endforeach()

  get_property(symbols GLOBAL PROPERTY ${REGION_OBJECT}_SYMBOLS)
  foreach(symbol ${symbols})
    get_property(name GLOBAL PROPERTY ${symbol}_NAME)
    get_property(expr GLOBAL PROPERTY ${symbol}_EXPR)
    if(NOT DEFINED expr)
      create_symbol(OBJECT ${REGION_OBJECT} SYMBOL __${name}_size
        EXPR "@(ADDR(${name})@"
        )
    endif()
  endforeach()

  # This is only a trick to get the memories
  set(groups)
  get_objects(LIST groups OBJECT ${REGION_OBJECT} TYPE GROUP)
  foreach(group ${groups})
    get_property(group_type  GLOBAL PROPERTY ${group}_OBJ_TYPE)
    get_property(parent      GLOBAL PROPERTY ${group}_PARENT)
    get_property(parent_type GLOBAL PROPERTY ${parent}_OBJ_TYPE)

    if(${group_type} STREQUAL GROUP)
      get_property(group_name GLOBAL PROPERTY ${group}_NAME)
      get_property(group_lma  GLOBAL PROPERTY ${group}_LMA)
      if(${group_name} STREQUAL ROM_REGION)
        set_property(GLOBAL PROPERTY ILINK_ROM_REGION_NAME ${group_lma})
      endif()
    endif()

    if(${parent_type} STREQUAL GROUP)
      get_property(vma GLOBAL PROPERTY ${parent}_VMA)
      get_property(lma GLOBAL PROPERTY ${parent}_LMA)

      set_property(GLOBAL PROPERTY ${group}_VMA ${vma})
      set_property(GLOBAL PROPERTY ${group}_LMA ${lma})
    endif()
  endforeach()

endfunction()

#
# String functions - start
#

function(system_to_string)
  cmake_parse_arguments(STRING "" "OBJECT;STRING" "" ${ARGN})

  get_property(name    GLOBAL PROPERTY ${STRING_OBJECT}_NAME)
  get_property(regions GLOBAL PROPERTY ${STRING_OBJECT}_REGIONS)
  get_property(format  GLOBAL PROPERTY ${STRING_OBJECT}_FORMAT)

  # Ilink specials
  # set(${STRING_STRING} "build for rom;\n")
  set(${STRING_STRING} "build for ram;\n")
  if("${format}" MATCHES "aarch64")
    set(${STRING_STRING} "${${STRING_STRING}}define memory mem with size = 16E;\n")
  else()
    set(${STRING_STRING} "${${STRING_STRING}}define memory mem with size = 4G;\n")
  endif()

  foreach(region ${regions})
    get_property(name    GLOBAL PROPERTY ${region}_NAME)
    get_property(address GLOBAL PROPERTY ${region}_ADDRESS)
    get_property(flags   GLOBAL PROPERTY ${region}_FLAGS)
    get_property(size    GLOBAL PROPERTY ${region}_SIZE)

    if(DEFINED flags)
      if(${flags} STREQUAL rx)
        set(flags " rom")
      elseif(${flags} STREQUAL ro)
        set(flags " rom")
      elseif(${flags} STREQUAL wx)
        set(flags " ram")
      elseif(${flags} STREQUAL rw)
        set(flags " ram")
      endif()
    endif()

    if(${name} STREQUAL IDT_LIST)
      # Need to use a untyped region for IDT_LIST
      set(flags "")
    endif()

    if(DEFINED address)
      set(start "${address}")
    endif()

    if(DEFINED size)
      set(size "${size}")
    endif()
    # define rom region FLASH    = mem:[from 0x0 size 0x40000];
    set(memory_region "define${flags} region ${name} = mem:[from ${start} size ${size}];")

    set(${STRING_STRING} "${${STRING_STRING}}${memory_region}\n")
    set(flags)
  endforeach()

  set(${STRING_STRING} "${${STRING_STRING}}\n\n")
  set_property(GLOBAL PROPERTY ILINK_SYMBOL_ICF)

  set(${STRING_STRING} "${${STRING_STRING}}\n")
  foreach(region ${regions})
    get_property(empty GLOBAL PROPERTY ${region}_EMPTY)
    if(NOT empty)
      get_property(name    GLOBAL PROPERTY ${region}_NAME)
      set(ILINK_CURRENT_NAME ${name})
      to_string(OBJECT ${region} STRING ${STRING_STRING})
      set(ILINK_CURRENT_NAME)
    endif()
  endforeach()
  set(${STRING_STRING} "${${STRING_STRING}}\n")

  get_property(symbols_icf GLOBAL PROPERTY ILINK_SYMBOL_ICF)
  foreach(image_symbol ${symbols_icf})
    set(${STRING_STRING} "${${STRING_STRING}}define image symbol ${image_symbol};\n")
  endforeach()

  get_property(symbols_icf GLOBAL PROPERTY ILINK_REGION_SYMBOL_ICF)
  set(${STRING_STRING} "${${STRING_STRING}}\n")
  foreach(image_symbol ${symbols_icf})
    set(${STRING_STRING} "${${STRING_STRING}}${image_symbol};\n")
  endforeach()

  if(IAR_LIBC)
    set(${STRING_STRING} "${${STRING_STRING}}if (K_HEAP_MEM_POOL_SIZE>0)\n{\n")
    set(${STRING_STRING} "${${STRING_STRING}}  define block HEAP with alignment=8 { symbol kheap__system_heap };\n")
    set(${STRING_STRING} "${${STRING_STRING}}}\nelse\n{\n")
    set(${STRING_STRING} "${${STRING_STRING}}  define block HEAP with alignment=8, expanding size { };\n")
    set(${STRING_STRING} "${${STRING_STRING}}}\n")
    set(${STRING_STRING} "${${STRING_STRING}}\"DLib heap\": place in RAM { block HEAP };\n")
#    set(${STRING_STRING} "${${STRING_STRING}}define exported symbol HEAP$$Base=kheap__system_heap;\n")
#    set(${STRING_STRING} "${${STRING_STRING}}define exported symbol HEAP$$Limit=END(kheap__system_heap);\n")
  endif()

  set(${STRING_STRING} ${${STRING_STRING}} PARENT_SCOPE)
endfunction()

function(group_to_string)
  cmake_parse_arguments(STRING "" "OBJECT;STRING" "" ${ARGN})

  get_property(type GLOBAL PROPERTY ${STRING_OBJECT}_OBJ_TYPE)
  if(${type} STREQUAL REGION)
    get_property(empty GLOBAL PROPERTY ${STRING_OBJECT}_EMPTY)
    if(empty)
      return()
    endif()
  endif()

  #_SECTIONS_FIXED need a place at address statement:
  get_property(sections GLOBAL PROPERTY ${STRING_OBJECT}_SECTIONS_FIXED)
  foreach(section ${sections})
    to_string(OBJECT ${section} STRING ${STRING_STRING})
    get_property(name       GLOBAL PROPERTY ${section}_NAME)
    get_property(name_clean GLOBAL PROPERTY ${section}_NAME_CLEAN)
    get_property(section_address GLOBAL PROPERTY ${section}_ADDRESS)
    set(${STRING_STRING} "${${STRING_STRING}}\"${name}\": place at address mem:${section_address} { block ${name_clean} };\n")
  endforeach()

  get_property(groups GLOBAL PROPERTY ${STRING_OBJECT}_GROUPS)
  foreach(group ${groups})
    to_string(OBJECT ${group} STRING ${STRING_STRING})
  endforeach()

  get_property(sections GLOBAL PROPERTY ${STRING_OBJECT}_SECTIONS)
  foreach(section ${sections})
    to_string(OBJECT ${section} STRING ${STRING_STRING})

    get_property(name     GLOBAL PROPERTY ${section}_NAME)

    get_property(name_clean GLOBAL PROPERTY ${section}_NAME_CLEAN)

    get_property(parent   GLOBAL PROPERTY ${section}_PARENT)
    get_property(noinit   GLOBAL PROPERTY ${section}_NOINIT)
    # This is only a trick to get the memories
    get_property(parent_type GLOBAL PROPERTY ${parent}_OBJ_TYPE)
    if(${parent_type} STREQUAL GROUP)
      get_property(vma GLOBAL PROPERTY ${parent}_VMA)
      get_property(lma GLOBAL PROPERTY ${parent}_LMA)
    endif()

    if(DEFINED vma)
      set(ILINK_CURRENT_NAME ${vma})
    elseif(DEFINED lma)
      set(ILINK_CURRENT_NAME ${lma})
    else()
      # message(FATAL_ERROR "Need either vma or lma")
    endif()

    set(${STRING_STRING} "${${STRING_STRING}}\"${name}\": place in ${ILINK_CURRENT_NAME} { block ${name_clean} };\n")
    if(DEFINED vma AND DEFINED lma AND NOT ${noinit})
      set(${STRING_STRING} "${${STRING_STRING}}\"${name}_init\": place in ${lma} { block ${name_clean}_init };\n")
    endif()

  endforeach()

  get_parent(OBJECT ${STRING_OBJECT} PARENT parent TYPE SYSTEM)
  get_property(regions GLOBAL PROPERTY ${parent}_REGIONS)
  list(REMOVE_ITEM regions ${STRING_OBJECT})

  foreach(region ${regions})
    get_property(vma GLOBAL PROPERTY ${region}_NAME)
    get_property(sections GLOBAL PROPERTY ${STRING_OBJECT}_${vma}_SECTIONS_FIXED)

    foreach(section ${sections})
      to_string(OBJECT ${section} STRING ${STRING_STRING})
    endforeach()

    get_property(groups GLOBAL PROPERTY ${STRING_OBJECT}_${vma}_GROUPS)
    foreach(group ${groups})
      to_string(OBJECT ${group} STRING ${STRING_STRING})
    endforeach()

    get_property(sections GLOBAL PROPERTY ${STRING_OBJECT}_${vma}_SECTIONS)
    foreach(section ${sections})
      to_string(OBJECT ${section} STRING ${STRING_STRING})
      get_property(name     GLOBAL PROPERTY ${section}_NAME)
      string(REGEX REPLACE "^[\.]" "" name_clean "${name}")
      string(REPLACE "." "_" name_clean "${name_clean}")
      set(${STRING_STRING} "${${STRING_STRING}}\"${name}\": place in ${vma} { block ${name_clean} };\n")

      # Insert 'do not initialize' here
      get_property(current_sections GLOBAL PROPERTY ILINK_CURRENT_SECTIONS)
      if(${name} STREQUAL .bss)
        if(DEFINED current_sections)
          set(${STRING_STRING} "${${STRING_STRING}}do not initialize\n")
          set(${STRING_STRING} "${${STRING_STRING}}{\n")
          foreach(section ${current_sections})
            set(${STRING_STRING} "${${STRING_STRING}}  ${section},\n")
          endforeach()
          set(${STRING_STRING} "${${STRING_STRING}}};\n")
          set(current_sections)
          set_property(GLOBAL PROPERTY ILINK_CURRENT_SECTIONS)
        endif()
      endif()

      if(${name_clean} STREQUAL last_ram_section)
        get_property(group_name_lma GLOBAL PROPERTY ILINK_ROM_REGION_NAME)
        set(${STRING_STRING} "${${STRING_STRING}}\n")
        if(${CONFIG_LINKER_LAST_SECTION_ID})
          set(${STRING_STRING} "${${STRING_STRING}}define section last_section_id { udata32 ${CONFIG_LINKER_LAST_SECTION_ID_PATTERN}; };\n")
          set(${STRING_STRING} "${${STRING_STRING}}define block last_section with fixed order { section last_section_id };\n")
        else()
          set(${STRING_STRING} "${${STRING_STRING}}define block last_section with fixed order { };\n")
        endif()
        # Not really the right place, we want the last used flash bytes not end of the world!
        # set(${STRING_STRING} "${${STRING_STRING}}\".last_section\": place at end of ${group_name_lma} { block last_section };\n")
        set(${STRING_STRING} "${${STRING_STRING}}\".last_section\": place in ${group_name_lma} { block last_section };\n")
        set(${STRING_STRING} "${${STRING_STRING}}keep { block last_section };\n")
      endif()

    endforeach()
  endforeach()

  get_property(symbols GLOBAL PROPERTY ${STRING_OBJECT}_SYMBOLS)
  set(${STRING_STRING} "${${STRING_STRING}}\n")
  foreach(symbol ${symbols})
    to_string(OBJECT ${symbol} STRING ${STRING_STRING})
  endforeach()

  set(${STRING_STRING} ${${STRING_STRING}} PARENT_SCOPE)
endfunction()


function(section_to_string)
  cmake_parse_arguments(STRING "" "SECTION;STRING" "" ${ARGN})

  get_property(name     GLOBAL PROPERTY ${STRING_SECTION}_NAME)
  get_property(address  GLOBAL PROPERTY ${STRING_SECTION}_ADDRESS)
  get_property(type     GLOBAL PROPERTY ${STRING_SECTION}_TYPE)
  get_property(align    GLOBAL PROPERTY ${STRING_SECTION}_ALIGN)
  get_property(subalign GLOBAL PROPERTY ${STRING_SECTION}_SUBALIGN)
  get_property(endalign GLOBAL PROPERTY ${STRING_SECTION}_ENDALIGN)
  get_property(vma      GLOBAL PROPERTY ${STRING_SECTION}_VMA)
  get_property(lma      GLOBAL PROPERTY ${STRING_SECTION}_LMA)
  get_property(noinput  GLOBAL PROPERTY ${STRING_SECTION}_NOINPUT)
  get_property(noinit   GLOBAL PROPERTY ${STRING_SECTION}_NOINIT)

  get_property(nosymbols  GLOBAL PROPERTY ${STRING_SECTION}_NOSYMBOLS)
  get_property(start_syms GLOBAL PROPERTY ${STRING_SECTION}_START_SYMBOLS)
  get_property(end_syms   GLOBAL PROPERTY ${STRING_SECTION}_END_SYMBOLS)

  get_property(parent   GLOBAL PROPERTY ${STRING_SECTION}_PARENT)

  get_property(parent_type GLOBAL PROPERTY ${parent}_OBJ_TYPE)
  if(${parent_type} STREQUAL GROUP)
    get_property(group_parent_vma GLOBAL PROPERTY ${parent}_VMA)
    get_property(group_parent_lma GLOBAL PROPERTY ${parent}_LMA)
    if(NOT DEFINED vma)
      get_property(vma GLOBAL PROPERTY ${parent}_VMA)
    endif()
    if(NOT DEFINED lma)
      get_property(lma GLOBAL PROPERTY ${parent}_LMA)
    endif()
  endif()

  if(DEFINED group_parent_vma AND DEFINED group_parent_lma)
    # Something to init
    set(part "rw ")
  else()
    set(part)
  endif()


  set_property(GLOBAL PROPERTY ILINK_CURRENT_SECTIONS)

  string(REGEX REPLACE "^[\.]" "" name_clean "${name}")
  string(REPLACE "." "_" name_clean "${name_clean}")

  # WA for 'Error[Lc036]: no block or place matches the pattern "ro data section .tdata_init"'
  if("${name_clean}" STREQUAL "tdata")
    set(TEMP "${TEMP}define block ${name_clean}_init { ro section .tdata_init };\n")
    set(TEMP "${TEMP}\"${name_clean}_init\": place in ${ILINK_CURRENT_NAME} { block ${name_clean}_init };\n\n")
  endif()

  get_property(indicies GLOBAL PROPERTY ${STRING_SECTION}_SETTINGS_INDICIES)
  # ZIP_LISTS partner
  get_property(next_indicies GLOBAL PROPERTY ${STRING_SECTION}_SETTINGS_INDICIES)
  list(POP_FRONT next_indicies first_index)

  set(first_index_section)
  set(first_index_section_name)
  if(DEFINED first_index)
    # Handle case where the first section has an offset
    get_property(first_index_offset
      GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${first_index}_OFFSET)
    get_property(keep   GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${first_index}_KEEP)
    if(DEFINED keep)
      set(root "root ")
    else()
      set(root)
    endif()
    if(DEFINED first_index_offset AND NOT first_index_offset EQUAL 0 )
      set(first_index_section_name "${name_clean}_${first_index}_offset")
      set(first_index_section
        "define ${root}section ${first_index_section_name} {};")
    else()
      set(first_index)
    endif()
  endif()

  foreach(start_symbol ${start_syms})
    set_property(GLOBAL APPEND PROPERTY ILINK_SYMBOL_ICF "${start_symbol} = ADDR(${name_clean})")
  endforeach()
  foreach(end_symbol ${end_syms})
    set_property(GLOBAL APPEND PROPERTY ILINK_SYMBOL_ICF "${end_symbol} = END(${name_clean})")
  endforeach()

  if(NOT nosymbols)
    if("${name_clean}" STREQUAL "tdata")
      set_property(GLOBAL APPEND PROPERTY ILINK_SYMBOL_ICF "__${name_clean}_start = (__iar_tls$$INIT_DATA$$Base)")
      set_property(GLOBAL APPEND PROPERTY ILINK_SYMBOL_ICF "__${name_clean}_end = (__iar_tls$$INIT_DATA$$Limit)")
    else()
      set_property(GLOBAL APPEND PROPERTY ILINK_SYMBOL_ICF "__${name_clean}_start = ADDR(${name_clean})")
      set_property(GLOBAL APPEND PROPERTY ILINK_SYMBOL_ICF "__${name_clean}_end = END(${name_clean})")
    endif()
  endif()
  # section patterns and blocks to keep { }
  set(to_be_kept "")

  if(DEFINED first_index_section)
    set(TEMP "${TEMP}${first_index_section}\n")
  endif()

  set(TEMP "${TEMP}define block ${name_clean} with fixed order")

  if(align)
    set(TEMP "${TEMP}, alignment=${align}")
  elseif(subalign)
    set(TEMP "${TEMP}, alignment = ${subalign}")
  elseif(part)
    set(TEMP "${TEMP}, alignment = input")
  else()
    set(TEMP "${TEMP}, alignment=4")
  endif()
  if(endalign)
    set(TEMP "${TEMP}, end alignment=${endalign}")
  endif()

  set(TEMP "${TEMP}\n{")

  # foreach(start_symbol ${start_syms})
  #   set(TEMP "${TEMP}\n  section ${start_symbol},")
  #   set_property(GLOBAL APPEND PROPERTY ILINK_CURRENT_SECTIONS "section ${start_symbol}")
  # endforeach()

  # if(NOT nosymbols)
  #   set(TEMP "${TEMP}\n  section __${name_clean}_start,")
  #   set_property(GLOBAL APPEND PROPERTY ILINK_CURRENT_SECTIONS "section __${name_clean}_start")
  # endif()

  list(GET indicies -1 last_index)
  list(LENGTH indicies length)

  if(NOT noinput)

    set(TEMP "${TEMP}\n  block ${name_clean}_winput")
    if(align)
      list(APPEND block_attr "alignment = ${align}")
    elseif(subalign)
      list(APPEND block_attr "alignment = ${subalign}")
    elseif(part)
      # list(APPEND block_attr "alignment = input")
    else()
      list(APPEND block_attr "alignment=4")
    endif()
    list(APPEND block_attr "fixed order")

    list(JOIN block_attr ", " block_attr_str)
    if(block_attr_str)
      set(TEMP "${TEMP} with ${block_attr_str}")
    endif()
    set(block_attr)
    set(block_attr_str)

    set(TEMP "${TEMP} { ${part}section ${name}, ${part}section ${name}.* }")
    if(${length} GREATER 0)
      set(TEMP "${TEMP},")
    endif()
    set_property(GLOBAL APPEND PROPERTY ILINK_CURRENT_SECTIONS "section ${name}")
    set_property(GLOBAL APPEND PROPERTY ILINK_CURRENT_SECTIONS "section ${name}.*")
  endif()

  foreach(idx idx_next IN ZIP_LISTS indicies next_indicies)
    get_property(align    GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_ALIGN)
    get_property(any      GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_ANY)
    get_property(first    GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_FIRST)
    get_property(keep     GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_KEEP)
    get_property(sort     GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_SORT)
    get_property(flags    GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_FLAGS)
    get_property(input    GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_INPUT)
    get_property(symbols  GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_SYMBOLS)
    # Get the next offset and use that as this ones size!
    get_property(offset   GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx_next}_OFFSET)

    if(keep)
      list(APPEND to_be_kept "block ${name_clean}_${idx}")
      foreach(setting ${input})
        list(APPEND to_be_kept "section ${setting}")
      endforeach()
    endif()
    if(DEFINED symbols)
      list(LENGTH symbols symbols_count)
      if(${symbols_count} GREATER 0)
        list(GET symbols 0 symbol_start)
      endif()
      if(${symbols_count} GREATER 1)
        list(GET symbols 1 symbol_end)
      endif()
    endif()

    if(DEFINED symbol_start)
      # set(TEMP "${TEMP}\n  section ${symbol_start},")
      # set_property(GLOBAL APPEND PROPERTY ILINK_CURRENT_SECTIONS "section ${symbol_start}")
      set_property(GLOBAL APPEND PROPERTY ILINK_SYMBOL_ICF "${symbol_start} = ADDR(${name_clean}_${idx})")
    endif()

    if(DEFINED first_index AND first_index EQUAL ${idx})
      # Create the offset
      set(TEMP "${TEMP}\n  block ${first_index_section_name}")
      list(APPEND block_attr "size = ${first_index_offset}")
      if(sort)
        if(${sort} STREQUAL NAME)
          list(APPEND block_attr "alphabetical order")
        endif()
      endif()
      if(align)
        list(APPEND block_attr "alignment = ${align}")
      elseif(subalign)
        list(APPEND block_attr "alignment = ${subalign}")
      elseif(part)
        # list(APPEND block_attr "alignment = input")
      else()
        list(APPEND block_attr "alignment=4")
      endif()
      list(APPEND block_attr "fixed order")

      list(JOIN block_attr ", " block_attr_str)
      if(block_attr_str)
        set(TEMP "${TEMP} with ${block_attr_str}")
      endif()
      set(block_attr)
      set(block_attr_str)

      set(TEMP "${TEMP} { section ${first_index_section_name} },\n")
    endif()

    # block init_100 with alphabetical order { section .z_init_EARLY_?_}
    set(TEMP "${TEMP}\n  block ${name_clean}_${idx}")
    if(DEFINED offset AND NOT offset EQUAL 0 )
      list(APPEND block_attr "size = ${offset}")
    elseif(DEFINED offset AND offset STREQUAL 0 )
      # Do nothing
    endif()
    if(sort)
      if(${sort} STREQUAL NAME)
        list(APPEND block_attr "alphabetical order")
      endif()
    endif()
    if(align)
      list(APPEND block_attr "alignment = ${align}")
    elseif(subalign)
      list(APPEND block_attr "alignment = ${subalign}")
    elseif(part)
      # list(APPEND block_attr "alignment = input")
    else()
      list(APPEND block_attr "alignment=4")
    endif()

    # LD
    # There are two ways to include more than one section:
    #
    # *(.text .rdata)
    # *(.text) *(.rdata)
    #
    # The difference between these is the order in which
    # the `.text' and `.rdata' input sections will appear in the output section.
    # In the first example, they will be intermingled,
    # appearing in the same order as they are found in the linker input.
    # In the second example, all `.text' input sections will appear first,
    # followed by all `.rdata' input sections.
    #
    # ILINK solved by adding 'fixed order'
    if(NOT sort AND NOT first)
      list(APPEND block_attr "fixed order")
    endif()

    list(JOIN block_attr ", " block_attr_str)
    if(block_attr_str)
      set(TEMP "${TEMP} with ${block_attr_str}")
    endif()
    set(block_attr)
    set(block_attr_str)

    if(empty)
      set(TEMP "${TEMP}\n  {")
      set(empty FALSE)
    endif()

    list(GET input -1 last_input)

    set(TEMP "${TEMP} {")
    if(NOT DEFINED input AND NOT any)
      set(TEMP "${TEMP} }")
    endif()

    foreach(setting ${input})
      if(first)
        set(TEMP "${TEMP} first")
        set(first "")
      endif()

      set(section_type "")

      # build for ram, no section_type
      # if("${lma}" STREQUAL "${vma}")
      #		# if("${vma}" STREQUAL "")
      #           set(section_type "")
      #		# else()
      #		#   set(section_type " readwrite")
      #		# endif()
      # elseif(NOT "${vma}" STREQUAL "")
      #		set(section_type " readwrite")
      # elseif(NOT "${lma}" STREQUAL "")
      #		set(section_type " readonly")
      # else()
      #		message(FATAL_ERROR "How to handle this? lma=${lma} vma=${vma}")
      # endif()

      set(TEMP "${TEMP}${section_type} ${part}section ${setting}")
      set_property(GLOBAL APPEND PROPERTY ILINK_CURRENT_SECTIONS "section ${setting}")
      set(section_type "")

      if("${setting}" STREQUAL "${last_input}")
        set(TEMP "${TEMP} }")
      else()
        set(TEMP "${TEMP}, ")
      endif()

      # set(TEMP "${TEMP}\n    *.o(${setting})")
    endforeach()

    if(any)
      if(NOT flags)
        message(FATAL_ERROR ".ANY requires flags to be set.")
      endif()
      set(ANY_FLAG "")
      foreach(flag ${flags})
        # if("${flag}" STREQUAL +RO OR "${flag}" STREQUAL +XO)
        #   set(ANY_FLAG "readonly")
        # # elseif("${flag}" STREQUAL +RW)
        # #   set(ANY_FLAG "readwrite")
        # else
        if("${flag}" STREQUAL +ZI)
          set(ANY_FLAG "zeroinit")
          set_property(GLOBAL APPEND PROPERTY ILINK_CURRENT_SECTIONS "${ANY_FLAG}")
        endif()
      endforeach()
      set(TEMP "${TEMP} ${ANY_FLAG} }")
    endif()

    if(DEFINED symbol_end)
      # set(TEMP "${TEMP},\n  section ${symbol_end}")
      # set_property(GLOBAL APPEND PROPERTY ILINK_CURRENT_SECTIONS "section ${symbol_end}")
      set_property(GLOBAL APPEND PROPERTY ILINK_SYMBOL_ICF "${symbol_end} = END(${name_clean}_${idx})")
    endif()
    if(${length} GREATER 0)
      if(NOT "${idx}" STREQUAL "${last_index}")
        set(TEMP "${TEMP},")
      elseif()
      endif()
    endif()

    set(symbol_start)
    set(symbol_end)
  endforeach()
  set(next_indicies)

  set(last_index)
  set(last_input)
  set(TEMP "${TEMP}")

  # if(NOT nosymbols)
  #   set(TEMP "${TEMP},\n  section __${name_clean}_end")
  #   set_property(GLOBAL APPEND PROPERTY ILINK_CURRENT_SECTIONS "section __${name_clean}_end")
  # endif()

  # foreach(end_symbol ${end_syms})
  #   set(TEMP "${TEMP},\n  section ${end_symbol}")
  #   set_property(GLOBAL APPEND PROPERTY ILINK_CURRENT_SECTIONS "section ${end_symbol}")
  # endforeach()

  set(TEMP "${TEMP}\n};")

  get_property(type GLOBAL PROPERTY ${parent}_OBJ_TYPE)
  if(${type} STREQUAL REGION)
    get_property(name GLOBAL PROPERTY ${parent}_NAME)
    get_property(address GLOBAL PROPERTY ${parent}_ADDRESS)
    get_property(size GLOBAL PROPERTY ${parent}_SIZE)

  endif()

  get_property(current_sections GLOBAL PROPERTY ILINK_CURRENT_SECTIONS)

  if(DEFINED group_parent_vma AND DEFINED group_parent_lma)
    if(${noinit})
      list(JOIN current_sections ", " SELECTORS)
      set(TEMP "${TEMP}\ndo not initialize {\n${SELECTORS}\n};")
    else()
      # Generate the _init block and the initialize manually statement.
      # Note that we need to have the X_init block defined even if we have
      # no sections, since there will come a "place in XXX" statement later.

      # "${TEMP}" is there too keep the ';' else it will be a list
      string(REGEX REPLACE "(block[ \t\r\n]+)([^ \t\r\n]+)" "\\1\\2_init" INIT_TEMP "${TEMP}")
      string(REGEX REPLACE "(rw)([ \t\r\n]+)(section[ \t\r\n]+)([^ \t\r\n,]+)" "\\1\\2\\3\\4_init" INIT_TEMP "${INIT_TEMP}")
      string(REGEX REPLACE "(rw)([ \t\r\n]+)(section[ \t\r\n]+)" "ro\\2\\3" INIT_TEMP "${INIT_TEMP}")
      string(REGEX REPLACE "alphabetical order, " "" INIT_TEMP "${INIT_TEMP}")
      string(REGEX REPLACE "{ readwrite }" "{ }" INIT_TEMP "${INIT_TEMP}")

      # If any content is marked as keep, is has to be applied to the init block
      # too, esp. for blocks that are not referenced (e.g. empty blocks wiht min_size)
      if(to_be_kept)
        list(APPEND to_be_kept "block ${name_clean}_init")
      endif()
      set(TEMP "${TEMP}\n${INIT_TEMP}\n")
      if(DEFINED current_sections)
        set(TEMP "${TEMP}\ninitialize manually with copy friendly\n")
        set(TEMP "${TEMP}{\n")
        foreach(section ${current_sections})
          set(TEMP "${TEMP}  ${section},\n")
        endforeach()
        set(TEMP "${TEMP}};")
        set(current_sections)
      endif()
    endif()
  endif()

  # Finally, add the keeps.
  if(to_be_kept)
    list(JOIN to_be_kept ", " K)
    set(TEMP "${TEMP}\nkeep { ${K} };\n")
  endif()

  set(${STRING_STRING} "${${STRING_STRING}}\n${TEMP}\n" PARENT_SCOPE)
endfunction()

function(symbol_to_string)
  cmake_parse_arguments(STRING "" "SYMBOL;STRING" "" ${ARGN})

  get_property(name     GLOBAL PROPERTY ${STRING_SYMBOL}_NAME)
  get_property(expr     GLOBAL PROPERTY ${STRING_SYMBOL}_EXPR)
  get_property(size     GLOBAL PROPERTY ${STRING_SYMBOL}_SIZE)
  get_property(symbol   GLOBAL PROPERTY ${STRING_SYMBOL}_SYMBOL)
  get_property(subalign GLOBAL PROPERTY ${STRING_SYMBOL}_SUBALIGN)

  string(REPLACE "\\" "" expr "${expr}")
  string(REGEX MATCHALL "@([^@]*)@" match_res ${expr})

  foreach(match ${match_res})
    string(REPLACE "@" "" match ${match})
    get_property(symbol_val GLOBAL PROPERTY SYMBOL_TABLE_${match})
    string(REPLACE "@${match}@" "${match}" expr ${expr})
  endforeach()

  list(LENGTH match_res match_res_count)

  if(match_res_count)
    if(${match_res_count} GREATER 1)
      set(${STRING_STRING}
        "${${STRING_STRING}}define image symbol ${symbol} = ${expr};\n"
        )
    else()
      if((expr MATCHES "Base|Limit|Length") OR (expr MATCHES "ADDR\\(|END\\(|SIZE\\("))
        # Anything like $$Base/$$Limit/$$Length should be an image symbol
        if( "${symbol}" STREQUAL "__tdata_size")
          # This will handle the alignment of the TBSS block
          set(${STRING_STRING}
            "${${STRING_STRING}}define image symbol ${symbol}=(__iar_tls$$INIT_DATA$$Limit-__iar_tls$$INIT_DATA$$Base);\n"
            )
        elseif( "${symbol}" STREQUAL "__tbss_size")
          # This will handle the alignment of the TBSS block by
          # pre-padding bytes
          set(${STRING_STRING}
            "${${STRING_STRING}}define image symbol ${symbol}=((tbss$$Limit-__iar_tls$$DATA$$Base)-(__iar_tls$$INIT_DATA$$Limit-__iar_tls$$INIT_DATA$$Base));\n"
            )
        else()
          set(${STRING_STRING}
            "${${STRING_STRING}}define image symbol ${symbol} = ${expr};\n"
            )
        endif()
      else()
        list(GET match_res 0 match)
        string(REPLACE "@" "" match ${match})
        get_property(symbol_val GLOBAL PROPERTY SYMBOL_TABLE_${match})
        if(symbol_val)
          set(${STRING_STRING}
            "${${STRING_STRING}}define image symbol ${symbol} = ${expr};\n"
            )
        else()
          # Treatmen of "zephyr_linker_symbol(SYMBOL z_arm_platform_init EXPR "@SystemInit@")"
          set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_FILE
            "--redirect ${symbol}=${expr}\n"
            )
        endif()
      endif()
    endif()
  else()
    # Handle things like ADDR(.ramfunc)
    if(${expr} MATCHES "^[A-Za-z]?ADDR\\(.+\\)")
      # string(REGEX REPLACE "^[A-Za-z]?ADDR\\((.+)\\)" "(\\1$$Base)" expr ${expr})
      set(${STRING_STRING}
        "${${STRING_STRING}}define image symbol ${symbol} = ${expr};\n"
        )
    else()
      set(${STRING_STRING}
        "${${STRING_STRING}}define exported symbol ${symbol} = ${expr};\n"
        )
    endif()
  endif()

  set(${STRING_STRING} ${${STRING_STRING}} PARENT_SCOPE)
endfunction()

include(${CMAKE_CURRENT_LIST_DIR}/../linker_script_common.cmake)

if(DEFINED STEERING_FILE)
  get_property(steering_content GLOBAL PROPERTY SYMBOL_STEERING_FILE)
  file(WRITE ${STEERING_FILE}  "/* AUTO-GENERATED - Do not modify\n")
  file(APPEND ${STEERING_FILE} " * AUTO-GENERATED - All changes will be lost\n")
  file(APPEND ${STEERING_FILE} " */\n")

  file(APPEND ${STEERING_FILE} ${steering_content})
endif()
