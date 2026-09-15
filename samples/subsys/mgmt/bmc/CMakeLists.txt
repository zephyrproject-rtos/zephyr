# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bmc)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_BMC_SENSORS app PRIVATE src/board.c)
target_sources_ifdef(CONFIG_BMC_REDFISH app PRIVATE src/redfish_product.c)
target_sources_ifdef(CONFIG_BMC_SAMPLE_WEB app PRIVATE src/webui.c)
target_sources_ifdef(CONFIG_BMC_SAMPLE_GENERATE_CERTS app PRIVATE src/tls.c)

# The firmware version reported over Redfish is the SHA of the tree the sample
# was built from, regenerated on every build so it cannot go stale.
find_package(Git QUIET)

set(git_sha_header ${gen_dir}/bmc_git_sha.h)
add_custom_target(bmc_git_sha ALL
  COMMAND ${CMAKE_COMMAND}
    -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
    -DSOURCE_DIR=${ZEPHYR_BASE}
    -DOUTPUT_FILE=${git_sha_header}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/gen_git_sha.cmake
  BYPRODUCTS ${git_sha_header}
  COMMENT "Generating the BMC git SHA header"
)
add_dependencies(app bmc_git_sha)

if(CONFIG_BMC_SAMPLE_WEB)
  foreach(web_resource IN ITEMS index.html logo.jpeg favicon.png)
    generate_inc_file_for_target(
      app
      web/${web_resource}
      ${gen_dir}/${web_resource}.gz.inc
      --gzip
    )
  endforeach()
endif()

# Browser libraries used by the dashboard. They are not redistributed with
# Zephyr, so the build picks them up from web/vendor/ once web/fetch_vendor.sh
# has downloaded them, and the dashboard degrades gracefully when they are
# missing. See web/README.md.
set(vendor_assets
  xterm.min.css
  xterm.min.js
  xterm-addon-attach.min.js
  xterm-addon-fit.min.js
  chart.umd.min.js
)

if(CONFIG_BMC_SAMPLE_WEB)
  set(vendor_dir ${CMAKE_CURRENT_SOURCE_DIR}/web/vendor)
  set(have_vendor_assets TRUE)

  foreach(asset IN LISTS vendor_assets)
    if(NOT EXISTS ${vendor_dir}/${asset})
      set(have_vendor_assets FALSE)
    endif()
  endforeach()

  if(have_vendor_assets)
    foreach(asset IN LISTS vendor_assets)
      generate_inc_file_for_target(
        app
        ${vendor_dir}/${asset}
        ${gen_dir}/${asset}.gz.inc
        --gzip
      )
    endforeach()
    zephyr_compile_definitions(BMC_SAMPLE_WEB_VENDOR_ASSETS=1)
  else()
    message(STATUS
      "BMC sample: web/vendor/ is incomplete, building the dashboard without "
      "the terminal and chart libraries. Run web/fetch_vendor.sh to get them, "
      "see web/README.md."
    )
  endif()
endif()

if(CONFIG_BMC_SAMPLE_GENERATE_CERTS)
  find_program(OPENSSL_EXECUTABLE openssl REQUIRED)
  find_program(BASH_EXECUTABLE bash REQUIRED)

  set(certs_dir ${CMAKE_CURRENT_BINARY_DIR}/generated_certs)
  set(server_cert_der ${certs_dir}/server_cert.der)
  set(server_privkey_der ${certs_dir}/server_privkey.der)

  add_custom_command(
    OUTPUT ${server_cert_der} ${server_privkey_der}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${certs_dir}
    COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/certs/gen_ca_cert.sh
            ${OPENSSL_EXECUTABLE}
    COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/certs/gen_server_cert.sh
            ${OPENSSL_EXECUTABLE}
    WORKING_DIRECTORY ${certs_dir}
    DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/certs/gen_ca_cert.sh
      ${CMAKE_CURRENT_SOURCE_DIR}/certs/gen_server_cert.sh
    BYPRODUCTS
      ${certs_dir}/ca_cert.pem
      ${certs_dir}/ca_privkey.pem
      ${certs_dir}/ca.srl
      ${certs_dir}/server_cert.pem
      ${certs_dir}/server_privkey.pem
      ${certs_dir}/server_csr.pem
      ${certs_dir}/server_csr.ext
    COMMENT "Generating TLS certificates for the BMC sample"
    VERBATIM
  )

  add_custom_target(bmc_generate_certs DEPENDS ${server_cert_der} ${server_privkey_der})
  add_dependencies(app bmc_generate_certs)

  foreach(inc_file IN ITEMS server_cert.der server_privkey.der)
    generate_inc_file_for_target(app ${certs_dir}/${inc_file} ${gen_dir}/${inc_file}.inc)
  endforeach()
endif()
