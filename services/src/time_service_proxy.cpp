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

#include "time_service_proxy.h"
#include "iremote_broker.h"
#include "time_log.h"
#include "time_common.h"

namespace OHOS {
namespace MiscServices {
using namespace OHOS::HiviewDFX;

TimeServiceProxy::TimeServiceProxy(const sptr<IRemoteObject> &object) : IRemoteProxy<ITimeService>(object)
{
}

bool TimeServiceProxy::SetTime(const int64_t time)
{
    MessageParcel data, reply;
    MessageOption option;

    data.WriteInterfaceToken(GetDescriptor());
    data.WriteInt64(time);

    int32_t ret = Remote()->SendRequest(SET_TIME, data, reply, option);
    if (ret != SUCCESS) {
        TIME_HILOGE("SetTime, ret = %{public}d", ret);
        return false;
    }
    bool result = reply.ReadBool();
    return result;
}
} // namespace MiscServices
} // namespace OHOS