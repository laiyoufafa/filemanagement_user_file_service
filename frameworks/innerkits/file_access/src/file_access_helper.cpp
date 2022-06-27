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

#include "file_access_helper.h"

#include "file_access_framework_errno.h"
#include "hilog_wrapper.h"
#include "ifile_access_ext_base.h"

namespace OHOS {
namespace FileAccessFwk {
FileAccessHelper::FileAccessHelper(const std::shared_ptr<OHOS::AbilityRuntime::Context> &context,
    const AAFwk::Want &want, const sptr<IFileAccessExtBase> &fileAccessExtProxy)
{
    token_ = context->GetToken();
    want_ = want;
    fileAccessExtProxy_ = fileAccessExtProxy;
    fileAccessExtConnection_ = FileAccessExtConnection::GetInstance();
}

void FileAccessHelper::AddFileAccessDeathRecipient(const sptr<IRemoteObject> &token)
{
    if (token != nullptr && callerDeathRecipient_ != nullptr) {
        token->RemoveDeathRecipient(callerDeathRecipient_);
    }
    if (callerDeathRecipient_ == nullptr) {
        callerDeathRecipient_ =
            new FileAccessDeathRecipient(std::bind(&FileAccessHelper::OnSchedulerDied, this, std::placeholders::_1));
    }
    if (token != nullptr) {
        token->AddDeathRecipient(callerDeathRecipient_);
    }
}

void FileAccessHelper::OnSchedulerDied(const wptr<IRemoteObject> &remote)
{
    auto object = remote.promote();
    object = nullptr;
    fileAccessExtProxy_ = nullptr;
}

FileAccessHelper::FileAccessHelper(const sptr<IRemoteObject> &token,
    const AAFwk::Want &want, const sptr<IFileAccessExtBase> &fileAccessExtProxy)
{
    token_ = token;
    want_ = want;
    fileAccessExtProxy_ = fileAccessExtProxy;
    fileAccessExtConnection_ = FileAccessExtConnection::GetInstance();
}

std::shared_ptr<FileAccessHelper> FileAccessHelper::Creator(
    const sptr<IRemoteObject> &token, const AAFwk::Want &want)
{
    sptr<IFileAccessExtBase> fileAccessExtProxy = nullptr;

    sptr<FileAccessExtConnection> fileAccessExtConnection = FileAccessExtConnection::GetInstance();
    if (!fileAccessExtConnection->IsExtAbilityConnected()) {
        fileAccessExtConnection->ConnectFileExtAbility(want, token);
    }
    fileAccessExtProxy = fileAccessExtConnection->GetFileExtProxy();
    if (fileAccessExtProxy == nullptr) {
        HILOG_WARN("FileAccessHelper::Creator get invalid fileAccessExtProxy");
    }

    FileAccessHelper *ptrDataShareHelper = new (std::nothrow) FileAccessHelper(token, want, fileAccessExtProxy);
    if (ptrDataShareHelper == nullptr) {
        HILOG_ERROR("FileAccessHelper::Creator failed, create FileAccessHelper failed");
        return nullptr;
    }

    return std::shared_ptr<FileAccessHelper>(ptrDataShareHelper);
}

std::shared_ptr<FileAccessHelper> FileAccessHelper::Creator(
    const std::shared_ptr<OHOS::AbilityRuntime::Context> &context, const AAFwk::Want &want)
{
    if (context == nullptr) {
        HILOG_ERROR("%{public}s failed, context == nullptr", __func__);
        return nullptr;
    }

    sptr<IFileAccessExtBase> fileAccessExtProxy = nullptr;
    sptr<FileAccessExtConnection> fileAccessExtConnection = FileAccessExtConnection::GetInstance();
    if (!fileAccessExtConnection->IsExtAbilityConnected()) {
        fileAccessExtConnection->ConnectFileExtAbility(want, context->GetToken());
    }
    fileAccessExtProxy = fileAccessExtConnection->GetFileExtProxy();
    if (fileAccessExtProxy == nullptr) {
        HILOG_WARN("%{public}s get invalid fileAccessExtProxy", __func__);
    }

    FileAccessHelper *ptrFileAccessHelper = new (std::nothrow) FileAccessHelper(context, want, fileAccessExtProxy);
    if (ptrFileAccessHelper == nullptr) {
        HILOG_ERROR("%{public}s failed, create FileAccessHelper failed", __func__);
        return nullptr;
    }

    return std::shared_ptr<FileAccessHelper>(ptrFileAccessHelper);
}

bool FileAccessHelper::Release()
{
    if (fileAccessExtConnection_->IsExtAbilityConnected()) {
        fileAccessExtConnection_->DisconnectFileExtAbility();
    }
    fileAccessExtProxy_ = nullptr;
    return true;
}

bool FileAccessHelper::GetProxy()
{
    if (!fileAccessExtConnection_->IsExtAbilityConnected()) {
        fileAccessExtConnection_->ConnectFileExtAbility(want_, token_);
    }
    fileAccessExtProxy_ = fileAccessExtConnection_->GetFileExtProxy();
    if (isSystemCaller_ && fileAccessExtProxy_) {
        AddFileAccessDeathRecipient(fileAccessExtProxy_->AsObject());
    }
    
    if (fileAccessExtProxy_ == nullptr) {
        HILOG_ERROR("%{public}s failed with invalid fileAccessExtProxy_", __func__);
        return false;
    }
    return true;
}

int FileAccessHelper::OpenFile(Uri &uri, int flags)
{
    int fd = ERR_ERROR;
    if (!GetProxy()) {
        HILOG_ERROR("%{public}s failed with invalid fileAccessExtProxy_", __func__);
        return fd;
    }

    fd = fileAccessExtProxy_->OpenFile(uri, flags);
    return fd;
}

int FileAccessHelper::CreateFile(Uri &parent, const std::string &displayName, Uri &newFile)
{
    int index = ERR_ERROR;
    if (!GetProxy()) {
        HILOG_ERROR("%{public}s failed with invalid fileAccessExtProxy_", __func__);
        return index;
    }

    index = fileAccessExtProxy_->CreateFile(parent, displayName, newFile);
    return index;
}

int FileAccessHelper::Mkdir(Uri &parent, const std::string &displayName, Uri &newDir)
{
    int index = ERR_ERROR;
    if (fileAccessExtProxy_ == nullptr) {
        HILOG_ERROR("%{public}s failed with invalid fileAccessExtProxy_", __func__);
        return index;
    }

    index = fileAccessExtProxy_->Mkdir(parent, displayName, newDir);
    return index;
}

int FileAccessHelper::Delete(Uri &selectFile)
{
    int index = ERR_ERROR;
    if (!GetProxy()) {
        HILOG_ERROR("%{public}s failed with invalid fileAccessExtProxy_", __func__);
        return index;
    }

    index = fileAccessExtProxy_->Delete(selectFile);
    return index;
}

int FileAccessHelper::Move(Uri &sourceFile, Uri &targetParent, Uri &newFile)
{
    int index = ERR_ERROR;
    if (!GetProxy()) {
        HILOG_ERROR("%{public}s failed with invalid fileAccessExtProxy_", __func__);
        return index;
    }

    index = fileAccessExtProxy_->Move(sourceFile, targetParent, newFile);
    return index;
}

int FileAccessHelper::Rename(Uri &sourceFile, const std::string &displayName, Uri &newFile)
{
    int index = ERR_ERROR;
    if (!GetProxy()) {
        HILOG_ERROR("%{public}s failed with invalid fileAccessExtProxy_", __func__);
        return index;
    }

    index = fileAccessExtProxy_->Rename(sourceFile, displayName, newFile);
    return index;
}

std::vector<FileInfo> FileAccessHelper::ListFile(Uri &sourceFile)
{
    std::vector<FileInfo> results;
    if (!GetProxy()) {
        HILOG_ERROR("%{public}s failed with invalid fileAccessExtProxy_", __func__);
        return results;
    }

    results = fileAccessExtProxy_->ListFile(sourceFile);
    return results;
}

std::vector<DeviceInfo> FileAccessHelper::GetRoots()
{
    std::vector<DeviceInfo> results;

    if (!GetProxy()) {
        HILOG_ERROR("%{public}s failed with invalid fileAccessExtProxy_", __func__);
        return results;
    }

    results = fileAccessExtProxy_->GetRoots();
    return results;
}

void FileAccessDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    if (handler_) {
        handler_(remote);
    }
}

FileAccessDeathRecipient::FileAccessDeathRecipient(RemoteDiedHandler handler) : handler_(handler)
{
}

FileAccessDeathRecipient::~FileAccessDeathRecipient()
{
}
} // namespace FileAccessFwk
} // namespace OHOS