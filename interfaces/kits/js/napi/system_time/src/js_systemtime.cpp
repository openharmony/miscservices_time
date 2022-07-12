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
#include <initializer_list>

#include "time_service_client.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "js_native_api.h"
#include "time_common.h"
#include "js_systemtime.h"

namespace OHOS {
namespace MiscServicesNapi {
using namespace OHOS::MiscServices;
const size_t ARGC_TYPE_SET1 = 1;
const size_t ARGC_TYPE_SET2 = 2;

napi_value TimeGetCallbackErrorValue(napi_env env, int errCode)
{
    napi_value result = nullptr;
    napi_value eCode = nullptr;
    NAPI_CALL(env, napi_create_int32(env, errCode, &eCode));
    NAPI_CALL(env, napi_create_object(env, &result));
    NAPI_CALL(env, napi_set_named_property(env, result, "code", eCode));
    return result;
}

napi_value TimeNapiGetNull(napi_env env)
{
    napi_value result = nullptr;
    napi_get_null(env, &result);
    return result;
}

void TimeSetPromise(const napi_env &env, const napi_deferred &deferred, const int &errorCode, const napi_value &result)
{
    if (errorCode == NO_ERROR) {
        NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, deferred, result));
        return;
    }
    NAPI_CALL_RETURN_VOID(env, napi_reject_deferred(env, deferred, result));
}

void TimeSetCallback(const napi_env &env, const napi_ref &callbackIn, const int &errorCode, const napi_value &result)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value callback = nullptr;
    napi_value resultout = nullptr;
    napi_get_reference_value(env, callbackIn, &callback);
    napi_value results[ARGS_TWO] = {0};
    results[PARAM0] = TimeGetCallbackErrorValue(env, errorCode);
    results[PARAM1] = result;
    NAPI_CALL_RETURN_VOID(env, napi_call_function(env, undefined, callback, ARGS_TWO, &results[PARAM0], &resultout));
}

void TimeReturnCallbackPromise(const napi_env &env, const TimeCallbackPromiseInfo &info, const napi_value &result)
{
    if (info.isCallback) {
        TimeSetCallback(env, info.callback, info.errorCode, result);
    } else {
        TimeSetPromise(env, info.deferred, info.errorCode, result);
    }
}

napi_value TimeJSParaError(const napi_env &env, const napi_ref &callback)
{
    if (callback) {
        return TimeNapiGetNull(env);
    } else {
        napi_value promise = nullptr;
        napi_deferred deferred = nullptr;
        napi_create_promise(env, &deferred, &promise);
        TimeSetPromise(env, deferred, ERROR, TimeNapiGetNull(env));
        return promise;
    }
}

void TimePaddingAsyncCallbackInfo(const napi_env &env,
    AsyncContext *&asynccallbackinfo,
    const napi_ref &callback,
    napi_value &promise)
{
    if (callback) {
        asynccallbackinfo->callbackRef = callback;
        asynccallbackinfo->isCallback = true;
    } else {
        napi_deferred deferred = nullptr;
        NAPI_CALL_RETURN_VOID(env, napi_create_promise(env, &deferred, &promise));
        asynccallbackinfo->deferred = deferred;
        asynccallbackinfo->isCallback = false;
    }
}

