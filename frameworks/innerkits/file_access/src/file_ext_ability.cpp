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

#include "file_ext_ability.h"

#include "ability_loader.h"
#include "connection_manager.h"
#include "extension_context.h"
#include "hilog_wrapper.h"
#include "js_file_ext_ability.h"
#include "runtime.h"

namespace OHOS {
namespace FileAccessFwk {
using namespace OHOS::AppExecFwk;

CreatorFunc FileExtAbility::creator_ = nullptr;
void FileExtAbility::SetCreator(const CreatorFunc& creator)
{
    creator_ = creator;
}

FileExtAbility* FileExtAbility::Create(const std::unique_ptr<Runtime>& runtime)
{
    HILOG_INFO("%{public}s begin.", __func__);
    if (!runtime) {
        return new FileExtAbility();
    }
    if (creator_) {
        return creator_(runtime);
    }
    HILOG_INFO("%{public}s runtime", __func__);
    switch (runtime->GetLanguage()) {
        case Runtime::Language::JS:
            HILOG_INFO("%{public}s Runtime::Language::JS --> JsFileExtAbility", __func__);
            return JsFileExtAbility::Create(runtime);

        default:
            HILOG_INFO("%{public}s default --> FileExtAbility", __func__);
            return new FileExtAbility();
    }
    HILOG_INFO("%{public}s begin.", __func__);
}

void FileExtAbility::Init(const std::shared_ptr<AbilityLocalRecord> &record,
    const std::shared_ptr<OHOSApplication> &application,
    std::shared_ptr<AbilityHandler> &handler,
    const sptr<IRemoteObject> &token)
{
    HILOG_INFO("%{public}s begin.", __func__);
    ExtensionBase<>::Init(record, application, handler, token);
    HILOG_INFO("%{public}s end.", __func__);
}

int FileExtAbility::OpenFile(const Uri &uri, int flags)
{
    HILOG_INFO("%{public}s begin.", __func__);
    HILOG_INFO("%{public}s end.", __func__);
    return 0;
}

int FileExtAbility::CreateFile(const Uri &parentUri, const std::string &displayName,  Uri &newFileUri)
{
    HILOG_INFO("%{public}s begin.", __func__);
    HILOG_INFO("%{public}s end.", __func__);
    return 0;
}

int FileExtAbility::Mkdir(const Uri &parentUri, const std::string &displayName, Uri &newFileUri)
{
    HILOG_INFO("%{public}s begin.", __func__);
    HILOG_INFO("%{public}s end.", __func__);
    return 0;
}

int FileExtAbility::Delete(const Uri &sourceFileUri)
{
    HILOG_INFO("%{public}s begin.", __func__);
    HILOG_INFO("%{public}s end.", __func__);
    return 0;
}

int FileExtAbility::Move(const Uri &sourceFileUri, const Uri &targetParentUri, Uri &newFileUri)
{
    return 0;
}

int FileExtAbility::Rename(const Uri &sourceFileUri, const std::string &displayName, Uri &newFileUri)
{
    HILOG_INFO("%{public}s begin.", __func__);
    HILOG_INFO("%{public}s end.", __func__);
    return 0;
}

std::vector<FileInfo> FileExtAbility::ListFile(const Uri &sourceFileUri)
{
    std::vector<FileInfo> vec;
    return vec;
}

std::vector<DeviceInfo> FileExtAbility::GetRoots()
{
    std::vector<DeviceInfo> vec;
    return vec;
}
} // namespace FileAccessFwk
} // namespace OHOS