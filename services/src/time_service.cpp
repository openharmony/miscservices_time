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

#include "time_service.h"

#include <string>
#include <unistd.h>
#include <sys/time.h>
#include <cerrno>
#include "system_ability.h"
#include "system_ability_definition.h"
#include "iservice_registry.h"
#include "time_log.h"
#include "time_common.h"

namespace OHOS {
namespace MiscServices {
using namespace OHOS::HiviewDFX;

REGISTER_SYSTEM_ABILITY_BY_ID(TimeService, TIME_SERVICE_ID, true);

const std::int64_t INIT_INTERVAL = 10000L;
std::mutex TimeService::instanceLock_;
sptr<TimeService> TimeService::instance_;

std::shared_ptr<AppExecFwk::EventHandler> TimeService::serviceHandler_;

TimeService::TimeService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate), state_(ServiceRunningState::STATE_NOT_START)
{
}

TimeService::TimeService() : state_(ServiceRunningState::STATE_NOT_START)
{
}

TimeService::~TimeService()
{
}

sptr<TimeService> TimeService::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> autoLock(instanceLock_);
        if (instance_ == nullptr) {
            instance_ = new TimeService;
        }
    }
    return instance_;
}

void TimeService::OnStart()
{
    TIME_HILOGI("Enter OnStart.");
    if (state_ == ServiceRunningState::STATE_RUNNING) {
        TIME_HILOGI("TimeService is already running.");
        return;
    }

    InitServiceHandler();
    if (Init() != SUCCESS) {
        auto callback = [=]() { Init(); };
        serviceHandler_->PostTask(callback, INIT_INTERVAL);
        TIME_HILOGE("Init failed. Try again 10s later");
        return;
    }

    TIME_HILOGI("Start TimeService success.");
    return;
}

int32_t TimeService::Init()
{
    bool ret = Publish(TimeService::GetInstance());
    if (!ret) {
        TIME_HILOGE("Publish failed.");
        return E_PUBLISH_FAIL;
    }
    TIME_HILOGI("Publish success.");
    state_ = ServiceRunningState::STATE_RUNNING;
    return SUCCESS;
}

void TimeService::OnStop()
{
    TIME_HILOGI("OnStop started.");
    if (state_ != ServiceRunningState::STATE_RUNNING) {
        return;
    }
    serviceHandler_ = nullptr;

    state_ = ServiceRunningState::STATE_NOT_START;
    TIME_HILOGI("OnStop end.");
}

void TimeService::InitServiceHandler()
{
    TIME_HILOGI("InitServiceHandler started.");
    if (serviceHandler_ != nullptr) {
        TIME_HILOGI("InitServiceHandler already init.");
        return;
    }
    std::shared_ptr<AppExecFwk::EventRunner> runner = AppExecFwk::EventRunner::Create("TimeService");
    serviceHandler_ = std::make_shared<AppExecFwk::EventHandler>(runner);

    TIME_HILOGI("InitServiceHandler succeeded.");
}

bool TimeService::SetTime(const int64_t time)
{
    TIME_HILOGI("Setting time of day to milliseconds");
    if (time <= 0 || time / 1000LL >= INT_MAX) {
        TIME_HILOGE("SetTime input param error");
        return false;
    }

    struct timeval tv;
    tv.tv_sec = (time_t) (time / 1000LL);
    tv.tv_usec = (suseconds_t) ((time % 1000LL) * 1000LL);

    int result = settimeofday(&tv, NULL);
    if (result < 0) {
        TIME_HILOGE("settimeofday fail: %{public}s", strerror(errno));
        return false;
    }

    return  true;
}
} // namespace MiscServices
} // namespace OHOS
