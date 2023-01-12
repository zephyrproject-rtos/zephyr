/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define PW_LOG_MODULE_NAME "ServiceReader"

#include "rpc_demo/service/service_reader.h"

#include <pw_hdlc/rpc_channel.h>
#include <pw_hdlc/rpc_packets.h>
#include <pw_log/log.h>
#include <pw_span/span.h>

namespace rpc_demo {

ServiceReader::ServiceReader(pw::stream::Reader* reader,
                             pw::stream::Writer* writer,
                             pw::hdlc::RpcChannelOutput* output,
                             pw::span<pw::rpc::Channel> channels)
    : reader_(reader), writer_(writer), output_(output), channels_(channels) {}

bool ServiceReader::ParsePacket() {
  std::array<std::byte, CONFIG_RPC_BUFFER_SIZE> decode_buffer{};
  pw::rpc::Server server(channels_);
  pw::hdlc::Decoder decoder(decode_buffer);
  DemoService service;

  server.RegisterService(service);

  // Input buffer
  PW_LOG_INFO("Starting pw_rpc service...");
  while (true) {
    std::byte data;

    if (auto result = reader_->Read(&data, 1); !result.ok()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      continue;
    }

    PW_LOG_DEBUG("0x%02x", static_cast<uint8_t>(data));
    auto result = decoder.Process(data);
    if (!result.ok()) {
      continue;
    }

    auto& frame = result.value();
    if (frame.address() != pw::hdlc::kDefaultRpcAddress) {
      decoder.Clear();
      return false;
    }

    PW_LOG_INFO("Service got packet!");
    auto server_result = server.ProcessPacket(frame.data());
    PW_LOG_DEBUG("Client ProcessPacket status (%s)",
                 pw_StatusString(server_result));
    decoder.Clear();
    return true;
  }
}

}  // namespace rpc_demo
