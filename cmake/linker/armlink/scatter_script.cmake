cmake_minimum_required(VERSION 3.17)

set(SORT_TYPE_NAME Lexical)


function(memory_content)
  cmake_parse_arguments(MC "" "CONTENT;NAME;START;SIZE;FLAGS" "" ${ARGN})

  set(TEMP)
  if(MC_NAME)
    set(TEMP "${TEMP} ${MC_NAME}")
  endif()

  if(MC_FLAGS)
    set(TEMP "${TEMP} (${MC_FLAGS})")
  endif()

  if(MC_START)
    set(TEMP "${TEMP} : ORIGIN = (${MC_START})")
  endif()

  if(MC_SIZE)
    set(TEMP "${TEMP}, LENGTH = (${MC_SIZE})")
  endif()

  set(${MC_CONTENT} "${${MC_CONTENT}}\n${TEMP}" PARENT_SCOPE)
endfunction()

function(section_content)
  cmake_parse_arguments(SEC "" "REGION_NAME;REGION_FLAGS;REGION_ADDRESS;CONTENT;NAME;ADDRESS;TYPE;ALIGN;SUBALIGN;VMA;LMA;NOINPUT;NOINIT" "PASS" ${ARGN})

  if("${SEC_REGION_NAME}" STREQUAL "${SEC_VMA}"
     AND NOT SEC_LMA
     OR  "${SEC_REGION_NAME}" STREQUAL "${SEC_LMA}"
  )
    if(DEFINED SEC_PASS AND NOT "${PASS}" IN_LIST SEC_PASS)
      # This section is not active in this pass, ignore.
      return()
    endif()

    # SEC_NAME is required, test for that.
    string(REGEX REPLACE "^[\.]" "" SEC_NAME_CLEAN "${SEC_NAME}")
    string(REPLACE "." "_" SEC_NAME_CLEAN "${SEC_NAME_CLEAN}")
