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

#include "timer_manager.h"
#include <utility>
#include <vector>
#include <algorithm>
#include <ctime>
#include <iostream>

#include "bundle_mgr_interface.h"
#include "if_system_ability_manager.h"
#include "system_ability_definition.h"
#include "iservice_registry.h"

namespace OHOS {
namespace MiscServices {
using namespace std::chrono;
using namespace OHOS::AppExecFwk;
namespace {
static int TIME_CHANGED_BITS = 16;
static uint32_t TIME_CHANGED_MASK = 1 << TIME_CHANGED_BITS;
const int ONE_THOUSAND = 1000;
const float_t BATCH_WINDOW_COE = 0.75;
const auto MIN_FUTURITY = seconds(5);
const auto ZERO_FUTURITY = seconds(0);
const auto MIN_INTERVAL_FIVE_SECONDS = seconds(5);
const auto MIN_INTERVAL_ONE_SECONDS = seconds(1);
const auto MAX_INTERVAL = hours(24 * 365);
const auto INTERVAL_HOUR = hours(1);
const auto INTERVAL_HALF_DAY = hours(12);
const auto MIN_FUZZABLE_INTERVAL = milliseconds(10000);
}

extern bool AddBatchLocked(std::vector<std::shared_ptr<Batch>> &list, const std::shared_ptr<Batch> &batch);
extern steady_clock::time_point MaxTriggerTime(steady_clock::time_point now,
                                               steady_clock::time_point triggerAtTime,
                                               milliseconds interval);

std::shared_ptr<TimerManager> TimerManager::Create()
{
    auto impl = TimerHandler::Create();
    if (impl == nullptr) {
        TIME_HILOGE(TIME_MODULE_SERVICE, "Create Timer handle failed.");
        return nullptr;
    }
    return std::shared_ptr<TimerManager>(new TimerManager(impl));
}

TimerManager::TimerManager(std::shared_ptr<TimerHandler> impl)
    : random_ {static_cast<uint64_t>(time(nullptr))},
      runFlag_ {false},
      handler_ {std::move(impl)},
      lastTimeChangeClockTime_ {system_clock::time_point::min()},
      lastTimeChangeRealtime_ {steady_clock::time_point::min()}
{
    runFlag_ = true;
    alarmThread_.reset(new std::thread(&TimerManager::TimerLooper, this));
}

uint64_t TimerManager::CreateTimer(int type,
                                   uint64_t windowLength,
                                   uint64_t interval,
                                   int flag,
                                   std::function<void (const uint64_t)> callback,
                                   int uid)
{
    TIME_HILOGI(TIME_MODULE_SERVICE,
        "Create timer: %{public}d windowLength:%{public}" PRId64 "interval:%{public}" PRId64 "flag:%{public}d",
        type,
        windowLength,
        interval,
        flag);
    uint64_t timerNumber = 0;
    while (timerNumber == 0) {
        timerNumber = random_();
    }
    auto timerInfo = std::make_shared<TimerEntry>(TimerEntry {
        timerNumber,
        type,
        windowLength,
        interval,
        flag,
        std::move(callback),
        uid
        });
    std::lock_guard<std::mutex> lock(entryMapMutex_);
    timerEntryMap_.insert(std::make_pair(timerNumber, timerInfo));
    return timerNumber;
}

bool TimerManager::StartTimer(uint64_t timerNumber, uint64_t triggerTime)
{
    std::lock_guard<std::mutex> lock(entryMapMutex_);
    auto it = timerEntryMap_.find(timerNumber);
    if (it == timerEntryMap_.end()) {
        TIME_HILOGE(TIME_MODULE_SERVICE, "Timer id not found: %{public}" PRId64 "", timerNumber);
        return false;
    }
    TIME_HILOGI(TIME_MODULE_SERVICE, "Start timer : %{public}" PRId64 "", timerNumber);
    TIME_HILOGI(TIME_MODULE_SERVICE, "TriggerTime : %{public}" PRId64 "", triggerTime);
    auto timerInfo = it->second;
    SetHandler(timerInfo->id,
               timerInfo->type,
               triggerTime,
               timerInfo->windowLength,
               timerInfo->interval,
               timerInfo->flag,
               timerInfo->callback,
               timerInfo->uid);
    return true;
}

bool TimerManager::StopTimer(uint64_t timerNumber)
{
    TIME_HILOGD(TIME_MODULE_SERVICE, "start.");
    std::lock_guard<std::mutex> lock(entryMapMutex_);
    auto it = timerEntryMap_.find(timerNumber);
    if (it == timerEntryMap_.end()) {
        TIME_HILOGD(TIME_MODULE_SERVICE, "end.");
        return false;
    }
    RemoveHandler(timerNumber);
    TIME_HILOGD(TIME_MODULE_SERVICE, "end.");
    return true;
}

bool TimerManager::DestroyTimer(uint64_t timerNumber)
{
    TIME_HILOGD(TIME_MODULE_SERVICE, "start id: %{public}" PRId64 "", timerNumber);
    std::lock_guard<std::mutex> lock(entryMapMutex_);
    auto it = timerEntryMap_.find(timerNumber);
    if (it == timerEntryMap_.end()) {
        TIME_HILOGD(TIME_MODULE_SERVICE, "end.");
        return false;
    }
    RemoveHandler(timerNumber);
    timerEntryMap_.erase(it);
    TIME_HILOGD(TIME_MODULE_SERVICE, "end.");
    return true;
}

bool TimerManager::IsSystemUid(int uid)
{
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        TIME_HILOGD(TIME_MODULE_SERVICE, "GetSystemAbilityManager is null.");
        return false;
    }
    auto bundleMgrSa = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (bundleMgrSa == nullptr) {
        TIME_HILOGD(TIME_MODULE_SERVICE, "GetSystemAbility is null.");
        return false;
    }
    return iface_cast<IBundleMgr>(bundleMgrSa)->CheckIsSystemAppByUid(uid);
}

void TimerManager::SetHandler(uint64_t id,
                              int type,
                              uint64_t triggerAtTime,
                              uint64_t windowLength,
                              uint64_t interval,
                              int flag,
                              std::function<void (const uint64_t)> callback,
                              int uid)
{
    TIME_HILOGI(TIME_MODULE_SERVICE, "start id: %{public}" PRId64 "", id);
    TIME_HILOGI(TIME_MODULE_SERVICE,
        "start type:%{public}d windowLength:%{public}" PRId64"interval:%{public}" PRId64"flag:%{public}d",
        type, windowLength, interval, flag);
    auto windowLengthDuration = milliseconds(windowLength);
    if (windowLengthDuration > INTERVAL_HALF_DAY) {
        windowLengthDuration = INTERVAL_HOUR;
    }
    auto minInterval = (IsSystemUid(uid)) ? MIN_INTERVAL_ONE_SECONDS : MIN_INTERVAL_FIVE_SECONDS;
    auto intervalDuration = milliseconds(interval);
    if (intervalDuration > milliseconds::zero() && intervalDuration < minInterval) {
        intervalDuration = minInterval;
    } else if (intervalDuration > MAX_INTERVAL) {
        intervalDuration = MAX_INTERVAL;
    }

    auto nowElapsed = steady_clock::now();
    auto nominalTrigger = ConvertToElapsed(milliseconds(triggerAtTime), type);
    if (nominalTrigger < nowElapsed) {
        TIME_HILOGI(TIME_MODULE_SERVICE, "invalid trigger time end.");
        return;
    }
    auto minTrigger =  (IsSystemUid(uid)) ? (nowElapsed + ZERO_FUTURITY) : (nowElapsed + MIN_FUTURITY);
    auto triggerElapsed = (nominalTrigger > minTrigger) ? nominalTrigger : minTrigger;

    steady_clock::time_point maxElapsed;
    if (windowLengthDuration == milliseconds::zero()) {
        maxElapsed = triggerElapsed;
    } else if (windowLengthDuration < milliseconds::zero()) {
        maxElapsed = MaxTriggerTime(nominalTrigger, triggerElapsed, intervalDuration);
        windowLengthDuration = duration_cast<milliseconds>(maxElapsed - triggerElapsed);
    } else {
        maxElapsed = triggerElapsed + windowLengthDuration;
    }
    TIME_HILOGI(TIME_MODULE_SERVICE, "Try get lock");
    std::lock_guard<std::mutex> lockGuard(mutex_);
    TIME_HILOGI(TIME_MODULE_SERVICE, "Lock guard");
    SetHandlerLocked(id,
                     type,
                     milliseconds(triggerAtTime),
                     triggerElapsed,
                     windowLengthDuration,
                     maxElapsed,
                     intervalDuration,
                     std::move(callback),
                     static_cast<uint32_t>(flag),
                     true,
                     uid);
}

void TimerManager::SetHandlerLocked(uint64_t id, int type,
                                    std::chrono::milliseconds when,
                                    std::chrono::steady_clock::time_point whenElapsed,
                                    std::chrono::milliseconds windowLength,
                                    std::chrono::steady_clock::time_point maxWhen,
                                    std::chrono::milliseconds interval,
                                    std::function<void (const uint64_t)> callback,
                                    uint32_t flags,
                                    bool doValidate,
                                    uint64_t callingUid)
{
    TIME_HILOGI(TIME_MODULE_SERVICE, "start id: %{public}" PRId64 "", id);
    auto alarm = std::make_shared<TimerInfo>(id, type, when, whenElapsed, windowLength, maxWhen,
                                      interval, std::move(callback), flags, callingUid);
    SetHandlerLocked(alarm, false, doValidate);
    TIME_HILOGI(TIME_MODULE_SERVICE, "end");
}

void TimerManager::RemoveHandler(uint64_t id)
{
    TIME_HILOGI(TIME_MODULE_SERVICE, "start");
    std::lock_guard<std::mutex> lock(mutex_);
    RemoveLocked(id);
    TIME_HILOGI(TIME_MODULE_SERVICE, "end");
}

void TimerManager::RemoveLocked(uint64_t id)
{
    TIME_HILOGI(TIME_MODULE_SERVICE, "start id: %{public}" PRId64 "", id);
    auto whichAlarms = [id](const TimerInfo &timer) {
        return timer.id == id;
    };

    bool didRemove = false;
    for (auto it = alarmBatches_.begin(); it != alarmBatches_.end();) {
        auto batch = *it;
        didRemove = batch->Remove(whichAlarms);
        if (batch->Size() == 0) {
            TIME_HILOGD(TIME_MODULE_SERVICE, "erase");
            it = alarmBatches_.erase(it);
        } else {
            ++it;
        }
    }

    if (didRemove) {
        ReBatchAllTimersLocked(true);
    }
    TIME_HILOGI(TIME_MODULE_SERVICE, "end");
}

void TimerManager::SetHandlerLocked(std::shared_ptr<TimerInfo> alarm, bool rebatching, bool doValidate)
{
    TIME_HILOGI(TIME_MODULE_SERVICE, "start rebatching= %{public}d, doValidate= %{public}d", rebatching, doValidate);
    InsertAndBatchTimerLocked(std::move(alarm));
    if (!rebatching) {
        RescheduleKernelTimerLocked();
    }
    TIME_HILOGI(TIME_MODULE_SERVICE, "end");
}

void TimerManager::ReBatchAllTimers()
{
    TIME_HILOGI(TIME_MODULE_SERVICE, "start");
    ReBatchAllTimersLocked(true);
    TIME_HILOGI(TIME_MODULE_SERVICE, "end");
}

void TimerManager::ReBatchAllTimersLocked(bool doValidate)
{
    TIME_HILOGI(TIME_MODULE_SERVICE, "start");
    auto oldSet = alarmBatches_;
    alarmBatches_.clear();
    auto nowElapsed = steady_clock::now();
    for (const auto &batch : oldSet) {
        auto n = batch->Size();
        for (unsigned int i = 0; i < n; i++) {
            ReAddTimerLocked(batch->Get(i), nowElapsed, doValidate);
        }
    }
    RescheduleKernelTimerLocked();
    TIME_HILOGI(TIME_MODULE_SERVICE, "end");
}

void TimerManager::ReAddTimerLocked(std::shared_ptr<TimerInfo> timer,
                                    std::chrono::steady_clock::time_point nowElapsed,
                                    bool doValidate)
{
    TIME_HILOGI(TIME_MODULE_SERVICE, "start");
    timer->when = timer->origWhen;
    auto whenElapsed = ConvertToElapsed(timer->when, timer->type);
    if (whenElapsed < nowElapsed) {
        TIME_HILOGI(TIME_MODULE_SERVICE, "invalid timer end.");
        return;
    }
    steady_clock::time_point maxElapsed;
    if (timer->windowLength == milliseconds::zero()) {
        maxElapsed = whenElapsed;
    } else {
        maxElapsed = (timer->windowLength > milliseconds::zero()) ? (whenElapsed + timer->windowLength)
            : MaxTriggerTime(nowElapsed, whenElapsed, timer->repeatInterval);
    }
    timer->whenElapsed = whenElapsed;
    timer->maxWhenElapsed = maxElapsed;
    SetHandlerLocked(timer, true, doValidate);
    TIME_HILOGI(TIME_MODULE_SERVICE, "end");
}

std::chrono::steady_clock::time_point TimerManager::ConvertToElapsed(std::chrono::milliseconds when, int type)
{
    TIME_HILOGI(TIME_MODULE_SERVICE, "start");
    if (type == RTC || type == RTC_WAKEUP) {
        auto systemTimeNow = system_clock::now().time_since_epoch();
        auto offset = when - systemTimeNow;
        TIME_HILOGI(TIME_MODULE_SERVICE, "systemTimeNow : %{public}lld", systemTimeNow.count());
        TIME_HILOGI(TIME_MODULE_SERVICE, "offset : %{public}lld", offset.count());
        return steady_clock::now() + offset;
    }
    auto bootTimeNow = steady_clock::now().time_since_epoch();
    auto offset = when - bootTimeNow;
    TIME_HILOGI(TIME_MODULE_SERVICE, "bootTimeNow : %{public}lld", bootTimeNow.count());
    TIME_HILOGI(TIME_MODULE_SERVICE, "offset : %{public}lld", offset.count());
    TIME_HILOGI(TIME_MODULE_SERVICE, "end");
    return steady_clock::now() + offset;
}

void TimerManager::TimerLooper()
{
    TIME_HILOGI(TIME_MODULE_SERVICE, "Start timer wait loop");
    std::vector<std::shared_ptr<TimerInfo>> triggerList;
    while (runFlag_) {
        uint32_t result = 0;
        do {
            result = handler_->WaitForAlarm();
        } while (result < 0 && errno == EINTR);
        TIME_HILOGI(TIME_MODULE_SERVICE, "result= %{public}d", result);

        auto nowRtc = std::chrono::system_clock::now();
        auto nowElapsed = std::chrono::steady_clock::now();
        triggerList.clear();

        if ((result & TIME_CHANGED_MASK) != 0) {
            TIME_HILOGI(TIME_MODULE_SERVICE, "time changed");
            system_clock::time_point lastTimeChangeClockTime;
            system_clock::time_point expectedClockTime;
            std::lock_guard<std::mutex> lock(mutex_);
            lastTimeChangeClockTime = lastTimeChangeClockTime_;
            expectedClockTime = lastTimeChangeClockTime + (duration_cast<milliseconds>(nowElapsed.time_since_epoch()) -
                duration_cast<milliseconds>(lastTimeChangeRealtime_.time_since_epoch()));
            if (lastTimeChangeClockTime == system_clock::time_point::min()
                || nowRtc < (expectedClockTime - milliseconds(ONE_THOUSAND))
                || nowRtc > (expectedClockTime + milliseconds(ONE_THOUSAND))) {
                ReBatchAllTimers();
                lastTimeChangeClockTime_ = nowRtc;
                lastTimeChangeRealtime_ = nowElapsed;
            }
        }

        if (result != TIME_CHANGED_MASK) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto hasWakeup = TriggerTimersLocked(triggerList, nowElapsed);
            TIME_HILOGI(TIME_MODULE_SERVICE, "hasWakeup= %{public}d", hasWakeup);
            DeliverTimersLocked(triggerList, nowElapsed);
            RescheduleKernelTimerLocked();
        } else {
            std::lock_guard<std::mutex> lock(mutex_);
            RescheduleKernelTimerLocked();
        }
    }
}

