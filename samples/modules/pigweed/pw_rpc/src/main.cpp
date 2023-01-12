/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define PW_LOG_MODULE_NAME "MAIN"

#include <pw_hdlc/rpc_channel.h>
#include <pw_hdlc/rpc_packets.h>
#include <pw_log/log.h>
#include <pw_status/status.h>

#include "proto/demo.pb.h"
#include "rpc_demo/client/client_reader.h"
#include "rpc_demo/service/service_reader.h"
#include "rpc_demo/rxtx.h"


namespace {
// Create the client channel output with a built-in buffer.
pw::hdlc::RpcChannelOutput
    client_to_service_channel_output(ClientToServiceStream().as_writer(),
                                     pw::hdlc::kDefaultRpcAddress,
                                     "HDLC client output");
pw::hdlc::RpcChannelOutput
    service_to_client_channel_output(ServiceToClientStream().as_writer(),
                                     pw::hdlc::kDefaultRpcAddress,
                                     "HDLC service output");

// Configure the channel. Is this an output channel only?
pw::rpc::Channel client_channels[] = {
    pw::rpc::Channel::Create<rpc_demo_RpcClientChannelId_DEFAULT,
                             rpc_demo_RpcClientChannelId>(
        &client_to_service_channel_output),
};
pw::rpc::Channel service_channels[] = {
    pw::rpc::Channel::Create<rpc_demo_RpcClientChannelId_DEFAULT,
                             rpc_demo_RpcClientChannelId>(
        &service_to_client_channel_output),
};

pw::rpc::Client rpc_client(client_channels);
rpc_demo::pw_rpc::nanopb::DemoService::Client
    service_client(rpc_client, rpc_demo_RpcClientChannelId_DEFAULT);

void GetVersionResponse(const rpc_demo_GetVersionResponse &response,
                               pw::Status status) {
  if (status.ok()) {
    PW_LOG_INFO("Received version #%u", response.version_number);
  } else {
    PW_LOG_ERROR("Failed to get version number, status: %s",
                 pw_StatusString(status));
  }
}

} // namespace

int main(void) {
  PW_LOG_INFO("Starting RPC demo...");
  rpc_demo::ServiceReader ec_service(
      &ClientToServiceStream().as_reader(),
      &ServiceToClientStream().as_writer(),
      reinterpret_cast<pw::hdlc::RpcChannelOutput*>(
          &service_to_client_channel_output),
      service_channels);
  rpc_demo::ClientReader ap_client(ServiceToClientStream().as_reader(), &rpc_client);

  auto response = service_client.GetVersion(rpc_demo_Empty_init_default,
                                            GetVersionResponse);
  PW_ASSERT(response.active());
  PW_LOG_INFO("Service waiting for data...");
  PW_ASSERT(ec_service.ParsePacket());
  PW_LOG_INFO("Client waiting for response...");
  PW_ASSERT(ap_client.ParsePacket());
  return 0;
}