napi_value ParseParametersBySetTime(const napi_env &env, const napi_value (&argv)[SET_TIME_MAX_PARA],
    const size_t &argc, int64_t &times, napi_ref &callback)
{
    NAPI_ASSERT(env, argc >= SET_TIME_MAX_PARA - 1, "Wrong number of arguments");
    napi_valuetype valueType = napi_undefined;

    // argv[0]: times or date object
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    NAPI_ASSERT(env, valueType == napi_number || valueType == napi_object, "Wrong argument type. string expected.");
    if (valueType == napi_number) {
        napi_get_value_int64(env, argv[0], &times);
        NAPI_ASSERT(env, times >= 0, "Wrong argument timer. Positive number expected.");
    } else {
        bool hasProperty = false;
        napi_valuetype resValueType = napi_undefined;
        NAPI_CALL(env, napi_has_named_property(env, argv[0], "getTime", &hasProperty));
        NAPI_ASSERT(env, hasProperty, "type expected.");
        napi_value getTimeFunc = nullptr;
        napi_get_named_property(env, argv[0], "getTime", &getTimeFunc);
        napi_value getTimeResult = nullptr;
        napi_call_function(env, argv[0], getTimeFunc, 0, nullptr, &getTimeResult);
        NAPI_CALL(env, napi_typeof(env, getTimeResult, &resValueType));
        NAPI_ASSERT(env, resValueType == napi_number, "type mismatch");
        napi_get_value_int64(env, getTimeResult, &times);
    }

    // argv[1]:callback
    if (argc >= SET_TIME_MAX_PARA) {
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "Wrong argument type. Function expected.");
        napi_create_reference(env, argv[1], 1, &callback);
    }
    return TimeNapiGetNull(env);
}

napi_value JSSystemTimeSetTime(napi_env env, napi_callback_info info)
{
    size_t argc = SET_TIME_MAX_PARA;
    napi_value argv[SET_TIME_MAX_PARA] = {0};
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    int64_t times = INVALID_TIME;
    napi_ref callback = nullptr;
    if (ParseParametersBySetTime(env, argv, argc, times, callback) == nullptr) {
        return TimeJSParaError(env, callback);
    }
    AsyncContext *asyncContext = new (std::nothrow)AsyncContext {.env = env, .time = times};
    if (!asyncContext) {
        return TimeJSParaError(env, callback);
    }
    napi_value promise = nullptr;
    TimePaddingAsyncCallbackInfo(env, asyncContext, callback, promise);
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JSSystemTimeSetTime", NAPI_AUTO_LENGTH, &resource);
    napi_create_async_work(env,
        nullptr,
        resource,
        [](napi_env env, void *data) {
            AsyncContext *asyncContext = (AsyncContext*)data;
            asyncContext->isOK = TimeServiceClient::GetInstance()->SetTime(asyncContext->time);
        },
        [](napi_env env, napi_status status, void *data) {
            AsyncContext *asyncContext = (AsyncContext*)data;
            if (asyncContext == nullptr) {
                return;
            }

            if (!asyncContext->isOK) {
                asyncContext->errorCode = ERROR;
            }
            TimeCallbackPromiseInfo info;
            info.isCallback = asyncContext->isCallback;
            info.callback = asyncContext->callbackRef;
            info.deferred = asyncContext->deferred;
            info.errorCode = asyncContext->errorCode;
            napi_value result = 0;
            napi_get_null(env, &result);
            TimeReturnCallbackPromise(env, info, result);
            napi_delete_async_work(env, asyncContext->work);
            delete asyncContext;
            asyncContext = nullptr;
        },
        (void*)asyncContext,
        &asyncContext->work);
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    if (asyncContext->isCallback) {
        return TimeNapiGetNull(env);
    } else {
        return promise;
    }
}

napi_value ParseParametersBySetTimezone(const napi_env &env, const napi_value (&argv)[SET_TIMEZONE_MAX_PARA],
    const size_t &argc, std::string &timezoneId, napi_ref &callback)
{
    NAPI_ASSERT(env, argc >= SET_TIMEZONE_MAX_PARA - 1, "Wrong number of arguments");
    napi_valuetype valueType = napi_undefined;

    // argv[0]: timezoneid
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    NAPI_ASSERT(env, valueType == napi_string, "Wrong argument type. string expected.");
    char timeZoneChars[MAX_TIME_ZONE_ID];
    size_t copied;
    napi_get_value_string_utf8(env, argv[0], timeZoneChars, MAX_TIME_ZONE_ID - 1, &copied);
    TIME_HILOGD(TIME_MODULE_JNI, "timezone str: %{public}s", timeZoneChars);

    timezoneId = std::string(timeZoneChars);

    // argv[1]:callback
    if (argc >= SET_TIMEZONE_MAX_PARA) {
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "Wrong argument type. Function expected.");
        napi_create_reference(env, argv[1], 1, &callback);
    }
    return TimeNapiGetNull(env);
}

