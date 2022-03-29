/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
namespace {
constexpr int32_t SYSTEM_UID = 1000;
constexpr int32_t ROOT_UID = 0;
constexpr int32_t MIN_SYSTEM_UID = 2100;
constexpr int32_t MAX_SYSTEM_UID = 2899;
}
sptr<AppExecFwk::IBundleMgr> TimePermission::bundleMgrProxy_;

TimePermission::TimePermission() {};
TimePermission::~TimePermission() {};

bool TimePermission::CheckSelfPermission(std::string permName)
{
    return true;
}

bool TimePermission::CheckCallingPermission(int32_t uid, std::string permName)
{
    if ((uid == SYSTEM_UID) || (uid == ROOT_UID)) {
        TIME_HILOGD(TIME_MODULE_COMMON, "root uid return true");
        return true;
    }
    if (IsSystemUid(uid)) {
        TIME_HILOGD(TIME_MODULE_COMMON, "system uid 2100 ~ 2899");
        return true;
    }
    auto callingToken = IPCSkeleton::GetCallingTokenID();
    
    auto tokenType = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(callingToken);
    if (tokenType == Security::AccessToken::ATokenTypeEnum::TOKEN_NATIVE) {
        TIME_HILOGD(TIME_MODULE_COMMON, "native token.");
        return true;
    }
	auto result = Security::AccessToken::AccessTokenKit::VerifyAccessToken(callingToken, permName);
    if (result == Security::AccessToken::TypePermissionState::PERMISSION_DENIED) {
        return false;
    }
    return true;
}

sptr<AppExecFwk::IBundleMgr> TimePermission::GetBundleManager()
{
    if (bundleMgrProxy_ == nullptr) {
        sptr<ISystemAbilityManager> systemManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (systemManager != nullptr) {
            bundleMgrProxy_ =
                iface_cast<AppExecFwk::IBundleMgr>(systemManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID));
        } else {
            TIME_HILOGE(TIME_MODULE_COMMON, "fail to get SAMGR");
        }
    }
    return bundleMgrProxy_;
}

bool TimePermission::IsSystemUid(const int32_t &uid) const
{
    TIME_HILOGE(TIME_MODULE_COMMON, "enter");

    if (uid >= MIN_SYSTEM_UID && uid <= MAX_SYSTEM_UID) {
        return true;
    }

    return false;
}
} // namespace MiscServices
} // namespace OHOS
