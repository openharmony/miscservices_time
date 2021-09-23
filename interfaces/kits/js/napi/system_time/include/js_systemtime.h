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

#ifndef N_JS_SYSTEMTIME_H
#define N_JS_SYSTEMTIME_H

#include <string>
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "js_native_api.h"

constexpr int RESOLVED = 1;
constexpr int REJECT = 0;

constexpr int NONE_PARAMETER = 0;
constexpr int ONE_PARAMETER = 1;
constexpr int TWO_PARAMETERS = 2;
constexpr int THREE_PARAMETERS = 3;
constexpr int MAX_TIME_ZONE_ID = 1024;

#define GET_PARAMS(env, info, num)                                \
    size_t argc = num;             \
    napi_value argv[num] = {0};    \
    napi_value thisVar = nullptr;  \
    void *data;                    \
    napi_get_cb_info(env, info, &argc, argv, &thisVar, &data)

typedef struct AsyncContext {
    napi_env env;
    napi_async_work work;
    int64_t time;
    std::string timeZone;
    napi_deferred deferred;
    napi_ref callbackRef;
    int status;
} AsyncContext;


#endif