TimerManager::~TimerManager()
{
    if (alarmThread_ && alarmThread_->joinable()) {
        alarmThread_->join();
    }
}

bool TimerManager::TriggerTimersLocked(std::vector<std::shared_ptr<TimerInfo>> &triggerList,
                                       std::chrono::steady_clock::time_point nowElapsed)
{
    TIME_HILOGI(TIME_MODULE_SERVICE, "alarmBatches_.size= %{public}d", static_cast<int>(alarmBatches_.size()));
    bool hasWakeup = false;
    while (!alarmBatches_.empty()) {
        auto batch = alarmBatches_.at(0);
        TIME_HILOGI(TIME_MODULE_SERVICE, "batch->GetStart()= %{public}lld, nowElapsed= %{public}lld",
            time_point_cast<nanoseconds>(batch->GetStart()).time_since_epoch().count(),
            time_point_cast<nanoseconds>(nowElapsed).time_since_epoch().count());
        if (batch->GetStart() > nowElapsed) {
            TIME_HILOGI(TIME_MODULE_SERVICE, "break alarmBatches_.size= %{public}d",
                static_cast<int>(alarmBatches_.size()));
            break;
        }
        alarmBatches_.erase(alarmBatches_.begin());
        TIME_HILOGI(TIME_MODULE_SERVICE, "after erase alarmBatches_.size= %{public}d",
            static_cast<int>(alarmBatches_.size()));

        const auto n = batch->Size();
        for (unsigned int i = 0; i < n; ++i) {
            auto alarm = batch->Get(i);
            alarm->count = 1;
            triggerList.push_back(alarm);
            TIME_HILOGI(TIME_MODULE_SERVICE, "alarm uid= %{public}d", alarm->uid);
            if (alarm->repeatInterval > milliseconds::zero()) {
                alarm->count += duration_cast<milliseconds>(nowElapsed -
                    alarm->expectedWhenElapsed) / alarm->repeatInterval;
                auto delta = alarm->count * alarm->repeatInterval;
                auto nextElapsed = alarm->whenElapsed + delta;
                SetHandlerLocked(alarm->id, alarm->type, alarm->when + delta, nextElapsed, alarm->windowLength,
                    MaxTriggerTime(nowElapsed, nextElapsed, alarm->repeatInterval), alarm->repeatInterval,
                    alarm->callback, alarm->flags, true, alarm->uid);
            }
            if (alarm->wakeup) {
                hasWakeup = true;
            }
        }
    }
    std::sort(triggerList.begin(),
        triggerList.end(),
        [] (const std::shared_ptr<TimerInfo> &l, const std::shared_ptr<TimerInfo> &r) {
            return l->whenElapsed < r->whenElapsed;
        });

    return hasWakeup;
}

