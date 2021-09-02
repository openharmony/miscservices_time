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
#include "time_common.h"

#include <vector>

namespace OHOS{
namespace MiscServices{

std::mutex TimeZoneInfo::instanceLock_;
sptr<TimeZoneInfo> TimeZoneInfo::instance_;

TimeZoneInfo::TimeZoneInfo()
{
    std::vector<struct zoneInfoEntry> timezoneList = { 
        {"Anadyr, Russia", "ANA", 12, "Anadyr Time"},
        {"Honiara, SolomonIslands", "SBT", 11, "Solomon Islands Time"},
        {"Melbourne, Australia", "AEST", 10, "Australian Eastern Standard Time"},
        {"Tokyo, Japan", "JST", 9, "Japan Standard Time"},
        {"Beijing, China", "CST", 8, "China Standard Time"},
        {"Jakarta, Indonesia", "WIB", 7, "Western Indonesian Time"},
        {"Dhaka, Bangladesh", "BST", 6, "Bangladesh Standard Time"},
        {"Tashkent, Uzbekistan", "UZT", 5, "Uzbekistan Time"},
        {"Dubai, U.A.E.", "GST", 4, "Gulf Standard Time"},
        {"Moscow, Russia", "MSK", 3, "Moscow Standard Time"},
        {"Brussels, Belgium", "CEST", 2, "Central European Summer Time"},
        {"London, England", "BST", 1, "British Summer Time"},
        {"Accra, Ghana", "GMT", 0, "Greenwich Mean Time"},
        {"Accra, Ghana", "UTC", 0, "Universal Time Coordinated"},
        {"Praia, CaboVerde", "CVT", -1, "Cabo Verde Time"},
        {"Nuuk, Greenland", "WGS", -2, "Western Greenland Summer Time"},
        {"Buenos Aires, Argentina", "ART", -3, "Argentina Time"},
        {"New York, U.S.A.", "EDT", -4, "Eastern Daylight Time"},
        {"Mexico City, Mexico", "CDT", -5, "Central Daylight Time"},
        {"Guatemala City, Guatemala", "CST", -6, "Central Standard Time"},
        {"Los Angeles, U.S.A.", "PDT", -7, "Pacific Daylight Time"},
        {"Anchorage, U.S.A.", "AKD", -8, "Alaska Daylight Time"},
        {"Adak, U.S.A.", "HDT", -9, "Hawaii-Aleutian Daylight Time"},
        {"Honolulu, U.S.A.", "HST", -10, "Hawaii Standard Time"},
        {"Alofi, Niue", "NUT", -11, "Niue Time"},
        {"Baker Island, U.S.A.", "AoE", -12, "Anywhere on Earth"},
    };

    for (auto tz : timezoneList) {
        timezoneInfoMap_[tz.ID] = tz;
    }
    
}

TimeZoneInfo::~TimeZoneInfo(){
    timezoneInfoMap_.clear();
}

int32_t TimeZoneInfo::GetOffset(const std::string timezoneId, int &offset){
    auto itEntry = timezoneInfoMap_.find(timezoneId);
    if (itEntry != timezoneInfoMap_.end()) {
        auto zoneInfo = itEntry->second;
        offset = zoneInfo.utcOffsetHours;
        curTimezoneId_ = timezoneId;
        return ERR_OK;
    }
    TIME_HILOGE(TIME_MODULE_SERVICE, "TimezoneId not found.");
    return E_TIME_NOT_FOUND;
}

int32_t TimeZoneInfo::GetTimezoneId( std::string &timezoneId ){
    timezoneId = curTimezoneId_;
    return ERR_OK;
}

sptr<TimeZoneInfo> TimeZoneInfo::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> autoLock(instanceLock_);
        if (instance_ == nullptr) {
            instance_ = new TimeZoneInfo;
        }
    }
    return instance_;
}

}
}