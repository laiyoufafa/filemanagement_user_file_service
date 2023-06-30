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

#include <cstdio>
#include <thread>
#include <unistd.h>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "accesstoken_kit.h"
#include "file_access_extension_info.h"
#include "file_access_framework_errno.h"
#include "file_access_helper.h"
#include "iservice_registry.h"
#include "image_source.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

namespace {
using namespace std;
using namespace OHOS;
using namespace FileAccessFwk;
using namespace OHOS::Media;
using json = nlohmann::json;
const int ABILITY_ID = 5003;
const int INIT_THREADS_NUMBER = 4;
const int ACTUAL_SUCCESS_THREADS_NUMBER = 1;
int g_num = 0;
shared_ptr<FileAccessHelper> g_fah = nullptr;
Uri g_newDirUri("");
const int UID_TRANSFORM_TMP = 20000000;
const int UID_DEFAULT = 0;

void SetNativeToken()
{
    uint64_t tokenId;
    const char **perms = new const char *[1];
    perms[0] = "ohos.permission.FILE_ACCESS_MANAGER";
    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 1,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .aplStr = "system_core",
    };

    infoInstance.processName = "SetUpTestCase";
    tokenId = GetAccessTokenId(&infoInstance);
    const uint64_t systemAppMask = (static_cast<uint64_t>(1) << 32);
    tokenId |= systemAppMask;
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
    delete[] perms;
}

class FileAccessHelperTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        cout << "FileAccessHelperTest code test" << endl;
        SetNativeToken();
        auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        auto remoteObj = saManager->GetSystemAbility(ABILITY_ID);
        AAFwk::Want want;
        vector<AAFwk::Want> wantVec;
        setuid(UID_TRANSFORM_TMP);
        int ret = FileAccessHelper::GetRegisteredFileAccessExtAbilityInfo(wantVec);
        EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
        bool sus = false;
        for (size_t i = 0; i < wantVec.size(); i++) {
            auto element = wantVec[i].GetElement();
            if (element.GetBundleName() == "com.ohos.medialibrary.medialibrarydata" &&
                element.GetAbilityName() == "FileExtensionAbility") {
                want = wantVec[i];
                sus = true;
                break;
            }
        }
        EXPECT_TRUE(sus);
        vector<AAFwk::Want> wants{want};
        g_fah = FileAccessHelper::Creator(remoteObj, wants);
        if (g_fah == nullptr) {
            GTEST_LOG_(ERROR) << "medialibrary_file_access_test g_fah is nullptr";
            exit(1);
        }
        setuid(UID_DEFAULT);
    }
    static void TearDownTestCase()
    {
        g_fah->Release();
        g_fah = nullptr;
    };
    void SetUp(){};
    void TearDown(){};
};