void TimerManager::RescheduleKernelTimerLocked()
{
    TIME_HILOGI(TIME_MODULE_SERVICE, "start alarmBatches_.size= %{public}d", static_cast<int>(alarmBatches_.size()));
    auto nextNonWakeup = std::chrono::steady_clock::time_point::min();
    if (!alarmBatches_.empty()) {
        auto firstWakeup = FindFirstWakeupBatchLocked();
        auto firstBatch = alarmBatches_.front();
        if (firstWakeup != nullptr) {
            auto alarmPtr = firstWakeup->Get(0);
            if (alarmPtr != nullptr) {
                TIME_HILOGI(TIME_MODULE_SERVICE, "wake up alarm id :%{public}" PRId64 "", alarmPtr->id);
            }
            SetLocked(ELAPSED_REALTIME_WAKEUP, firstWakeup->GetStart().time_since_epoch());
        }
        if (firstBatch != firstWakeup) {
            auto alarmPtr = firstBatch->Get(0);
            if (alarmPtr != nullptr) {
                TIME_HILOGI(TIME_MODULE_SERVICE, "nonwakeup alarm id :%{public}" PRId64 "", alarmPtr->id);
            }
            nextNonWakeup = firstBatch->GetStart();
        }
    }

    if (nextNonWakeup != std::chrono::steady_clock::time_point::min()) {
        SetLocked(ELAPSED_REALTIME, nextNonWakeup.time_since_epoch());
    }
    TIME_HILOGI(TIME_MODULE_SERVICE, "end");
}

