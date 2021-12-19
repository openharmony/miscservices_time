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

#include "time_zone_info.h"
#include "time_file_utils.h"

namespace OHOS {
namespace MiscServices {
namespace {
const std::string TIMEZONE_FILE_PATH = "/data/misc/zoneinfo/timezone.json";
}

TimeZoneInfo::TimeZoneInfo()
{
    std::vector<struct zoneInfoEntry> timezoneList = {
        {"Antarctica/McMurdo", "AQ", 12},
        {"America/Argentina/Buenos_Aires", "AR", -3},
        {"Australia/Sydney", "AU", 10},
        {"America/Noronha", "BR", -2},
        {"America/St_Johns", "CA", -3},
        {"Africa/Kinshasa", "CD", 1},
        {"America/Santiago", "CL", -3},
        {"Asia/Shanghai", "CN", 8},
        {"Asia/Nicosia", "CY", 3},
        {"Europe/Berlin", "DE", 2},
        {"America/Guayaquil", "CEST", -5},
        {"Europe/Madrid", "ES", 2},
        {"Pacific/Pohnpei", "FM", 11},
        {"America/Godthab", "GL", -2},
        {"Asia/Jakarta", "ID", 7},
        {"Pacific/Tarawa", "KI", 12},
        {"Asia/Almaty", "KZ", 6},
        {"Pacific/Majuro", "MH", 12},
        {"Asia/Ulaanbaatar", "MN", 8},
        {"America/Mexico_City", "MX", -5},
        {"Asia/Kuala_Lumpur", "MY", 8},
        {"Pacific/Auckland", "NZ", 12},
        {"Pacific/Tahiti", "PF", -10},
        {"Pacific/Port_Moresby", "PG", 10},
        {"Asia/Gaza", "PS", 3},
        {"Europe/Lisbon", "PT", 1},
        {"Europe/Moscow", "RU", 3},
        {"Europe/Kiev", "UA", 3},
        {"Pacific/Wake", "UM", 12},
        {"America/New_York", "US", -4},
        {"Asia/Tashkent", "UZ", 5}
    };

    for (auto tz : timezoneList) {
        timezoneInfoMap_[tz.ID] = tz;
    }
}

TimeZoneInfo::~TimeZoneInfo()
{
    timezoneInfoMap_.clear();
}

void TimeZoneInfo::Init()
{
    TIME_HILOGD(TIME_MODULE_SERVICE, "start.");
    std::string timezoneId;
    int gmtOffset;
    if (!InitStorage()) {
        TIME_HILOGE(TIME_MODULE_SERVICE, "end, InitStorage failed.");
        return;
    }
    if (!GetTimezoneFromFile(timezoneId)) {
        TIME_HILOGE(TIME_MODULE_SERVICE, "end, GetTimezoneFromFile failed.");
        timezoneId = "Asia/Shanghai";
    }
    if (!GetOffsetById(timezoneId, gmtOffset)) {
        TIME_HILOGE(TIME_MODULE_SERVICE, "end, GetOffsetById failed.");
        return;
    }
    if (!SetOffsetToKernel(gmtOffset)) {
        TIME_HILOGE(TIME_MODULE_SERVICE, "end, SetOffsetToKernel failed.");
        return;
    }
    curTimezoneId_ = timezoneId;
    TIME_HILOGD(TIME_MODULE_SERVICE, "end.");
}

bool TimeZoneInfo::InitStorage()
{
    auto filePath = TIMEZONE_FILE_PATH.c_str();
    if (!TimeFileUtils::IsExistFile(filePath)) {
        TIME_HILOGD(TIME_MODULE_SERVICE, "Timezone file not existed :%{public}s.", filePath);
        const std::string dir = TimeFileUtils::GetPathDir(filePath);
        if (dir.empty()) {
            TIME_HILOGE(TIME_MODULE_SERVICE, "Filepath invalid.");
            return false;
        }
        if (!TimeFileUtils::IsExistDir(dir.c_str())) {
            if (!TimeFileUtils::MkRecursiveDir(dir.c_str(), true)) {
                TIME_HILOGE(TIME_MODULE_SERVICE, "Create filepath failed :%{public}s.", filePath);
                return false;
            }
            TIME_HILOGD(TIME_MODULE_SERVICE, "Create filepath success :%{public}s.", filePath);
        }
    }
    return true;
}

bool TimeZoneInfo::SetTimezone(std::string timezoneId)
{
    int gmtOffset;
    if (!GetOffsetById(timezoneId, gmtOffset)) {
        TIME_HILOGE(TIME_MODULE_SERVICE, "Timezone unsupport %{public}s.", timezoneId.c_str());
        return false;
    }
    if (!SetOffsetToKernel(gmtOffset)) {
        TIME_HILOGE(TIME_MODULE_SERVICE, "Set kernel failed.");
        return false;
    }
    if (!SaveTimezoneToFile(timezoneId)) {
        TIME_HILOGE(TIME_MODULE_SERVICE, "Save file failed.");
        return false;
    }
    curTimezoneId_ = timezoneId;
    return true;
}

bool TimeZoneInfo::GetTimezone(std::string &timezoneId)
{
    timezoneId = curTimezoneId_;
    return true;
}

bool TimeZoneInfo::SetOffsetToKernel(int offsetHour)
{
    std::stringstream TZstrs;
    time_t timeNow;
    TZstrs << "UTC-" << offsetHour;
    (void)time(&timeNow);
    if (setenv("TZ", TZstrs.str().data(), 1) == 0) {
        tzset();
        (void)time(&timeNow);
        return true;
    }
    TIME_HILOGE(TIME_MODULE_SERVICE, "Set timezone failed %{public}s", TZstrs.str().data());
    return false;
}

bool TimeZoneInfo::GetTimezoneFromFile(std::string &timezoneId)
{
    Json::Value root;
    std::ifstream ifs;
    ifs.open(TIMEZONE_FILE_PATH);
    Json::CharReaderBuilder builder;
    builder["collectComments"] = true;
    JSONCPP_STRING errs;
    if (!parseFromStream(builder, ifs, &root, &errs)) {
        ifs.close();
        TIME_HILOGE(TIME_MODULE_SERVICE, "Read file failed %{public}s.", errs.c_str());
        return false;
    }
    timezoneId = root["TimezoneId"].asString();
    TIME_HILOGE(TIME_MODULE_SERVICE, "Read file %{public}s.", timezoneId.c_str());
    ifs.close();
    return true;
}

bool TimeZoneInfo::SaveTimezoneToFile(std::string timezoneId)
{
    std::ofstream ofs;
    ofs.open(TIMEZONE_FILE_PATH);
    Json::Value root;
    root["TimezoneId"] = timezoneId;
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    ofs << json_file;
    ofs.close();
    TIME_HILOGD(TIME_MODULE_SERVICE, "Write file %{public}s.", timezoneId.c_str());
    return true;
}

bool TimeZoneInfo::GetOffsetById(const std::string timezoneId, int &offset)
{
    auto itEntry = timezoneInfoMap_.find(timezoneId);
    if (itEntry != timezoneInfoMap_.end()) {
        auto zoneInfo = itEntry->second;
        offset = zoneInfo.utcOffsetHours;
        return true;
    }
    TIME_HILOGE(TIME_MODULE_SERVICE, "TimezoneId not found.");
    return false;
}
}
}