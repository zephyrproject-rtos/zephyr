# Copyright (c) 2023 Meta Platforms
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_CMSIS_DSP)

  set(CMSIS_DSP_DIR ${ZEPHYR_CURRENT_MODULE_DIR})
  set(cmsis_glue_path ${ZEPHYR_CMSIS_MODULE_DIR})

  zephyr_library()

  zephyr_library_compile_options($<TARGET_PROPERTY:compiler,optimization_fast>)

  zephyr_include_directories(
    ${CMSIS_DSP_DIR}/Include
  )

  zephyr_library_include_directories(
    ${CMSIS_DSP_DIR}/PrivateInclude
    ${cmsis_glue_path}/CMSIS/Core/Include
  )

  # Define ZEPHYR_INCLUDE_TOOLCHAIN_STDINT_H_ to avoid using Zephyr's stdint.h
  zephyr_library_compile_definitions(ZEPHYR_INCLUDE_TOOLCHAIN_STDINT_H_)

  # Global Feature Definitions
  zephyr_library_compile_definitions_ifdef(CONFIG_CMSIS_DSP_NEON                  ARM_MATH_NEON)
  zephyr_library_compile_definitions_ifdef(CONFIG_CMSIS_DSP_NEON_EXPERIMENTAL     ARM_MATH_NEON_EXPERIMENTAL)
  zephyr_library_compile_definitions_ifdef(CONFIG_CMSIS_DSP_LOOPUNROLL            ARM_MATH_LOOPUNROLL)
  zephyr_library_compile_definitions_ifdef(CONFIG_CMSIS_DSP_ROUNDING              ARM_MATH_ROUNDING)
  zephyr_library_compile_definitions_ifdef(CONFIG_CMSIS_DSP_MATRIXCHECK           ARM_MATH_MATRIX_CHECK)
  zephyr_library_compile_definitions_ifndef(CONFIG_CMSIS_DSP_FLOAT16              DISABLEFLOAT16)
  zephyr_library_compile_definitions_ifdef(CONFIG_CMSIS_DSP_AUTOVECTORIZE         ARM_MATH_AUTOVECTORIZE)

  # Helium is automatically enabled by CMSIS-DSP if the processor supports it
  zephyr_library_compile_definitions_ifdef(CONFIG_CMSIS_DSP_HELIUM_EXPERIMENTAL   ARM_MATH_HELIUM_EXPERIMENTAL)

  zephyr_compile_definitions_ifndef(CONFIG_ARM __GNUC_PYTHON__)

  if (CONFIG_CMSIS_DSP_NEON OR CONFIG_CMSIS_DSP_NEON_EXPERIMENTAL)
    zephyr_library_include_directories("${CMSIS_DSP_DIR}/ComputeLibrary/Include")
  endif()

  if (CONFIG_ARMV8_1_M_MVEI OR CONFIG_ARMV8_1_M_MVEF)
    if (CONFIG_CMSIS_DSP_LAX_VECTOR_CONVERSIONS)
      zephyr_library_compile_options($<$<STREQUAL:${CMAKE_C_COMPILER_ID},GNU>:-flax-vector-conversions>)
      zephyr_library_compile_options($<$<STREQUAL:${CMAKE_C_COMPILER_ID},ARMClang>:-flax-vector-conversions=integer>)
    else()
      zephyr_library_compile_options($<$<STREQUAL:${CMAKE_C_COMPILER_ID},GNU>:-fno-lax-vector-conversions>)
      zephyr_library_compile_options($<$<STREQUAL:${CMAKE_C_COMPILER_ID},ARMClang>:-flax-vector-conversions=none>)
    endif()
  endif()

  zephyr_compile_definitions_ifndef(CONFIG_ARM __GNUC_PYTHON__)

  # BasicMathFunctions
  if(CONFIG_CMSIS_DSP_BASICMATH)
    set(SRCF64 ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_abs_f64.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_add_f64.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_dot_prod_f64.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_mult_f64.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_negate_f64.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_offset_f64.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_scale_f64.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_sub_f64.c
    )

    set(SRCF32 ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_abs_f32.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_add_f32.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_clip_f32.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_dot_prod_f32.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_mult_f32.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_negate_f32.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_offset_f32.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_scale_f32.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_sub_f32.c
    )

    set(SRCF16 ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_abs_f16.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_add_f16.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_clip_f16.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_dot_prod_f16.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_mult_f16.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_negate_f16.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_offset_f16.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_scale_f16.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_sub_f16.c
    )

    set(SRCQ31 ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_abs_q31.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_add_q31.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_clip_q31.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_dot_prod_q31.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_mult_q31.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_negate_q31.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_offset_q31.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_scale_q31.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_shift_q31.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_sub_q31.c
    )

    set(SRCQ15 ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_abs_q15.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_add_q15.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_clip_q15.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_dot_prod_q15.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_mult_q15.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_negate_q15.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_offset_q15.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_scale_q15.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_shift_q15.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_sub_q15.c
    )

    set(SRCQ7  ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_abs_q7.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_add_q7.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_clip_q7.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_dot_prod_q7.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_mult_q7.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_negate_q7.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_offset_q7.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_scale_q7.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_shift_q7.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_sub_q7.c
    )

    set(SRCU32 ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_and_u32.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_not_u32.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_or_u32.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_xor_u32.c
    )

    set(SRCU16 ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_and_u16.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_not_u16.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_or_u16.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_xor_u16.c
    )

    set(SRCU8  ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_and_u8.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_or_u8.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_not_u8.c
      ${CMSIS_DSP_DIR}/Source/BasicMathFunctions/arm_xor_u8.c)

    zephyr_library_sources(${SRCF64})
    zephyr_library_sources(${SRCF32})

    if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
      zephyr_library_sources(${SRCF16})
    endif()

    zephyr_library_sources(${SRCQ31})
    zephyr_library_sources(${SRCQ15})
    zephyr_library_sources(${SRCQ7})

    zephyr_library_sources(${SRCU32})
    zephyr_library_sources(${SRCU16})
    zephyr_library_sources(${SRCU8})
  endif()

  # BayesFunctions
  if (CONFIG_CMSIS_DSP_BAYES)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/BayesFunctions/arm_gaussian_naive_bayes_predict_f32.c)

    if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/BayesFunctions/arm_gaussian_naive_bayes_predict_f16.c)
    endif()
  endif()

  # Common Tables
  zephyr_library_sources(${CMSIS_DSP_DIR}/Source/CommonTables/arm_common_tables.c
    ${CMSIS_DSP_DIR}/Source/CommonTables/arm_common_tables_f16.c)


  zephyr_library_sources(${CMSIS_DSP_DIR}/Source/CommonTables/arm_const_structs.c)
  zephyr_library_sources(${CMSIS_DSP_DIR}/Source/CommonTables/arm_const_structs_f16.c)


  if (CONFIG_CMSIS_DSP_NEON OR CONFIG_CMSIS_DSP_NEON_EXPERIMENTAL)
    zephyr_library_sources("${CMSIS_DSP_DIR}/ComputeLibrary/Source/arm_cl_tables.c")
  endif()

  if (CONFIG_ARMV8_1_M_MVEI OR CONFIG_ARMV8_1_M_MVEF)
    zephyr_library_sources("${CMSIS_DSP_DIR}/Source/CommonTables/arm_mve_tables.c")
    zephyr_library_sources("${CMSIS_DSP_DIR}/Source/CommonTables/arm_mve_tables_f16.c")
  endif()


  if (WRAPPER)
    target_compile_definitions(CMSISDSP PUBLIC ARM_TABLE_BITREV_1024)
    target_compile_definitions(CMSISDSP PUBLIC ARM_TABLE_TWIDDLECOEF_F32_4096)
    target_compile_definitions(CMSISDSP PUBLIC ARM_TABLE_TWIDDLECOEF_Q31_4096)
    target_compile_definitions(CMSISDSP PUBLIC ARM_TABLE_TWIDDLECOEF_Q15_4096)
    if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
      target_compile_definitions(CMSISDSP PUBLIC ARM_TABLE_TWIDDLECOEF_F16_4096)
    endif()
  endif()

  # ComplexMathFunctions
  # MVE code is using a table for computing the fast sqrt arm_cmplx_mag_q31
  # There is the possibility of not compiling this function and not including
  # the table.
  if (CONFIG_CMSIS_DSP_COMPLEXMATH)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mag_q31.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mag_q15.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mag_fast_q15.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_conj_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_conj_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_conj_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_dot_prod_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_dot_prod_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_dot_prod_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mag_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mag_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mag_squared_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mag_squared_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mag_squared_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mag_squared_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mult_cmplx_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mult_cmplx_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mult_cmplx_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mult_cmplx_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mult_real_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mult_real_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mult_real_q31.c)

    if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_conj_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_dot_prod_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mag_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mag_squared_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mult_cmplx_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ComplexMathFunctions/arm_cmplx_mult_real_f16.c)
    endif()
  endif()

  # Controller Functions
  if (CONFIG_CMSIS_DSP_CONTROLLER)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ControllerFunctions/arm_pid_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ControllerFunctions/arm_pid_init_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ControllerFunctions/arm_pid_init_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ControllerFunctions/arm_pid_reset_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ControllerFunctions/arm_pid_reset_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ControllerFunctions/arm_pid_reset_q31.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ControllerFunctions/arm_sin_cos_f32.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/ControllerFunctions/arm_sin_cos_q31.c)
  endif()

  # Distance Functions
  if (CONFIG_CMSIS_DSP_DISTANCE)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_boolean_distance.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_braycurtis_distance_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_canberra_distance_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_chebyshev_distance_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_chebyshev_distance_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_cityblock_distance_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_cityblock_distance_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_correlation_distance_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_cosine_distance_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_cosine_distance_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_dice_distance.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_euclidean_distance_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_euclidean_distance_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_hamming_distance.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_jaccard_distance.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_jensenshannon_distance_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_kulsinski_distance.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_minkowski_distance_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_rogerstanimoto_distance.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_russellrao_distance.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_sokalmichener_distance.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_sokalsneath_distance.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_yule_distance.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_dtw_distance_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_dtw_path_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_dtw_init_window_q7.c)


    zephyr_library_include_directories("${CMSIS_DSP_DIR}/Source/DistanceFunctions")

    if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_braycurtis_distance_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_canberra_distance_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_chebyshev_distance_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_cityblock_distance_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_correlation_distance_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_cosine_distance_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_euclidean_distance_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_jensenshannon_distance_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/DistanceFunctions/arm_minkowski_distance_f16.c)
    endif()
  endif()

  # Fast Math Functions
  if (CONFIG_CMSIS_DSP_FASTMATH)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_cos_f32.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_cos_q15.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_cos_q31.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_sin_f32.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_sin_q15.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_sin_q31.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_sqrt_q31.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_sqrt_q15.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_vlog_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_vlog_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_vexp_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_vexp_f64.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_vlog_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_vlog_q15.c)


    if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_vlog_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_vexp_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_vinverse_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_atan2_f16.c)
    endif()

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_divide_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_divide_q31.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_atan2_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_atan2_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FastMathFunctions/arm_atan2_q15.c)
  endif()

  # Filtering Functions
  if (CONFIG_CMSIS_DSP_FILTERING)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_lms_norm_init_q31.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_lms_norm_init_q15.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df1_32x64_init_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df1_32x64_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df1_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df1_fast_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df1_fast_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df1_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df1_init_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df1_init_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df1_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df1_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df2T_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df2T_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df2T_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df2T_init_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_stereo_df2T_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_stereo_df2T_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_fast_opt_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_fast_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_fast_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_opt_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_opt_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_partial_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_partial_fast_opt_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_partial_fast_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_partial_fast_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_partial_opt_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_partial_opt_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_partial_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_partial_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_partial_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_conv_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_correlate_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_correlate_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_correlate_fast_opt_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_correlate_fast_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_correlate_fast_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_correlate_opt_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_correlate_opt_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_correlate_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_correlate_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_correlate_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_decimate_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_decimate_fast_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_decimate_fast_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_decimate_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_decimate_init_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_decimate_init_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_decimate_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_decimate_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_fast_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_fast_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_init_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_init_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_init_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_init_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_interpolate_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_interpolate_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_interpolate_init_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_interpolate_init_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_interpolate_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_interpolate_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_lattice_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_lattice_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_lattice_init_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_lattice_init_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_lattice_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_lattice_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_sparse_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_sparse_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_sparse_init_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_sparse_init_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_sparse_init_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_sparse_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_sparse_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_sparse_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_iir_lattice_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_iir_lattice_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_iir_lattice_init_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_iir_lattice_init_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_iir_lattice_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_iir_lattice_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_lms_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_lms_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_lms_init_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_lms_init_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_lms_norm_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_lms_norm_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_lms_norm_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_lms_norm_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_lms_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_lms_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_levinson_durbin_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_levinson_durbin_q31.c)

    if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_fir_init_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df1_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df1_init_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df2T_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_df2T_init_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_stereo_df2T_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_biquad_cascade_stereo_df2T_init_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_correlate_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/FilteringFunctions/arm_levinson_durbin_f16.c)
    endif()
  endif()

  # Interpolation Functions
  if (CONFIG_CMSIS_DSP_INTERPOLATION)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/InterpolationFunctions/arm_bilinear_interp_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/InterpolationFunctions/arm_bilinear_interp_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/InterpolationFunctions/arm_bilinear_interp_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/InterpolationFunctions/arm_bilinear_interp_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/InterpolationFunctions/arm_linear_interp_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/InterpolationFunctions/arm_linear_interp_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/InterpolationFunctions/arm_linear_interp_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/InterpolationFunctions/arm_linear_interp_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/InterpolationFunctions/arm_spline_interp_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/InterpolationFunctions/arm_spline_interp_init_f32.c)


    if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/InterpolationFunctions/arm_bilinear_interp_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/InterpolationFunctions/arm_linear_interp_f16.c)
    endif()
  endif()

  # Matrix Functions
  if (CONFIG_CMSIS_DSP_MATRIX)
    set(SRCF64 ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_cholesky_f64.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_inverse_f64.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_ldlt_f64.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_mult_f64.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_solve_lower_triangular_f64.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_solve_upper_triangular_f64.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_sub_f64.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_trans_f64.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_qr_f64.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_householder_f64.c
    )

    set(SRCF32 ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_add_f32.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_cholesky_f32.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_cmplx_mult_f32.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_cmplx_trans_f32.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_init_f32.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_inverse_f32.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_ldlt_f32.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_mult_f32.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_scale_f32.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_solve_lower_triangular_f32.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_solve_upper_triangular_f32.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_sub_f32.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_trans_f32.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_vec_mult_f32.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_qr_f32.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_householder_f32.c
    )

    set(SRCQ31 ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_add_q31.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_cmplx_mult_q31.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_cmplx_trans_q31.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_init_q31.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_mult_fast_q31.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_mult_opt_q31.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_mult_q31.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_scale_q31.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_sub_q31.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_trans_q31.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_vec_mult_q31.c
    )

    set(SRCQ15 ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_add_q15.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_cmplx_mult_q15.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_cmplx_trans_q15.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_init_q15.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_mult_fast_q15.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_mult_q15.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_scale_q15.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_sub_q15.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_trans_q15.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_vec_mult_q15.c
    )

    set(SRCQ7 ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_mult_q7.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_vec_mult_q7.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_trans_q7.c
    )

    zephyr_library_sources(${SRCF64})
    zephyr_library_sources(${SRCF32})

    zephyr_library_sources(${SRCQ31})
    zephyr_library_sources(${SRCQ15})
    zephyr_library_sources(${SRCQ7})


    if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_add_f16.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_cholesky_f16.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_cmplx_mult_f16.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_cmplx_trans_f16.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_init_f16.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_inverse_f16.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_mult_f16.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_scale_f16.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_solve_lower_triangular_f16.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_solve_upper_triangular_f16.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_sub_f16.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_trans_f16.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_vec_mult_f16.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_mat_qr_f16.c
      ${CMSIS_DSP_DIR}/Source/MatrixFunctions/arm_householder_f16.c
      )
    endif()
  endif()

  # Quaternion Math Functions
  if (CONFIG_CMSIS_DSP_QUATERNIONMATH)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/QuaternionMathFunctions/arm_quaternion_norm_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/QuaternionMathFunctions/arm_quaternion_inverse_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/QuaternionMathFunctions/arm_quaternion_conjugate_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/QuaternionMathFunctions/arm_quaternion_normalize_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/QuaternionMathFunctions/arm_quaternion_product_single_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/QuaternionMathFunctions/arm_quaternion_product_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/QuaternionMathFunctions/arm_quaternion2rotation_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/QuaternionMathFunctions/arm_rotation2quaternion_f32.c)
  endif()

  # Statistics Functions
  if (CONFIG_CMSIS_DSP_STATISTICS)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_entropy_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_entropy_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_kullback_leibler_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_kullback_leibler_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_logsumexp_dot_prod_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_logsumexp_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_max_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_max_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_max_no_idx_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_max_no_idx_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_min_no_idx_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_min_no_idx_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_max_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_max_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_max_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_max_no_idx_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_max_no_idx_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_max_no_idx_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_mean_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_mean_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_mean_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_mean_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_mean_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_min_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_min_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_min_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_min_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_min_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_min_no_idx_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_min_no_idx_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_min_no_idx_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_power_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_power_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_power_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_power_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_power_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_rms_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_rms_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_rms_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_std_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_std_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_std_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_std_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_var_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_var_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_var_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_var_q31.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmax_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmax_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmax_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmax_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmax_q7.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmin_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmin_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmin_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmin_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmin_q7.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmax_no_idx_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmax_no_idx_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmax_no_idx_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmax_no_idx_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmax_no_idx_q7.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmin_no_idx_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmin_no_idx_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmin_no_idx_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmin_no_idx_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmin_no_idx_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_mse_q7.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_mse_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_mse_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_mse_f16.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_mse_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_mse_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_accumulate_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_accumulate_f32.c)


    if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_max_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_min_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_mean_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_power_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_rms_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_std_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_var_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_entropy_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_kullback_leibler_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_logsumexp_dot_prod_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_logsumexp_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_max_no_idx_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_min_no_idx_f16.c)

      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmax_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmin_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmax_no_idx_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_absmin_no_idx_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/StatisticsFunctions/arm_accumulate_f16.c)
    endif()
  endif()

  # Support Functions
  if (CONFIG_CMSIS_DSP_SUPPORT)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_barycenter_f32.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_bitonic_sort_f32.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_bubble_sort_f32.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_copy_f32.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_copy_f64.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_copy_q15.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_copy_q31.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_copy_q7.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_f16_to_float.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_f16_to_q15.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_fill_f32.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_fill_f64.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_fill_q15.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_fill_q31.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_fill_q7.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_f64_to_float.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_f64_to_q31.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_f64_to_q15.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_f64_to_q7.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_float_to_f64.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_float_to_q15.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_float_to_q31.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_float_to_q7.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_heap_sort_f32.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_insertion_sort_f32.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_merge_sort_f32.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_merge_sort_init_f32.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_q15_to_f64.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_q15_to_float.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_q15_to_q31.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_q15_to_q7.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_q31_to_f64.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_q31_to_float.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_q31_to_q15.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_q31_to_q7.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_q7_to_f64.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_q7_to_float.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_q7_to_q15.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_q7_to_q31.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_quick_sort_f32.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_selection_sort_f32.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_sort_f32.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_sort_init_f32.c
      ${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_weighted_average_f32.c
    )

    if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_copy_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_fill_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_f16_to_q15.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_q15_to_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_float_to_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_f16_to_float.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_weighted_average_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_barycenter_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_f16_to_f64.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SupportFunctions/arm_f64_to_f16.c)
    endif()
  endif()

  # SVM Functions
  if (CONFIG_CMSIS_DSP_SVM)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SVMFunctions/arm_svm_linear_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SVMFunctions/arm_svm_rbf_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SVMFunctions/arm_svm_linear_predict_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SVMFunctions/arm_svm_rbf_predict_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SVMFunctions/arm_svm_polynomial_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SVMFunctions/arm_svm_sigmoid_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SVMFunctions/arm_svm_polynomial_predict_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SVMFunctions/arm_svm_sigmoid_predict_f32.c)

    if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SVMFunctions/arm_svm_linear_init_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SVMFunctions/arm_svm_rbf_init_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SVMFunctions/arm_svm_linear_predict_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SVMFunctions/arm_svm_rbf_predict_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SVMFunctions/arm_svm_polynomial_init_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SVMFunctions/arm_svm_sigmoid_init_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SVMFunctions/arm_svm_polynomial_predict_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/SVMFunctions/arm_svm_sigmoid_predict_f16.c)
    endif()
  endif()

  # Transform Functions
  if (CONFIG_CMSIS_DSP_TRANSFORM)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_bitreversal.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_bitreversal2.c)

    if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_bitreversal_f16.c)
    endif()

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix2_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix8_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_init_f32.c)

    if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix2_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_init_f16.c)
    endif()

    if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_init_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_f16.c)
    endif()

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_init_f64.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix2_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_init_q15.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix2_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_init_q31.c)


    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_dct4_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_dct4_init_f32.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_f32.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_dct4_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_dct4_init_q31.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_init_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_init_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_init_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_q31.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_dct4_init_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_dct4_q15.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_init_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_init_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_init_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_q15.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_fast_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_fast_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix8_f32.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_fast_f64.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_fast_init_f64.c)

    if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_fast_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_fast_init_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_init_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix8_f16.c)
    endif()

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_f32.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_init_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_init_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_q15.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_init_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_rfft_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_init_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_q31.c)

    if (WRAPPER)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix2_init_q15.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix2_init_q31.c)
    endif()

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_init_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix4_init_q31.c)


    # For scipy or wrappers or benchmarks
    if (WRAPPER)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix2_init_f32.c)
      if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
        zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_cfft_radix2_init_f16.c)
      endif()
    endif()

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_mfcc_init_f32.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_mfcc_f32.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_mfcc_init_q31.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_mfcc_q31.c)

    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_mfcc_init_q15.c)
    zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_mfcc_q15.c)

    if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_mfcc_init_f16.c)
      zephyr_library_sources(${CMSIS_DSP_DIR}/Source/TransformFunctions/arm_mfcc_f16.c)
    endif()
  endif()

  # Window Functions
  if (CONFIG_CMSIS_DSP_WINDOW)
    set(SRCF64 ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_welch_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_bartlett_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hamming_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hanning_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_nuttall3_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_nuttall4_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_nuttall3a_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_nuttall3b_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_nuttall4a_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_blackman_harris_92db_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_nuttall4b_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_nuttall4c_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hft90d_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hft95_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hft116d_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hft144d_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hft169d_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hft196d_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hft223d_f64.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hft248d_f64.c
    )

    set(SRCF32 ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_welch_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_bartlett_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hamming_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hanning_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_nuttall3_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_nuttall4_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_nuttall3a_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_nuttall3b_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_nuttall4a_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_blackman_harris_92db_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_nuttall4b_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_nuttall4c_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hft90d_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hft95_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hft116d_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hft144d_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hft169d_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hft196d_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hft223d_f32.c
      ${CMSIS_DSP_DIR}/Source/WindowFunctions/arm_hft248d_f32.c
    )

    zephyr_library_sources(${SRCF64})
    zephyr_library_sources(${SRCF32})
  endif()
endif()
