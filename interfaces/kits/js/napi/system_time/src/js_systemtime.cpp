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

#include "js_systemtime.h"

#include <string>
#include "time_service_client.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "js_native_api.h"

#include "time_common.h"
#include <initializer_list>


using namespace OHOS::MiscServices;

static napi_value JSSystemTimeSetTime(napi_env env, napi_callback_info info)
{
    TIME_HILOGI(TIME_MODULE_JS_NAPI,"JSSystemTimeSetTime start");
    GET_PARAMS(env, info, 2);
    NAPI_ASSERT(env, argc == ONE_PARAMETER || argc == TWO_PARAMETER, "type mismatch");

    AsyncContext* asyncContext = new (std::nothrow)AsyncContext();
    asyncContext->env = env;

    for (size_t i = 0; i < argc; i++) {
        napi_valuetype valueType;
        napi_typeof(env, argv[i], &valueType);
        if (i == 0 && valueType == napi_number) {
            napi_get_value_int64(env, argv[i], &asyncContext->time);
        } else if (i == 0 && valueType == napi_object){
            bool hasProperty = false;
            napi_valuetype resValueType;
            NAPI_CALL(env, napi_has_named_property(env, argv[i], "getTime", &hasProperty));
            NAPI_ASSERT(env, hasProperty, "type expected.");
            napi_value getTimeFunc = nullptr;
            napi_get_named_property(env, argv[i], "getTime", &getTimeFunc);
            napi_value getTimeResult = nullptr;
            napi_call_function(env, argv[i], getTimeFunc, 0, nullptr, &getTimeResult);
            NAPI_CALL(env, napi_typeof(env, getTimeResult, &resValueType));
            NAPI_ASSERT(env, resValueType == napi_number, "type mismatch");
            int64_t dateValue = 0;
            napi_get_value_int64(env, getTimeResult, &dateValue);
            asyncContext->time = dateValue;
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

    napi_create_async_work(env, nullptr, resource,[](napi_env env, void* data) {
            AsyncContext* asyncContext = (AsyncContext*)data;
            bool setTimeResult;
            setTimeResult = TimeServiceClient::GetInstance()->SetTime(asyncContext->time);

            if (setTimeResult) {
                asyncContext->status = RESOLVED;
            } else {
                asyncContext->status = REJECT;
            }
        },
        [](napi_env env, napi_status status, void* data) {
            AsyncContext* asyncContext = (AsyncContext*)data;
            napi_value result[2] = { 0 };
            if (asyncContext->status == RESOLVED) {
                napi_get_undefined(env, &result[0]);
                napi_get_boolean(env, true, &result[1]);
            } else {
                napi_value message = nullptr;
                napi_create_string_utf8(env, "Set fail", NAPI_AUTO_LENGTH, &message);
                napi_create_error(env, nullptr, message, &result[0]);
                napi_get_undefined(env, &result[1]);
            }
            if (asyncContext->deferred) {
                if (asyncContext->status == RESOLVED) {
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

static napi_value JSSystemTimeSetTimeZone(napi_env env, napi_callback_info info){
    TIME_HILOGI(TIME_MODULE_JS_NAPI,"JSSystemTimeSetTimeZone start");
    GET_PARAMS(env, info, 2);
    NAPI_ASSERT(env, argc == ONE_PARAMETER || argc == TWO_PARAMETER, "type mismatch");

    AsyncContext* asyncContext = new AsyncContext();
    asyncContext->env = env;

    for (size_t i = 0; i < argc; i++) {
        napi_valuetype valueType;
        napi_typeof(env, argv[i], &valueType);
        if (i == 0 && valueType == napi_string) {
            char timeZoneChars[MAX_TIME_ZONE_ID];
            size_t timeZoneCharsSize;
            if (napi_ok !=  napi_get_value_string_utf8(env, argv[i], timeZoneChars, MAX_TIME_ZONE_ID-1, &timeZoneCharsSize)){
               delete asyncContext;
               NAPI_ASSERT(env, false, "input para invalid");
            }
            std::string timeZoneStr(timeZoneChars, timeZoneCharsSize);
            asyncContext->timeZone = timeZoneStr;
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
    napi_create_string_utf8(env, "JSSystemTimeSetTimeZone", NAPI_AUTO_LENGTH, &resource);

    napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            AsyncContext* asyncContext = (AsyncContext*)data;
            auto setTimeResult = TimeServiceClient::GetInstance()->SetTimeZone(asyncContext->timeZone);
            if (setTimeResult) {
                asyncContext->status = RESOLVED;
            } else {
                asyncContext->status = REJECT;
            }
        },
        [](napi_env env, napi_status status, void* data) {
            AsyncContext* asyncContext = (AsyncContext*)data;
            napi_value result[2] = { 0 };
            if (asyncContext->status == RESOLVED) {
                napi_get_undefined(env, &result[0]);
                napi_get_boolean(env, true, &result[1]);
            } else {
                napi_value message = nullptr;
                napi_create_string_utf8(env, "Set fail", NAPI_AUTO_LENGTH, &message);
                napi_create_error(env, nullptr, message, &result[0]);
                napi_get_undefined(env, &result[1]);
            }
            if (asyncContext->deferred) {
                if (asyncContext->status == RESOLVED) {
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
        (void*)asyncContext, 
        &asyncContext->work);
    napi_queue_async_work(env, asyncContext->work);
    return result;
}

EXTERN_C_START
napi_value SystemTimeExport(napi_env env, napi_value exports)
{
    static napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("setTime", JSSystemTimeSetTime), 
        DECLARE_NAPI_FUNCTION("setDate", JSSystemTimeSetTime),
        DECLARE_NAPI_FUNCTION("setTimezone", JSSystemTimeSetTimeZone)
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