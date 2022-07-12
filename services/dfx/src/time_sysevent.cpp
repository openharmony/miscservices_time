/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd.
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

#include "time_sysevent.h"

#include "hisysevent.h"
#include "time_hilog_wreapper.h"

namespace OHOS {
namespace MiscServices {
namespace {
using HiSysEventNameSpace = OHOS::HiviewDFX::HiSysEvent;
const std::string DOMAIN_STR = std::string(HiSysEventNameSpace::Domain::TIME);
} // namespace

void StatisticReporter(int32_t callerPid, int32_t callerUid, int32_t type, int64_t triggerTime, uint64_t interval)
{
    int ret = HiSysEventNameSpace::Write(DOMAIN_STR, "MISC_TIME_STATISTIC_REPORT",
        HiSysEventNameSpace::EventType::STATISTIC, "CALLER_PID", callerPid, "CALLER_UID", callerUid, "TIMER_TYPE",
        type, "TRIGGER_TIME", triggerTime, "INTERVAL", interval);
    if (ret != 0) {
        TIME_HILOGE(TIME_MODULE_SERVICE,
            "hisysevent Statistic failed! pid %{public}d,uid %{public}d,timer type %{public}d", callerPid, callerUid,
            type);
    }
}
} // namespace MiscServices
} // namespace OHOS