napi_value JSSystemTimeSetTimeZone(napi_env env, napi_callback_info info)
{
    size_t argc = SET_TIMEZONE_MAX_PARA;
    napi_value argv[SET_TIMEZONE_MAX_PARA] = {0};
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    std::string timezoneId;
    napi_ref callback = nullptr;
    if (ParseParametersBySetTimezone(env, argv, argc, timezoneId, callback) == nullptr) {
        return TimeJSParaError(env, callback);
    }
    AsyncContext *asyncContext = new (std::nothrow)AsyncContext {.env = env, .timeZone = timezoneId};
    if (!asyncContext) {
        return TimeJSParaError(env, callback);
    }
    napi_value promise = nullptr;
    TimePaddingAsyncCallbackInfo(env, asyncContext, callback, promise);
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JSSystemTimeSetTimeZone", NAPI_AUTO_LENGTH, &resource);
    napi_create_async_work(env,
        nullptr,
        resource,
        [](napi_env env, void *data) {
            AsyncContext *asyncContext = (AsyncContext*)data;
            asyncContext->isOK = TimeServiceClient::GetInstance()->SetTimeZone(asyncContext->timeZone);
        },
        [](napi_env env, napi_status status, void *data) {
            AsyncContext *asyncContext = (AsyncContext*)data;
            if (asyncContext == nullptr) {
                return;
            }
            if (!asyncContext->isOK) {
                asyncContext->errorCode = ERROR;
            }
            TimeCallbackPromiseInfo info;
            info.isCallback = asyncContext->isCallback;
            info.callback = asyncContext->callbackRef;
            info.deferred = asyncContext->deferred;
            info.errorCode = asyncContext->errorCode;
            napi_value result = 0;
            napi_get_null(env, &result);
            TimeReturnCallbackPromise(env, info, result);
            napi_delete_async_work(env, asyncContext->work);
            delete asyncContext;
            asyncContext = nullptr;
        },
        (void*)asyncContext,
        &asyncContext->work);
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    if (asyncContext->isCallback) {
        return TimeNapiGetNull(env);
    } else {
        return promise;
    }
}

napi_value ParseParametersGet(const napi_env &env, const napi_value (&argv)[SET_TIMEZONE_MAX_PARA],
    const size_t &argc, napi_ref &callback)
{
    napi_valuetype valueType = napi_undefined;
    if (argc == ARGC_TYPE_SET1) {
        NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "Wrong argument type. Function expected.");
        napi_create_reference(env, argv[0], ARGC_TYPE_SET1, &callback);
    }
    return TimeNapiGetNull(env);
}

napi_value ParseParametersGetNA(const napi_env &env, const napi_value (&argv)[SET_TIMEZONE_MAX_PARA],
    const size_t &argc, napi_ref &callback, bool *isNano)
{
    napi_valuetype valueType = napi_undefined;
    if (argc == ARGC_TYPE_SET1) {
        NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
        if (valueType == napi_function) {
            napi_create_reference(env, argv[0], ARGC_TYPE_SET1, &callback);
        } else if (valueType == napi_boolean) {
            napi_get_value_bool(env, argv[0], isNano);
        }
    } else if (argc == ARGC_TYPE_SET2) {
        napi_get_value_bool(env, argv[0], isNano);
        napi_create_reference(env, argv[1], ARGC_TYPE_SET1, &callback);
    }
    return TimeNapiGetNull(env);
}