std::shared_ptr<Batch> TimerManager::FindFirstWakeupBatchLocked()
{
    auto it = std::find_if(alarmBatches_.begin(),
        alarmBatches_.end(),
        [](const std::shared_ptr<Batch> &batch) {
            return batch->HasWakeups();
        });
    return (it != alarmBatches_.end()) ? *it : nullptr;
}

void TimerManager::SetLocked(int type, std::chrono::nanoseconds when)
{
    TIME_HILOGI(TIME_MODULE_SERVICE, "start when.count= %{public}lld", when.count());
    handler_->Set(static_cast<uint32_t>(type), when);
    TIME_HILOGI(TIME_MODULE_SERVICE, "end");
}

void TimerManager::InsertAndBatchTimerLocked(std::shared_ptr<TimerInfo> alarm)
{
    TIME_HILOGI(TIME_MODULE_SERVICE, "start");
    int64_t whichBatch = (alarm->flags & static_cast<uint32_t>(STANDALONE)) ? -1
        : AttemptCoalesceLocked(alarm->whenElapsed, alarm->maxWhenElapsed);
    TIME_HILOGI(TIME_MODULE_SERVICE, "whichBatch= %{public}" PRId64 "", whichBatch);
    if (whichBatch < 0) {
        AddBatchLocked(alarmBatches_, std::make_shared<Batch>(*alarm));
    } else {
        auto batch = alarmBatches_.at(whichBatch);
        if (batch->Add(alarm)) {
            alarmBatches_.erase(alarmBatches_.begin() + whichBatch);
            AddBatchLocked(alarmBatches_, batch);
        }
    }
    TIME_HILOGI(TIME_MODULE_SERVICE, "end");
}

