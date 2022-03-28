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
#include <chrono>
#include <thread>
#include <cinttypes>
#include <ctime>
#include "time_common.h"
#include "time_service_notify.h"
#include "timer_manager_interface.h"
#include "time_service.h"
#include "common_timer_errors.h"
#include "time_tick_notify.h"

using namespace std::chrono;

namespace OHOS {
namespace MiscServices {
namespace {
constexpr uint64_t MINUTE_TO_MILLISECOND = 60000;
constexpr uint64_t MICRO_TO_MILESECOND = 1000;
}

TimeTickNotify::TimeTickNotify() : timer_("TickTimer") {};
TimeTickNotify::~TimeTickNotify() {};

void TimeTickNotify::Init()
{
    TIME_HILOGD(TIME_MODULE_SERVICE, "Tick notify start.");
    
    uint32_t ret = timer_.Setup();
    if (ret != Utils::TIMER_ERR_OK) {
        TIME_HILOGE(TIME_MODULE_SERVICE, "Timer Setup failed: %{public}d", ret);
        return;
    }
    auto callback = [this]() {
        this->Callback();
    };
    RefreshNextTriggerTime();
    TIME_HILOGD(TIME_MODULE_SERVICE, "Tick notify triggertime: %{public}" PRId64 "", nextTriggerTime_);
    timerId_ = timer_.Register(callback, nextTriggerTime_);
    TIME_HILOGD(TIME_MODULE_SERVICE, "Tick timer ID: %{public}d", timerId_);
}

void TimeTickNotify::Callback()
{
    auto currentTime = steady_clock::now().time_since_epoch().count();
    DelayedSingleton<TimeServiceNotify>::GetInstance()->PublishTimeTickEvents(currentTime);
    timer_.Unregister(timerId_);
    RefreshNextTriggerTime();
    auto callback = [this]() {
        this->Callback();
    };
    timerId_ = timer_.Register(callback, nextTriggerTime_);
    TIME_HILOGD(TIME_MODULE_SERVICE, "Tick notify triggertime: %{public}" PRId64 "", nextTriggerTime_);
    TIME_HILOGD(TIME_MODULE_SERVICE, "Tick timer ID: %{public}d", timerId_);
}

void TimeTickNotify::RefreshNextTriggerTime()
{
    time_t t = time(NULL);
    struct tm *tblock = localtime(&t);
    TIME_HILOGI(TIME_MODULE_SERVICE, "Time now: %{public}s", asctime(tblock));
    auto UTCTimeMicro = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
    TIME_HILOGD(TIME_MODULE_SERVICE, "Time Now Mirc: %{public}lld", UTCTimeMicro);
    auto timeMilliseconds = GetMillisecondsFromUTC(UTCTimeMicro);
    nextTriggerTime_ = MINUTE_TO_MILLISECOND - timeMilliseconds;
}

void TimeTickNotify::Stop()
{
    TIME_HILOGD(TIME_MODULE_SERVICE, "start.");
    timer_.Shutdown();
}

uint64_t TimeTickNotify::GetMillisecondsFromUTC(uint64_t UTCtimeMicro)
{
    TIME_HILOGD(TIME_MODULE_SERVICE, "Time micro: %{public}" PRId64 "", UTCtimeMicro);
    auto miliseconds = (UTCtimeMicro / MICRO_TO_MILESECOND) % MINUTE_TO_MILLISECOND;
    TIME_HILOGD(TIME_MODULE_SERVICE, "Time mili: %{public}" PRId64 "", miliseconds);
    return miliseconds;
}
} // MiscServices
} // OHOS