napi_value JSSystemTimeGetCurrentTime(napi_env env, napi_callback_info info)
{
    size_t argc = SET_TIMEZONE_MAX_PARA;
    napi_value argv[SET_TIMEZONE_MAX_PARA] = {0};
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    napi_ref callback = nullptr;
    bool isNano = false;
    if (ParseParametersGetNA(env, argv, argc, callback, &isNano) == nullptr) {
        return TimeJSParaError(env, callback);
    }
    AsyncContext* asyncContext = new (std::nothrow)AsyncContext {.env = env};
    if (!asyncContext) {
        return TimeJSParaError(env, callback);
    }
    napi_value promise = nullptr;
    TimePaddingAsyncCallbackInfo(env, asyncContext, callback, promise);
    asyncContext->isNano = isNano;
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JSSystemTimeGetCurrentTime", NAPI_AUTO_LENGTH, &resource);
    napi_create_async_work(env,
        nullptr,
        resource,
        [](napi_env env, void* data) {
            AsyncContext* asyncContext = (AsyncContext*)data;
            if (asyncContext->isNano) {
                asyncContext->time = TimeServiceClient::GetInstance()->GetWallTimeNs();
            } else {
                asyncContext->time = TimeServiceClient::GetInstance()->GetWallTimeMs();
            }
        },
        [](napi_env env, napi_status status, void* data) {
            AsyncContext* asyncContext = (AsyncContext*)data;
            if (asyncContext == nullptr) {
                return;
            }
            if (asyncContext->time < 0) {
            }
            TimeCallbackPromiseInfo info;
            info.isCallback = asyncContext->isCallback;
            info.callback = asyncContext->callbackRef;
            info.deferred = asyncContext->deferred;
            info.errorCode = asyncContext->errorCode;
            napi_value result = nullptr;
            napi_create_int64(env, asyncContext->time, &result);
            TimeReturnCallbackPromise(env, info, result);
            napi_delete_async_work(env, asyncContext->work);
            delete asyncContext;
            asyncContext = nullptr;
        },
        (void*)asyncContext,
        &asyncContext->work);
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    if (asyncContext->isCallback) {
        return TimeNapiGetNull(env);
    } else {
        return promise;
    }
}

napi_value JSSystemTimeGetRealActiveTime(napi_env env, napi_callback_info info)
{
    size_t argc = SET_TIMEZONE_MAX_PARA;
    napi_value argv[SET_TIMEZONE_MAX_PARA] = {0};
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    napi_ref callback = nullptr;
    bool isNano = false;
    if (ParseParametersGetNA(env, argv, argc, callback, &isNano) == nullptr) {
        return TimeJSParaError(env, callback);
    }
    AsyncContext* asyncContext = new (std::nothrow)AsyncContext {.env = env};
    if (!asyncContext) {
        return TimeJSParaError(env, callback);
    }
    napi_value promise = nullptr;
    TimePaddingAsyncCallbackInfo(env, asyncContext, callback, promise);
    asyncContext->isNano = isNano;
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JSSystemTimeGetRealActiveTime", NAPI_AUTO_LENGTH, &resource);
    napi_create_async_work(env,
        nullptr,
        resource,
        [](napi_env env, void* data) {
            AsyncContext* asyncContext = (AsyncContext*)data;
            if (asyncContext->isNano) {
                asyncContext->time = TimeServiceClient::GetInstance()->GetMonotonicTimeNs();
            } else {
                asyncContext->time = TimeServiceClient::GetInstance()->GetMonotonicTimeMs();
            }
        },
        [](napi_env env, napi_status status, void* data) {
            AsyncContext* asyncContext = (AsyncContext*)data;
            if (asyncContext == nullptr) {
                return;
            }
            if (asyncContext->time < 0) {
                asyncContext->errorCode = ERROR;
            }
            TimeCallbackPromiseInfo info;
            info.isCallback = asyncContext->isCallback;
            info.callback = asyncContext->callbackRef;
            info.deferred = asyncContext->deferred;
            info.errorCode = asyncContext->errorCode;
            napi_value result = nullptr;
            napi_create_int64(env, asyncContext->time, &result);
            TimeReturnCallbackPromise(env, info, result);
            napi_delete_async_work(env, asyncContext->work);
            delete asyncContext;
            asyncContext = nullptr;
        },
        (void*)asyncContext,
        &asyncContext->work);
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    if (asyncContext->isCallback) {
        return TimeNapiGetNull(env);
    } else {
        return promise;
    }
}

