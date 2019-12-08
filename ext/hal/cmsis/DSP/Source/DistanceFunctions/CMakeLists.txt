# Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  arm_boolean_distance.c
  arm_boolean_distance_template.h
  arm_braycurtis_distance_f32.c
  arm_canberra_distance_f32.c
  arm_chebyshev_distance_f32.c
  arm_cityblock_distance_f32.c
  arm_correlation_distance_f32.c
  arm_cosine_distance_f32.c
  arm_dice_distance.c
  arm_euclidean_distance_f32.c
  arm_hamming_distance.c
  arm_jaccard_distance.c
  arm_jensenshannon_distance_f32.c
  arm_kulsinski_distance.c
  arm_minkowski_distance_f32.c
  arm_rogerstanimoto_distance.c
  arm_russellrao_distance.c
  arm_sokalmichener_distance.c
  arm_sokalsneath_distance.c
  arm_yule_distance.c
  )
