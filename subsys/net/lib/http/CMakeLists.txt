target_compile_definitions(zephyr PUBLIC -DHTTP_PARSER_STRICT)

target_sources(zephyr PRIVATE ${CMAKE_CURRENT_LIST_DIR}/http_parser.c)
