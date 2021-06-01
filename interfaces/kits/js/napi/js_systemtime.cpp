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

#include <string>
#include "time_service_manager.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "time_log.h"

#define GET_PARAMS(env, info, num) \
    size_t argc = num;             \
    napi_value argv[num];          \
    napi_value thisVar;            \
    void* data;                    \
    napi_get_cb_info(env, info, &argc, argv, &thisVar, &data)

typedef struct _SetTimeAsyncContext {
    napi_env env;
    napi_async_work work;

    int64_t time;
    napi_deferred deferred;
    napi_ref callbackRef;

    int status;
} SetTimeAsyncContext;

static napi_value JSSystemTimeSetTime(napi_env env, napi_callback_info info)
{
    TIME_HILOGI("JSSystemTimeSetTime start");
    GET_PARAMS(env, info, 2);
    NAPI_ASSERT(env, argc >= 1, "requires 1 parameter");

    SetTimeAsyncContext* asyncContext = new SetTimeAsyncContext();
    asyncContext->env = env;

    for (size_t i = 0; i < argc; i++) {
        napi_valuetype valueType;
        napi_typeof(env, argv[i], &valueType);
        if (i == 0 && valueType == napi_number) {
            napi_get_value_int64(env, argv[i], &asyncContext->time);
        } else if (i == 1 && valueType == napi_function) {
            napi_create_reference(env, argv[i], 1, &asyncContext->callbackRef);
        } else {
            delete asyncContext;
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }

    napi_value result = nullptr;

    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &result);
    } else {
        napi_get_undefined(env, &result);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JSSystemTimeSetTime", NAPI_AUTO_LENGTH, &resource);

    napi_create_async_work(
        env, nullptr, resource,
        [](napi_env env, void* data) {
            SetTimeAsyncContext* asyncContext = (SetTimeAsyncContext*)data;
            bool setTimeResult = OHOS::MiscServices::TimeServiceManager::GetInstance()->SetTime(asyncContext->time);
            if (setTimeResult) {
                asyncContext->status = 0;
            } else {
                asyncContext->status = 1;
            }
        },
        [](napi_env env, napi_status status, void* data) {
            SetTimeAsyncContext* asyncContext = (SetTimeAsyncContext*)data;
            napi_value result[2] = { 0 };
            if (!asyncContext->status) {
                napi_get_undefined(env, &result[0]);
                napi_get_boolean(env, true, &result[1]);
            } else {
                napi_value message = nullptr;
                napi_create_string_utf8(env, "SetTime fail", NAPI_AUTO_LENGTH, &message);
                napi_create_error(env, nullptr, message, &result[0]);
                napi_get_undefined(env, &result[1]);
            }
            if (asyncContext->deferred) {
                if (!asyncContext->status) {
                    napi_resolve_deferred(env, asyncContext->deferred, result[1]);
                } else {
                    napi_reject_deferred(env, asyncContext->deferred, result[0]);
                }
            } else {
                napi_value callback = nullptr;
                napi_get_reference_value(env, asyncContext->callbackRef, &callback);
                // 2 -> result size
                napi_call_function(env, nullptr, callback, 2, result, nullptr);
                napi_delete_reference(env, asyncContext->callbackRef);
            }
            napi_delete_async_work(env, asyncContext->work);
            delete asyncContext;
        },
        (void*)asyncContext, &asyncContext->work);
    napi_queue_async_work(env, asyncContext->work);

    return result;
}

EXTERN_C_START
napi_value SystemTimeExport(napi_env env, napi_value exports)
{
    static napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("setTime", JSSystemTimeSetTime)
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}
EXTERN_C_END

static napi_module system_time_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = SystemTimeExport,
    .nm_modname = "systemTime",
    .nm_priv = ((void*)0),
    .reserved = {0}
};

extern "C" __attribute__((constructor)) void SystemTimeRegister()
{
    napi_module_register(&system_time_module);
}