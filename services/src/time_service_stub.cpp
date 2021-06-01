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

#include "time_service_stub.h"
#include <cinttypes>
#include "parcel.h"
#include "ipc_skeleton.h"
#include "time_log.h"
#include "time_common.h"

namespace OHOS {
namespace MiscServices {
using namespace OHOS::HiviewDFX;

int32_t TimeServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    TIME_HILOGI("OnRemoteRequest started, code = %{public}d", code);

    auto descriptorToken = data.ReadInterfaceToken();
    if (descriptorToken != GetDescriptor()) {
        TIME_HILOGE("Remote descriptor not the same as local descriptor.");
        return E_TRANSACT_ERROR;
    }

    switch (code) {
        case SET_TIME:
            return OnSetTime(data, reply);
        default:
            TIME_HILOGE("Default value received, check needed.");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
}

int32_t TimeServiceStub::OnSetTime(Parcel& data, Parcel& reply)
{
    int64_t time = data.ReadInt64();
    bool result = SetTime(time);
    CHECK_RET((!reply.WriteBool(result)), TIME_HILOGE("WriteBool failed"), E_WRITE_PARCEL_ERROR);
    return SUCCESS;
}

bool TimeServiceStub::SetTime(const int64_t time)
{
    return false;
}
} // namespace MiscServices
} // namespace OHOS