int64_t TimerManager::AttemptCoalesceLocked(std::chrono::steady_clock::time_point whenElapsed,
                                            std::chrono::steady_clock::time_point maxWhen)
{
    TIME_HILOGI(TIME_MODULE_SERVICE, "start");
    int64_t i = 0;
    for (const auto &item : alarmBatches_) {
        if ((item->GetFlags() & static_cast<uint32_t>(STANDALONE)) == 0 && item->CanHold(whenElapsed, maxWhen)) {
            return i;
        }
    }
    return -1;
}

void TimerManager::DeliverTimersLocked(const std::vector<std::shared_ptr<TimerInfo>> &triggerList,
                                       std::chrono::steady_clock::time_point nowElapsed)
{
    TIME_HILOGI(TIME_MODULE_SERVICE, "start");
    for (const auto &alarm : triggerList) {
        if (alarm->callback) {
            CallbackAlarmIfNeed(alarm);
        }
    }
    TIME_HILOGI(TIME_MODULE_SERVICE, "end");
}

void TimerManager::CallbackAlarmIfNeed(std::shared_ptr<TimerInfo> alarm)
{
    if (!alarm->callback) {
        TIME_HILOGE(TIME_MODULE_SERVICE, "Callback is nullptr!");
        return;
    }
    int uid = alarm->uid;
    TIME_HILOGI(TIME_MODULE_SERVICE, "Check uid: %{public}d", alarm->uid);
    std::lock_guard<std::mutex> lock(proxyMutex_);
    auto it = proxyUids_.find(uid);
    if (it == proxyUids_.end()) {
        alarm->callback(alarm->id);
        TIME_HILOGI(TIME_MODULE_SERVICE, "Trigger id: %{public}" PRId64 "", alarm->id);
        return;
    }
    TIME_HILOGI(TIME_MODULE_SERVICE, "Alarm is proxy!");
    if (proxyMap_.count(uid) == 0) {
        std::vector<std::shared_ptr<TimerInfo>> timeInfoVec;
        timeInfoVec.push_back(alarm);
        proxyMap_[uid] = timeInfoVec;
    } else {
        std::vector<std::shared_ptr<TimerInfo>> timeInfoVec = proxyMap_[uid];
        timeInfoVec.push_back(alarm);
        proxyMap_[uid] = timeInfoVec;
    }
}

