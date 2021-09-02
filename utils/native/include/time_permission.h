/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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


#ifndef TIME_PERMISSION_H
#define TIME_PERMISSION_H

#include "bundle_mgr_interface.h"
#include "time_common.h"
#include "mock_permission.h"
#include "system_ability_definition.h"
#include "iservice_registry.h"
#include "refbase.h"

#include <mutex>
#include <string>
namespace OHOS {
namespace MiscServices {
class TimePermission : public RefBase{
public:
    static sptr<TimePermission> GetInstance();
    bool CheckSelfPermission(const std::string permName);
    bool CheckCallingPermission(const int32_t uid, const std::string permName);

private:
    TimePermission();
    ~TimePermission();
    sptr<AppExecFwk::IBundleMgr> GetBundleManager();

    static std::mutex instanceLock_;
    static sptr<TimePermission> instance_;
    static sptr<AppExecFwk::IBundleMgr> bundleMgrProxy_;
};
} // namespace MiscServices
} // namespace OHOS
#endif // TIME_PERMISSION_H
