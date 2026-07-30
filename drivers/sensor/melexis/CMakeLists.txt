# Copyright (c) 2024 Florian Weber <Florian.Weber@live.de>
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_MLX90394 mlx90394)
add_subdirectory_ifdef(CONFIG_MLX90632 mlx90632)
# zephyr-keep-sorted-stop
