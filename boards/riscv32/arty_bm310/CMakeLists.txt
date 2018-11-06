zephyr_compile_options(
  -march=rv32imc
  -mabi=ilp32
)

zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
