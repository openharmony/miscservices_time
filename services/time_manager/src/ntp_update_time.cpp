/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#include <string>
#include <fstream>
#include <mutex>

#include "ntp_trusted_time.h"
#include "time_common.h"
#include "json/json.h"
#include "time_service.h"
#include "nitz_subscriber.h"
#include "time_zone_info.h"
#include "ntp_update_time.h"

using namespace std::chrono;
namespace OHOS {
namespace MiscServices {
namespace {
constexpr uint64_t NANO_TO_MILLISECOND = 1000000;
constexpr uint64_t DAY_TO_MILLISECOND = 86400000;
constexpr uint64_t MINUTES_TO_SECOND = 20;
const std::string AUTOTIME_FILE_PATH = "/data/misc/zoneinfo/autotime.json";
const std::string NETWORK_TIME_STATUS_ON = "ON";
const std::string NETWORK_TIME_STATUS_OFF = "OFF";
const std::string NTP_CN_SERVER = "ntp.aliyun.com";
const int64_t INVALID_TIMES = -1;
const int MAX_RETRY = 30;
}

NtpUpdateTime::NtpUpdateTime() {};
NtpUpdateTime::~NtpUpdateTime() {};

void NtpUpdateTime::Init()
{
    TIME_HILOGD(TIME_MODULE_SERVICE, "Ntp Update Time start.");
    SubscriberNITZTimeChangeCommonEvent();
    if (!GetAutoTimeInfoFromFile(autoTimeInfo_)) {
        autoTimeInfo_.lastUpdateTime = INVALID_TIMES;
        autoTimeInfo_.NTP_SERVER = NTP_CN_SERVER;
        autoTimeInfo_.status = NETWORK_TIME_STATUS_ON;
        if (!SaveAutoTimeInfoToFile(autoTimeInfo_)) {
            TIME_HILOGE(TIME_MODULE_SERVICE, "end, SaveAutoTimeInfoToFile failed.");
            return;
        }
        if (!GetAutoTimeInfoFromFile(autoTimeInfo_)) {
            TIME_HILOGE(TIME_MODULE_SERVICE, "end, GetAutoTimeInfoFromFile failed.");
            return;
        }
    }
    if (autoTimeInfo_.status == NETWORK_TIME_STATUS_ON) {
        std::thread([this] {
            for (int i = 0; i < MAX_RETRY; i++) {
                if (!this->ThreadSetSystemTime()) {
                    TIME_HILOGE(TIME_MODULE_SERVICE, "thread set ntp time failed, retry");
                    std::this_thread::sleep_for(seconds(MINUTES_TO_SECOND));
                } else {
                    TIME_HILOGD(TIME_MODULE_SERVICE, "thread set ntp time success");
                    break;
                }
            }
        }).detach();
    }

    int32_t timerType = ITimerManager::TimerType::ELAPSED_REALTIME;
    auto callback = [this](uint64_t id) {
        this->RefreshNetworkTimeByTimer(id);
    };
    timerId_ = TimeService::GetInstance()->CreateTimer(timerType, 0, DAY_TO_MILLISECOND, 0, callback);
    TIME_HILOGD(TIME_MODULE_SERVICE, "Ntp update timerId: %{public}" PRId64 "", timerId_);
    RefreshNextTriggerTime();
    TIME_HILOGD(TIME_MODULE_SERVICE, "Ntp update triggertime: %{public}" PRId64 "", nextTriggerTime_);
    TimeService::GetInstance()->StartTimer(timerId_, nextTriggerTime_);
}

void NtpUpdateTime::SubscriberNITZTimeChangeCommonEvent()
{
    // Broadcast subscription
    MatchingSkills matchingSkills;
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_NITZ_TIME_UPDATED);
    CommonEventSubscribeInfo subscriberInfo(matchingSkills);
    std::shared_ptr<NITZSubscriber> subscriberPtr =
        std::make_shared<NITZSubscriber>(subscriberInfo);
    if (subscriberPtr != nullptr) {
        bool subscribeResult = CommonEventManager::SubscribeCommonEvent(subscriberPtr);
        if (!subscribeResult) {
            TIME_HILOGE(TIME_MODULE_SERVICE, "SubscribeCommonEvent failed");
        }
    }
}

void NtpUpdateTime::RefreshNetworkTimeByTimer(const uint64_t timerId)
{
    if (!(CheckStatus())) {
        TIME_HILOGD(TIME_MODULE_SERVICE, "Network time status off.");
        return;
    }
    if (IsNITZTimeInvalid()) {
        TIME_HILOGD(TIME_MODULE_SERVICE, "NITZ Time is valid.");
        return;
    }
    SetSystemTime();
    SaveAutoTimeInfoToFile(autoTimeInfo_);
    TIME_HILOGD(TIME_MODULE_SERVICE, "Ntp update triggertime: %{public}" PRId64 "", nextTriggerTime_);
}

void NtpUpdateTime::UpdateNITZSetTime()
{
    auto BootTimeNano = steady_clock::now().time_since_epoch().count();
    auto BootTimeMilli = BootTimeNano / NANO_TO_MILLISECOND;
    TIME_HILOGD(TIME_MODULE_SERVICE, "nitz time changed.");
    nitzUpdateTimeMili_ = BootTimeMilli;
}

void NtpUpdateTime::SetSystemTime()
{
    TIME_HILOGD(TIME_MODULE_SERVICE, "start.");
    if (!DelayedSingleton<NtpTrustedTime>::GetInstance()->ForceRefresh(autoTimeInfo_.NTP_SERVER)) {
        TIME_HILOGE(TIME_MODULE_SERVICE, "get ntp time failed.");
        return;
    }
    int64_t currentTime = DelayedSingleton<NtpTrustedTime>::GetInstance()->CurrentTimeMillis();
    if (currentTime == INVALID_TIMES) {
        TIME_HILOGD(TIME_MODULE_SERVICE, "Ntp update time failed");
        return;
    }
    if (currentTime <= 0) {
        TIME_HILOGD(TIME_MODULE_SERVICE, "current time invalid.");
        return;
    }
    TIME_HILOGD(TIME_MODULE_SERVICE, "Ntp UTC Time: %{public}" PRId64 "", currentTime);
    auto timeOffsetMs = DelayedSingleton<TimeZoneInfo>::GetInstance()->GetCurrentOffsetMs();
    currentTime = currentTime + timeOffsetMs;
    TIME_HILOGD(TIME_MODULE_SERVICE, "Ntp UTC+TIMEZONE tTime: %{public}" PRId64 "", currentTime);
    TimeService::GetInstance()->SetTime(currentTime);
    autoTimeInfo_.lastUpdateTime = currentTime;
    TIME_HILOGD(TIME_MODULE_SERVICE, "Ntp update currentTime: %{public}" PRId64 "", currentTime);
    TIME_HILOGD(TIME_MODULE_SERVICE, "end.");
}

bool NtpUpdateTime::ThreadSetSystemTime()
{
    TIME_HILOGD(TIME_MODULE_SERVICE, "start.");
    if (!DelayedSingleton<NtpTrustedTime>::GetInstance()->ForceRefresh(autoTimeInfo_.NTP_SERVER)) {
        TIME_HILOGE(TIME_MODULE_SERVICE, "get ntp time failed.");
        return false;
    }
    int64_t currentTime = DelayedSingleton<NtpTrustedTime>::GetInstance()->CurrentTimeMillis();
    if (currentTime == INVALID_TIMES) {
        TIME_HILOGD(TIME_MODULE_SERVICE, "Ntp update time failed");
        return false;
    }
    if (currentTime <= 0) {
        TIME_HILOGD(TIME_MODULE_SERVICE, "current time invalid.");
        return false;
    }
    TIME_HILOGD(TIME_MODULE_SERVICE, "Ntp UTC Time: %{public}" PRId64 "", currentTime);
    auto timeOffsetMs = DelayedSingleton<TimeZoneInfo>::GetInstance()->GetCurrentOffsetMs();
    currentTime = currentTime + timeOffsetMs;
    TIME_HILOGD(TIME_MODULE_SERVICE, "Ntp UTC+TIMEZONE tTime: %{public}" PRId64 "", currentTime);
    TimeService::GetInstance()->SetTime(currentTime);
    autoTimeInfo_.lastUpdateTime = currentTime;
    TIME_HILOGD(TIME_MODULE_SERVICE, "Ntp update currentTime: %{public}" PRId64 "", currentTime);
    TIME_HILOGD(TIME_MODULE_SERVICE, "end.");
    return true;
}

void NtpUpdateTime::RefreshNextTriggerTime()
{
    auto BootTimeNano = steady_clock::now().time_since_epoch().count();
    auto BootTimeMilli = BootTimeNano / NANO_TO_MILLISECOND;
    nextTriggerTime_ = BootTimeMilli +  DAY_TO_MILLISECOND;
}

void NtpUpdateTime::UpdateStatusOff()
{
    TIME_HILOGD(TIME_MODULE_SERVICE, "start");
    autoTimeInfo_.lastUpdateTime = INVALID_TIMES;
    autoTimeInfo_.NTP_SERVER = NTP_CN_SERVER;
    autoTimeInfo_.status = NETWORK_TIME_STATUS_OFF;
    if (!SaveAutoTimeInfoToFile(autoTimeInfo_)) {
        TIME_HILOGE(TIME_MODULE_SERVICE, "end, SaveAutoTimeInfoToFile failed.");
    }
    TIME_HILOGD(TIME_MODULE_SERVICE, "end");
}

void NtpUpdateTime::UpdateStatusOn()
{
    TIME_HILOGD(TIME_MODULE_SERVICE, "start");
    if (CheckStatus()) {
        TIME_HILOGD(TIME_MODULE_SERVICE, "network update time status is already on.");
        return;
    }
    SetSystemTime();
    autoTimeInfo_.status = NETWORK_TIME_STATUS_ON;
    if (!SaveAutoTimeInfoToFile(autoTimeInfo_)) {
        TIME_HILOGE(TIME_MODULE_SERVICE, "end, SaveAutoTimeInfoToFile failed.");
    }
    TIME_HILOGD(TIME_MODULE_SERVICE, "end");
}

bool NtpUpdateTime::CheckStatus()
{
    return autoTimeInfo_.status == NETWORK_TIME_STATUS_ON;
}

bool NtpUpdateTime::IsNITZTimeInvalid()
{
    auto BootTimeNano = steady_clock::now().time_since_epoch().count();
    auto BootTimeMilli = BootTimeNano / NANO_TO_MILLISECOND;
    return (BootTimeMilli - nitzUpdateTimeMili_) < DAY_TO_MILLISECOND;
}

void NtpUpdateTime::StartTimer()
{
    TimeService::GetInstance()->StartTimer(timerId_, nextTriggerTime_);
}

void NtpUpdateTime::Stop()
{
    TIME_HILOGD(TIME_MODULE_SERVICE, "start.");
    TimeService::GetInstance()->DestroyTimer(timerId_);
}

bool NtpUpdateTime::GetAutoTimeInfoFromFile(autoTimeInfo &info)
{
    Json::Value jsonValue;
    std::ifstream ifs;
    ifs.open(AUTOTIME_FILE_PATH);
    Json::CharReaderBuilder builder;
    builder["collectComments"] = true;
    JSONCPP_STRING errs;
    if (!parseFromStream(builder, ifs, &jsonValue, &errs)) {
        ifs.close();
        TIME_HILOGE(TIME_MODULE_SERVICE, "Read file failed %{public}s.", errs.c_str());
        return false;
    }
    info.status = jsonValue["status"].asString();
    info.NTP_SERVER = jsonValue["ntpServer"].asString();
    info.lastUpdateTime = jsonValue["lastUpdateTime"].asInt64();
    TIME_HILOGD(TIME_MODULE_SERVICE, "Read file %{public}s.", info.status.c_str());
    TIME_HILOGD(TIME_MODULE_SERVICE, "Read file %{public}s.", info.NTP_SERVER.c_str());
    TIME_HILOGD(TIME_MODULE_SERVICE, "Read file %{public}" PRId64 "", info.lastUpdateTime);
    ifs.close();
    return true;
}

bool NtpUpdateTime::SaveAutoTimeInfoToFile(autoTimeInfo &info)
{
    Json::Value jsonValue;
    std::ofstream ofs;
    ofs.open(AUTOTIME_FILE_PATH);
    jsonValue["status"] = info.status;
    jsonValue["ntpServer"] = info.NTP_SERVER;
    jsonValue["lastUpdateTime"] = info.lastUpdateTime;
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, jsonValue);
    ofs << json_file;
    ofs.close();
    TIME_HILOGD(TIME_MODULE_SERVICE, "Write file %{public}s.", info.status.c_str());
    TIME_HILOGD(TIME_MODULE_SERVICE, "Write file %{public}s.", info.NTP_SERVER.c_str());
    TIME_HILOGD(TIME_MODULE_SERVICE, "Write file %{public}" PRId64 "", info.lastUpdateTime);
    return true;
}
} // MiscServices
} // OHOS