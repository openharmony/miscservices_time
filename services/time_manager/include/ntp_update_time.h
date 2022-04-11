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

#ifndef NTP_UPDATE_TIME_H
#define NTP_UPDATE_TIME_H

#include <singleton.h>

namespace OHOS {
namespace MiscServices {
struct autoTimeInfo {
    std::string NTP_SERVER;
    std::string status;
    int64_t lastUpdateTime;
};

class NtpUpdateTime : public DelayedSingleton<NtpUpdateTime> {
    DECLARE_DELAYED_SINGLETON(NtpUpdateTime);
public:
    DISALLOW_COPY_AND_MOVE(NtpUpdateTime);
    void RefreshNetworkTimeByTimer(const uint64_t timerid);
    void UpdateStatusOff();
    void UpdateStatusOn();
    void UpdateNITZSetTime();
    void SetSystemTime();
    void Stop();
    void Init();
private:
    bool GetAutoTimeInfoFromFile(autoTimeInfo &info);
    bool SaveAutoTimeInfoToFile(autoTimeInfo &info);
    void SubscriberNITZTimeChangeCommonEvent();
    bool ThreadSetSystemTime();
    void StartTimer();
    void RefreshNextTriggerTime();
    bool CheckStatus();
    bool IsNITZTimeInvalid();
    autoTimeInfo autoTimeInfo_;
    uint64_t timerId_;
    uint64_t nitzUpdateTimeMili_;
    uint64_t nextTriggerTime_;
};
} // MiscServices
} // OHOS
#endif