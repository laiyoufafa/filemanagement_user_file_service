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
#ifndef STORAGE_SERVICES_CMD_RESPONSE_H
#define STORAGE_SERVICES_CMD_RESPONSE_H

#include <memory>
#include <string>
#include <vector>

#include "file_info.h"
#include "log.h"
#include "parcel.h"
namespace OHOS {
namespace FileManagerService {
class CmdResponse : public Parcelable {
public:
    CmdResponse() = default;
    CmdResponse(int &err, std::string &uri, std::vector<std::shared_ptr<FileInfo>> &fileInfoList)
        : err_(err), uri_(uri), vecFileInfo_(fileInfoList)
    {}
    ~CmdResponse() = default;

    void SetErr(const int err)
    {
        err_ = err;
    }

    int GetErr() const
    {
        return err_;
    }

    void SetUri(const std::string &uri)
    {
        uri_ = uri;
    }

    std::string GetUri() const
    {
        return uri_;
    }

    void SetFileInfoList(std::vector<std::shared_ptr<FileInfo>> &fileInfoList)
    {
        vecFileInfo_ = fileInfoList;
    }

    std::vector<std::shared_ptr<FileInfo>> GetFileInfoList()
    {
        return vecFileInfo_;
    }

    virtual bool Marshalling(Parcel &parcel) const override
    {
        parcel.WriteInt32(err_);
        parcel.WriteString(uri_);
        size_t fileCount = vecFileInfo_.size();
        parcel.WriteUint64(fileCount);
        for (size_t i = 0; i < fileCount; i++) {
            if (parcel.WriteParcelable(vecFileInfo_[i].get()) != true) {
                ERR_LOG("Marshalling FileInfo fails!");
                return false;
            }
        }
        return true;
    }

    static CmdResponse* Unmarshalling(Parcel &parcel)
    {
        auto *obj = new (std::nothrow) CmdResponse();
        if (obj == nullptr) {
            return nullptr;
        }
        obj->err_ = parcel.ReadInt32();
        obj->uri_ = parcel.ReadString();
        size_t fileCount = parcel.ReadUint64();
        for (size_t i = 0; i < fileCount; i++) {
            std::shared_ptr<FileInfo> file(parcel.ReadParcelable<FileInfo>());
            obj->vecFileInfo_.emplace_back(file);
        }
        return obj;
    }
private:
    int err_;
    std::string uri_;
    std::vector<std::shared_ptr<FileInfo>> vecFileInfo_;
};
} // FileManagerService
} // namespace OHOS
#endif // STORAGE_SERVICES_CMD_RESPONSE_H