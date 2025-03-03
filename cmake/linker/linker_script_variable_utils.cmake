# Copyright (c) 2025 IAR Systems AB
#
# SPDX-License-Identifier: Apache-2.0

# This file contains utilities for handling @variables@ for the linker script
# generator
# There are some tests that cna be run like so:
# cmake -D TEST_AT_VARIABLE_UTILS=1 -P linker_script_variable_utils.cmake -B .

# @ variables are used for the cmake linker generator to refer to values not
# known at configure time. this is provides the glue needed to bring
# information together from the different scripts and passes, allowing e.g.
# the size of kobject from gen_kobject.py to be used when defining sections
#
# The syntax is:
# pattern <= @(<variable-name>[:undef=<pattern>][:<def-name>=<pattern>]*)@
# variable-name <= pattern
# def-name <= cmake-variable-name
#
# each of the <def-name>=<pattern> are used to set variables during the
# evaluation of this expression.
# As a special case unedf=<pattern> is used if <variable-name> is not defined
#
# As a shortcut the parenthesis in @()@ can be omitted if there are no nested
# patterns. (e.g) @foo:undef=1@
#
# The value of a variable @FOO@ is stored in ordinary (scoped)
# cmake-variable AT_VAR_FOO, or if it is not defined FOO.
#
# Example variable definitions:               Expands to:
# V1 = "value"                                "value"
# V2 = "a ref to @V1@"                        "a ref to value"
# V2a = "shortcut @V1@ or @(V1)@"             "shortcut value or value"
# V3 = "A maybe ref @MAYBE:undef=foo@"        "A maybe ref foo"
# V4 = "A recursive @(@OTHER:undef=V1@)@"     "A recursive value"
# DOUBLE ="@arg@@arg@"
# V5 = "Two @(@DOUBLE@:arg=bar)@"             "Two barbar"
# REF = "@(@ref@@index@)@"
# V6 = "@V2@ is @REF:ref=V:index=2@"          "a ref to value is a ref to value"
#

# Expand variable expressions in input non-expressions are copied over
# Expressions according to above: @(name[:undef=<str>][:name1=<value>])@
# where:
#    name - the name of a variable ([a-zA-Z_-]+) or a @-variable
#           expression expanding to a name
#    undef=<str> - uses <str> if no definition of name is found
#    name1=<value> - define @name1@=<value> during expansion of name.
# Where the undef=VALUE gets picked if we dont find name
# and foo=bar defines a (local) variable in the current expansion
function(expand_variables input output_var)
  set(result "")

  while(NOT input STREQUAL "")
    string(FIND "${input}" "@" pos)
    # Move everything from start of input to pos into result:
    string(SUBSTRING "${input}" 0 ${pos} before_at)
    string(APPEND result "${before_at}")
    if(pos EQUAL -1)
      # no more at found, done
      break()
    else()
      # We now have a @ at pos, check if it is @( or @
      string(SUBSTRING "${input}" ${pos} 2 opening)
      if(opening STREQUAL "@(")
        math(EXPR var_start "${pos} + 2")
        string(SUBSTRING "${input}" ${var_start} -1 var_expr_input)
        parse_paren_at_expression("${var_expr_input}" var_expr input_remainder)
      else()
        # The case @foo@
        math(EXPR var_start "${pos} + 1")
        string(SUBSTRING "${input}" ${var_start} -1 var_expr_input)
        parse_single_at_expression("${var_expr_input}" var_expr input_remainder)
      endif()
      # message("Found @ at ${pos} expr = \"${var_expr}\" remainder: \"${input_remainder}\"")
      #Now var_expr is the content inside the expression separators
      expand_parsed_expression("${var_expr}" var_result)
      # Let someone on the outside preview the expression expansion. This is
      # used to capture expressions for the between-linker-passes-evaluation
      preview_var_replacement(value "${var_expr}" "${var_result}")

      string(APPEND result ${var_result})
      set(input "${input_remainder}")
    endif()
  endwhile()
  set(${output_var} "${result}" PARENT_SCOPE)
