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

#include "file_ext_connection.h"

#include "ability_manager_client.h"
#include "bytrace.h"
#include "file_ext_proxy.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace AppExecFwk {
sptr<FileExtConnection> FileExtConnection::instance_ = nullptr;
std::mutex FileExtConnection::mutex_;

sptr<FileExtConnection> FileExtConnection::GetInstance()
{
    HILOG_INFO("tag dsa %{public}s begin.", __func__);
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = sptr<FileExtConnection>(new (std::nothrow) FileExtConnection());
        }
    }
    HILOG_INFO("tag dsa %{public}s end.", __func__);
    return instance_;
}

void FileExtConnection::OnAbilityConnectDone(
    const AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject, int resultCode)
{
    HILOG_INFO("tag dsa %{public}s called begin", __func__);
    BYTRACE_NAME(BYTRACE_TAG_DISTRIBUTEDDATA, __PRETTY_FUNCTION__);
    if (remoteObject == nullptr) {
        HILOG_ERROR("tag dsa FileExtConnection::OnAbilityConnectDone failed, remote is nullptr");
        return;
    }
    fileExtProxy_ = iface_cast<FileExtProxy>(remoteObject);
    if (fileExtProxy_ == nullptr) {
        HILOG_ERROR("tag dsa FileExtConnection::OnAbilityConnectDone failed, fileExtProxy_ is nullptr");
        return;
    }
    isConnected_.store(true);
    HILOG_INFO("tag dsa %{public}s end.", __func__);
}

void FileExtConnection::OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int resultCode)
{
    HILOG_INFO("tag dsa %{public}s called begin", __func__);
    BYTRACE_NAME(BYTRACE_TAG_DISTRIBUTEDDATA, __PRETTY_FUNCTION__);
    fileExtProxy_ = nullptr;
    isConnected_.store(false);
    HILOG_INFO("tag dsa %{public}s called end", __func__);
}

void FileExtConnection::ConnectFileExtAbility(const AAFwk::Want &want, const sptr<IRemoteObject> &token)
{
    HILOG_INFO("tag dsa %{public}s called begin", __func__);
    ErrCode ret = AAFwk::AbilityManagerClient::GetInstance()->ConnectAbility(want, this, token);
    HILOG_INFO("tag dsa %{public}s called end, ret=%{public}d", __func__, ret);
}

void FileExtConnection::DisconnectFileExtAbility()
{
    HILOG_INFO("tag dsa %{public}s called begin", __func__);
    fileExtProxy_ = nullptr;
    isConnected_.store(false);
    ErrCode ret = AAFwk::AbilityManagerClient::GetInstance()->DisconnectAbility(this);
    HILOG_INFO("tag dsa %{public}s called end, ret=%{public}d", __func__, ret);
}

bool FileExtConnection::IsExtAbilityConnected()
{
    HILOG_INFO("tag dsa %{public}s called begin", __func__);
    return isConnected_.load();
}

sptr<IFileExtBase> FileExtConnection::GetFileExtProxy()
{
    HILOG_INFO("tag dsa %{public}s called begin", __func__);
    return fileExtProxy_;
}
}  // namespace AppExecFwk
}  // namespace OHOS