napi_value JSSystemTimeGetRealTime(napi_env env, napi_callback_info info)
{
    size_t argc = SET_TIMEZONE_MAX_PARA;
    napi_value argv[SET_TIMEZONE_MAX_PARA] = {0};
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    napi_ref callback = nullptr;
    bool isNano = false;
    if (ParseParametersGetNA(env, argv, argc, callback, &isNano) == nullptr) {
        return TimeJSParaError(env, callback);
    }
    AsyncContext* asyncContext = new (std::nothrow)AsyncContext {.env = env};
    if (!asyncContext) {
        return TimeJSParaError(env, callback);
    }
    napi_value promise = nullptr;
    TimePaddingAsyncCallbackInfo(env, asyncContext, callback, promise);
    asyncContext->isNano = isNano;
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JSSystemTimeGetRealTime", NAPI_AUTO_LENGTH, &resource);
    napi_create_async_work(env,
        nullptr,
        resource,
        [](napi_env env, void* data) {
            AsyncContext* asyncContext = (AsyncContext*)data;
            if (asyncContext->isNano) {
                asyncContext->time = TimeServiceClient::GetInstance()->GetBootTimeNs();
            } else {
                asyncContext->time = TimeServiceClient::GetInstance()->GetBootTimeMs();
            }
        },
        [](napi_env env, napi_status status, void* data) {
            AsyncContext* asyncContext = (AsyncContext*)data;
            if (asyncContext == nullptr) {
                return;
            }
            if (asyncContext->time < 0) {
                asyncContext->errorCode = ERROR;
            }
            TimeCallbackPromiseInfo info;
            info.isCallback = asyncContext->isCallback;
            info.callback = asyncContext->callbackRef;
            info.deferred = asyncContext->deferred;
            info.errorCode = asyncContext->errorCode;
            napi_value result = nullptr;
            napi_create_int64(env, asyncContext->time, &result);
            TimeReturnCallbackPromise(env, info, result);
            napi_delete_async_work(env, asyncContext->work);
            delete asyncContext;
            asyncContext = nullptr;
        },
        (void*)asyncContext,
        &asyncContext->work);
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    if (asyncContext->isCallback) {
        return TimeNapiGetNull(env);
    } else {
        return promise;
    }
}

napi_value JSSystemTimeGetDate(napi_env env, napi_callback_info info)
{
    size_t argc = SET_TIMEZONE_MAX_PARA;
    napi_value argv[SET_TIMEZONE_MAX_PARA] = {0};
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    napi_ref callback = nullptr;
    if (ParseParametersGet(env, argv, argc, callback) == nullptr) {
        return TimeJSParaError(env, callback);
    }
    AsyncContext* asyncContext = new (std::nothrow)AsyncContext {.env = env};
    if (!asyncContext) {
        return TimeJSParaError(env, callback);
    }
    napi_value promise = nullptr;
    TimePaddingAsyncCallbackInfo(env, asyncContext, callback, promise);
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JSSystemTimeGetDate", NAPI_AUTO_LENGTH, &resource);
    napi_create_async_work(env,
        nullptr,
        resource,
        [](napi_env env, void* data) {
            AsyncContext* asyncContext = (AsyncContext*)data;
            asyncContext->time = TimeServiceClient::GetInstance()->GetWallTimeMs();
        },
        [](napi_env env, napi_status status, void* data) {
            AsyncContext* asyncContext = (AsyncContext*)data;
            if (asyncContext == nullptr) {
                return;
            }
            if (asyncContext->time < 0) {
                asyncContext->errorCode = ERROR;
            }
            TimeCallbackPromiseInfo info;
            info.isCallback = asyncContext->isCallback;
            info.callback = asyncContext->callbackRef;
            info.deferred = asyncContext->deferred;
            info.errorCode = asyncContext->errorCode;
            napi_value result = nullptr;
            napi_create_date(env, asyncContext->time, &result);
            TimeReturnCallbackPromise(env, info, result);
            napi_delete_async_work(env, asyncContext->work);
            delete asyncContext;
            asyncContext = nullptr;
        },
        (void*)asyncContext,
        &asyncContext->work);
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    if (asyncContext->isCallback) {
        return TimeNapiGetNull(env);
    } else {
        return promise;
    }
}

