/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef TIME_PERMISSION_H
#define TIME_PERMISSION_H

#include <cstdint>
#include <mutex>
#include <string>
#include <singleton.h>

#include "bundle_mgr_proxy.h"
#include "time_common.h"
#include "mock_permission.h"
#include "system_ability_definition.h"
#include "iservice_registry.h"

namespace OHOS {
namespace MiscServices {
    namespace Permission {
        static const std::string SET_TIME = "ohos.permission.SET_TIME";
        static const std::string SET_TIME_ZONE = "ohos.permission.SET_TIME_ZONE";
    }

class TimePermission {
public:
    static bool GetBundleNameByUid(int32_t uid, std::string &bundleName);
    static bool CheckCallingPermission(const std::string &permissionName);
};
} // namespace MiscServices
} // namespace OHOS
#endif // TIME_PERMISSION_H
