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
#include <cstdlib>
#include <ctime>
#include "timer_info_test.h"
#include "time_service_test.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MiscServices;

class TimeServiceTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void TimeServiceTest::SetUpTestCase(void) {
}

void TimeServiceTest::TearDownTestCase(void) {
}

void TimeServiceTest::SetUp(void) {
}

void TimeServiceTest::TearDown(void) {
}

/**
* @tc.name: TimezoneTest01
* @tc.desc: Set system timezone.
* @tc.type: FUNC
*/
HWTEST_F(TimeServiceTest, SetTimezone01, TestSize.Level0) {
    time_t t; // 定义变量
    time(&t); // 获取系统时间
    TIME_HILOGI(TIME_MODULE_CLIENT, "Time before: %{public}s", asctime(localtime(&t)));
    if (setenv("TZ", "GMT-10", 1) ==0) {
        tzset(); // 设置UNIX时间兼容
        (void)time(&t); // 获取系统时间
        TIME_HILOGI(TIME_MODULE_CLIENT, "Time now: %{public}s", asctime(localtime(&t)));
    }
}