napi_value JSSystemTimeGetTimeZone(napi_env env, napi_callback_info info)
{
    size_t argc = SET_TIMEZONE_MAX_PARA;
    napi_value argv[SET_TIMEZONE_MAX_PARA] = {0};
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    napi_ref callback = nullptr;
    if (ParseParametersGet(env, argv, argc, callback) == nullptr) {
        return TimeJSParaError(env, callback);
    }
    AsyncContext* asyncContext = new (std::nothrow)AsyncContext {.env = env};
    if (!asyncContext) {
        return TimeJSParaError(env, callback);
    }
    TIME_HILOGI(TIME_MODULE_JS_NAPI, " jsgetTimezone start==");
    napi_value promise = nullptr;
    TimePaddingAsyncCallbackInfo(env, asyncContext, callback, promise);
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JSSystemTimeGetTimeZone", NAPI_AUTO_LENGTH, &resource);
    napi_create_async_work(env,
        nullptr,
        resource,
        [](napi_env env, void* data) {
            AsyncContext* asyncContext = (AsyncContext*)data;
            asyncContext->timeZone = TimeServiceClient::GetInstance()->GetTimeZone();
        },
        [](napi_env env, napi_status status, void* data) {
            AsyncContext* asyncContext = (AsyncContext*)data;
            if (asyncContext == nullptr) {
                return;
            }
            if (asyncContext->timeZone == "") {
                asyncContext->errorCode = ERROR;
            }
            TimeCallbackPromiseInfo info;
            info.isCallback = asyncContext->isCallback;
            info.callback = asyncContext->callbackRef;
            info.deferred = asyncContext->deferred;
            info.errorCode = asyncContext->errorCode;
            napi_value result = nullptr;
            napi_create_string_utf8(env, asyncContext->timeZone.c_str(), asyncContext->timeZone.length(), &result);
            TimeReturnCallbackPromise(env, info, result);
            napi_delete_async_work(env, asyncContext->work);
            delete asyncContext;
            asyncContext = nullptr;
        },
        (void*)asyncContext,
        &asyncContext->work);
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    if (asyncContext->isCallback) {
        return TimeNapiGetNull(env);
    } else {
        return promise;
    }
}

EXTERN_C_START
napi_value SystemTimeExport(napi_env env, napi_value exports)
{
    static napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("setTime", JSSystemTimeSetTime),
        DECLARE_NAPI_FUNCTION("setDate", JSSystemTimeSetTime),
        DECLARE_NAPI_FUNCTION("setTimezone", JSSystemTimeSetTimeZone),
        DECLARE_NAPI_FUNCTION("getCurrentTime", JSSystemTimeGetCurrentTime),
        DECLARE_NAPI_FUNCTION("getRealActiveTime", JSSystemTimeGetRealActiveTime),
        DECLARE_NAPI_FUNCTION("getRealTime", JSSystemTimeGetRealTime),
        DECLARE_NAPI_FUNCTION("getDate", JSSystemTimeGetDate),
        DECLARE_NAPI_FUNCTION("getTimezone", JSSystemTimeGetTimeZone),
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
} // MiscServicesNapi
} // OHOS