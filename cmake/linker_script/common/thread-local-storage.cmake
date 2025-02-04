# The contents of this file is based on include/zephyr/linker/thread-local-storage.ld
# Please keep in sync

if(CONFIG_THREAD_LOCAL_STORAGE)
  zephyr_linker_section(NAME .tdata LMA FLASH NOINPUT)
  zephyr_linker_section_configure(SECTION .tdata INPUT ".tdata")
  zephyr_linker_section_configure(SECTION .tdata INPUT ".tdata.*")
  zephyr_linker_section_configure(SECTION .tdata INPUT ".gnu.linkonce.td.*")
  # GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

  zephyr_linker_section(NAME .tbss LMA FLASH NOINPUT)
  zephyr_linker_section_configure(SECTION .tbss INPUT ".tbss")
  zephyr_linker_section_configure(SECTION .tbss INPUT ".tbss.*")
  zephyr_linker_section_configure(SECTION .tbss INPUT ".gnu.linkonce.tb.*")
  zephyr_linker_section_configure(SECTION .tbss INPUT ".tcommon")
  # GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

  #
  # These needs to be outside of the tdata/tbss
  # sections or else they would be considered
  # thread-local variables, and the code would use
  # the wrong values.
  #
  # This scheme is not yet handled
  if(CONFIG_XIP)
#	/* The "master copy" of tdata should be only in flash on XIP systems */
#	PROVIDE(__tdata_start = LOADADDR(tdata));
  else()
#	PROVIDE(__tdata_start = ADDR(tdata));
  endif()
#	PROVIDE(__tdata_size = SIZEOF(tdata));
#	PROVIDE(__tdata_end = __tdata_start + __tdata_size);
#	PROVIDE(__tdata_align = ALIGNOF(tdata));
#
#	PROVIDE(__tbss_start = ADDR(tbss));
#	PROVIDE(__tbss_size = SIZEOF(tbss));
#	PROVIDE(__tbss_end = __tbss_start + __tbss_size);
#	PROVIDE(__tbss_align = ALIGNOF(tbss));
#
#	PROVIDE(__tls_start = __tdata_start);
#	PROVIDE(__tls_end = __tbss_end);
#	PROVIDE(__tls_size = __tbss_end - __tdata_start);

endif()
