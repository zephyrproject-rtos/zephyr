# cmake 3.3.2 does not know CMAKE_SYSTEM_NAME=FreeRTOS, we set it to Generic
include (cross-generic-gcc)
string (TOLOWER "FreeRTOS"                PROJECT_SYSTEM)
string (TOUPPER "FreeRTOS"                PROJECT_SYSTEM_UPPER)

# vim: expandtab:ts=2:sw=2:smartindent