endfunction()

# ----------------------------------------------------------------------------
# Utility functions
#
# like string(FIND input substring result) but with a starting position
function(find_next_occurrence input substring result start)
  string(SUBSTRING "${input}" ${start} -1 part)
  string(FIND "${part}" "${substring}" pos)
  if(NOT pos EQUAL -1)
    math(EXPR pos "${pos} + ${start}")
  endif()
  set(${result} ${pos} PARENT_SCOPE)
endfunction()

# find the first )@ @ or @( after starting_pos
# result_pos   - is the index AFTER that marker
# result_marker- is the marker
function(find_next_opening_or_close input starting_pos result_pos result_marker)
  find_next_occurrence("${input}" "@" at ${starting_pos})
  if(at EQUAL -1) #nothing found
    set(${result_marker} "" PARENT_SCOPE)
    set(${result_pos} -1 PARENT_SCOPE)
  else()
    # is it a )@ (after starting_pos)?
    if(at GREATER starting_pos)
      math(EXPR paren_at "${at} - 1")
      string(SUBSTRING "${input}" ${paren_at} 2 maybe_closing)
      if(maybe_closing STREQUAL ")@")
        #found ")@" at paren_at
        math(EXPR pos "${at} + 1")
        set(${result_marker} ")@" PARENT_SCOPE)
        set(${result_pos} ${pos} PARENT_SCOPE)
        return()
      endif()
    endif()
    string(SUBSTRING "${input}" ${at} 2 at_str)
    if(at_str STREQUAL "@(")
      math(EXPR pos "${at} + 2")
    else()
      math(EXPR pos "${at} + 1")
      set(at_str "@")
    endif()
    set(${result_marker} ${at_str} PARENT_SCOPE)
    set(${result_pos} ${pos} PARENT_SCOPE)
  endif()
endfunction()

# input starts with a var expression that started with a @
# on return var_expr is the expression split into parts
#           remainder is the rest of input (excluding the closing @)
function(parse_single_at_expression input var_expr remainder)
  string(FIND "${input}" "@" end_at)
  if(end_at EQUAL -1)
    message(FATAL_ERROR "Syntax error in @ expression \"${input}\"")
  else()
    string(SUBSTRING "${input}" 0 ${end_at} var_str)
    math(EXPR remainder_start "${end_at} + 1")
    string(SUBSTRING "${input}" ${remainder_start} -1 input_remainder)

    #split the expression into a parts list.
    string(REPLACE ":" ";" var_str "${var_str}")
  endif()
  set(${var_expr} "${var_str}" PARENT_SCOPE)
  set(${remainder} "${input_remainder}" PARENT_SCOPE)
endfunction()

