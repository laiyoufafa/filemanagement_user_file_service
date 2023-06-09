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

#ifndef OHOS_APPEXECFWK_FILEEXT_STUB_H
#define OHOS_APPEXECFWK_FILEEXT_STUB_H

#include <iremote_stub.h>
#include <map>

#include "ifile_ext_base.h"

namespace OHOS {
namespace AppExecFwk {
class FileExtStub : public IRemoteStub<IFileExtBase> {
public:
    FileExtStub();
    ~FileExtStub();
    int OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option) override;
private:
    ErrCode CmdOpenFile(MessageParcel &data, MessageParcel &reply);
    ErrCode CmdCloseFile(MessageParcel &data, MessageParcel &reply);
    ErrCode CmdCreateFile(MessageParcel &data, MessageParcel &reply);
    ErrCode CmdMkdir(MessageParcel &data, MessageParcel &reply);
    ErrCode CmdDelete(MessageParcel &data, MessageParcel &reply);
    ErrCode CmdMove(MessageParcel &data, MessageParcel &reply);
    ErrCode CmdRename(MessageParcel &data, MessageParcel &reply);
    using RequestFuncType = int (FileExtStub::*)(MessageParcel &data, MessageParcel &reply);
    std::map<uint32_t, RequestFuncType> stubFuncMap_;
};
} // namespace AppExecFwk
} // namespace OHOS
#endif // OHOS_APPEXECFWK_FILEEXT_STUB_H

