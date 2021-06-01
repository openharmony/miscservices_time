/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "time_service_manager.h"
#include <cinttypes>
#include "system_ability_definition.h"
#include "iservice_registry.h"
#include "time_log.h"

namespace OHOS {
namespace MiscServices {
std::mutex TimeServiceManager::instanceLock_;
sptr<TimeServiceManager> TimeServiceManager::instance_;
sptr<ITimeService> TimeServiceManager::timeServiceProxy_;
sptr<TimeSaDeathRecipient> TimeServiceManager::deathRecipient_;

TimeServiceManager::TimeServiceManager()
{
}

TimeServiceManager::~TimeServiceManager(){}

sptr<TimeServiceManager> TimeServiceManager::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> autoLock(instanceLock_);
        if (instance_ == nullptr) {
            instance_ = new TimeServiceManager;
            timeServiceProxy_ = GetTimeServiceProxy();
        }
    }
    return instance_;
}

bool TimeServiceManager::SetTime(const int64_t time)
{
    if (timeServiceProxy_ == nullptr) {
        TIME_HILOGW("Redo GetTimeServiceProxy");
        timeServiceProxy_ = GetTimeServiceProxy();
    }

    if (timeServiceProxy_ == nullptr) {
        TIME_HILOGE("SetTime quit because redoing GetTimeServiceProxy failed.");
        return false;
    }

    return timeServiceProxy_->SetTime(time);
}

sptr<ITimeService> TimeServiceManager::GetTimeServiceProxy()
{
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        TIME_HILOGE("Getting SystemAbilityManager failed.");
        return nullptr;
    }

    auto systemAbility = systemAbilityManager->GetSystemAbility(TIME_SERVICE_ID, "");
    if (systemAbility == nullptr) {
        TIME_HILOGE("Get SystemAbility failed.");
        return nullptr;
    }
    deathRecipient_ = new TimeSaDeathRecipient();
    systemAbility->AddDeathRecipient(deathRecipient_);
    sptr<ITimeService> timeServiceProxy = iface_cast<ITimeService>(systemAbility);
    if (timeServiceProxy == nullptr) {
        TIME_HILOGE("Get TimeServiceProxy from SA failed.");
        return nullptr;
    }

    TIME_HILOGD("Getting TimeServiceProxy succeeded.");
    return timeServiceProxy;
}

void TimeServiceManager::OnRemoteSaDied(const wptr<IRemoteObject> &remote)
{
    timeServiceProxy_ = GetTimeServiceProxy();
}

TimeSaDeathRecipient::TimeSaDeathRecipient()
{
}

void TimeSaDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    TIME_HILOGE("TimeSaDeathRecipient on remote systemAbility died.");
    TimeServiceManager::GetInstance()->OnRemoteSaDied(object);
}
} // namespace MiscServices
} // namespace OHOS