#    set(SEC_NAME ${SEC_NAME_CLEAN})
    set(TEMP "  ${SEC_NAME_CLEAN}")
    if(SEC_ADDRESS)
      set(TEMP "${TEMP} ${SEC_ADDRESS}")
    elseif(SEC_REGION_ADDRESS)
      set(TEMP "${TEMP} ${SEC_REGION_ADDRESS}")
    else()
      set(TEMP "${TEMP} +0")
    endif()

    if(SEC_NOINIT)
      # Currently we simply uses offset +0, but we must support offset defined
      # externally.
      set(TEMP "${TEMP} UNINIT")
    endif()

    if(SEC_SUBALIGN)
      # Currently we simply uses offset +0, but we must support offset defined
      # externally.
      set(TEMP "${TEMP} ALIGN ${SEC_SUBALIGN}")
    endif()


    string(TOUPPER ${REGION_${SEC_VMA}_FLAGS} VMA_FLAGS)
    if(NOT SEC_NOINPUT)
      set(TEMP "${TEMP}\n  {")

      if("${SEC_TYPE}" STREQUAL NOLOAD)
        set(TEMP "${TEMP}\n    *.o(${SEC_NAME}*)")
        set(TEMP "${TEMP}\n    *.o(${SEC_NAME}*.*)")
      elseif(VMA_FLAGS)
        # ToDo: Proper names as provided by armclang
  #      set(TEMP "${TEMP}\n    *.o(${SEC_NAME}*, +${VMA_FLAGS})")
  #      set(TEMP "${TEMP}\n    *.o(${SEC_NAME}*.*, +${VMA_FLAGS})")
        set(TEMP "${TEMP}\n    *.o(${SEC_NAME}*)")
        set(TEMP "${TEMP}\n    *.o(${SEC_NAME}*.*)")
      else()
        set(TEMP "${TEMP}\n    *.o(${SEC_NAME}*)")
        set(TEMP "${TEMP}\n    *.o(${SEC_NAME}*.*)")
      endif()
    else()
      set(empty TRUE)
    endif()

    foreach(group "" "_SORT")
      # message("processing <${group}>")
      set(INDEX_KEY    SECTION_${SEC_NAME}${group}_INDEX)
      set(SETTINGS_KEY SECTION_${SEC_NAME}_SETTINGS${group})

      # message("Index key: ${INDEX_KEY}=${${INDEX_KEY}}")
      # message("settings : ${SETTINGS_KEY}_0=${${SETTINGS_KEY}_0}")

      if(DEFINED ${INDEX_KEY})
        foreach(idx RANGE 0 ${${INDEX_KEY}})
          cmake_parse_arguments(SETTINGS "" "ANY;KEEP;FIRST;ALIGN;SORT" "FLAGS;INPUT;SYMBOLS" ${${SETTINGS_KEY}_${idx}})

          if(SETTINGS_SORT)
            if(empty)
              set(TEMP "${TEMP} EMPTY 0x0\n  {")
              set(empty FALSE)
            endif()
            set(TEMP "${TEMP}\n  }")
            set(TEMP "${TEMP}\n  ${SEC_NAME_CLEAN}_${idx} +0 SORTTYPE ${SORT_TYPE_${SETTINGS_SORT}}\n  {")
            # Symbols translation here.
            set(steering_postfixes Base Limit)
            foreach(symbol ${SETTINGS_SYMBOLS})
              list(POP_FRONT steering_postfixes postfix)
              set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C
                "Image$$${SEC_NAME_CLEAN}_${idx}$$${postfix}"
              )
              set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_FILE
                "RESOLVE ${symbol} AS Image$$${SEC_NAME_CLEAN}_${idx}$$${postfix}\n"
              )
            endforeach()
          elseif(DEFINED SETTINGS_SYMBOLS AND ${INDEX_KEY} EQUAL 1 AND SEC_NOINPUT)
            # Symbols translation here.
            set(steering_postfixes Base Limit)
            foreach(symbol ${SETTINGS_SYMBOLS})
              list(POP_FRONT steering_postfixes postfix)
              set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C
                "Image$$${SEC_NAME_CLEAN}$$${postfix}"
              )
              set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_FILE
                "RESOLVE ${symbol} AS Image$$${SEC_NAME_CLEAN}$$${postfix}\n"
              )
            endforeach()
          elseif(DEFINED SETTINGS_SYMBOLS)
            message(WARNING "SYMBOL defined with: ${SETTINGS_INPUT}---${SETTINGS_SYMBOLS} at Section: ${SEC_NAME}")
          endif()

          if(empty)
            set(TEMP "${TEMP}\n  {")
            set(empty FALSE)
          endif()

          #if(SETTINGS_INPUT)
          foreach(setting ${SETTINGS_INPUT})
            #set(SETTINGS ${SETTINGS_INPUT})

            if(SETTINGS_ALIGN)
               set(SETTINGS "${setting}, OVERALIGN ${SETTINGS_ALIGN}")
            endif()

            #if(SETTINGS_KEEP)
            # armlink has --keep=<section_id>, but is there an scatter equivalant ?
            #endif()

            if(SETTINGS_FIRST)
               set(setting "${setting}, +First")
               set(SETTINGS_FIRST "")
            endif()

            set(TEMP "${TEMP}\n    *.o(${setting})")
          #endif()
          endforeach()

          if(SETTINGS_ANY)
            if(NOT SETTINGS_FLAGS)
              message(FATAL_ERROR ".ANY requires flags to be set.")
            endif()
            string(REPLACE ";" " " SETTINGS_FLAGS "${SETTINGS_FLAGS}")

            set(TEMP "${TEMP}\n    .ANY (${SETTINGS_FLAGS})")
          endif()
        endforeach()
      endif()
    endforeach()

    if(SECTION_${SEC_NAME}_SORT_INDEX)
      set(TEMP "${TEMP}\n  }")
      set(TEMP "${TEMP}\n  ${SEC_NAME_CLEAN}_end +0 EMPTY 0x0\n  {")
    endif()


    #  if(SEC_TYPE)
    #    set(TEMP "${TEMP} (${SEC_TYPE})")
    #  endif()


    set(TEMP "${TEMP}")
    # ToDo: add patterns here.

    if("${SEC_TYPE}" STREQUAL BSS)
      set(ZI "$$ZI")
    endif()

    # Symbols translation here.
    set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C "Image$$${SEC_NAME_CLEAN}${ZI}$$Base")
    set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C "Image$$${SEC_NAME_CLEAN}${ZI}$$Length")
    set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C "Load$$${SEC_NAME_CLEAN}${ZI}$$Base")

    set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_FILE
      "RESOLVE __${SEC_NAME_CLEAN}_start AS Image$$${SEC_NAME_CLEAN}${ZI}$$Base\n"
      "RESOLVE __${SEC_NAME_CLEAN}_size AS Image$$${SEC_NAME_CLEAN}${ZI}$$Length\n"
      "RESOLVE __${SEC_NAME_CLEAN}_load_start AS Load$$${SEC_NAME_CLEAN}${ZI}$$Base\n"
      "EXPORT  __${SEC_NAME_CLEAN}_start AS __${SEC_NAME_CLEAN}_start\n"
    )

    set(__${SEC_NAME_CLEAN}_start      "${SEC_NAME_CLEAN}" PARENT_SCOPE)
    set(__${SEC_NAME_CLEAN}_size       "${SEC_NAME_CLEAN}" PARENT_SCOPE)
    set(__${SEC_NAME_CLEAN}_load_start "${SEC_NAME_CLEAN}" PARENT_SCOPE)

    if(DEFINED ${INDEX_KEY})
      set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C "Image$$${SEC_NAME_CLEAN}_end$$Limit")
      set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_FILE
        "RESOLVE __${SEC_NAME_CLEAN}_end AS Image$$${SEC_NAME_CLEAN}_end$$Limit\n"
      )
      set(__${SEC_NAME_CLEAN}_end        "${SEC_NAME_CLEAN}_end" PARENT_SCOPE)
    else()
      set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C "Image$$${SEC_NAME_CLEAN}${ZI}$$Limit")
      set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_FILE
        "RESOLVE __${SEC_NAME_CLEAN}_end AS Image$$${SEC_NAME_CLEAN}${ZI}$$Limit\n"
      )
      set(__${SEC_NAME_CLEAN}_end        "${SEC_NAME_CLEAN}" PARENT_SCOPE)
    endif()


    set(TEMP "${TEMP}\n  }")

    #  if(SEC_LMA)
    #    set(TEMP "${TEMP} > ${SEC_LMA}")
    #  endif()

    set(${SEC_CONTENT} "${${SEC_CONTENT}}\n${TEMP}\n" PARENT_SCOPE)
  endif()
