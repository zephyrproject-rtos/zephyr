target_compile_definitions(board PRIVATE NO_POSIX_CHEATS)
target_link_libraries(board INTERFACE -lm)

# Override the C standard used for compilation to C 2011
# This is due to some tests using _Static_assert which is a 2011 feature, but
# otherwise relying on compilers supporting it also when set to C99.
# This was in general ok, but with some host compilers and C library versions
# it led to problems. So we override it to 2011 for native_posix.
set_property(GLOBAL PROPERTY CSTD c11)
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            hw_models_top.c
            timer_model.c
            native_rtc.c
            irq_handler.c
            irq_ctrl.c
            main.c
            tracing.c
            cmdline_common.c
            cmdline.c
)

target_sources(board PRIVATE ${PRIVATE_SOURCES})
