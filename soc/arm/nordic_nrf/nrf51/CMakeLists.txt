# NB: The define 'NRF51822' is deprecated. 'NRF51' should be used in
# it's place.
zephyr_compile_definitions_ifdef(
  CONFIG_SOC_SERIES_NRF51X
  NRF51822
  )

zephyr_sources(
  power.c
  soc.c
  )
