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

#ifndef BASE_SMALLSERVICE_TIME_LOG_H
#define BASE_SMALLSERVICE_TIME_LOG_H

#include <cstdint>

#define CONFIG_TIME_HILOG
#ifdef CONFIG_TIME_HILOG
#include "hilog/log.h"

#ifdef TIME_HILOGI
#undef TIME_HILOGI
#endif

#ifdef TIME_HILOGE
#undef TIME_HILOGE
#endif

#ifdef TIME_HILOGW
#undef TIME_HILOGW
#endif

#ifdef TIME_HILOGI
#undef TIME_HILOGI
#endif

#ifdef TIME_HILOGD
#undef TIME_HILOGD
#endif

static constexpr OHOS::HiviewDFX::HiLogLabel g_SMALL_SERVICES_LABEL = {
    LOG_CORE,
    0xD001C00,
    "TimeKit"
};


#define TIME_HILOGD(fmt, ...) (void)OHOS::HiviewDFX::HiLog::Debug(g_SMALL_SERVICES_LABEL, \
    "line: %d, function: %s," fmt, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define TIME_HILOGE(fmt, ...) (void)OHOS::HiviewDFX::HiLog::Error(g_SMALL_SERVICES_LABEL, \
    "line: %d, function: %s," fmt, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define TIME_HILOGF(fmt, ...) (void)OHOS::HiviewDFX::HiLog::Fatal(g_SMALL_SERVICES_LABEL, \
    "line: %d, function: %s," fmt, __LINE__FILE__, __FUNCTION__, ##__VA_ARGS__)
#define TIME_HILOGI(fmt, ...) (void)OHOS::HiviewDFX::HiLog::Info(g_SMALL_SERVICES_LABEL, \
    "line: %d, function: %s," fmt, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define TIME_HILOGW(fmt, ...) (void)OHOS::HiviewDFX::HiLog::Warn(g_SMALL_SERVICES_LABEL,\
    "line: %d, function: %s," fmt, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#else

#define TIME_HILOGF(...)
#define TIME_HILOGE(...)
#define TIME_HILOGW(...)
#define TIME_HILOGI(...)
#define TIME_HILOGD(...)
#endif // CONFIG_HILOG

#define CHECK_RET(bCondition, action, ret) if ((bCondition)) {action; return ret;}
#define CHECK_VOID_RET(bCondition, action) if ((bCondition)) {action; return;}
#define LOG_WRITE_PARCEL_ERROR  TIME_HILOGE("write parcel data ret %{public}d", ret);

#endif // BASE_SMALLSERVICE_TIME_LOG_H
