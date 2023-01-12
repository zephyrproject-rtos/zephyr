/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proto/demo.rpc.pb.h"

namespace rpc_demo {

class DemoService : public pw_rpc::nanopb::DemoService::Service<DemoService> {
 public:
  ::pw::Status GetVersion(const ::rpc_demo_Empty& request,
                          ::rpc_demo_GetVersionResponse& response);

  ::pw::Status GetSensorList(const ::rpc_demo_Empty& request,
                             ::rpc_demo_GetSensorListResponse& response);

  ::pw::Status UpdateSensorFrequency(
      const ::rpc_demo_UpdateSensorFrequencyRequest& request,
      ::rpc_demo_UpdateSensorFrequencyResponse& response);

  ::pw::Status GetSensorSamples(
      const ::rpc_demo_GetSensorSamplesRequest& request,
      ::rpc_demo_GetSensorSamplesResponse& response);
};

}  // namespace rpc_demo