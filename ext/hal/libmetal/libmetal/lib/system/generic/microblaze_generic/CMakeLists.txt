collect (PROJECT_LIB_HEADERS sys.h)

collect (PROJECT_LIB_SOURCES sys.c)

check_include_files(xintc.h HAS_XINTC)
if (HAS_XINTC)
  add_definitions(-DHAS_XINTC)
endif(HAS_XINTC)

# vim: expandtab:ts=2:sw=2:smartindent
