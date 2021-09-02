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
#ifndef SERVICES_INCLUDE_TIME_ZONE_INFO_H
#define SERVICES_INCLUDE_TIME_ZONE_INFO_H

#include <map>
#include <mutex>
#include "refbase.h"

namespace OHOS{
namespace MiscServices{
using namespace std;

struct zoneInfoEntry
{
   std::string ID;
   std::string alias;
   int utcOffsetHours;
   std::string description;
};

class TimeZoneInfo : public RefBase{
    TimeZoneInfo();
    ~TimeZoneInfo();

public:
    static sptr<TimeZoneInfo>  GetInstance();
    void Init();
    int32_t GetOffset(const std::string timezoneId, int &offset);
    int32_t GetTimezoneId(std::string &timezoneId);

private:
    std::string curTimezoneId_;
    int32_t lastOffset_;
    static std::mutex instanceLock_;
    static sptr<TimeZoneInfo>  instance_;
    std::map<std::string, struct zoneInfoEntry> timezoneInfoMap_;
};
} // MiscServices
} // OHOS

#endif // SERVICES_INCLUDE_TIME_ZONE_INFO_H