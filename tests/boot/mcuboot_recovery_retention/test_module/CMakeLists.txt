if(CONFIG_MCUBOOT)
  # Append source file which outputs the current mode of mcuboot
  target_sources(app PRIVATE src/hook.c)
endif()
