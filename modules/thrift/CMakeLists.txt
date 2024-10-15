# Copyright 2022 Meta
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_THRIFT)

zephyr_library()

zephyr_library_include_directories(
  src
  include
  ${ZEPHYR_THRIFT_MODULE_DIR}/lib/cpp/src
)

zephyr_library_sources(
  src/thrift/server/TFDServer.cpp
  ${ZEPHYR_THRIFT_MODULE_DIR}/lib/cpp/src/thrift/protocol/TProtocol.cpp
  ${ZEPHYR_THRIFT_MODULE_DIR}/lib/cpp/src/thrift/server/TConnectedClient.cpp
  ${ZEPHYR_THRIFT_MODULE_DIR}/lib/cpp/src/thrift/server/TSimpleServer.cpp
  ${ZEPHYR_THRIFT_MODULE_DIR}/lib/cpp/src/thrift/transport/SocketCommon.cpp
  ${ZEPHYR_THRIFT_MODULE_DIR}/lib/cpp/src/thrift/transport/TBufferTransports.cpp
  ${ZEPHYR_THRIFT_MODULE_DIR}/lib/cpp/src/thrift/transport/TFDTransport.cpp
  ${ZEPHYR_THRIFT_MODULE_DIR}/lib/cpp/src/thrift/transport/TTransportException.cpp
  ${ZEPHYR_THRIFT_MODULE_DIR}/lib/cpp/src/thrift/transport/TServerSocket.cpp
  ${ZEPHYR_THRIFT_MODULE_DIR}/lib/cpp/src/thrift/transport/TSocket.cpp
  ${ZEPHYR_THRIFT_MODULE_DIR}/lib/cpp/src/thrift/TApplicationException.cpp
  ${ZEPHYR_THRIFT_MODULE_DIR}/lib/cpp/src/thrift/TOutput.cpp

  # Replace with upstream equivalents when Zephyr's std::thread, etc, are fixed
  src/thrift/concurrency/Mutex.cpp
  src/thrift/server/TServerFramework.cpp
)

zephyr_library_sources_ifdef(CONFIG_THRIFT_SSL_SOCKET
  # Replace with upstream equivalents when Zephyr's std::thread, etc, are fixed
  src/thrift/transport/TSSLSocket.cpp
  src/thrift/transport/TSSLServerSocket.cpp
)

# needed because std::iterator was deprecated with -std=c++17
zephyr_library_compile_options(-Wno-deprecated-declarations)

endif(CONFIG_THRIFT)
