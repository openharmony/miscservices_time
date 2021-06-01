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

#ifndef SERVICES_INCLUDE_TIME_SERVICES_MANAGER_H
#define SERVICES_INCLUDE_TIME_SERVICES_MANAGER_H

#include "refbase.h"
#include "time_service_stub.h"
#include "iremote_object.h"

namespace OHOS {
namespace MiscServices {
class TimeSaDeathRecipient : public IRemoteObject::DeathRecipient {
public:
     explicit TimeSaDeathRecipient();
    ~TimeSaDeathRecipient() = default;

    void OnRemoteDied(const wptr<IRemoteObject> &object) override;
};

class TimeServiceManager : public RefBase {
public:
    TimeServiceManager();
    ~TimeServiceManager();
    static sptr<TimeServiceManager> GetInstance();
    bool SetTime(const int64_t time);
    void OnRemoteSaDied(const wptr<IRemoteObject> &object);

private:
    static sptr<ITimeService> GetTimeServiceProxy();

    static std::mutex instanceLock_;
    static sptr<TimeServiceManager> instance_;
    static sptr<ITimeService> timeServiceProxy_;
    static sptr<TimeSaDeathRecipient> deathRecipient_;
};
} // MiscServices
} // OHOS
#endif // SERVICES_INCLUDE_TIME_SERVICES_MANAGER_H