endfunction()



# Strategy:
# - Find all
#set(OUT "MEMORY\n{")


# Sorting the memory sections in ascending order.
foreach(region ${MEMORY_REGIONS})
  if("${region}" MATCHES "^{(.*)}$")
    cmake_parse_arguments(REGION "" "START" "" ${CMAKE_MATCH_1})
    math(EXPR start_dec "${REGION_START}" OUTPUT_FORMAT DECIMAL)
    set(region_${start_dec} ${region})
    string(REPLACE ";" "\;" region_${start_dec} "${region_${start_dec}}")
    list(APPEND region_sort ${start_dec})
  endif()
endforeach()

list(SORT region_sort COMPARE NATURAL)
set(MEMORY_REGIONS_SORTED)
foreach(region_start ${region_sort})
    list(APPEND MEMORY_REGIONS_SORTED "${region_${region_start}}")
endforeach()
# sorting complete.


foreach(region ${MEMORY_REGIONS_SORTED})
  string(REGEX MATCH "^{(.*)}$" ignore "${region}")
  cmake_parse_arguments(REGION "" "NAME;START;FLAGS" "" ${CMAKE_MATCH_1})
  set(REGION_${REGION_NAME}_START ${REGION_START})
  set(REGION_${REGION_NAME}_FLAGS ${REGION_FLAGS})
  list(APPEND MEMORY_REGIONS_NAMES ${REGION_NAME})
endforeach()

