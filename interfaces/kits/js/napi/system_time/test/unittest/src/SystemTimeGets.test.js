/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

// @ts-nocheck
import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from 'deccjsunit/index'
import systemTime from '@ohos.systemTime'

describe('TimeTest', function () {
    const MILLI_TO_BASE = 1000;
    const NANO_TO_BASE = 1000000000;
    const NANO_TO_MILLI = NANO_TO_BASE / MILLI_TO_BASE;

    beforeAll(async function () {
        console.info('beforeAll')
    })

    afterAll(async function () {
        console.info('afterAll')
    })

    /**
     * @tc.number    systemTime_getCurrentTime_test1
     * @tc.name      Test systemTime.getCurrentTime Invalid value
     * @tc.desc      Test systemTime_getCurrentTime API functionality.
     * @tc.size      : MEDIUM
     * @tc.type      : Function
     * @tc.level     : Level 0
     */
    it('systemTime_getCurrentTime_test1', 0, async function (done) {
        console.log("systemTime_getCurrentTime_test1 start");

        const nowTime = new Date().getTime();

        const milliTime = await systemTime.getCurrentTime();
        console.log('Get current time is ' + milliTime);

        expect(milliTime > nowTime).assertTrue;
        expect((milliTime - nowTime) < 500).assertTrue;

        console.log('systemTime_getCurrentTime_test1 end');
        done();
    })

    /**
     * @tc.number    systemTime_getCurrentTime_Nano_test1
     * @tc.name      Test systemTime.getCurrentTime Invalid value
     * @tc.desc      Test systemTime_getCurrentTime API functionality with nano.
     * @tc.size      : MEDIUM
     * @tc.type      : Function
     * @tc.level     : Level 0
     */
    it('systemTime_getCurrentTime_Nano_test1', 0, async function (done) {
        console.log("systemTime_getCurrentTime_Nano_test1 start");

        const nowTime = new Date().getTime();

        const nanoTime = await systemTime.getCurrentTime(true);
        console.log('Get current nano time is ' + nanoTime);

        const milliTime = nanoTime / NANO_TO_MILLI;

        expect(milliTime > nowTime).assertTrue;
        expect((milliTime - nowTime) < 500).assertTrue;

        console.log('systemTime_getCurrentTime_Nano_test1 end');
        done();
    })

    /**
     * @tc.number    systemTime_getCurrentTime_test1
     * @tc.name      Test systemTime.getCurrentTime Invalid value
     * @tc.desc      Test systemTime_getCurrentTime API functionality.
     * @tc.size      : MEDIUM
     * @tc.type      : Function
     * @tc.level     : Level 0
     */
    it('systemTime_getRealActiveTime_test1', 0, async function (done) {
        console.log("systemTime_getRealActiveTime_test1 start");

        const milliTime = await systemTime.getRealActiveTime();
        console.log('Get real active time is ' + milliTime);

        expect(milliTime / MILLI_TO_BASE > 0).assertTrue;

        console.log('systemTime_getRealActiveTime_test1 end');
        done();
    })

    /**
     * @tc.number    systemTime_getRealActiveTime_Nano_test1
     * @tc.name      Test systemTime.getRealActiveTime Invalid value
     * @tc.desc      Test systemTime_getRealActiveTime API functionality with nano.
     * @tc.size      : MEDIUM
     * @tc.type      : Function
     * @tc.level     : Level 0
     */
    it('systemTime_getRealActiveTime_Nano_test1', 0, async function (done) {
        console.log("systemTime_getRealActiveTime_Nano_test1 start");

        const nanoTime = await systemTime.getRealActiveTime(true);
        console.log('Get real active nano time is ' + nanoTime);

        expect(nanoTime / NANO_TO_BASE > 0).assertTrue;

        console.log('systemTime_getRealActiveTime_Nano_test1 end');
        done();
    })

    /**
     * @tc.number    systemTime_getRealTime_test1
     * @tc.name      Test systemTime.getRealTime Invalid value
     * @tc.desc      Test systemTime_getRealTime API functionality.
     * @tc.size      : MEDIUM
     * @tc.type      : Function
     * @tc.level     : Level 0
     */
    it('systemTime_getRealTime_test1', 0, async function (done) {
        console.log("systemTime_getRealTime_test1 start");

        const milliTime = await systemTime.getRealTime();
        console.log('Get real time is ' + milliTime);

        expect(milliTime / MILLI_TO_BASE > 0).milliTime;

        console.log('systemTime_getRealTime_test1 end');
        done();
    })

    /**
     * @tc.number    systemTime_getRealTime_test1
     * @tc.name      Test systemTime.getRealTime Invalid value
     * @tc.desc      Test systemTime_getRealTime API functionality with nano.
     * @tc.size      : MEDIUM
     * @tc.type      : Function
     * @tc.level     : Level 0
     */
    it('systemTime_getRealTime_Nano_test1', 0, async function (done) {
        console.log("systemTime_getRealTime_Nano_test1 start");

        const nanoTime = await systemTime.getRealTime(true);
        console.log('Get real nano time is ' + nanoTime);

        expect(nanoTime / NANO_TO_BASE > 0).assertTrue;

        console.log('systemTime_getRealTime_Nano_test1 end');
        done();
    })

    /**
     * @tc.number    systemTime_getDate_test1
     * @tc.name      Test systemTime.getDate Invalid value
     * @tc.desc      Test systemTime_getDate API functionality.
     * @tc.size      : MEDIUM
     * @tc.type      : Function
     * @tc.level     : Level 0
     */
    it('systemTime_getDate_test1', 0, async function (done) {
        console.log("systemTime_getDate_test1 start");

        const nowTime = new Date().getTime();

        const date = await systemTime.getDate();
        const milliTime = date.getTime();
        expect(milliTime > nowTime).assertTrue;
        expect((milliTime - nowTime) < 500).assertTrue;

        console.log('systemTime_getDate_test1 end');
        done();
    })

    /**
     * @tc.number    systemTime_getTimezone_test1
     * @tc.name      Test systemTime.getTimezone Invalid value
     * @tc.desc      Test systemTime_getTimezone API functionality.
     * @tc.size      : MEDIUM
     * @tc.type      : Function
     * @tc.level     : Level 0
     */
    it('systemTime_getTimezone_test1', 0, async function (done) {
        const timezone = await systemTime.getTimezone();
        expect('Asia/Shanghai').assertEqual(timezone);
        done();
    })
})
