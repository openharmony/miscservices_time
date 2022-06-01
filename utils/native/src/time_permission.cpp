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

#include "ipc_skeleton.h"
#include "accesstoken_kit.h"
#include "time_permission.h"

namespace OHOS {
namespace MiscServices {
bool TimePermission::CheckCallingPermission(const std::string &permissionName)
{
    if (permissionName.empty()) {
        TIME_HILOGE(TIME_MODULE_COMMON, "permission check failed，permission name is empty.");
        return false;
    }

    auto callerToken = IPCSkeleton::GetCallingTokenID();
    int result = Security::AccessToken::AccessTokenKit::VerifyAccessToken(callerToken, permissionName);
    if (result == Security::AccessToken::PERMISSION_GRANTED) {
        TIME_HILOGE(TIME_MODULE_COMMON, "permission check Success.");
        return true;
    } else {
        TIME_HILOGE(TIME_MODULE_COMMON, "permission check failed, permission:%{public}s, callerToken:%{public}u",
                    permissionName.c_str(), callerToken);
        return false;
	}
}
} // namespace MiscServices
} // namespace OHOS