# input starts with a var expression that started with a @(
# on return var_expr is the expression split into parts
#           remainder is the rest of input (excluding the closing @)
function(parse_paren_at_expression input var_expr remainder )
  # Find the matching )@ looking out for enclosed @expr@ and @()@.
  set(pos 0)
  string(LENGTH "${input}" length)
  set(nesting_depth 0)

  # To split the expression into parts, we need to replace , with ; (cmake
  # list separator), but only if we are not inside a nested @-expression.
  # We build this list as a string, appending bits to it after maybe
  # list-splitting
  set(parts)
  set(nesting_start)

  while(pos LESS_EQUAL length)
    find_next_opening_or_close("${input}" ${pos} next_pos marker)
    if(marker STREQUAL "") # nothing found
      break()
    endif()

    if(nesting_depth EQUAL 0)
      # We are outside of any nested expressions, so the bit up to the marker
      # should be considered for splitting into expression parts.
      string(LENGTH "${marker}" marker_length )
      math(EXPR marker_start_pos "${next_pos} - ${marker_length}")
      math(EXPR part_len "${marker_start_pos} - ${pos}")
      string(SUBSTRING "${input}" ${pos} ${part_len} part_str)
      string(REPLACE ":" ";" part_str "${part_str}")
      string(APPEND parts "${part_str}")
      if(marker STREQUAL ")@") # final closing
      else()
        # This is a nesting start
        set(nesting_start ${marker_start_pos})
      endif()
    endif()

    if(marker STREQUAL "@")
      #Found the start of @foo@ so grab everything to the next @
      find_next_occurrence("${input}" "@" at_end_pos ${next_pos} )
      if(at_end_pos EQUAL -1)
        break()
      else()
        #swallow the enclosed @foo@ by continuing after at_end_pos
        math(EXPR next_pos "${at_end_pos} + 1")
      endif()
    elseif(marker STREQUAL "@(")
      math(EXPR nesting_depth "${nesting_depth} + 1")
    elseif(marker STREQUAL ")@")
      #this will set nesting_depth to -1 when we find our end marker.
      math(EXPR nesting_depth "${nesting_depth} - 1")
    else()
      message(FATAL_ERROR "unhandled marker ${marker} in \"${input}\"")
    endif()

    if(nesting_depth LESS_EQUAL 0 AND DEFINED nesting_start)
      # we reached a non-nesting state so add the nested bits to the parts
      math(EXPR nested_length "${next_pos} - ${nesting_start}")
      string(SUBSTRING "${input}" ${nesting_start} ${nested_length} nested)
      # Yes, string append since this is nested and should not be split into
      # separate parts
      string(APPEND parts "${nested}")
      set(nesting_start)
    endif()

    if(nesting_depth EQUAL -1)
      # found the matching end marker!
      math(EXPR expr_end "${next_pos} - 2")
      string(SUBSTRING "${input}" ${next_pos} -1 remainder_str)
      set(${var_expr} "${parts}" PARENT_SCOPE)
      set(${remainder} "${remainder_str}" PARENT_SCOPE)
      return() # This is the everything is good return.
    endif()

    set(pos ${next_pos})
  endwhile()
  message(FATAL_ERROR "Syntax error in @ expression ${input}")
endfunction()

