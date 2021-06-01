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

#ifndef SERVICES_INCLUDE_TIME_SERVICES_H
#define SERVICES_INCLUDE_TIME_SERVICES_H

#include <mutex>
#include "system_ability.h"
#include "time_service_stub.h"
#include "event_handler.h"

namespace OHOS {
namespace MiscServices {
enum class ServiceRunningState {
    STATE_NOT_START,
    STATE_RUNNING
};

class TimeService : public SystemAbility, public TimeServiceStub {
    DECLARE_SYSTEM_ABILITY(TimeService);

public:
    DISALLOW_COPY_AND_MOVE(TimeService);

    TimeService(int32_t systemAbilityId, bool runOnCreate);
    TimeService();
    ~TimeService();
    static sptr<TimeService> GetInstance();
    bool SetTime(const int64_t time) override;

protected:
    void OnStart() override;
    void OnStop() override;

private:
    int32_t Init();
    ServiceRunningState state_;

    void InitServiceHandler();

    static std::mutex instanceLock_;
    static sptr<TimeService> instance_;

    static std::shared_ptr<AppExecFwk::EventHandler> serviceHandler_;
};
} // MiscServices
} // OHOS
#endif // SERVICES_INCLUDE_TIME_SERVICES_H