foreach(section ${SECTIONS})
  if("${section}" MATCHES "^{(.*)}$")
    cmake_parse_arguments(SEC "" "VMA;LMA" "" ${CMAKE_MATCH_1})
    string(REPLACE ";" "\;" section "${section}")
    if(NOT SEC_LMA)
      list(APPEND SECTIONS_${SEC_VMA} "${section}")
    else()
      list(APPEND SECTIONS_${SEC_LMA}_${SEC_VMA} "${section}")
    endif()
  endif()
endforeach()





foreach(settings ${SECTION_SETTINGS})
  if("${settings}" MATCHES "^{(.*)}$")
    cmake_parse_arguments(SETTINGS "" "PASS;SECTION;SORT" "" ${CMAKE_MATCH_1})

    if(DEFINED SETTINGS_PASS AND NOT "${PASS}" IN_LIST SETTINGS_PASS)
      # This section setting is not active in this pass, ignore.
      continue()
    endif()

    if(SETTINGS_SORT)
      set(INDEX_KEY    SECTION_${SETTINGS_SECTION}_SORT_INDEX)
      set(SETTINGS_KEY SECTION_${SETTINGS_SECTION}_SETTINGS_SORT)
      # message("Adding to: SECTION_${SETTINGS_SECTION}_SETTINGS_SORT")
    else()
      set(INDEX_KEY    SECTION_${SETTINGS_SECTION}_INDEX)
      set(SETTINGS_KEY SECTION_${SETTINGS_SECTION}_SETTINGS)
    endif()

    if(NOT ${INDEX_KEY})
      set(${INDEX_KEY} 0)
    endif()

    set(${SETTINGS_KEY}_${${INDEX_KEY}} ${CMAKE_MATCH_1})
    # message("Adding to: ${SETTINGS_KEY}_${${INDEX_KEY}}=${CMAKE_MATCH_1}")
    math(EXPR ${INDEX_KEY} "${${INDEX_KEY}} + 1")
  endif()
endforeach()

foreach(region ${MEMORY_REGIONS_SORTED})
  string(REGEX MATCH "^{(.*)}$" ignore "${region}")
  cmake_parse_arguments(REGION "" "NAME;START;FLAGS;SIZE" "" ${CMAKE_MATCH_1})
  if(DEFINED REGION_SIZE)
    if(${REGION_SIZE} MATCHES "^([0-9]*)[kK]$")
      math(EXPR REGION_SIZE "1024 * ${CMAKE_MATCH_1}" OUTPUT_FORMAT HEXADECIMAL)
    elseif(${REGION_SIZE} MATCHES "^([0-9]*)[mM]$")
      math(EXPR REGION_SIZE "1024 * 1024 * ${CMAKE_MATCH_1}" OUTPUT_FORMAT HEXADECIMAL)
    elseif(NOT (${REGION_SIZE} MATCHES "^([0-9]*)$" OR ${REGION_SIZE} MATCHES "^0x([0-9a-fA-F]*)$"))
      # ToDo: Handle hex sizes
      message(FATAL_ERROR "SIZE format is onknown.")
    endif()
  endif()
  set(OUT)
  set(ADDRESS ${REGION_START})
  foreach(section ${SECTIONS_${REGION_NAME}})
    string(REGEX MATCH "^{(.*)}$" ignore "${section}")
    section_content(REGION_NAME ${REGION_NAME} REGION_FLAGS ${REGION_FLAGS} REGION_ADDRESS ${ADDRESS} CONTENT OUT ${CMAKE_MATCH_1})
    set(SECTIONS_${REGION_NAME})
    set(ADDRESS "+0")
  endforeach()

  foreach(section ${SECTIONS_${REGION_NAME}_${REGION_NAME}})
    string(REGEX MATCH "^{(.*)}$" ignore "${section}")
    section_content(REGION_NAME ${REGION_NAME} REGION_FLAGS ${REGION_FLAGS} CONTENT OUT ${CMAKE_MATCH_1})
    set(SECTIONS_${REGION_NAME}_${REGION_NAME})
  endforeach()

  foreach(vma_region ${MEMORY_REGIONS_NAMES})
    set(ADDRESS ${REGION_${vma_region}_START})
    foreach(section ${SECTIONS_${REGION_NAME}_${vma_region}})
      string(REGEX MATCH "^{(.*)}$" ignore "${section}")
      section_content(REGION_NAME ${REGION_NAME} REGION_FLAGS ${REGION_FLAGS} REGION_ADDRESS ${ADDRESS} CONTENT OUT ${CMAKE_MATCH_1})
      set(ADDRESS "+0")
      set(SECTIONS_${REGION_NAME}_${vma_region})
    endforeach()
  endforeach()

  foreach(symbol ${SYMBOLS})
    if("${symbol}" MATCHES "^{(.*)}$")
      cmake_parse_arguments(SYM "" "EXPR;SIZE;SUBALIGN;SYMBOL" "" ${CMAKE_MATCH_1})
      string(REPLACE "\\" "" SYM_EXPR "${SYM_EXPR}")
      string(REGEX MATCHALL "%([^%]*)%" MATCH_RES ${SYM_EXPR})
      foreach(match ${MATCH_RES})
        string(REPLACE "%" "" match ${match})
        string(REPLACE "%${match}%" "ImageBase(${${match}})" SYM_EXPR ${SYM_EXPR})
      endforeach()

      if(DEFINED SYM_SUBALIGN)
        set(SYM_SUBALIGN "ALIGN ${SYM_SUBALIGN}")
      endif()

      if(NOT DEFINED SYM_SIZE)
        set(SYM_SIZE "0x0")
      endif()

      set(OUT "${OUT}\n  ${SYM_SYMBOL} ${SYM_EXPR} ${SYM_SUBALIGN} EMPTY ${SYM_SIZE}\n  {\n  }\n")
      set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C "Image$$${SYM_SYMBOL}$$Base")

      set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_FILE
        "RESOLVE ${SYM_SYMBOL} AS Image$$${SYM_SYMBOL}$$Base\n"
      )
      set(${SYM_SYMBOL} "${SYM_SYMBOL}")
    endif()
  endforeach()
  set(SYMBOLS "")

  if(NOT "${OUT}" STREQUAL "")
    set(SCATTER_OUT "${SCATTER_OUT}\n${REGION_NAME} ${REGION_START} ${REGION_SIZE}\n{")
    set(SCATTER_OUT "${SCATTER_OUT}\n${OUT}\n}")
  endif()