# Expands a list of expression parts into result
# expression_parts - a list of expression parts (name,undef,defs)
# result           - is set to the expanded value
function(expand_parsed_expression expression_parts result)
  set(original_parts "${expression_parts}")
  # Take the variable name from the list
  list(POP_FRONT expression_parts name)

  # Find and handle var=<value> parts, and undef
  set(undef_value)
  foreach( p IN LISTS expression_parts)
    # message("   Looking at part ${p}")
    if(p MATCHES "([^=]+)=(.*)")
      # Define a local variable, in this function scope.
      set(AT_VAR_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")
      if(CMAKE_MATCH_1 STREQUAL "undef")
        set(undef_value "${CMAKE_MATCH_2}")
      endif()
    endif()
  endforeach()

  # expand the variable-name (handle "pointers" @(@(FOO)@)@ ):
  expand_variables("${name}" expanded_name)

  # Look up the value as AT_VAR_name first, or else as name
  set(actual_name)
  foreach(n IN ITEMS "AT_VAR_${expanded_name}" "${expanded_name}")
    if(DEFINED ${n})
      set(actual_name ${n})
    endif()
  endforeach()

  # Load the actual value
  if(actual_name)
    set(value ${${actual_name}})
  elseif(DEFINED undef_value)
    # Expand the undef value:
    expand_variables(${undef_value} value)
  endif()
  # Handle variable expressions inside expressions
  # Expand the value until the result does not change:
  while(1)
    expand_variables("${value}" new_value)
    if(new_value STREQUAL value)
      if(CMAKE_VERBOSE_MAKEFILE)
        message("Expanded variable ${original_parts} with value ${value}")
      endif()
      set(${result} "${value}" PARENT_SCOPE)
      return()
    endif()
    set(value "${new_value}")
  endwhile()
endfunction()

#-----------------------------------------------------------------------------
# Some tests.

if(TEST_AT_VARIABLE_UTILS)

  function(preview_var_replacement result expression expanded)
    message("  PREVIEW: ${expression} => ${expanded}")
    set(${result} "${expanded}" PARENT_SCOPE)
  endfunction()

  function(check_equal description actual expected)
    if(NOT actual STREQUAL expected)
      set(MAYBE_FATAL "FATAL_ERROR")
    else()
      set(MAYBE_FATAL )
    endif()
    message(${MAYBE_FATAL} " ${description}: \"${actual}\" expected \"${expected}\"")
  endfunction()

  function(test_parse_paren_at_expression expand exp_expr exp_remainder)
    message("==========================================")
    message("input = ${expand}")
    parse_paren_at_expression("${expand}" var_expr remainder)
    check_equal("expression" "${var_expr}" "${exp_expr}")
    check_equal("remainder" "${remainder}" "${exp_remainder}")
  endfunction()

  test_parse_paren_at_expression("foo)@" "foo" "")
  test_parse_paren_at_expression("foo)@apa" "foo" "apa")
  test_parse_paren_at_expression("foo)@@apa@" "foo" "@apa@")
  test_parse_paren_at_expression("@foo@)@" "@foo@" "")
  test_parse_paren_at_expression("@(foo)@)@" "@(foo)@" "")
  test_parse_paren_at_expression("a@(b)@@(apa@(foo)@papa)@)@" "a@(b)@@(apa@(foo)@papa)@" "")

  test_parse_paren_at_expression("@foo:undef=u@)@" "@foo:undef=u@" "")
  test_parse_paren_at_expression("@(inner:undef=j)@:undef=u)@" "@(inner:undef=j)@;undef=u" "")
  test_parse_paren_at_expression("@(inner:undef=j)@:undef=u)@,apa" "@(inner:undef=j)@;undef=u" ",apa")

  function(test_vars expand expected)
    message("==========================================")
    message("args = ${expand}")
    expand_variables("${expand}" result)
    check_equal("expanssion" "${result}" "${expected}")
  endfunction()

  set(AT_VAR_VAR1 "hello")
  set(AT_VAR_VAR2 "world")
  set(AT_VAR_REFER_TO_VAR2 "Something plus @VAR2@ is nice")
  set(AT_VAR_WITH_PARAM "Some value @arg:undef=no_arg_given@")
  set(AT_VAR_ARG1_OR_ARG2 "@(ARG1:undef=@ARG2@)@")

  set(AT_VAR_MPU_ALIGN "MAX(0 , 2 << LOG2CEIL(@region_size@) )")
  set(AT_VAR_SMEM_PARTITION_ALIGN "@MPU_ALIGN@")
  set(AT_VAR_EVAL_MPU_ALIGN "@(Z_EVALUATE;undef=0;expr=MAX(32 , 2 << LOG2CEIL(@region_size@) ))@")

  test_vars("@VAR1@" "hello")
  test_vars("@VAR1@ @VAR2@" "hello world")
  test_vars("@VAR1@ @REFER_TO_VAR2@ @VAR2@" "hello Something plus world is nice world")
  test_vars("@SOME_FUNKY:undef=undefined@" "undefined")
  test_vars("@(SOME_FUNKY:undef=@VAR1@)@" "hello")
  test_vars("@WITH_PARAM@" "Some value no_arg_given")
  test_vars("@WITH_PARAM:arg=something_given@" "Some value something_given")
  test_vars("@(MY_OWN_DEF:MY_OWN_DEF=self)@" "self")
  test_vars("@(ARG1_OR_ARG2:ARG2=self)@" "self")
  test_vars("recursive @(@OTHER:undef=VAR1@)@" "recursive hello")

  test_vars("@(SMEM_PARTITION_ALIGN:undef=42:region_size=end-start)@" "MAX(0 , 2 << LOG2CEIL(end-start) )")

  test_vars("@(EVAL_MPU_ALIGN:undef=4077:region_size=the_size)@" "0")
  test_vars("@(EVAL_MPU_ALIGN:undef=4077:region_size=the_size:Z_EVALUATE=@(expr)@)@" "MAX(32 , 2 << LOG2CEIL(the_size) )")
endif()