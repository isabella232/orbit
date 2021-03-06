// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ORBIT_CORE_FUNCTION_UTILS_H_
#define ORBIT_CORE_FUNCTION_UTILS_H_

#include <cstdint>
#include <string>

#include "capture_data.pb.h"
#include "symbol.pb.h"

namespace FunctionUtils {

inline const std::string& GetDisplayName(const orbit_client_protos::FunctionInfo& func) {
  return func.pretty_name().empty() ? func.name() : func.pretty_name();
}

std::string GetLoadedModuleName(const orbit_client_protos::FunctionInfo& func);
uint64_t GetHash(const orbit_client_protos::FunctionInfo& func);

// Calculates and returns the absolute address of the function.
uint64_t Offset(const orbit_client_protos::FunctionInfo& func);
inline uint64_t GetAbsoluteAddress(const orbit_client_protos::FunctionInfo& func) {
  return func.address() + func.module_base_address() - func.load_bias();
}

bool IsOrbitFunc(const orbit_client_protos::FunctionInfo& func);

std::unique_ptr<orbit_client_protos::FunctionInfo> CreateFunctionInfo(
    const orbit_grpc_protos::SymbolInfo& symbol_info, uint64_t load_bias,
    const std::string& module_path, uint64_t module_base_address);

bool SetOrbitTypeFromName(orbit_client_protos::FunctionInfo* func);

}  // namespace FunctionUtils

#endif  // ORBIT_CORE_FUNCTION_UTILS_H_
