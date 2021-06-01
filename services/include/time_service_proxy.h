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

#ifndef SERVICES_INCLUDE_TIME_SERVICE_PROXY_H
#define SERVICES_INCLUDE_TIME_SERVICE_PROXY_H

#include "iremote_proxy.h"
#include "time_service_interface.h"

namespace OHOS {
namespace MiscServices {
class TimeServiceProxy : public IRemoteProxy<ITimeService> {
public:
    explicit TimeServiceProxy(const sptr<IRemoteObject> &object);
    ~TimeServiceProxy() = default;
    DISALLOW_COPY_AND_MOVE(TimeServiceProxy);
    bool SetTime(const int64_t time) override;

private:
    static inline BrokerDelegator<TimeServiceProxy> delegator_;
};
} // namespace MiscServices
} // namespace OHOS
#endif // SERVICES_INCLUDE_TIME_SERVICE_PROXY_H