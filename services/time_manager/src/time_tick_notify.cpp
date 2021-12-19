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
#include "time_common.h"
#include "time_service_notify.h"
#include "timer_manager_interface.h"
#include "time_service.h"
#include "time_tick_notify.h"
using namespace std::chrono;

namespace OHOS {
namespace MiscServices {
namespace {
constexpr uint64_t SECOND_TO_MILESECOND = 1000;
constexpr uint64_t MINUTE_TO_MILLISECOND = 6000;
constexpr uint64_t NANO_TO_SECOND = 1000000000;
constexpr uint64_t  NANO_TO_MILESECOND = 100000;
constexpr uint64_t SECOND_TO_MINUTE = 60;
}
TimeTickNotify::TimeTickNotify() {};
TimeTickNotify::~TimeTickNotify() {};

void TimeTickNotify::Init()
{
    TIME_HILOGD(TIME_MODULE_SERVICE, "Tick notify start.");
    int32_t timerType = ITimerManager::TimerType::ELAPSED_REALTIME;
    auto callback = [this](uint64_t id) {
        this->Callback(id);
    };
    timerId_ = TimeService::GetInstance()->CreateTimer(timerType, 0, 0, 0, callback);
    TIME_HILOGD(TIME_MODULE_SERVICE, "Tick notify timerId: %{public}" PRId64 "", timerId_);
    RefreshNextTriggerTime();
    TIME_HILOGD(TIME_MODULE_SERVICE, "Tick notify triggertime: %{public}" PRId64 "", nextTriggerTime_);
    TimeService::GetInstance()->StartTimer(timerId_, nextTriggerTime_);
}

void TimeTickNotify::Callback(const uint64_t timerId)
{
    TIME_HILOGD(TIME_MODULE_SERVICE, "Tick notify timerId: %{public}" PRId64 "", timerId);
    auto currentTime = steady_clock::now().time_since_epoch().count();
    DelayedSingleton<TimeServiceNotify>::GetInstance()->PublishTimeTickEvents(currentTime);
    timerId_ = timerId;
    RefreshNextTriggerTime();
    auto startFunc = [this]() {
        this->StartTimer();
    };
    std::thread startTimerThread(startFunc);
    startTimerThread.detach();
    TIME_HILOGD(TIME_MODULE_SERVICE, "Tick notify triggertime: %{public}" PRId64 "", nextTriggerTime_);
}

void TimeTickNotify::RefreshNextTriggerTime()
{
    uint64_t timeNowMilliseconds;
    auto UTCTimeNano = system_clock::now().time_since_epoch().count();
    auto BootTimeNano = steady_clock::now().time_since_epoch().count();
    auto BootTimeMilli = BootTimeNano / NANO_TO_MILESECOND;
    auto timeMilliseconds = GetMillisecondsFromUTC(UTCTimeNano);
    auto timeSeconds = GetSecondsFromUTC(UTCTimeNano);
    timeNowMilliseconds = timeSeconds * SECOND_TO_MILESECOND + timeMilliseconds;
    nextTriggerTime_ = BootTimeMilli+ (MINUTE_TO_MILLISECOND - timeNowMilliseconds);
    return ;
}

void TimeTickNotify::StartTimer()
{
    TimeService::GetInstance()->StartTimer(timerId_, nextTriggerTime_);
}

void TimeTickNotify::Stop()
{
    TIME_HILOGD(TIME_MODULE_SERVICE, "start.");
    TimeService::GetInstance()->DestroyTimer(timerId_);
}

uint64_t TimeTickNotify::GetMillisecondsFromUTC(uint64_t UTCtimeNano)
{
    return (UTCtimeNano / NANO_TO_MILESECOND) % SECOND_TO_MILESECOND;
}

uint64_t TimeTickNotify::GetSecondsFromUTC(uint64_t UTCtimeNano)
{
    return (UTCtimeNano / NANO_TO_SECOND) % SECOND_TO_MINUTE;
}
} // MiscServices
} // OHOS

