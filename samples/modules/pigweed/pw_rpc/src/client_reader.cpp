/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define PW_LOG_MODULE_NAME "ClientReader"

#include "rpc_demo/client/client_reader.h"

#include <pw_hdlc/decoder.h>
#include <pw_hdlc/rpc_packets.h>
#include <pw_log/log.h>
#include <pw_status/status.h>

namespace rpc_demo {

ClientReader::ClientReader(pw::stream::Reader& rx, pw::rpc::Client* rpc_client)
    : rx_(rx), rpc_client_(rpc_client), decoder_(decode_buffer_) {}

bool ClientReader::ParsePacket() {
  PW_LOG_INFO("Reading bytes...");
  while (true) {
    std::byte data;
    if (auto result = rx_.Read(&data, 1); !result.ok()) {
      PW_LOG_ERROR("Failed to read data");
      continue;
    }
    PW_LOG_DEBUG("0x%02x", static_cast<uint8_t>(data));

    pw::Result<pw::hdlc::Frame> result = decoder_.Process(data);
    if (!result.ok()) {
      continue;
    }

    pw::hdlc::Frame& frame = result.value();
    if (frame.address() != pw::hdlc::kDefaultRpcAddress) {
      return false;
    }

    PW_LOG_INFO("Client got packet!");
    auto client_result = rpc_client_->ProcessPacket(frame.data());
    PW_LOG_DEBUG("Client ProcessPacket status (%s)",
                 pw_StatusString(client_result));
    decoder_.Clear();
    return true;
  }
}

}  // namespace rpc_demo