bool TimerManager::ProxyTimer(int32_t uid, bool isProxy)
{
    std::lock_guard<std::mutex> lock(proxyMutex_);
    TIME_HILOGI(TIME_MODULE_SERVICE, "start");
    if (isProxy) {
        proxyUids_.insert(uid);
    } else {
        if (proxyUids_.count(uid) > 0) {
            proxyUids_.erase(uid);
        } else {
            TIME_HILOGI(TIME_MODULE_SERVICE, "Uid: %{public}d doesn't exist in the proxy list." PRId64 "", uid);
            return false;
        }
        if (proxyMap_.count(uid) > 0) {
            auto timeInfoVec = proxyMap_[uid];
            for (auto alarm : timeInfoVec) {
                if (!alarm->callback) {
                    TIME_HILOGE(TIME_MODULE_SERVICE, "Callback is nullptr!");
                    return false;
                }
                TIME_HILOGD(TIME_MODULE_SERVICE, "Shut down proxy, proxyUid: %{public}d, alarmId: %{public}" PRId64 "", uid, alarm->id);
            }
            proxyMap_.erase(uid);
        }
    }
    std::set<int>::iterator it;
    for (it = proxyUids_.begin(); it != proxyUids_.end(); it++) {
        TIME_HILOGI(TIME_MODULE_SERVICE, "proxyUid:%{public}d", *it);
    }
    return true;
}

