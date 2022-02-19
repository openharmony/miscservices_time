
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

#ifndef SNTP_CLIENT_NTP_TRUSTED_TIME_H
#define SNTP_CLIENT_NTP_TRUSTED_TIME_H

#include <singleton.h>
#include <string>
#include <fstream>
#include <mutex>
#include <sys/time.h>
#include "refbase.h"
#include "time.h"
#include "time_common.h"
#include "json/json.h"

namespace OHOS {
namespace MiscServices {
class NtpTrustedTime : public DelayedSingleton<NtpTrustedTime> {
    DECLARE_DELAYED_SINGLETON(NtpTrustedTime)
public:
    bool ForceRefresh(std::string ntpServer);
    bool HasCache();
    int64_t GetCacheAge();
    int64_t CurrentTimeMillis();
    int64_t GetCachedNtpTime();
    int64_t GetCachedNtpTimeReference();
    class TimeResult : std::enable_shared_from_this<TimeResult> {
    public:
        TimeResult();
        TimeResult(int64_t mTimeMillis, int64_t mElapsedRealtimeMills, int64_t mCertaintyMillis);
        ~TimeResult();
        int64_t GetTimeMillis();
        int64_t GetElapsedRealtimeMillis();
        int64_t GetCertaintyMillis();
        int64_t CurrentTimeMillis();
        int64_t GetAgeMillis();
        void Clear();
    private:
        int64_t mTimeMillis;
        int64_t mElapsedRealtimeMillis;
        int64_t mCertaintyMillis;
    };

private:
    std::shared_ptr<TimeResult> mTimeResult;
};
} // MiscServices
} // OHOS

#endif // SNTP_CLIENT_NTP_TRUSTED_TIME_H
