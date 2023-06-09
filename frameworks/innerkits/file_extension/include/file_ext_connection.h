/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef FOUNDATION_APPEXECFWK_OHOS_FILEEXT_CONNECTION_H
#define FOUNDATION_APPEXECFWK_OHOS_FILEEXT_CONNECTION_H

#include <memory>

#include "ability_connect_callback_stub.h"
#include "event_handler.h"
#include "ifile_ext_base.h"
#include "want.h"

namespace OHOS {
namespace AppExecFwk {
class FileExtConnection : public AAFwk::AbilityConnectionStub {
public:
    FileExtConnection() = default;
    virtual ~FileExtConnection() = default;


    static sptr<FileExtConnection> GetInstance();

    void OnAbilityConnectDone(
        const AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject, int resultCode) override;

    void OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int resultCode) override;


    void ConnectFileExtAbility(const AAFwk::Want &want, const sptr<IRemoteObject> &token);


    void DisconnectFileExtAbility();


    bool IsExtAbilityConnected();


    sptr<IFileExtBase> GetFileExtProxy();

private:
    static sptr<FileExtConnection> instance_;
    static std::mutex mutex_;
    std::atomic<bool> isConnected_ = {false};
    sptr<IFileExtBase> fileExtProxy_;
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_OHOS_DATASHARE_CONNECTION_H