bool TimerManager::ResetAllProxy()
{
    std::lock_guard<std::mutex> lock(proxyMutex_);
    TIME_HILOGI(TIME_MODULE_SERVICE, "start");
    for (auto it = proxyMap_.begin(); it != proxyMap_.end(); it++) {
        auto timeInfoVec = it->second;
        for (auto alarm : timeInfoVec) {
            if (!alarm->callback) {
                TIME_HILOGE(TIME_MODULE_SERVICE, "Callback is nullptr!");
                return false;
            }
            alarm->callback(alarm->id);
            TIME_HILOGD(TIME_MODULE_SERVICE, "Reset all proxy, proxyUid: %{public}d, alarmId: %{public}" PRId64 "", it->first, alarm->id);
        }
        timeInfoVec.clear();
    }
    proxyMap_.clear();
    std::set<int>::iterator it;
    for (it = proxyUids_.begin(); it != proxyUids_.end(); it++) {
        TIME_HILOGI(TIME_MODULE_SERVICE, "proxyUid:%{public}d", *it);
    }
    proxyUids_.clear();
    return true;
}

bool AddBatchLocked(std::vector<std::shared_ptr<Batch>> &list, const std::shared_ptr<Batch> &newBatch)
{
    TIME_HILOGI(TIME_MODULE_SERVICE, "start");
    auto it = std::upper_bound(list.begin(),
                               list.end(),
                               newBatch,
                               [](const std::shared_ptr<Batch> &first, const std::shared_ptr<Batch> &second) {
                                    return first->GetStart() < second->GetStart();
                                });
    list.insert(it, newBatch);
    TIME_HILOGI(TIME_MODULE_SERVICE, "end");
    return it == list.begin();
}

steady_clock::time_point MaxTriggerTime(steady_clock::time_point now,
                                        steady_clock::time_point triggerAtTime,
                                        milliseconds interval)
{
    milliseconds futurity = (interval == milliseconds::zero()) ? 
        (duration_cast<milliseconds>(triggerAtTime - now)) : interval;
    if (futurity < MIN_FUZZABLE_INTERVAL) {
        futurity = milliseconds::zero();
    }
    return triggerAtTime + milliseconds(static_cast<long>(BATCH_WINDOW_COE * futurity.count()));
}
} // MiscServices
} // OHOS