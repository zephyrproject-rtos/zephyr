/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rpc_demo/service/demo_service.h"

namespace rpc_demo {

// Method definitions for pw_demo.DemoHostService.
::pw::Status DemoService::GetVersion(const rpc_demo_Empty& request,
                                     rpc_demo_GetVersionResponse& response) {
  static_cast<void>(request);
  response.version_number = VERSION;
  return ::pw::OkStatus();
}

::pw::Status DemoService::GetSensorList(
    const rpc_demo_Empty& request, rpc_demo_GetSensorListResponse& response) {
  // TODO: Read the request as appropriate for your application
  static_cast<void>(request);
  // TODO: Fill in the response as appropriate for your application
  static_cast<void>(response);
  return ::pw::Status::Unimplemented();
}

::pw::Status DemoService::UpdateSensorFrequency(
    const rpc_demo_UpdateSensorFrequencyRequest& request,
    rpc_demo_UpdateSensorFrequencyResponse& response) {
  // TODO: Read the request as appropriate for your application
  static_cast<void>(request);
  // TODO: Fill in the response as appropriate for your application
  static_cast<void>(response);
  return ::pw::Status::Unimplemented();
}

::pw::Status DemoService::GetSensorSamples(
    const rpc_demo_GetSensorSamplesRequest& request,
    rpc_demo_GetSensorSamplesResponse& response) {
  // TODO: Read the request as appropriate for your application
  static_cast<void>(request);
  // TODO: Fill in the response as appropriate for your application
  static_cast<void>(response);
  return ::pw::Status::Unimplemented();
}

}  // namespace rpc_demo
