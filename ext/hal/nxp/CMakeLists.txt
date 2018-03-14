add_subdirectory_ifdef(
  CONFIG_HAS_MCUX
  mcux
  )

add_subdirectory_ifdef(
  CONFIG_HAS_IMX_HAL
  imx
  )
