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

#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include <sys/time.h>

#include "time_log.h"
#include "time_service_manager.h"

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

void TimeServiceTest::SetUpTestCase(void)
{}

void TimeServiceTest::TearDownTestCase(void)
{}

void TimeServiceTest::SetUp(void)
{
}

void TimeServiceTest::TearDown(void)
{}

/**
* @tc.name: SetTime001
* @tc.desc: get system time.
* @tc.type: FUNC
*/
HWTEST_F(TimeServiceTest, SetTime001, TestSize.Level0)
{
    int64_t time = 1627307312000;
    bool result = TimeServiceManager::GetInstance()->SetTime(time);
    EXPECT_TRUE(result);
}
