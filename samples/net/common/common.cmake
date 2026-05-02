# SPDX-License-Identifier: Apache-2.0

# Common routines used in net samples

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/samples/net/common/)
target_sources(app PRIVATE ${ZEPHYR_BASE}/samples/net/common/net_sample_common.c)

target_sources_ifdef(CONFIG_NET_VLAN app PRIVATE
  ${ZEPHYR_BASE}/samples/net/common/vlan.c)

target_sources_ifdef(CONFIG_NET_L2_IPIP app PRIVATE
  ${ZEPHYR_BASE}/samples/net/common/tunnel.c)

target_sources_ifdef(CONFIG_QUIC app PRIVATE
  ${ZEPHYR_BASE}/samples/net/common/quic.c)

if(CONFIG_QUIC)
  if(NOT DEFINED gen_dir)
    set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated)
  endif()

  foreach(inc_file
    quic_server_ec_cert.pem
    quic_server_ec_key.pem
  )
    generate_inc_file_for_target(
      app
      ${ZEPHYR_BASE}/samples/net/common/quic_certs/${inc_file}
      ${gen_dir}/${inc_file}.inc
    )
  endforeach()

  target_include_directories(app PRIVATE ${ZEPHYR_BASE}/samples/net/common/quic_certs/)
endif()