endforeach()



#
#
#
#    memory_content(CONTENT OUT ${CMAKE_MATCH_1})
#  endif()
#endforeach()
#set(OUT "${OUT}\n}\n")
#
#
#foreach(section ${SECTIONS})
##  message("${section}")
#  if("${section}" MATCHES "^{(.*)}$")
#    message("${section}")
#    set(FLASH)
#    section_content(${CMAKE_MATCH_1})
#  endif()
#endforeach()

if(DEFINED STEERING_C)
  get_property(symbols_c GLOBAL PROPERTY SYMBOL_STEERING_C)
  file(WRITE ${STEERING_C} "/* AUTO-GENERATED - Do not modify\n")
  file(APPEND ${STEERING_C} " * AUTO-GENERATED - All changes will be lost\n")
  file(APPEND ${STEERING_C} " */\n")
  foreach(symbol ${symbols_c})
    file(APPEND ${STEERING_C} "extern char ${symbol}[];\n")
  endforeach()

  file(APPEND ${STEERING_C} "\nint __armlink_symbol_steering(void) {\n")
  file(APPEND ${STEERING_C} "\treturn\n")
  foreach(symbol ${symbols_c})
    file(APPEND ${STEERING_C} "\t\t${OPERAND} (int)${symbol}\n")
    set(OPERAND "&")
  endforeach()
  file(APPEND ${STEERING_C} "\t;\n}\n")
endif()

if(DEFINED STEERING_FILE)
  get_property(steering_content GLOBAL PROPERTY SYMBOL_STEERING_FILE)
  file(WRITE ${STEERING_FILE}  "; AUTO-GENERATED - Do not modify\n")
  file(APPEND ${STEERING_FILE} "; AUTO-GENERATED - All changes will be lost\n")
  file(APPEND ${STEERING_FILE} ${steering_content})
endif()




if(OUT_FILE)
  file(WRITE ${OUT_FILE} "${SCATTER_OUT}")
else()
  message("${SCATTER_OUT}")
endif()
