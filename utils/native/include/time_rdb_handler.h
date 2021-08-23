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

#include "rdb_errno.h"
#include "rdb_helper.h"
#include <mutex>

namespace OHOS {
namespace MiscServices {
    using namespace OHOS::NativeRdb;
    class TimeOpenCallback : public RdbOpenCallback {
    public:
        int OnCreate(RdbStore &rdbStore) override;
        int OnUpgrade(RdbStore &rdbStore, int oldVersion, int newVersion) override;
        static const std::string CREATE_TIMEZONE_DB;
    };
    
    std::string const TimeOpenCallback::CREATE_TIMEZONE_DB = std::string("CREATE TABLE IF NOT EXISTS timezone ")
                                                              + std::string("(id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                                                            "timezoneId TEXT NOT NULL)");

    int TimeOpenCallback::OnCreate(RdbStore &store)
    {
        return store.ExecuteSql(CREATE_TIMEZONE_DB);
    }

    int TimeOpenCallback::OnUpgrade(RdbStore &store, int oldVersion, int newVersion)
    {
        return E_OK;
    }

    bool InsertTimeZoneIdToRdb(const std::string timeZoneId){
        const std::string dbPath_ = "/data/misc/zoneinfo/";
        const std::string dbName_ = "systime.db";
        
        RdbStoreConfig config(dbPath_+ dbName_);

        TimeOpenCallback helper;
        int errCode = E_OK;
        auto store = RdbHelper::GetRdbStore(config, 1, helper, errCode);
        if (store == nullptr){
            return false;
        }
        int deletedRows;
        store->Delete(deletedRows, "timezone", "id = 1");

        int64_t id;
        ValuesBucket values;
        values.PutInt("id", 1);
        values.PutString("timezoneId", timeZoneId);
        auto ret = store->Insert(id, "timezone", values);

        TIME_HILOGD(TIME_MODULE_SERVICE,"end.");
        return ret == E_OK;
    }

    bool GetTimeZoneId(std::string &timeZoneId){
        const std::string dbPath_ = "/data/misc/zoneinfo/";
        const std::string dbName_ = "systime.db";
        
        RdbStoreConfig config(dbPath_+ dbName_);
        TimeOpenCallback helper;
        int errCode = E_OK;
        auto store = RdbHelper::GetRdbStore(config, 1, helper, errCode);
        if (store == nullptr){
            return false;
        }
        std::unique_ptr<ResultSet> resultSet = store->QuerySql("SELECT * FROM timezone");
        if (resultSet == nullptr){
            TIME_HILOGD(TIME_MODULE_SERVICE,"not found");
            return false;
        }
        int columnIndex;
        std::string strVal;
        auto ret =  resultSet->GoToFirstRow();
        if (ret != E_OK){
            return false;
        }
        ret = resultSet->GetColumnIndex("timezoneId", columnIndex);
        if (ret != E_OK){
            return false;
        }
        ret = resultSet->GetString(columnIndex, strVal);
        if (ret != E_OK){
            return false;
        }
        timeZoneId = strVal;
        return E_OK;
    }
}
}