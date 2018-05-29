if (DOXYGEN_FOUND)

  configure_file (Doxyfile.in Doxyfile @ONLY)

  add_custom_target (doc ALL
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
  DESTINATION share/doc/${PROJECT_NAME})
install (FILES ${PROJECT_SOURCE_DIR}/README.md
  DESTINATION share/doc/${PROJECT_NAME})
install (FILES ${PROJECT_SOURCE_DIR}/LICENSE.md
  DESTINATION share/doc/${PROJECT_NAME})

endif (DOXYGEN_FOUND)

# vim: expandtab:ts=2:sw=2:smartindent
