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
#ifndef STORAGE_FILE_MANAGER_PROXY_H
#define STORAGE_FILE_MANAGER_PROXY_H

#include "file_manager_service_stub.h"
#include "ifms_client.h"
#include "iremote_proxy.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace FileManagerService {
class FileManagerProxy : public IRemoteProxy<IFileManagerService>, public IFmsClient {
public:
    explicit FileManagerProxy(const sptr<IRemoteObject> &impl);
    virtual ~FileManagerProxy() = default;
    int Mkdir(const std::string &name, const std::string &path) override;
    int ListFile(const std::string &type, const std::string &path, const CmdOptions &option,
        std::vector<std::shared_ptr<FileInfo>> &fileRes) override;
    int CreateFile(const std::string &path, const std::string &fileName,
        const CmdOptions &option, std::string &uri) override;
    int GetRoot(const CmdOptions &option, std::vector<std::shared_ptr<FileInfo>> &fileRes) override;
private:
    static inline BrokerDelegator<FileManagerProxy> delegator_;
};
} // namespace FileManagerService
} // namespace OHOS
#endif // STORAGE_FILE_MANAGER_PROXY_H