static Uri GetParentUri()
{
    vector<RootInfo> info;
    int result = g_fah->GetRoots(info);
    EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
    Uri parentUri("");
    if (info.size() > OHOS::FileAccessFwk::ERR_OK) {
        parentUri = Uri(info[0].uri + "/file");
        GTEST_LOG_(ERROR) << parentUri.ToString();
    }
    return parentUri;
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_OpenFile_0000
 * @tc.name: medialibrary_file_access_OpenFile_0000
 * @tc.desc: Test function of OpenFile interface for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_OpenFile_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_OpenFile_0000";
    try {
        Uri parentUri = GetParentUri();
        bool isExist = false;
        int result = g_fah->Access(g_newDirUri, isExist);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        if (!isExist) {
            result = g_fah->Mkdir(parentUri, "Download", g_newDirUri);
            EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        }
        Uri newDirUriTest("file://media/root/file");
        FileInfo fileInfo;
        fileInfo.uri = newDirUriTest.ToString();
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        FileFilter filter;
        result = g_fah->ListFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_GE(fileInfoVec.size(), OHOS::FileAccessFwk::ERR_OK);
        for (size_t i = 0; i < fileInfoVec.size(); i++) {
            if (fileInfoVec[i].fileName.compare("Download") == 0) {
                g_newDirUri = Uri(fileInfoVec[i].uri);
                break;
            }
        }
        result = g_fah->Mkdir(g_newDirUri, "test1", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri newFileUri("");
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_OpenFile_0000.txt", newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        int fd;
        result = g_fah->OpenFile(newFileUri, WRITE_READ, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "OpenFile_0000 result:" << result;
        close(fd);
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_accsess_OpenFile_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_OpenFile_0000";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_OpenFile_0001
 * @tc.name: medialibrary_file_access_OpenFile_0001
 * @tc.desc: Test function of OpenFile interface for ERROR which Uri is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_OpenFile_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_OpenFile_0001";
    try {
        Uri uri("");
        int fd;
        int result = g_fah->OpenFile(uri, WRITE_READ, fd);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "OpenFile_0001 result:" << result;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_OpenFile_0001 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_OpenFile_0001";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_OpenFile_0002
 * @tc.name: medialibrary_file_access_OpenFile_0002
 * @tc.desc: Test function of OpenFile interface for ERROR which Uri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_OpenFile_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_OpenFile_0002";
    try {
        Uri uri("storage/media/100/local/files/Download/medialibrary_file_access_OpenFile_0002.txt");
        int fd;
        int result = g_fah->OpenFile(uri, WRITE_READ, fd);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "OpenFile_0002 result:" << result;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_OpenFile_0002 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_OpenFile_0002";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_OpenFile_0003
 * @tc.name: medialibrary_file_access_OpenFile_0003
 * @tc.desc: Test function of OpenFile interface for ERROR which Uri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_OpenFile_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_OpenFile_0003";
    try {
        Uri uri("~!@#$%^&*()_");
        int fd;
        int result = g_fah->OpenFile(uri, WRITE_READ, fd);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "OpenFile_0003 result:" << result;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_OpenFile_0003 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_OpenFile_0003";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_OpenFile_0004
 * @tc.name: medialibrary_file_access_OpenFile_0004
 * @tc.desc: Test function of OpenFile interface for ERROR which flag is -1.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_OpenFile_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_OpenFile_0004";
    try {
        Uri newFileUri("");
        int result = g_fah->CreateFile(g_newDirUri, "medialibrary_file_access_OpenFile_0004.txt", newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        int fd;
        int flag = -1;
        result = g_fah->OpenFile(newFileUri, flag, fd);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "OpenFile_0004 result:" << result;
        result = g_fah->Delete(newFileUri);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_OpenFile_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_OpenFile_0004";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_OpenFile_0005
 * @tc.name: medialibrary_file_access_OpenFile_0005
 * @tc.desc: Test function of OpenFile interface for SUCCESS which flag is 0.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_OpenFile_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_OpenFile_0005";
    try {
        Uri newFileUri("");
        int result = g_fah->CreateFile(g_newDirUri, "medialibrary_file_access_OpenFile_0005.txt", newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        int fd;
        result = g_fah->OpenFile(newFileUri, READ, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "OpenFile_0005 result:" << result;
        close(fd);
        result = g_fah->Delete(newFileUri);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_OpenFile_0005 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_OpenFile_0005";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_OpenFile_0006
 * @tc.name: medialibrary_file_access_OpenFile_0006
 * @tc.desc: Test function of OpenFile interface for SUCCESS which flag is 1.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_OpenFile_0006, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_OpenFile_0006";
    try {
        Uri newFileUri("");
        int result = g_fah->CreateFile(g_newDirUri, "medialibrary_file_access_OpenFile_0006.txt", newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        int fd;
        result = g_fah->OpenFile(newFileUri, WRITE, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "OpenFile_0006 result:" << result;
        close(fd);
        result = g_fah->Delete(newFileUri);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_OpenFile_0006 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_OpenFile_0006";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_OpenFile_0007
 * @tc.name: medialibrary_file_access_OpenFile_0007
 * @tc.desc: Test function of OpenFile interface for SUCCESS which flag is 2.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_OpenFile_0007, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_OpenFile_0007";
    try {
        Uri newFileUri("");
        int result = g_fah->CreateFile(g_newDirUri, "medialibrary_file_access_OpenFile_0007.txt", newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        int fd;
        result = g_fah->OpenFile(newFileUri, WRITE_READ, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "OpenFile_0007 result:" << result;
        close(fd);
        result = g_fah->Delete(newFileUri);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_OpenFile_0007 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_OpenFile_0007";
}

static void OpenFileTdd(shared_ptr<FileAccessHelper> fahs, Uri uri, int flag, int fd)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_OpenFileTdd";
    int ret = fahs->OpenFile(uri, flag, fd);
    if (ret != OHOS::FileAccessFwk::ERR_OK) {
        GTEST_LOG_(ERROR) << "OpenFileTdd get result error, code:" << ret;
        return;
    }
    EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
    EXPECT_GE(fd, OHOS::FileAccessFwk::ERR_OK);
    g_num++;
    close(fd);
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_OpenFileTdd";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_OpenFile_0008
 * @tc.name: medialibrary_file_access_OpenFile_0008
 * @tc.desc: Test function of OpenFile interface for SUCCESS which Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_OpenFile_0008, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_OpenFile_0008";
    try {
        Uri newFileUri("");
        int fd;
        g_num = 0;
        std::string displayName = "test1.txt";
        int result = g_fah->CreateFile(g_newDirUri, displayName, newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        for (int j = 0; j < INIT_THREADS_NUMBER; j++) {
            std::thread execthread(OpenFileTdd, g_fah, newFileUri, WRITE_READ, fd);
            execthread.join();
        }
        EXPECT_EQ(g_num, INIT_THREADS_NUMBER);
        result = g_fah->Delete(newFileUri);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_OpenFile_0008 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_OpenFile_0008";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_CreateFile_0000
 * @tc.name: medialibrary_file_access_CreateFile_0000
 * @tc.desc: Test function of CreateFile interface for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_CreateFile_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_CreateFile_0000";
    try {
        Uri newFileUri("");
        int result = g_fah->CreateFile(g_newDirUri, "medialibrary_file_access_CreateFile_0000.txt", newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "CreateFile_0000 result:" << result;
        result = g_fah->Delete(newFileUri);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_CreateFile_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_CreateFile_0000";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_CreateFile_0001
 * @tc.name: medialibrary_file_access_CreateFile_0001
 * @tc.desc: Test function of CreateFile interface for ERROR which parentUri is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_CreateFile_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_CreateFile_0001";
    try {
        Uri newFileUri("");
        Uri parentUri("");
        int result = g_fah->CreateFile(parentUri, "medialibrary_file_access_CreateFile_0001.txt", newFileUri);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "CreateFile_0001 result:" << result;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_CreateFile_0001 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_CreateFile_0001";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_CreateFile_0002
 * @tc.name: medialibrary_file_access_CreateFile_0002
 * @tc.desc: Test function of CreateFile interface for ERROR which parentUri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_CreateFile_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_CreateFile_0002";
    try {
        Uri newFileUri("");
        Uri parentUri("storage/media/100/local/files/Download");
        int result = g_fah->CreateFile(parentUri, "medialibrary_file_access_CreateFile_0002.txt", newFileUri);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "CreateFile_0002 result:" << result;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_CreateFile_0002 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_CreateFile_0002";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_CreateFile_0003
 * @tc.name: medialibrary_file_access_CreateFile_0003
 * @tc.desc: Test function of CreateFile interface for ERROR which parentUri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_CreateFile_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_CreateFile_0002";
    try {
        Uri newFileUri("");
        Uri parentUri("~!@#$%^&*()_");
        int result = g_fah->CreateFile(parentUri, "medialibrary_file_access_CreateFile_0003.txt", newFileUri);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "CreateFile_0003 result:" << result;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_CreateFile_0003 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_CreateFile_0003";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_CreateFile_0004
 * @tc.name: medialibrary_file_access_CreateFile_0004
 * @tc.desc: Test function of CreateFile interface for ERROR which displayName is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_CreateFile_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_CreateFile_0004";
    try {
        Uri newFileUri("");
        string displayName = "";
        int result = g_fah->CreateFile(g_newDirUri, displayName, newFileUri);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "CreateFile_0004 result:" << result;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_CreateFile_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_CreateFile_0004";
}

static void CreateFileTdd(shared_ptr<FileAccessHelper> fahs, Uri parent, std::string displayName, Uri newDir)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_CreateFileTdd";
    int ret = fahs->CreateFile(parent, displayName, newDir);
    if (ret != OHOS::FileAccessFwk::ERR_OK) {
        GTEST_LOG_(ERROR) << "CreateFileTdd get result error, code:" << ret;
        return;
    }
    EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
    EXPECT_NE(newDir.ToString(), "");
    g_num++;
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_CreateFileTdd";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_CreateFile_0005
 * @tc.name: medialibrary_file_access_CreateFile_0005
 * @tc.desc: Test function of CreateFile interface for SUCCESS while Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_CreateFile_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_CreateFile_0005";
    try {
        Uri newFileUri1("");
        Uri newFileUri2("");
        Uri newFileUri3("");
        std::string displayName1 = "test1";
        std::string displayName2 = "test2";
        std::string displayName3 = "test3.txt";
        int result = g_fah->Mkdir(g_newDirUri, displayName1, newFileUri1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Mkdir(newFileUri1, displayName2, newFileUri2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        g_num = 0;
        for (int j = 0; j < INIT_THREADS_NUMBER; j++) {
            std::thread execthread(CreateFileTdd, g_fah, newFileUri2, displayName3, newFileUri3);
            execthread.join();
        }
        EXPECT_GE(g_num, ACTUAL_SUCCESS_THREADS_NUMBER);
        GTEST_LOG_(INFO) << "g_newDirUri.ToString() =" << g_newDirUri.ToString();
        result = g_fah->Delete(newFileUri1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_CreateFile_0005 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_CreateFile_0005";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Mkdir_0000
 * @tc.name: medialibrary_file_access_Mkdir_0000
 * @tc.desc: Test function of Mkdir interface for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Mkdir_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Mkdir_0000";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "medialibrary_file_access_Mkdir_0000", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Mkdir_0000 result:" << result;
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Mkdir_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Mkdir_0000";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Mkdir_0001
 * @tc.name: medialibrary_file_access_Mkdir_0001
 * @tc.desc: Test function of Mkdir interface for ERROR which parentUri is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Mkdir_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Mkdir_0001";
    try {
        Uri newDirUriTest("");
        Uri parentUri("");
        int result = g_fah->Mkdir(parentUri, "medialibrary_file_access_Mkdir_0001", newDirUriTest);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Mkdir_0001 result:" << result;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Mkdir_0001 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Mkdir_0001";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Mkdir_0002
 * @tc.name: medialibrary_file_access_Mkdir_0002
 * @tc.desc: Test function of Mkdir interface for ERROR which parentUri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Mkdir_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Mkdir_0002";
    try {
        Uri newDirUriTest("");
        Uri parentUri("storage/media/100/local/files/Download");
        int result = g_fah->Mkdir(parentUri, "medialibrary_file_access_Mkdir_0002", newDirUriTest);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Mkdir_0002 result:" << result;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Mkdir_0002 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Mkdir_0002";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Mkdir_0003
 * @tc.name: medialibrary_file_access_Mkdir_0003
 * @tc.desc: Test function of Mkdir interface for ERROR which parentUri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Mkdir_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Mkdir_0002";
    try {
        Uri newDirUriTest("");
        Uri parentUri("~!@#$%^&*()_");
        int result = g_fah->Mkdir(parentUri, "medialibrary_file_access_Mkdir_0003", newDirUriTest);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Mkdir_0003 result:" << result;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Mkdir_0003 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Mkdir_0003";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Mkdir_0004
 * @tc.name: medialibrary_file_access_Mkdir_0004
 * @tc.desc: Test function of Mkdir interface for ERROR which displayName is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Mkdir_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Mkdir_0004";
    try {
        Uri newDirUriTest("");
        string displayName = "";
        int result = g_fah->Mkdir(g_newDirUri, displayName, newDirUriTest);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Mkdir_0004 result:" << result;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Mkdir_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Mkdir_0004";
}

static void MkdirTdd(shared_ptr<FileAccessHelper> fahs, Uri parent, std::string displayName, Uri newDir)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_MkdirTdd";
    int ret = fahs->Mkdir(parent, displayName, newDir);
    if (ret != OHOS::FileAccessFwk::ERR_OK) {
        GTEST_LOG_(ERROR) << "MkdirTdd get result error, code:" << ret;
        return;
    }
    EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
    EXPECT_NE(newDir.ToString(), "");
    g_num++;
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_MkdirTdd";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Mkdir_0005
 * @tc.name: medialibrary_file_access_Mkdir_0005
 * @tc.desc: Test function of Mkdir interface for SUCCESS which Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Mkdir_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Mkdir_0005";
    try {
        Uri newFileUri1("");
        Uri newFileUri2("");
        Uri newFileUri3("");
        std::string displayName1 = "test1";
        std::string displayName2 = "test2";
        std::string displayName3 = "test3";
        int result = g_fah->Mkdir(g_newDirUri, displayName1, newFileUri1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Mkdir(newFileUri1, displayName2, newFileUri2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        g_num = 0;
        for (int j = 0; j < INIT_THREADS_NUMBER; j++) {
            std::thread execthread(MkdirTdd, g_fah, newFileUri2, displayName3, newFileUri3);
            execthread.join();
        }
        EXPECT_GE(g_num, ACTUAL_SUCCESS_THREADS_NUMBER);
        result = g_fah->Delete(newFileUri1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Mkdir_0005 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Mkdir_0005";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Delete_0000
 * @tc.name: medialibrary_file_access_Delete_0000
 * @tc.desc: Test function of Delete interface for SUCCESS which delete file.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Delete_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Delete_0000";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri newFileUri("");
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_Delete_0000.txt", newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(newFileUri);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Delete_0000 result:" << result;
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Delete_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Delete_0000";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Delete_0001
 * @tc.name: medialibrary_file_access_Delete_0001
 * @tc.desc: Test function of Delete interface for SUCCESS which delete folder.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Delete_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Delete_0001";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Delete_0001 result:" << result;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Delete_0001 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Delete_0001";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Delete_0002
 * @tc.name: medialibrary_file_access_Delete_0002
 * @tc.desc: Test function of Delete interface for ERROR which selectFileUri is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Delete_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Delete_0002";
    try {
        Uri selectFileUri("");
        int result = g_fah->Delete(selectFileUri);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Delete_0002 result:" << result;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Delete_0002 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Delete_0002";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Delete_0003
 * @tc.name: medialibrary_file_access_Delete_0003
 * @tc.desc: Test function of Delete interface for ERROR which selectFileUri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Delete_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Delete_0003";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri selectFileUri("storage/media/100/local/files/Download/test");
        result = g_fah->Delete(selectFileUri);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Delete_0003 result:" << result;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Delete_0003 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Delete_0003";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Delete_0004
 * @tc.name: medialibrary_file_access_Delete_0004
 * @tc.desc: Test function of Delete interface for ERROR which selectFileUri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Delete_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Delete_0004";
    try {
        Uri selectFileUri("!@#$%^&*()");
        int result = g_fah->Delete(selectFileUri);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Delete_0004 result:" << result;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Delete_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Delete_0004";
}

static void DeleteTdd(shared_ptr<FileAccessHelper> fahs, Uri selectFile)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_DeleteTdd";
    int ret = fahs->Delete(selectFile);
    if (ret < OHOS::FileAccessFwk::ERR_OK) {
        GTEST_LOG_(ERROR) << "DeleteTdd get result error, code:" << ret;
        return;
    }
    EXPECT_GE(ret, OHOS::FileAccessFwk::ERR_OK);
    g_num++;
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_DeleteTdd";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Delete_0005
 * @tc.name: medialibrary_file_access_Delete_0005
 * @tc.desc: Test function of Delete interface for SUCCESS which Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Delete_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Delete_0005";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri newFileUri("");
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_Delete_0005.txt", newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        std::string displayName = "test1.txt";
        Uri testUri2("");
        result = g_fah->CreateFile(newDirUriTest, displayName, testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        g_num = 0;
        for (int j = 0; j < INIT_THREADS_NUMBER; j++) {
            std::thread execthread(DeleteTdd, g_fah, testUri);
            execthread.join();
        }
        EXPECT_GE(g_num, ACTUAL_SUCCESS_THREADS_NUMBER);
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Delete_0005 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Delete_0005";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Move_0000
 * @tc.name: medialibrary_file_access_Move_0000
 * @tc.desc: Test function of Move interface for SUCCESS which move file.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Move_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Move_0000";
    try {
        Uri newDirUriTest1("");
        Uri newDirUriTest2("");
        int result = g_fah->Mkdir(g_newDirUri, "test1", newDirUriTest1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Mkdir(g_newDirUri, "test2", newDirUriTest2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest1, "test.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri2("");
        result = g_fah->Move(testUri, newDirUriTest2, testUri2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0000 result:" << result;
        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Move_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Move_0000";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Move_0001
 * @tc.name: medialibrary_file_access_Move_0001
 * @tc.desc: Test function of Move interface for SUCCESS which move folder.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Move_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Move_0001";
    try {
        Uri newDirUriTest1("");
        Uri newDirUriTest2("");
        int result = g_fah->Mkdir(g_newDirUri, "test1", newDirUriTest1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Mkdir(g_newDirUri, "test2", newDirUriTest2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest1, "test.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri2("");
        result = g_fah->Move(newDirUriTest1, newDirUriTest2, testUri2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0001 result:" << result;
        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Move_0001 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Move_0001";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Move_0002
 * @tc.name: medialibrary_file_access_Move_0002
 * @tc.desc: Test function of Move interface for ERROR which sourceFileUri is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Move_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Move_0002";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        Uri sourceFileUri("");
        result = g_fah->Move(sourceFileUri, newDirUriTest, testUri);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0002 result:" << result;
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Move_0002 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Move_0002";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Move_0003
 * @tc.name: medialibrary_file_access_Move_0003
 * @tc.desc: Test function of Move interface for ERROR which sourceFileUri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Move_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Move_0003";
    try {
        Uri newDirUriTest1("");
        Uri newDirUriTest2("");
        int result = g_fah->Mkdir(g_newDirUri, "test1", newDirUriTest1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Mkdir(g_newDirUri, "test2", newDirUriTest2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest1, "test.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri2("");
        Uri sourceFileUri("storage/media/100/local/files/Download/test1/test.txt");
        result = g_fah->Move(sourceFileUri, newDirUriTest2, testUri2);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0003 result:" << result;
        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Move_0003 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Move_0003";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Move_0004
 * @tc.name: medialibrary_file_access_Move_0004
 * @tc.desc: Test function of Move interface for ERROR which sourceFileUri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Move_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Move_0004";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        Uri sourceFileUri("~!@#$%^&*()_");
        result = g_fah->Move(sourceFileUri, newDirUriTest, testUri);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0004 result:" << result;
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Move_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Move_0004";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Move_0005
 * @tc.name: medialibrary_file_access_Move_0005
 * @tc.desc: Test function of Move interface for ERROR which targetParentUri is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Move_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Move_0005";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test1", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest, "test.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri2("");
        Uri targetParentUri("");
        result = g_fah->Move(testUri, targetParentUri, testUri2);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0005 result:" << result;
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Move_0005 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Move_0005";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Move_0006
 * @tc.name: medialibrary_file_access_Move_0006
 * @tc.desc: Test function of Move interface for ERROR which targetParentUri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Move_0006, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Move_0006";
    try {
        Uri newDirUriTest1("");
        Uri newDirUriTest2("");
        int result = g_fah->Mkdir(g_newDirUri, "test1", newDirUriTest1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Mkdir(g_newDirUri, "test2", newDirUriTest2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest1, "test.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri2("");
        Uri targetParentUri("storage/media/100/local/files/Download/test2");
        result = g_fah->Move(testUri, targetParentUri, testUri2);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0006 result:" << result;
        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Move_0006 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Move_0006";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Move_0007
 * @tc.name: medialibrary_file_access_Move_0007
 * @tc.desc: Test function of Move interface for ERROR which targetParentUri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Move_0007, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Move_0007";
    try {
        Uri newDirUriTest1("");
        Uri newDirUriTest2("");
        int result = g_fah->Mkdir(g_newDirUri, "test1", newDirUriTest1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Mkdir(g_newDirUri, "test2", newDirUriTest2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest1, "test.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri2("");
        Uri targetParentUri("~!@#$^%&*()_");
        result = g_fah->Move(testUri, targetParentUri, testUri2);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0007 result:" << result;
        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Move_0007 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Move_0007";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Move_0008
 * @tc.name: medialibrary_file_access_Move_0008
 * @tc.desc: Test function of Move interface for SUCCESS which move empty folder.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Move_0008, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Move_0008";
    try {
        Uri newDirUriTest1("");
        Uri newDirUriTest2("");
        int result = g_fah->Mkdir(g_newDirUri, "test1", newDirUriTest1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Mkdir(g_newDirUri, "test2", newDirUriTest2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri2("");
        result = g_fah->Move(newDirUriTest1, newDirUriTest2, testUri2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0008 result:" << result;
        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Move_0008 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Move_0008";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Move_0009
 * @tc.name: medialibrary_file_access_Move_0009
 * @tc.desc: Test function of Move interface for SUCCESS which move more file in folder.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Move_0009, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Move_0009";
    try {
        Uri newDirUriTest1("");
        Uri newDirUriTest2("");
        int result = g_fah->Mkdir(g_newDirUri, "test1", newDirUriTest1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Mkdir(g_newDirUri, "test2", newDirUriTest2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        size_t fileNumbers = 2000;
        for (size_t i = 0; i < fileNumbers; i++) {
            string fileName = "test" + ToString(i) + ".txt";
            result = g_fah->CreateFile(newDirUriTest1, fileName, testUri);
            EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        }
        Uri testUri2("");
        result = g_fah->Move(newDirUriTest1, newDirUriTest2, testUri2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0009 result:" << result;
        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Move_0009 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Move_0009";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Move_0010
 * @tc.name: medialibrary_file_access_Move_0010
 * @tc.desc: Test function of Move interface for SUCCESS which move Multilevel directory folder.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Move_0010, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Move_0010";
    try {
        Uri newDirUriTest1("");
        Uri newDirUriTest2("");
        int result = g_fah->Mkdir(g_newDirUri, "test1", newDirUriTest1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Mkdir(g_newDirUri, "test2", newDirUriTest2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        result = g_fah->Mkdir(newDirUriTest1, "test", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        size_t directoryNumbers = 50;
        for (size_t i = 0; i < directoryNumbers; i++) {
            result = g_fah->Mkdir(testUri, "test", testUri);
            EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        }
        Uri testUri2("");
        result = g_fah->Move(newDirUriTest1, newDirUriTest2, testUri2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0010 result:" << result;
        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Move_0010 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Move_0010";
}

static void MoveTdd(shared_ptr<FileAccessHelper> fahs, Uri sourceFile, Uri targetParent, Uri newFile)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_MoveTdd";
    int ret = fahs->Move(sourceFile, targetParent, newFile);
    if (ret != OHOS::FileAccessFwk::ERR_OK) {
        GTEST_LOG_(ERROR) << "MoveTdd get result error, code:" << ret;
        return;
    }
    EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
    EXPECT_NE(newFile.ToString(), "");
    g_num++;
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_MoveTdd";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Move_0011
 * @tc.name: medialibrary_file_access_Move_0011
 * @tc.desc: Test function of Move interface for SUCCESS which Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Move_0011, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Move_0011";
    try {
        Uri newDirUriTest1("");
        Uri newDirUriTest2("");
        int result = g_fah->Mkdir(g_newDirUri, "test1", newDirUriTest1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Mkdir(g_newDirUri, "test2", newDirUriTest2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri{""};
        Uri testUri2("");
        std::string displayName = "test1.txt";
        result = g_fah->CreateFile(newDirUriTest1, displayName, testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        for (int j = 0; j < INIT_THREADS_NUMBER; j++) {
            std::thread execthread(MoveTdd, g_fah, testUri, newDirUriTest2, testUri2);
            execthread.join();
        }
        EXPECT_GE(g_num, ACTUAL_SUCCESS_THREADS_NUMBER);
        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Move_0011 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Move_0011";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Copy_0000
 * @tc.name: medialibrary_file_access_Copy_0000
 * @tc.desc: Test function of Copy interface, copy a file and argument of force is false
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6UI3H
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Copy_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Copy_0000";
    try {
        Uri srcUri("");
        int result = g_fah->Mkdir(g_newDirUri, "Copy_0000_src", srcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri aFileUri("");
        result = g_fah->CreateFile(srcUri, "a.txt", aFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        int fd;
        result = g_fah->OpenFile(aFileUri, WRITE_READ, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        std::string aFileBuff = "Copy test content for a.txt";
        ssize_t aFileSize = write(fd, aFileBuff.c_str(), aFileBuff.size());
        close(fd);
        EXPECT_EQ(aFileSize, aFileBuff.size());

        Uri destUri("");
        result = g_fah->Mkdir(g_newDirUri, "Copy_0000_dest", destUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        vector<CopyResult> copyResult;
        result = g_fah->Copy(aFileUri, destUri, copyResult, false);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Delete(srcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(destUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Copy_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Copy_0000";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Copy_0001
 * @tc.name: medialibrary_file_access_Copy_0001
 * @tc.desc: Test function of Copy interface, copy a directory and argument of force is false
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6UI3H
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Copy_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Copy_0001";
    try {
        Uri srcUri("");
        int result = g_fah->Mkdir(g_newDirUri, "Copy_0001_src", srcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri aFileUri("");
        result = g_fah->CreateFile(srcUri, "a.txt", aFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        int fd;
        result = g_fah->OpenFile(aFileUri, WRITE_READ, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        std::string aFileBuff = "Copy test content for a.txt";
        ssize_t aFileSize = write(fd, aFileBuff.c_str(), aFileBuff.size());
        close(fd);
        EXPECT_EQ(aFileSize, aFileBuff.size());

        Uri bFileUri("");
        result = g_fah->CreateFile(srcUri, "b.txt", bFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->OpenFile(bFileUri, WRITE_READ, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        std::string bFileBuff = "Copy test content for b.txt";
        ssize_t bFileSize = write(fd, bFileBuff.c_str(), bFileBuff.size());
        close(fd);
        EXPECT_EQ(bFileSize, bFileBuff.size());

        Uri destUri("");
        result = g_fah->Mkdir(g_newDirUri, "Copy_0001_dest", destUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        vector<CopyResult> copyResult;
        result = g_fah->Copy(srcUri, destUri, copyResult, false);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Delete(srcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(destUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Copy_0001 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Copy_0001";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Copy_0002
 * @tc.name: medialibrary_file_access_Copy_0002
 * @tc.desc: Test function of Copy interface, copy a file with the same name and argument of force is false
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6UI3H
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Copy_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Copy_0002";
    try {
        Uri srcUri("");
        int result = g_fah->Mkdir(g_newDirUri, "Copy_0002_src", srcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri srcFileUri("");
        result = g_fah->CreateFile(srcUri, "a.txt", srcFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        int fd;
        result = g_fah->OpenFile(srcFileUri, WRITE_READ, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        std::string aFileBuff = "Copy test content for a.txt";
        ssize_t aFileSize = write(fd, aFileBuff.c_str(), aFileBuff.size());
        close(fd);
        EXPECT_EQ(aFileSize, aFileBuff.size());

        Uri destUri("");
        result = g_fah->Mkdir(g_newDirUri, "Copy_0002_dest_false", destUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri destFileUri("");
        result = g_fah->CreateFile(destUri, "a.txt", destFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        vector<CopyResult> copyResult;
        result = g_fah->Copy(srcFileUri, destUri, copyResult, false);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_GT(copyResult.size(), 0);

        result = g_fah->Delete(srcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(destUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Copy_0002 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Copy_0002";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Copy_0003
 * @tc.name: medialibrary_file_access_Copy_0003
 * @tc.desc: Test function of Copy interface, copy a file with the same name and argument of force is true
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6UI3H
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Copy_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Copy_0003";
    try {
        Uri srcUri("");
        int result = g_fah->Mkdir(g_newDirUri, "Copy_0003_src", srcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri srcFileUri("");
        result = g_fah->CreateFile(srcUri, "a.txt", srcFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        int fd;
        result = g_fah->OpenFile(srcFileUri, WRITE_READ, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        std::string aFileBuff = "Copy test content for a.txt";
        ssize_t aFileSize = write(fd, aFileBuff.c_str(), aFileBuff.size());
        close(fd);
        EXPECT_EQ(aFileSize, aFileBuff.size());

        Uri destUri("");
        result = g_fah->Mkdir(g_newDirUri, "Copy_0003_dest_true", destUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri destFileUri("");
        result = g_fah->CreateFile(destUri, "a.txt", destFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        vector<CopyResult> copyResult;
        result = g_fah->Copy(srcFileUri, destUri, copyResult, true);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Delete(srcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(destUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Copy_0003 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Copy_0003";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Copy_0004
 * @tc.name: medialibrary_file_access_Copy_0004
 * @tc.desc: Test function of Copy interface, copy a directory with the same name and argument of force is false
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6UI3H
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Copy_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Copy_0004";
    try {
        Uri srcUri("");
        int result = g_fah->Mkdir(g_newDirUri, "Copy_0004_src", srcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri aFileUri("");
        result = g_fah->CreateFile(srcUri, "a.txt", aFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        int fd;
        result = g_fah->OpenFile(aFileUri, WRITE_READ, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        std::string aFileBuff = "Copy test content for a.txt";
        ssize_t aFileSize = write(fd, aFileBuff.c_str(), aFileBuff.size());
        close(fd);
        EXPECT_EQ(aFileSize, aFileBuff.size());

        Uri bFileUri("");
        result = g_fah->CreateFile(srcUri, "b.txt", bFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->OpenFile(bFileUri, WRITE_READ, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        std::string bFileBuff = "Copy test content for b.txt";
        ssize_t bFileSize = write(fd, bFileBuff.c_str(), bFileBuff.size());
        close(fd);
        EXPECT_EQ(bFileSize, bFileBuff.size());

        Uri destUri("");
        result = g_fah->Mkdir(g_newDirUri, "Copy_0004_dest_false", destUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri destSrcUri("");
        result = g_fah->Mkdir(destUri, "Copy_0004_src", destSrcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri destSrcAUri("");
        result = g_fah->CreateFile(destSrcUri, "a.txt", destSrcAUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        vector<CopyResult> copyResult;
        result = g_fah->Copy(srcUri, destUri, copyResult, false);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_GT(copyResult.size(), 0);

        result = g_fah->Delete(srcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(destUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Copy_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Copy_0004";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Copy_0005
 * @tc.name: medialibrary_file_access_Copy_0005
 * @tc.desc: Test function of Copy interface, copy a directory with the same name and argument of force is true
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6UI3H
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Copy_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Copy_0005";
    try {
        Uri srcUri("");
        int result = g_fah->Mkdir(g_newDirUri, "Copy_0005_src", srcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri aFileUri("");
        result = g_fah->CreateFile(srcUri, "a.txt", aFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        int fd;
        result = g_fah->OpenFile(aFileUri, WRITE_READ, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        std::string aFileBuff = "Copy test content for a.txt";
        ssize_t aFileSize = write(fd, aFileBuff.c_str(), aFileBuff.size());
        close(fd);
        EXPECT_EQ(aFileSize, aFileBuff.size());

        Uri bFileUri("");
        result = g_fah->CreateFile(srcUri, "b.txt", bFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->OpenFile(bFileUri, WRITE_READ, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        std::string bFileBuff = "Copy test content for b.txt";
        ssize_t bFileSize = write(fd, bFileBuff.c_str(), bFileBuff.size());
        close(fd);
        EXPECT_EQ(bFileSize, bFileBuff.size());

        Uri destUri("");
        result = g_fah->Mkdir(g_newDirUri, "Copy_0005_dest_true", destUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri destSrcUri("");
        result = g_fah->Mkdir(destUri, "Copy_0005_src", destSrcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri destSrcAUri("");
        result = g_fah->CreateFile(destSrcUri, "a.txt", destSrcAUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        vector<CopyResult> copyResult;
        result = g_fah->Copy(srcUri, destUri, copyResult, true);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Delete(srcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(destUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Copy_0005 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Copy_0005";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Copy_0006
 * @tc.name: medialibrary_file_access_Copy_0006
 * @tc.desc: Test function of Copy interface, copy a file with the same name
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6UI3H
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Copy_0006, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Copy_0006";
    try {
        Uri srcUri("");
        int result = g_fah->Mkdir(g_newDirUri, "Copy_0006_src", srcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri srcFileUri("");
        result = g_fah->CreateFile(srcUri, "a.txt", srcFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        int fd;
        result = g_fah->OpenFile(srcFileUri, WRITE_READ, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        std::string aFileBuff = "Copy test content for a.txt";
        ssize_t aFileSize = write(fd, aFileBuff.c_str(), aFileBuff.size());
        close(fd);
        EXPECT_EQ(aFileSize, aFileBuff.size());

        Uri destUri("");
        result = g_fah->Mkdir(g_newDirUri, "Copy_0006_dest_false", destUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri destFileUri("");
        result = g_fah->CreateFile(destUri, "a.txt", destFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        vector<CopyResult> copyResult;
        result = g_fah->Copy(srcFileUri, destUri, copyResult);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_GT(copyResult.size(), 0);

        result = g_fah->Delete(srcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(destUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Copy_0006 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Copy_0006";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Copy_0007
 * @tc.name: medialibrary_file_access_Copy_0007
 * @tc.desc: Test function of Copy interface, copy a directory with the same name
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6UI3H
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Copy_0007, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Copy_0007";
    try {
        Uri srcUri("");
        int result = g_fah->Mkdir(g_newDirUri, "Copy_0007_src", srcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri aFileUri("");
        result = g_fah->CreateFile(srcUri, "a.txt", aFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        int fd;
        result = g_fah->OpenFile(aFileUri, WRITE_READ, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        std::string aFileBuff = "Copy test content for a.txt";
        ssize_t aFileSize = write(fd, aFileBuff.c_str(), aFileBuff.size());
        close(fd);
        EXPECT_EQ(aFileSize, aFileBuff.size());

        Uri bFileUri("");
        result = g_fah->CreateFile(srcUri, "b.txt", bFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->OpenFile(bFileUri, WRITE_READ, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        std::string bFileBuff = "Copy test content for b.txt";
        ssize_t bFileSize = write(fd, bFileBuff.c_str(), bFileBuff.size());
        close(fd);
        EXPECT_EQ(bFileSize, bFileBuff.size());

        Uri destUri("");
        result = g_fah->Mkdir(g_newDirUri, "Copy_0007_dest_false", destUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri destSrcUri("");
        result = g_fah->Mkdir(destUri, "Copy_0007_src", destSrcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri destSrcAUri("");
        result = g_fah->CreateFile(destSrcUri, "a.txt", destSrcAUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        vector<CopyResult> copyResult;
        result = g_fah->Copy(srcUri, destUri, copyResult);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_GT(copyResult.size(), 0);

        result = g_fah->Delete(srcUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(destUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Copy_0007 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Copy_0007";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Rename_0000
 * @tc.name: medialibrary_file_access_Rename_0000
 * @tc.desc: Test function of Rename interface for SUCCESS which rename file.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Rename_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Rename_0000";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest, "test.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri renameUri("");
        result = g_fah->Rename(testUri, "test2.txt", renameUri);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Rename_0000 result:" << result;
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Rename_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Rename_0000";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Rename_0001
 * @tc.name: medialibrary_file_access_Rename_0001
 * @tc.desc: Test function of Rename interface for SUCCESS which rename folder.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Rename_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Rename_0001";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri renameUri("");
        result = g_fah->Rename(newDirUriTest, "testRename", renameUri);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Rename_0001 result:" << result;
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Rename_0001 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Rename_0001";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Rename_0002
 * @tc.name: medialibrary_file_access_Rename_0002
 * @tc.desc: Test function of Rename interface for ERROR which sourceFileUri is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Rename_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Rename_0002";
    try {
        Uri renameUri("");
        Uri sourceFileUri("");
        int result = g_fah->Rename(sourceFileUri, "testRename.txt", renameUri);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Rename_0002 result:" << result;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Rename_0002 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Rename_0002";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Rename_0003
 * @tc.name: medialibrary_file_access_Rename_0003
 * @tc.desc: Test function of Rename interface for ERROR which sourceFileUri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Rename_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Rename_0003";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest, "test.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri renameUri("");
        Uri sourceFileUri("storage/media/100/local/files/Download/test/test.txt");
        result = g_fah->Rename(sourceFileUri, "testRename.txt", renameUri);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Rename_0003 result:" << result;
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Rename_0003 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Rename_0003";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Rename_0004
 * @tc.name: medialibrary_file_access_Rename_0004
 * @tc.desc: Test function of Rename interface for ERROR which sourceFileUri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Rename_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Rename_0004";
    try {
        Uri renameUri("");
        Uri sourceFileUri("~!@#$%^&*()_");
        int result = g_fah->Rename(sourceFileUri, "testRename.txt", renameUri);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Rename_0004 result:" << result;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Rename_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Rename_0004";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Rename_0005
 * @tc.name: medialibrary_file_access_Rename_0005
 * @tc.desc: Test function of Rename interface for ERROR which displayName is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Rename_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Rename_0005";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest, "test.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri renameUri("");
        result = g_fah->Rename(testUri, "", renameUri);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Rename_0005 result:" << result;
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Rename_0005 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Rename_0005";
}

static void RenameTdd(shared_ptr<FileAccessHelper> fahs, Uri sourceFile, std::string displayName, Uri newFile)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_RenameTdd";
    int ret = fahs->Rename(sourceFile, displayName, newFile);
    if (ret != OHOS::FileAccessFwk::ERR_OK) {
        GTEST_LOG_(ERROR) << "RenameTdd get result error, code:" << ret;
        return;
    }
    EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
    EXPECT_NE(newFile.ToString(), "");
    g_num++;
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_RenameTdd";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Rename_0006
 * @tc.name: medialibrary_file_access_Rename_0006
 * @tc.desc: Test function of Rename interface for SUCCESS which Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Rename_0006, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Rename_0006";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri{""};
        std::string displayName1 = "test1.txt";
        std::string displayName2 = "test2.txt";
        Uri renameUri("");
        result = g_fah->CreateFile(newDirUriTest, displayName1, testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        for (int j = 0; j < INIT_THREADS_NUMBER; j++) {
            std::thread execthread(RenameTdd, g_fah, testUri, displayName2, renameUri);
            execthread.join();
        }
        EXPECT_GE(g_num, ACTUAL_SUCCESS_THREADS_NUMBER);
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Rename_0006 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Rename_0006";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_ListFile_0000
 * @tc.name: medialibrary_file_access_ListFile_0000
 * @tc.desc: Test function of ListFile interface for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_ListFile_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_ListFile_0000";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_ListFile_0000.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        FileInfo fileInfo;
        fileInfo.uri = newDirUriTest.ToString();
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        FileFilter filter;
        result = g_fah->ListFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_GT(fileInfoVec.size(), OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "ListFile_0000 result:" << fileInfoVec.size() << endl;
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_ListFile_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_ListFile_0000";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_ListFile_0001
 * @tc.name: medialibrary_file_access_ListFile_0001
 * @tc.desc: Test function of ListFile interface for ERROR which Uri is nullptr.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_ListFile_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_ListFile_0001";
    try {
        Uri sourceFileUri("");
        FileInfo fileInfo;
        fileInfo.uri = sourceFileUri.ToString();
        int64_t offset = 0;
        int64_t maxCount = 1000;
        vector<FileAccessFwk::FileInfo> fileInfoVec;
        FileFilter filter({}, {}, {}, -1, -1, false, false);
        int result = g_fah->ListFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_EQ(fileInfoVec.size(), OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "ListFile_0001 result:" << fileInfoVec.size() << endl;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_ListFile_0001 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_ListFile_0001";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_ListFile_0002
 * @tc.name: medialibrary_file_access_ListFile_0002
 * @tc.desc: Test function of ListFile interface for ERROR which sourceFileUri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_ListFile_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_ListFile_0002";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest, "test.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri sourceFileUri("storage/media/100/local/files/Download/test/test.txt");
        FileInfo fileInfo;
        fileInfo.uri = sourceFileUri.ToString();
        int64_t offset = 0;
        int64_t maxCount = 1000;
        vector<FileAccessFwk::FileInfo> fileInfoVec;
        FileFilter filter({}, {}, {}, -1, -1, false, false);
        result = g_fah->ListFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_EQ(fileInfoVec.size(), OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "ListFile_0002 result:" << fileInfoVec.size() << endl;
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_ListFile_0002 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_ListFile_0002";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_ListFile_0003
 * @tc.name: medialibrary_file_access_ListFile_0003
 * @tc.desc: Test function of ListFile interface for ERROR which sourceFileUri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_ListFile_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_ListFile_0003";
    try {
        Uri sourceFileUri("~!@#$%^&*()_");
        FileInfo fileInfo;
        fileInfo.uri = sourceFileUri.ToString();
        int64_t offset = 0;
        int64_t maxCount = 1000;
        vector<FileAccessFwk::FileInfo> fileInfoVec;
        FileFilter filter;
        int result = g_fah->ListFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_EQ(fileInfoVec.size(), OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "ListFile_0003 result:" << fileInfoVec.size() << endl;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_ListFile_0003 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_ListFile_0003";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_ListFile_0004
 * @tc.name: medialibrary_file_access_ListFile_0004
 * @tc.desc: Test function of ListFile interface for ERROR which add filter.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HB855
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_ListFile_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_ListFile_0004";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri1("");
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_ListFile_0004.txt", testUri1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri2("");
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_ListFile_0004.docx", testUri2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        FileInfo fileInfo;
        fileInfo.uri = newDirUriTest.ToString();
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        FileFilter filter({".txt"}, {}, {}, -1, 1, false, true);
        result = g_fah->ListFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_EQ(fileInfoVec.size(), 1);
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_ListFile_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_ListFile_0004";
}

static void ListFileTdd(shared_ptr<FileAccessHelper> fahs, FileInfo fileInfo, int offset, int maxCount,
    FileFilter filter, std::vector<FileInfo> fileInfoVec)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_ListFileTdd";
    int ret = fahs->ListFile(fileInfo, offset, maxCount, filter, fileInfoVec);
    if (ret != OHOS::FileAccessFwk::ERR_OK) {
        GTEST_LOG_(ERROR) << "ListFileTdd get result error, code:" << ret;
        return;
    }
    EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
    EXPECT_EQ(fileInfoVec.size(), 1);
    g_num++;
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_ListFileTdd";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_ListFile_0005
 * @tc.name: medialibrary_file_access_ListFile_0005
 * @tc.desc: Test function of ListFile interface for ERROR which Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_ListFile_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_ListFile_0005";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri1("");
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_ListFile_0005.txt", testUri1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri2("");
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_ListFile_0005.docx", testUri2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        FileInfo fileInfo;
        fileInfo.uri = newDirUriTest.ToString();
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        g_num = 0;
        FileFilter filter({".txt"}, {}, {}, -1, -1, false, true);
        for (int j = 0; j < INIT_THREADS_NUMBER; j++) {
            std::thread execthread(ListFileTdd, g_fah, fileInfo, offset, maxCount, filter, fileInfoVec);
            execthread.join();
        }
        EXPECT_GE(g_num, ACTUAL_SUCCESS_THREADS_NUMBER);
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_ListFile_0005 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_ListFile_0005";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_ScanFile_0000
 * @tc.name: medialibrary_file_access_ScanFile_0000
 * @tc.desc: Test function of ScanFile interface for SUCCESS which scan root directory with filter.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HB866
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_ScanFile_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_ScanFile_0000";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        FileInfo fileInfo;
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_ScanFile_0000.q1w2e3r4", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_ScanFile_0000.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        fileInfo.uri = "file://media/root";
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        FileFilter filter({".q1w2e3r4"}, {}, {}, -1, -1, false, true);
        result = g_fah->ScanFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_GE(fileInfoVec.size(), 1);
        GTEST_LOG_(INFO) << "ScanFile_0000 result:" << fileInfoVec.size() << endl;
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_ScanFile_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_ScanFile_0000";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_ScanFile_0001
 * @tc.name: medialibrary_file_access_ScanFile_0001
 * @tc.desc: Test function of ScanFile interface for SUCCESS which scan root directory with no filter.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HB866
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_ScanFile_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_ScanFile_0001";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        FileInfo fileInfo;
        fileInfo.uri = "file://media/root";
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_ScanFile_0001.q1w2e3r4", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_ScanFile_0001.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        FileFilter filter({}, {}, {}, -1, -1, false, false);
        result = g_fah->ScanFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_GE(fileInfoVec.size(), 2);
        GTEST_LOG_(INFO) << "ScanFile_0000 result:" << fileInfoVec.size() << endl;
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_ScanFile_0001 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_ScanFile_0001";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_ScanFile_0002
 * @tc.name: medialibrary_file_access_ScanFile_0002
 * @tc.desc: Test function of ScanFile interface for SUCCESS which self created directory with filter.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HB866
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_ScanFile_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_ScanFile_0002";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_ScanFile_0002.q1w2e3r4", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_ScanFile_0000.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_ScanFile_0000.docx", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        FileInfo fileInfo;
        fileInfo.uri = newDirUriTest.ToString();
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        FileFilter filter({".q1w2e3r4"}, {}, {}, -1, -1, false, true);
        result = g_fah->ScanFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_EQ(fileInfoVec.size(), 1);
        FileFilter filter1({".q1w2e3r4", ".txt"}, {}, {}, -1, -1, false, true);
        result = g_fah->ScanFile(fileInfo, offset, maxCount, filter1, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_EQ(fileInfoVec.size(), 2);
        GTEST_LOG_(INFO) << "ScanFile_0002 result:" << fileInfoVec.size() << endl;
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_ScanFile_0002 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_ScanFile_0002";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_ScanFile_0003
 * @tc.name: medialibrary_file_access_ScanFile_0003
 * @tc.desc: Test function of ScanFile interface for SUCCESS which self created directory with filter.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HB866
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_ScanFile_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_ScanFile_0003";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_ScanFile_0003.q1w2e3r4", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        FileInfo fileInfo;
        fileInfo.uri = newDirUriTest.ToString();
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        FileFilter filter({".q1w2e3r4"}, {}, {}, -1, -1, false, true);
        result = g_fah->ScanFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_EQ(fileInfoVec.size(), 1);
        GTEST_LOG_(INFO) << "ScanFile_0003 result:" << fileInfoVec.size() << endl;
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_ScanFile_0003 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_ScanFile_0003";
}

static void ScanFileTdd(FileInfo fileInfo, int offset, int maxCount, FileFilter filter,
    std::vector<FileInfo> fileInfoVec)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_ScanFileTdd";
    int ret = g_fah->ScanFile(fileInfo, offset, maxCount, filter, fileInfoVec);
    if (ret != OHOS::FileAccessFwk::ERR_OK) {
        GTEST_LOG_(ERROR) << "ScanFileTdd get result error, code:" << ret;
        return;
    }
    EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
    EXPECT_EQ(fileInfoVec.size(), 1);
    g_num++;
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_ScanFileTdd";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_ScanFile_0004
 * @tc.name: medialibrary_file_access_ScanFile_0004
 * @tc.desc: Test function of ScanFile interface for SUCCESS which scan root directory with Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HB866
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_ScanFile_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_ScanFile_0004";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        FileInfo fileInfo;
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_ScanFile_0000.q1w2e3r4", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_ScanFile_0000.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        fileInfo.uri = "file://media/root";
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        FileFilter filter({".q1w2e3r4"}, {}, {}, -1, -1, false, true);
        g_num = 0;
        for (int j = 0; j < INIT_THREADS_NUMBER; j++) {
            std::thread execthread(ScanFileTdd, fileInfo, offset, maxCount, filter, fileInfoVec);
            execthread.join();
        }
        EXPECT_GE(g_num, ACTUAL_SUCCESS_THREADS_NUMBER);
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_ScanFile_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_ScanFile_0004";
}

static bool ReplaceBundleNameFromPath(std::string &path, const std::string &newName)
{
    Uri uri(path);
    std::string scheme = uri.GetScheme();
    if (scheme == FILE_SCHEME_NAME) {
        std::string curName = uri.GetAuthority();
        if (curName.empty()) {
            return false;
        }
        path.replace(path.find(curName), curName.length(), newName);
        return true;
    }

    std::string tPath = Uri(path).GetPath();
    if (tPath.empty()) {
        GTEST_LOG_(INFO) << "Uri path error.";
        return false;
    }

    if (tPath.front() != '/') {
        GTEST_LOG_(INFO) << "Uri path format error.";
        return false;
    }

    auto index = tPath.substr(1).find_first_of("/");
    auto bundleName = tPath.substr(1, index);
    if (bundleName.empty()) {
        GTEST_LOG_(INFO) << "bundleName empty.";
        return false;
    }

    path.replace(path.find(bundleName), bundleName.length(), newName);
    return true;
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_ScanFile_0005
 * @tc.name: medialibrary_file_access_ScanFile_0005
 * @tc.desc: Test function of ScanFile interface for FAILED because of GetProxyByUri failed.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HB866
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_ScanFile_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_ScanFile_0005";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_ScanFile_0005.q1w2e3r4", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        std::string str = testUri.ToString();
        if (!ReplaceBundleNameFromPath(str, "ohos.com.NotExistBundleName")) {
            GTEST_LOG_(INFO) << "replace BundleName failed.";
            EXPECT_TRUE(false);
        }
        FileInfo fileInfo;
        fileInfo.uri = str;
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        FileFilter filter({".q1w2e3r4"}, {}, {}, -1, -1, false, true);
        result = g_fah->ScanFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::E_IPCS);
        EXPECT_EQ(fileInfoVec.size(), 0);
        GTEST_LOG_(INFO) << "ScanFile_0005 result:" << fileInfoVec.size() << endl;
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_ScanFile_0005 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_ScanFile_0005";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetRoots_0000
 * @tc.name: medialibrary_file_access_GetRoots_0000
 * @tc.desc: Test function of GetRoots interface for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetRoots_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_GetRoots_0000";
    try {
        vector<RootInfo> info;
        int result = g_fah->GetRoots(info);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_GT(info.size(), OHOS::FileAccessFwk::ERR_OK);
        if (info.size() > OHOS::FileAccessFwk::ERR_OK) {
            GTEST_LOG_(INFO) << info[0].uri;
            GTEST_LOG_(INFO) << info[0].displayName;
            GTEST_LOG_(INFO) << info[0].deviceType;
            GTEST_LOG_(INFO) << info[0].deviceFlags;
        }
        string uri = "file://media/root";
        string displayName = "LOCAL";
        EXPECT_EQ(info[0].uri, uri);
        EXPECT_EQ(info[0].displayName, displayName);
        EXPECT_EQ(info[0].deviceType, DEVICE_LOCAL_DISK);
        EXPECT_EQ(info[0].deviceFlags, DEVICE_FLAG_SUPPORTS_READ | DEVICE_FLAG_SUPPORTS_WRITE);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetRoots_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_GetRoots_0000";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetFileInfoFromUri_0000
 * @tc.name: medialibrary_file_access_GetFileInfoFromUri_0000
 * @tc.desc: Test function of GetFileInfoFromUri interface.
 * @tc.desc: convert the root directory uri to fileinfo and call listfile for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HRLBS
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetFileInfoFromUri_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileExtensionHelperTest-begin medialibrary_file_access_GetFileInfoFromUri_0000";
    try {
        vector<RootInfo> infos;
        int result = g_fah->GetRoots(infos);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        for (size_t i = 0; i < infos.size(); i++) {
            Uri parentUri(infos[i].uri);
            FileInfo fileinfo;
            result = g_fah->GetFileInfoFromUri(parentUri, fileinfo);
            EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

            int64_t offset = 0;
            int64_t maxCount = 1000;
            FileFilter filter;
            std::vector<FileInfo> fileInfoVecTemp;
            result = g_fah->ListFile(fileinfo, offset, maxCount, filter, fileInfoVecTemp);
            EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
            EXPECT_GE(fileInfoVecTemp.size(), OHOS::FileAccessFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetFileInfoFromUri_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileExtensionHelperTest-end medialibrary_file_access_GetFileInfoFromUri_0000";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetFileInfoFromUri_0001
 * @tc.name: medialibrary_file_access_GetFileInfoFromUri_0001
 * @tc.desc: Test function of GetFileInfoFromUri interface.
 * @tc.desc: convert the general directory uri to fileinfo and call listfile for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HRLBS
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetFileInfoFromUri_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileExtensionHelperTest-begin medialibrary_file_access_GetFileInfoFromUri_0001";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "testDir", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        FileInfo dirInfo;
        result = g_fah->GetFileInfoFromUri(newDirUriTest, dirInfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        int64_t offset = 0;
        int64_t maxCount = 1000;
        FileFilter filter;
        std::vector<FileInfo> fileInfoVec;
        result = g_fah->ListFile(dirInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_GE(fileInfoVec.size(), OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Delete(newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetFileInfoFromUri_0001 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileExtensionHelperTest-end medialibrary_file_access_GetFileInfoFromUri_0001";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetFileInfoFromUri_0002
 * @tc.name: medialibrary_file_access_GetFileInfoFromUri_0002
 * @tc.desc: Test function of GetFileInfoFromUri interface.
 * @tc.desc: convert the regular file uri to fileinfo and call listfile for ERROR.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HRLBS
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetFileInfoFromUri_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileExtensionHelperTest-begin medialibrary_file_access_GetFileInfoFromUri_0002";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "testDir", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri newFileUri("");
        result = g_fah->CreateFile(newDirUriTest, "medialibrary_file_access_GetFileInfoFromUri_0002.txt", newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        FileInfo fileinfo;
        result = g_fah->GetFileInfoFromUri(newFileUri, fileinfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        int64_t offset = 0;
        int64_t maxCount = 1000;
        FileFilter filter;
        std::vector<FileInfo> fileInfoVecTemp;
        result = g_fah->ListFile(fileinfo, offset, maxCount, filter, fileInfoVecTemp);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_EQ(fileInfoVecTemp.size(), OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Delete(newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetFileInfoFromUri_0002 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileExtensionHelperTest-end medialibrary_file_access_GetFileInfoFromUri_0002";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetFileInfoFromUri_0003
 * @tc.name: medialibrary_file_access_GetFileInfoFromUri_0003
 * @tc.desc: Test function of GetFileInfoFromUri interface.
 * @tc.desc: convert the root directory uri to fileinfo for CheckUri failed.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HRLBS
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetFileInfoFromUri_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileExtensionHelperTest-begin medialibrary_file_access_GetFileInfoFromUri_0003";
    try {
        vector<RootInfo> info;
        int result = g_fah->GetRoots(info);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        for (size_t i = 0; i < info.size(); i++) {
            Uri parentUri(std::string("\?\?\?\?/") + info[i].uri);
            FileInfo fileinfo;
            result = g_fah->GetFileInfoFromUri(parentUri, fileinfo);
            EXPECT_EQ(result, OHOS::FileAccessFwk::E_URIS);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetFileInfoFromUri_0003 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileExtensionHelperTest-end medialibrary_file_access_GetFileInfoFromUri_0003";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetFileInfoFromUri_0004
 * @tc.name: medialibrary_file_access_GetFileInfoFromUri_0004
 * @tc.desc: Test function of GetFileInfoFromUri interface.
 * @tc.desc: convert the root directory uri to fileinfo failed because of GetProxyByUri failed.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HRLBS
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetFileInfoFromUri_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileExtensionHelperTest-begin medialibrary_file_access_GetFileInfoFromUri_0004";
    try {
        vector<RootInfo> info;
        int result = g_fah->GetRoots(info);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        for (size_t i = 0; i < info.size(); i++) {
            std::string str = info[i].uri;
            if (!ReplaceBundleNameFromPath(str, "ohos.com.NotExistBundleName")) {
                GTEST_LOG_(ERROR) << "replace BundleName failed.";
                EXPECT_TRUE(false);
            }
            Uri parentUri(str);
            FileInfo fileinfo;
            result = g_fah->GetFileInfoFromUri(parentUri, fileinfo);
            EXPECT_EQ(result, OHOS::FileAccessFwk::E_IPCS);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetFileInfoFromUri_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileExtensionHelperTest-end medialibrary_file_access_GetFileInfoFromUri_0004";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetFileInfoFromUri_0005
 * @tc.name: medialibrary_file_access_GetFileInfoFromUri_0005
 * @tc.desc: Test function of GetFileInfoFromUri interface.
 * @tc.desc: convert the invalid uri to fileinfo failed.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HRLBS
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetFileInfoFromUri_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileExtensionHelperTest-begin medialibrary_file_access_GetFileInfoFromUri_0005";
    try {
        Uri uri("~!@#$%^&*()_");
        FileInfo fileInfo;
        int result = g_fah->GetFileInfoFromUri(uri, fileInfo);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);

        uri = Uri("/");
        result = g_fah->GetFileInfoFromUri(uri, fileInfo);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);

        uri = Uri("");
        result = g_fah->GetFileInfoFromUri(uri, fileInfo);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetFileInfoFromUri_0005 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileExtensionHelperTest-end medialibrary_file_access_GetFileInfoFromUri_0005";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetFileInfoFromRelativePath_0000
 * @tc.name: medialibrary_file_access_GetFileInfoFromRelativePath_0000
 * @tc.desc: Test function of GetFileInfoFromRelativePath interface.
 * @tc.desc: convert the general directory relativePath to fileinfo for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HRLBS
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetFileInfoFromRelativePath_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileExtensionHelperTest-begin medialibrary_file_access_GetFileInfoFromRelativePath_0000";
    try {
        FileInfo fileInfo;
        string relativePath = "";
        int result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        relativePath = "Audios/";
        result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        relativePath = "Camera/";
        result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        relativePath = "Documents/";
        result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        relativePath = "Download";
        result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        relativePath = "Pictures";
        result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        relativePath = "Videos";
        result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetFileInfoFromRelativePath_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_GetFileInfoFromRelativePath_0000";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetFileInfoFromRelativePath_0001
 * @tc.name: medialibrary_file_access_GetFileInfoFromRelativePath_0001
 * @tc.desc: Test function of GetFileInfoFromRelativePath interface.
 * @tc.desc: convert the general directory relativePath to fileinfo for failed.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HRLBS
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetFileInfoFromRelativePath_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileExtensionHelperTest-begin medialibrary_file_access_GetFileInfoFromRelativePath_0001";
    try {
        FileInfo fileInfo;
        string relativePath = "test/";
        int result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);

        relativePath = "/";
        result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);

        relativePath = "~!@#$%^&*()_";
        result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);

        relativePath = "/d";
        result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetFileInfoFromRelativePath_0001 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_GetFileInfoFromRelativePath_0001";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetFileInfoFromRelativePath_0002
 * @tc.name: medialibrary_file_access_GetFileInfoFromRelativePath_0002
 * @tc.desc: Test function of GetFileInfoFromRelativePath interface.
 * @tc.desc: convert the general directory relativePath to fileinfo and call listfile for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HRLBS
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetFileInfoFromRelativePath_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileExtensionHelperTest-begin medialibrary_file_access_GetFileInfoFromRelativePath_0002";
    try {
        FileInfo fileInfo;
        string relativePath = "Download/";
        int result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        ASSERT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri parentUri(fileInfo.uri);
        Uri newFile("");
        result = g_fah->CreateFile(parentUri, "GetFileInfoFromRelativePath_0002.jpg", newFile);
        ASSERT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        int64_t offset = 0;
        int64_t maxCount = 1000;
        FileFilter filter;
        std::vector<FileInfo> fileInfoVec;
        result = g_fah->ListFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_GT(fileInfoVec.size(), OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Delete(newFile);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetFileInfoFromRelativePath_0002 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_GetFileInfoFromRelativePath_0002";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetFileInfoFromRelativePath_0003
 * @tc.name: medialibrary_file_access_GetFileInfoFromRelativePath_0003
 * @tc.desc: Test function of GetFileInfoFromRelativePath interface.
 * @tc.desc: convert the relative file path to fileinfo and call listfile for failed.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HRLBS
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetFileInfoFromRelativePath_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileExtensionHelperTest-begin medialibrary_file_access_GetFileInfoFromRelativePath_0003";
    try {
        FileInfo fileInfo;
        string relativePath = "Download/";
        int result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        ASSERT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri parentUri(fileInfo.uri);
        Uri newFile("");
        result = g_fah->CreateFile(parentUri, "GetFileInfoFromRelativePath_0003.jpg", newFile);
        ASSERT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        relativePath = "Download/GetFileInfoFromRelativePath_0003.jpg";
        result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        int64_t offset = 0;
        int64_t maxCount = 1000;
        FileFilter filter;
        std::vector<FileInfo> fileInfoVec;
        result = g_fah->ListFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Delete(newFile);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetFileInfoFromRelativePath_0003 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_GetFileInfoFromRelativePath_0003";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetFileInfoFromRelativePath_0004
 * @tc.name: medialibrary_file_access_GetFileInfoFromRelativePath_0004
 * @tc.desc: Test function of GetFileInfoFromRelativePath interface.
 * @tc.desc: convert the relative directory path to fileinfo and call listfile for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HRLBS
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetFileInfoFromRelativePath_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileExtensionHelperTest-begin medialibrary_file_access_GetFileInfoFromRelativePath_0004";
    try {
        FileInfo fileInfo;
        string relativePath = "Download/";
        int result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        ASSERT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri parentUri(fileInfo.uri);
        Uri newDir("");
        result = g_fah->Mkdir(parentUri, "DirGetFileInfoFromRelativePath_0004", newDir);
        ASSERT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri fileUri("");
        result = g_fah->CreateFile(newDir, "file1.jpg", fileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->CreateFile(newDir, "file2.jpg", fileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        relativePath = "Download/DirGetFileInfoFromRelativePath_0004";
        result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        int64_t offset = 0;
        int64_t maxCount = 1000;
        FileFilter filter;
        std::vector<FileInfo> fileInfoVec;
        result = g_fah->ListFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_GT(fileInfoVec.size(), OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Delete(newDir);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetFileInfoFromRelativePath_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_GetFileInfoFromRelativePath_0004";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetThumbnail_0000
 * @tc.name: medialibrary_file_access_GetThumbnail_0000
 * @tc.desc: Test function of GetThumbnail interface.
 * @tc.desc: Test function of GetThumbnail interface for SUCCESS which Uri's type is image.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6HLSK
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetThumbnail_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_GetThumbnail_0000";
    try {
        FileAccessFwk::FileInfo fileInfo;
        std::string relativePath = "Documents/CreateImageThumbnailTest_001.jpg";
        int result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri uri(fileInfo.uri);
        std::shared_ptr<PixelMap> getPixelMap = nullptr;
        ThumbnailSize thumbnailSize;
        thumbnailSize.width = 256;
        thumbnailSize.height = 256;
        result = g_fah->GetThumbnail(uri, thumbnailSize, getPixelMap);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        ASSERT_NE(getPixelMap, nullptr);
        EXPECT_EQ(getPixelMap->GetWidth(), thumbnailSize.width);
        EXPECT_EQ(getPixelMap->GetHeight(), thumbnailSize.height);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetThumbnail_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_GetThumbnail_0000";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetThumbnail_0001
 * @tc.name: medialibrary_file_access_GetThumbnail_0001
 * @tc.desc: Test function of GetThumbnail interface.
 * @tc.desc: Test function of GetThumbnail interface for SUCCESS which Uri's type is video.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6HLSK
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetThumbnail_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_GetThumbnail_0001";
    try {
        FileAccessFwk::FileInfo fileInfo;
        std::string relativePath = "Documents/CreateVideoThumbnailTest_001.mp4";
        int result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri uri(fileInfo.uri);
        std::shared_ptr<PixelMap> getPixelMap = nullptr;
        ThumbnailSize thumbnailSize;
        thumbnailSize.width = 300;
        thumbnailSize.height = 300;
        result = g_fah->GetThumbnail(uri, thumbnailSize, getPixelMap);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        ASSERT_NE(getPixelMap, nullptr);
        EXPECT_EQ(getPixelMap->GetWidth(), thumbnailSize.width);
        EXPECT_EQ(getPixelMap->GetHeight(), thumbnailSize.height);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetThumbnail_0001 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_GetThumbnail_0001";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetThumbnail_0002
 * @tc.name: medialibrary_file_access_GetThumbnail_0002
 * @tc.desc: Test function of GetThumbnail interface.
 * @tc.desc: Test function of GetThumbnail interface for SUCCESS which Uri's type is audio.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6HLSK
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetThumbnail_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_GetThumbnail_0002";
    try {
        FileAccessFwk::FileInfo fileInfo;
        std::string relativePath = "Documents/CreateAudioThumbnailTest_001.mp3";
        int result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri uri(fileInfo.uri);
        std::shared_ptr<PixelMap> getPixelMap = nullptr;
        ThumbnailSize thumbnailSize;
        thumbnailSize.width = 256;
        thumbnailSize.height = 256;
        result = g_fah->GetThumbnail(uri, thumbnailSize, getPixelMap);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        ASSERT_NE(getPixelMap, nullptr);
        EXPECT_EQ(getPixelMap->GetWidth(), thumbnailSize.width);
        EXPECT_EQ(getPixelMap->GetHeight(), thumbnailSize.height);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetThumbnail_0002 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_GetThumbnail_0002";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetThumbnail_0003
 * @tc.name: medialibrary_file_access_GetThumbnail_0003
 * @tc.desc: Test function of GetThumbnail interface.
 * @tc.desc: Test function of GetThumbnail interface for ERROR which Uri is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6HLSK
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetThumbnail_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_GetThumbnail_0003";
    try {
        Uri uri("");
        std::shared_ptr<PixelMap> getPixelMap = nullptr;
        ThumbnailSize thumbnailSize;
        thumbnailSize.width = 256;
        thumbnailSize.height = 256;
        int result = g_fah->GetThumbnail(uri, thumbnailSize, getPixelMap);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        ASSERT_EQ(getPixelMap, nullptr);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetThumbnail_0003 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_GetThumbnail_0003";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetThumbnail_0004
 * @tc.name: medialibrary_file_access_GetThumbnail_0004
 * @tc.desc: Test function of GetThumbnail interface.
 * @tc.desc: Test function of GetThumbnail interface for ERROR which Uri is not a local path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6HLSK
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetThumbnail_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_GetThumbnail_0004";
    try {
        Uri uri("//Pictures/CreateImageThumbnailTest_001.jpg");
        std::shared_ptr<PixelMap> getPixelMap = nullptr;
        ThumbnailSize thumbnailSize;
        thumbnailSize.width = 256;
        thumbnailSize.height = 256;
        int result = g_fah->GetThumbnail(uri, thumbnailSize, getPixelMap);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        ASSERT_EQ(getPixelMap, nullptr);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetThumbnail_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_GetThumbnail_0004";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetThumbnail_0005
 * @tc.name: medialibrary_file_access_GetThumbnail_0005
 * @tc.desc: Test function of GetThumbnail interface.
 * @tc.desc: Test function of GetThumbnail interface for ERROR which Uri is not a media path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6HLSK
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetThumbnail_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_GetThumbnail_0005";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest, "test.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        std::shared_ptr<PixelMap> getPixelMap = nullptr;
        ThumbnailSize thumbnailSize;
        thumbnailSize.width = 256;
        thumbnailSize.height = 256;
        result = g_fah->GetThumbnail(testUri, thumbnailSize, getPixelMap);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        ASSERT_EQ(getPixelMap, nullptr);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetThumbnail_0005 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_GetThumbnail_0005";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetThumbnail_0006
 * @tc.name: medialibrary_file_access_GetThumbnail_0006
 * @tc.desc: Test function of GetThumbnail interface.
 * @tc.desc: Test function of GetThumbnail interface for ERROR which Uri is unreadable code.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6HLSK
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetThumbnail_0006, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_GetThumbnail_0006";
    try {
        Uri uri("&*()*/?");
        std::shared_ptr<PixelMap> getPixelMap = nullptr;
        ThumbnailSize thumbnailSize;
        thumbnailSize.width = 256;
        thumbnailSize.height = 256;
        int result = g_fah->GetThumbnail(uri, thumbnailSize, getPixelMap);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        ASSERT_EQ(getPixelMap, nullptr);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetThumbnail_0006 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_GetThumbnail_0006";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetThumbnail_0007
 * @tc.name: medialibrary_file_access_GetThumbnail_0007
 * @tc.desc: Test function of GetThumbnail interface.
 * @tc.desc: Test function of GetThumbnail interface for ERROR which size is an invalid value.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6HLSK
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetThumbnail_0007, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_GetThumbnail_0007";
    try {
        FileAccessFwk::FileInfo fileInfo;
        std::string relativePath = "Documents/CreateImageThumbnailTest_001.jpg";
        int result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri uri(fileInfo.uri);
        std::shared_ptr<PixelMap> getPixelMap = nullptr;
        ThumbnailSize thumbnailSize;
        thumbnailSize.width = 0;
        thumbnailSize.height = 0;
        result = g_fah->GetThumbnail(uri, thumbnailSize, getPixelMap);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        ASSERT_EQ(getPixelMap, nullptr);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetThumbnail_0007 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_GetThumbnail_0007";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_GetThumbnail_0008
 * @tc.name: medialibrary_file_access_GetThumbnail_0008
 * @tc.desc: Test function of GetThumbnail interface.
 * @tc.desc: Test function of GetThumbnail interface for ERROR which size is an invalid value.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6HLSK
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_GetThumbnail_0008, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_GetThumbnail_0008";
    try {
        FileAccessFwk::FileInfo fileInfo;
        std::string relativePath = "Documents/CreateImageThumbnailTest_001.jpg";
        int result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri uri(fileInfo.uri);
        std::shared_ptr<PixelMap> getPixelMap = nullptr;
        ThumbnailSize thumbnailSize;
        thumbnailSize.width = -1;
        thumbnailSize.height = -1;
        result = g_fah->GetThumbnail(uri, thumbnailSize, getPixelMap);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        ASSERT_EQ(getPixelMap, nullptr);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_GetThumbnail_0008 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_GetThumbnail_0008";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Query_0000
 * @tc.name: medialibrary_file_access_Query_0000
 * @tc.desc: Test function of Query directory for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6S4VV
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Query_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Query_0000";
    try {
        FileAccessFwk::FileInfo fileInfo;
        std::string relativePath = "Documents/";
        std::string displayName = "Documents";
        int targetSize = 1347231;
        int result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri dirUriTest(fileInfo.uri);

        json testJson = {
            {RELATIVE_PATH, " "},
            {DISPLAY_NAME, " "},
            {FILE_SIZE, " "},
            {DATE_MODIFIED, " "},
            {DATE_ADDED, " "},
            {HEIGHT, " "},
            {WIDTH, " "},
            {DURATION, " "}
        };
        auto testJsonString = testJson.dump();
        int ret = g_fah->Query(dirUriTest, testJsonString);
        EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
        auto jsonObject = json::parse(testJsonString);
        EXPECT_EQ(jsonObject.at(DISPLAY_NAME), displayName);
        EXPECT_EQ(jsonObject.at(FILE_SIZE), targetSize);
        ASSERT_TRUE(jsonObject.at(DATE_MODIFIED) > 0);
        ASSERT_TRUE(jsonObject.at(DATE_ADDED) > 0);
        GTEST_LOG_(INFO) << testJsonString;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Query_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Query_0000";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Query_0001
 * @tc.name: medialibrary_file_access_Query_0001
 * @tc.desc: Test function of Query file for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6S4VV
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Query_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Query_0001";
    try {
        FileAccessFwk::FileInfo fileInfo;
        std::string relativePath = "Documents/Test/";
        std::string displayName = "CreateQueryTest_002.txt";
        int targetSize = 23;
        std::string filePath = "Documents/Test/CreateQueryTest_002.txt";
        int ret = g_fah->GetFileInfoFromRelativePath(filePath, fileInfo);
        EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri(fileInfo.uri);

        json testJson = {
            {RELATIVE_PATH, " "},
            {DISPLAY_NAME, " "},
            {FILE_SIZE, " "},
            {DATE_MODIFIED, " "},
            {DATE_ADDED, " "},
            {HEIGHT, " "},
            {WIDTH, " "},
            {DURATION, " "}
        };
        auto testJsonString = testJson.dump();
        ret = g_fah->Query(testUri, testJsonString);
        EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << testJsonString;
        auto jsonObject = json::parse(testJsonString);
        EXPECT_EQ(jsonObject.at(RELATIVE_PATH), relativePath);
        EXPECT_EQ(jsonObject.at(DISPLAY_NAME), displayName);
        EXPECT_EQ(jsonObject.at(FILE_SIZE), targetSize);
        ASSERT_TRUE(jsonObject.at(DATE_MODIFIED) > 0);
        ASSERT_TRUE(jsonObject.at(DATE_ADDED) > 0);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Query_0001 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Query_0001";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Query_0002
 * @tc.name: medialibrary_file_access_Query_0002
 * @tc.desc: Test function of Query directory size for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require:
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Query_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Query_0002";
    try {
        FileAccessFwk::FileInfo fileInfo;
        std::string relativePath = "Documents/";
        std::string displayName = "Documents";
        int targetSize = 1347231;
        int result = g_fah->GetFileInfoFromRelativePath(relativePath, fileInfo);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri dirUriTest(fileInfo.uri);

        json testJson = {
            {FILE_SIZE, " "}
        };
        auto testJsonString = testJson.dump();
        int ret = g_fah->Query(dirUriTest, testJsonString);
        EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
        auto jsonObject = json::parse(testJsonString);
        EXPECT_EQ(jsonObject.at(FILE_SIZE), targetSize);
        GTEST_LOG_(INFO) << testJsonString;
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Query_0002 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Query_0002";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Query_0003
 * @tc.name: medialibrary_file_access_Query_0003
 * @tc.desc: Test function of Query interface for ERROR which Uri is unreadable code.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6S4VV
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Query_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Query_0003";
    try {
        Uri testUri("&*()*/?");
        json testJson = {
            {RELATIVE_PATH, " "},
            {DISPLAY_NAME, " "}
        };
        auto testJsonString = testJson.dump();
        int ret = g_fah->Query(testUri, testJsonString);
        EXPECT_NE(ret, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Query_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Query_0003";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Query_0004
 * @tc.name: medialibrary_file_access_Query_0004
 * @tc.desc: Test function of Query interface for which all column nonexistence.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6S4VV
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Query_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Query_0004";
    try {
        Uri newDirUriTest("");
        std::string fileName = "test.txt";
        int ret = g_fah->Mkdir(g_newDirUri, "Query004", newDirUriTest);
        EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        ret = g_fah->CreateFile(newDirUriTest, fileName, testUri);
        EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
        json testJson = {
            {"001", " "},
            {"#", " "},
            {"test", " "},
            {"target", " "}
        };
        auto testJsonString = testJson.dump();
        ret = g_fah->Query(testUri, testJsonString);
        EXPECT_NE(ret, OHOS::FileAccessFwk::ERR_OK);
        ret = g_fah->Delete(newDirUriTest);
        EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Query_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Query_0004";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Query_0005
 * @tc.name: medialibrary_file_access_Query_0005
 * @tc.desc: Test function of Query interface for which part of column nonexistence.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6S4VV
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Query_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Query_0005";
    try {
        Uri newDirUriTest("");
        std::string fileName = "test.txt";
        int ret = g_fah->Mkdir(g_newDirUri, "Query005", newDirUriTest);
        EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        ret = g_fah->CreateFile(newDirUriTest, fileName, testUri);
        EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
        json testJson = {
            {RELATIVE_PATH, " "},
            {DISPLAY_NAME, " "},
            {"test", " "}
        };
        auto testJsonString = testJson.dump();
        ret = g_fah->Query(testUri, testJsonString);
        EXPECT_NE(ret, OHOS::FileAccessFwk::ERR_OK);
        ret = g_fah->Delete(newDirUriTest);
        EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Query_0005 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Query_0005";
}

/**
 * @tc.number: user_file_service_medialibrary_file_access_Query_0006
 * @tc.name: medialibrary_file_access_Query_0006
 * @tc.desc: Test function of Query interface for which column is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: I6S4VV
 */
HWTEST_F(FileAccessHelperTest, medialibrary_file_access_Query_0006, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin medialibrary_file_access_Query_0006";
    try {
        Uri newDirUriTest("");
        std::string fileName = "test.txt";
        std::string relativePath = "test/test.txt";
        int ret = g_fah->Mkdir(g_newDirUri, "Query006", newDirUriTest);
        EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri("");
        ret = g_fah->CreateFile(newDirUriTest, fileName, testUri);
        EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
        json testJson;
        auto testJsonString = testJson.dump();
        ret = g_fah->Query(testUri, testJsonString);
        EXPECT_NE(ret, OHOS::FileAccessFwk::ERR_OK);
        ret = g_fah->Delete(newDirUriTest);
        EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "medialibrary_file_access_Query_0006 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end medialibrary_file_access_Query_0006";
}
} // namespace
