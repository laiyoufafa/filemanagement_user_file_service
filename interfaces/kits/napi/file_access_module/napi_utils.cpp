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

#include "napi_utils.h"

#include <cinttypes>
#include <string>
#include <vector>

#include "file_access_extension_info.h"
#include "file_access_framework_errno.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace FileAccessFwk {
int IsDirectory(const int64_t mode)
{
    if ((mode & DOCUMENT_FLAG_REPRESENTS_DIR) != DOCUMENT_FLAG_REPRESENTS_DIR) {
        HILOG_ERROR("current FileInfo is not dir");
        return ERR_INVALID_PARAM;
    }

    if ((mode & DOCUMENT_FLAG_REPRESENTS_FILE) == DOCUMENT_FLAG_REPRESENTS_FILE) {
        HILOG_ERROR("file mode(%{public}" PRId64 ") is incorrect", mode);
        return ERR_INVALID_PARAM;
    }

    return ERR_OK;
}

int GetFileFilterParam(const NVal &argv, FileFilter &filter)
{
    bool ret = false;
    filter.SetHasFilter(false);
    if (argv.HasProp("suffix")) {
        std::vector<std::string> suffixs;
        std::tie(ret, suffixs, std::ignore) = argv.GetProp("suffix").ToStringArray();
        if (!ret) {
            HILOG_ERROR("FileFilter get suffix param fail.");
            return ERR_INVALID_PARAM;
        }

        filter.SetSuffix(suffixs);
        filter.SetHasFilter(true);
    } else {
        return ERR_INVALID_PARAM;
    }

    if (argv.HasProp("display_name")) {
        std::vector<std::string> displayNames;
        std::tie(ret, displayNames, std::ignore) = argv.GetProp("display_name").ToStringArray();
        if (!ret) {
            HILOG_ERROR("FileFilter get display_name param fail.");
            return ERR_INVALID_PARAM;
        }

        filter.SetDisplayName(displayNames);
        filter.SetHasFilter(true);
    }

    if (argv.HasProp("mime_type")) {
        std::vector<std::string> mimeTypes;
        std::tie(ret, mimeTypes, std::ignore) = argv.GetProp("mime_type").ToStringArray();
        if (!ret) {
            HILOG_ERROR("FileFilter get mime_type param fail.");
            return ERR_INVALID_PARAM;
        }

        filter.SetMimeType(mimeTypes);
        filter.SetHasFilter(true);
    }

    if (argv.HasProp("file_size_over")) {
        int64_t fileSizeOver;
        std::tie(ret, fileSizeOver) = argv.GetProp("file_size_over").ToInt64();
        if (!ret) {
            HILOG_ERROR("FileFilter get file_size_over param fail.");
            return ERR_INVALID_PARAM;
        }

        filter.SetFileSizeOver(fileSizeOver);
        filter.SetHasFilter(true);
    }

    if (argv.HasProp("last_modified_after")) {
        double lastModifiedAfter;
        std::tie(ret, lastModifiedAfter) = argv.GetProp("last_modified_after").ToDouble();
        if (!ret) {
            HILOG_ERROR("FileFilter get last_modified_after param fail.");
            return ERR_INVALID_PARAM;
        }

        filter.SetLastModifiedAfter(lastModifiedAfter);
        filter.SetHasFilter(true);
    }

    if (argv.HasProp("exclude_media")) {
        bool excludeMedia;
        std::tie(ret, excludeMedia) = argv.GetProp("exclude_media").ToBool();
        if (!ret) {
            HILOG_ERROR("FileFilter get exclude_media param fail.");
            return ERR_INVALID_PARAM;
        }

        filter.SetExcludeMedia(excludeMedia);
        filter.SetHasFilter(true);
    }

    if (!filter.GetHasFilter()) {
        HILOG_ERROR("FileFilter must have one property.");
        return ERR_INVALID_PARAM;
    }
    return ERR_OK;
}
} // namespace FileAccessFwk
} // namespace OHOS