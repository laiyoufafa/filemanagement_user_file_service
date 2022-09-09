/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
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

#include "base/security/access_token/interfaces/innerkits/accesstoken/include/accesstoken_kit.h"
#include "base/security/access_token/interfaces/innerkits/token_setproc/include/token_setproc.h"
#include "file_access_framework_errno.h"
#include "file_access_helper.h"
#include "iservice_registry.h"


namespace {
using namespace std;
using namespace OHOS;
using namespace FileAccessFwk;
const int ABILITY_ID = 5003;
shared_ptr<FileAccessHelper> g_fah = nullptr;
OHOS::Security::AccessToken::AccessTokenID g_tokenId;
Uri g_newDirUri("");
const int UID_TRANSFORM_TMP = 20000000;
const int UID_DEFAULT = 0;

// permission state
OHOS::Security::AccessToken::PermissionStateFull g_infoManagerTestState = {
    .permissionName = "ohos.permission.FILE_ACCESS_MANAGER",
    .isGeneral = true,
    .resDeviceID = {"local"},
    .grantStatus = {OHOS::Security::AccessToken::PermissionState::PERMISSION_GRANTED},
    .grantFlags = {1}
};

OHOS::Security::AccessToken::PermissionStateFull g_infoManagerTestState1 = {
    .permissionName = "ohos.permission.READ_MEDIA",
    .isGeneral = true,
    .resDeviceID = {"local"},
    .grantStatus = {OHOS::Security::AccessToken::PermissionState::PERMISSION_GRANTED},
    .grantFlags = {1}
};

OHOS::Security::AccessToken::PermissionStateFull g_infoManagerTestState2 = {
    .permissionName = "ohos.permission.WRITE_MEDIA",
    .isGeneral = true,
    .resDeviceID = {"local"},
    .grantStatus = {OHOS::Security::AccessToken::PermissionState::PERMISSION_GRANTED},
    .grantFlags = {1}
};

// permission define
OHOS::Security::AccessToken::PermissionDef g_infoManagerTestPermDef = {
    .permissionName = "ohos.permission.FILE_ACCESS_MANAGER",
    .bundleName = "ohos.acts.multimedia.mediaLibrary",
    .grantMode = 1,
    .availableLevel = OHOS::Security::AccessToken::APL_NORMAL,
    .label = "label",
    .labelId = 1,
    .description = "FILE_ACCESS_MANAGER",
    .descriptionId = 1
};

OHOS::Security::AccessToken::PermissionDef g_infoManagerTestPermDef1 = {
    .permissionName = "ohos.permission.READ_MEDIA",
    .bundleName = "ohos.acts.multimedia.mediaLibrary",
    .grantMode = 1,
    .availableLevel = OHOS::Security::AccessToken::APL_NORMAL,
    .label = "label",
    .labelId = 1,
    .description = "READ_MEDIA",
    .descriptionId = 1
};

OHOS::Security::AccessToken::PermissionDef g_infoManagerTestPermDef2 = {
    .permissionName = "ohos.permission.WRITE_MEDIA",
    .bundleName = "ohos.acts.multimedia.mediaLibrary",
    .grantMode = 1,
    .availableLevel = OHOS::Security::AccessToken::APL_NORMAL,
    .label = "label",
    .labelId = 1,
    .description = "WRITE_MEDIA",
    .descriptionId = 1
};

// permission info
OHOS::Security::AccessToken::HapPolicyParams g_infoManagerTestPolicyPrams = {
    .apl = OHOS::Security::AccessToken::APL_NORMAL,
    .domain = "test.domain",
    .permList = {g_infoManagerTestPermDef, g_infoManagerTestPermDef1, g_infoManagerTestPermDef2},
    .permStateList = {g_infoManagerTestState, g_infoManagerTestState1, g_infoManagerTestState2}
};

// hap info
OHOS::Security::AccessToken::HapInfoParams const g_infoManagerTestInfoParms = {
    .userID = 1,
    .bundleName = "fileAccessHelperTest",
    .instIndex = 0,
    .appIDDesc = "testtesttesttest"
};

class FileAccessHelperTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        cout << "FileAccessHelperTest code test" << endl;

        auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        auto remoteObj = saManager->GetSystemAbility(ABILITY_ID);
        AAFwk::Want want;
        vector<AAFwk::Want> wantVec;
        setuid(UID_TRANSFORM_TMP);
        int ret = FileAccessHelper::GetRegisteredFileAccessExtAbilityInfo(wantVec);
        setuid(UID_DEFAULT);
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
        vector<AAFwk::Want> wants {want};
        g_fah = FileAccessHelper::Creator(remoteObj, wants);

        OHOS::Security::AccessToken::AccessTokenIDEx tokenIdEx = {0};
        tokenIdEx = OHOS::Security::AccessToken::AccessTokenKit::AllocHapToken
            (g_infoManagerTestInfoParms, g_infoManagerTestPolicyPrams);
        g_tokenId = tokenIdEx.tokenIdExStruct.tokenID;
        SetSelfTokenID(g_tokenId);
    }
    static void TearDownTestCase() {
        g_fah->Release();
        g_fah = nullptr;
        OHOS::Security::AccessToken::AccessTokenKit::DeleteToken(g_tokenId);
    };
    void SetUp() {};
    void TearDown() {};
};

/**
 * @tc.number: user_file_service_file_access_helper_OpenFile_0000
 * @tc.name: file_access_helper_OpenFile_0000
 * @tc.desc: Test function of OpenFile interface for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_OpenFile_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_OpenFile_0000";
    try {
        vector<RootInfo> info;
        int result = g_fah->GetRoots(info);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri parentUri("");
        if (info.size() > 0) {
            parentUri = Uri(info[0].uri + "/file");
            GTEST_LOG_(INFO) << parentUri.ToString();
        }

        result = g_fah->Mkdir(parentUri, "Download", g_newDirUri);
        EXPECT_LE(result, OHOS::FileAccessFwk::ERR_OK);
        g_newDirUri = Uri("datashare:///media/file/1");

        Uri newDirUriTest("");
        result = g_fah->Mkdir(g_newDirUri, "test1", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri newFileUri("");
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_OpenFile_0000.txt", newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        int fd;
        result = g_fah->OpenFile(newFileUri, 0, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        GTEST_LOG_(INFO) << "OpenFile_0000 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_OpenFile_0000";
}

/**
 * @tc.number: user_file_service_file_access_helper_OpenFile_0001
 * @tc.name: file_access_helper_OpenFile_0001
 * @tc.desc: Test function of OpenFile interface for ERROR which Uri is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_OpenFile_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_OpenFile_0001";
    try {
        Uri uri("");
        int fd;
        int result = g_fah->OpenFile(uri, 0, fd);
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "OpenFile_0001 result:" << result << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_OpenFile_0001";
}

/**
 * @tc.number: user_file_service_file_access_helper_OpenFile_0002
 * @tc.name: file_access_helper_OpenFile_0002
 * @tc.desc: Test function of OpenFile interface for ERROR which Uri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_OpenFile_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_OpenFile_0002";
    try {
        Uri newFileUri("");
        int result = g_fah->CreateFile(g_newDirUri, "file_access_helper_OpenFile_0002.txt", newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri uri("storage/media/100/local/files/Download/file_access_helper_OpenFile_0002.txt");
        int fd;
        result = g_fah->OpenFile(uri, 0, fd);
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "OpenFile_0002 result:" << result << endl;

        result = g_fah->Delete(newFileUri);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_OpenFile_0002";
}

/**
 * @tc.number: user_file_service_file_access_helper_OpenFile_0003
 * @tc.name: file_access_helper_OpenFile_0003
 * @tc.desc: Test function of OpenFile interface for ERROR which Uri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_OpenFile_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_OpenFile_0003";
    try {
        Uri uri("~!@#$%^&*()_");
        int fd;
        int result = g_fah->OpenFile(uri, 0, fd);
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "OpenFile_0003 result:" << result << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_OpenFile_0003";
}

/**
 * @tc.number: user_file_service_file_access_helper_OpenFile_0004
 * @tc.name: file_access_helper_OpenFile_0004
 * @tc.desc: Test function of OpenFile interface for ERROR which flag is -1.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_OpenFile_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_OpenFile_0004";
    try {
        Uri newFileUri("");
        int result = g_fah->CreateFile(g_newDirUri, "file_access_helper_OpenFile_0004.txt", newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        int fd;
        result = g_fah->OpenFile(newFileUri, -1, fd);
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "OpenFile_0004 result:" << result << endl;

        result = g_fah->Delete(newFileUri);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_OpenFile_0004";
}

/**
 * @tc.number: user_file_service_file_access_helper_OpenFile_0005
 * @tc.name: file_access_helper_OpenFile_0005
 * @tc.desc: Test function of OpenFile interface for SUCCESS which flag is 1.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_OpenFile_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_OpenFile_0005";
    try {
        Uri newFileUri("");
        int result = g_fah->CreateFile(g_newDirUri, "file_access_helper_OpenFile_0005.txt", newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        int fd;
        result = g_fah->OpenFile(newFileUri, 1, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "OpenFile_0005 result:" << result << endl;

        result = g_fah->Delete(newFileUri);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_OpenFile_0005";
}

/**
 * @tc.number: user_file_service_file_access_helper_OpenFile_0006
 * @tc.name: file_access_helper_OpenFile_0006
 * @tc.desc: Test function of OpenFile interface for SUCCESS which flag is 2.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_OpenFile_0006, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_OpenFile_0006";
    try {
        Uri newFileUri("");
        int result = g_fah->CreateFile(g_newDirUri, "file_access_helper_OpenFile_0006.txt", newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        int fd;
        result = g_fah->OpenFile(newFileUri, 2, fd);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "OpenFile_0006 result:" << result << endl;

        result = g_fah->Delete(newFileUri);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_OpenFile_0006";
}

void OpenFileTdd(shared_ptr<FileAccessHelper> fahs, Uri uri, int flags, int fd)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_Access_helper_OpenFileTdd";
    int ret = fahs->OpenFile(uri, flags, fd);
    EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
    if (ret != OHOS::FileAccessFwk::ERR_OK) {
        GTEST_LOG_(INFO) << "OpenFileTdd get result error, code:" << ret;
    }
    EXPECT_GE(fd, 0);
    int result = fahs->Delete(uri);
    EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_Access_helper_OpenFileTdd";
}

/**
 * @tc.number: user_file_service_file_access_helper_OpenFile_0007
 * @tc.name: file_access_helper_OpenFile_0007
 * @tc.desc: Test function of OpenFile interface for SUCCESS which Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_OpenFile_0007, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_OpenFile_0007";
    try {
        Uri newFileUri("");
        int flags = 0;
        int fd;
        std::string displayName[4] = {"test1.txt", "test2.txt", "test3.txt", "test4.txt"};
        for (int j = 0; j < 4; j++) {
            int result = g_fah->CreateFile(g_newDirUri, displayName[j], newFileUri);
            EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
            std::thread execthread(OpenFileTdd, g_fah, newFileUri, flags, fd);
            execthread.join();
        }
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_OpenFile_0007";
}

/**
 * @tc.number: user_file_service_file_access_helper_CreateFile_0000
 * @tc.name: file_access_helper_CreateFile_0000
 * @tc.desc: Test function of CreateFile interface for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_CreateFile_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_CreateFile_0000";
    try {
        Uri newFileUri("");
        int result = g_fah->CreateFile(g_newDirUri, "file_access_helper_CreateFile_0000.txt", newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "CreateFile_0000 result:" << result << endl;

        result = g_fah->Delete(newFileUri);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_CreateFile_0000";
}

/**
 * @tc.number: user_file_service_file_access_helper_CreateFile_0001
 * @tc.name: file_access_helper_CreateFile_0001
 * @tc.desc: Test function of CreateFile interface for ERROR which parentUri is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_CreateFile_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_CreateFile_0001";
    try {
        Uri newFileUri("");
        Uri parentUri("");
        int result = g_fah->CreateFile(parentUri, "file_access_helper_CreateFile_0001.txt", newFileUri);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "CreateFile_0001 result:" << result << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_CreateFile_0001";
}

/**
 * @tc.number: user_file_service_file_access_helper_CreateFile_0002
 * @tc.name: file_access_helper_CreateFile_0002
 * @tc.desc: Test function of CreateFile interface for ERROR which parentUri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_CreateFile_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_CreateFile_0002";
    try {
        Uri newFileUri("");
        Uri parentUri("storage/media/100/local/files/Download");
        int result = g_fah->CreateFile(parentUri, "file_access_helper_CreateFile_0002.txt", newFileUri);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "CreateFile_0002 result:" << result << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_CreateFile_0002";
}

/**
 * @tc.number: user_file_service_file_access_helper_CreateFile_0003
 * @tc.name: file_access_helper_CreateFile_0003
 * @tc.desc: Test function of CreateFile interface for ERROR which parentUri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_CreateFile_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_CreateFile_0002";
    try {
        Uri newFileUri("");
        Uri parentUri("~!@#$%^&*()_");
        int result = g_fah->CreateFile(parentUri, "file_access_helper_CreateFile_0003.txt", newFileUri);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "CreateFile_0003 result:" << result << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_CreateFile_0003";
}

/**
 * @tc.number: user_file_service_file_access_helper_CreateFile_0004
 * @tc.name: file_access_helper_CreateFile_0004
 * @tc.desc: Test function of CreateFile interface for ERROR which displayName is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_CreateFile_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_CreateFile_0004";
    try {
        Uri newFileUri("");
        string displayName = "";
        int result = g_fah->CreateFile(g_newDirUri, displayName, newFileUri);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "CreateFile_0004 result:" << result << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_CreateFile_0004";
}

void CreateFileTdd(shared_ptr<FileAccessHelper> fahs, Uri parent, std::string displayName, Uri newDir)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_Access_helper_CreateFileTdd";
    int ret = fahs->CreateFile(parent, displayName, newDir);
    EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
    if (ret < OHOS::FileAccessFwk::ERR_OK) {
        GTEST_LOG_(INFO) << "CreateFileTdd get result error, code:" << ret;
    }
    EXPECT_NE(newDir.ToString(), "");
    int result = fahs->Delete(newDir);
    EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_Access_helper_CreateFileTdd";
}

/**
 * @tc.number: user_file_service_file_access_helper_CreateFile_0005
 * @tc.name: file_access_helper_CreateFile_0005
 * @tc.desc: Test function of CreateFile interface for SUCCESS while Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_CreateFile_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_CreateFile_0005";
    try {
        Uri newFileUri("");
        std::string displayName[4] = {"test1", "test2", "test3", "test4"};
        for (int j = 0; j < 4; j++) {
            std::thread execthread(CreateFileTdd, g_fah, g_newDirUri, displayName[j], newFileUri);
            execthread.join();
        }
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_CreateFile_0005";
}

/**
 * @tc.number: user_file_service_file_access_helper_Mkdir_0000
 * @tc.name: file_access_helper_Mkdir_0000
 * @tc.desc: Test function of Mkdir interface for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Mkdir_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Mkdir_0000";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "file_access_helper_Mkdir_0000", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Mkdir_0000 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Mkdir_0000";
}

/**
 * @tc.number: user_file_service_file_access_helper_Mkdir_0001
 * @tc.name: file_access_helper_Mkdir_0001
 * @tc.desc: Test function of Mkdir interface for ERROR which parentUri is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Mkdir_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Mkdir_0001";
    try {
        Uri newDirUriTest("");
        Uri parentUri("");
        int result = g_fah->Mkdir(parentUri, "file_access_helper_Mkdir_0001", newDirUriTest);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Mkdir_0001 result:" << result << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Mkdir_0001";
}

/**
 * @tc.number: user_file_service_file_access_helper_Mkdir_0002
 * @tc.name: file_access_helper_Mkdir_0002
 * @tc.desc: Test function of Mkdir interface for ERROR which parentUri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Mkdir_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Mkdir_0002";
    try {
        Uri newDirUriTest("");
        Uri parentUri("storage/media/100/local/files/Download");
        int result = g_fah->Mkdir(parentUri, "file_access_helper_Mkdir_0002", newDirUriTest);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Mkdir_0002 result:" << result << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Mkdir_0002";
}

/**
 * @tc.number: user_file_service_file_access_helper_Mkdir_0003
 * @tc.name: file_access_helper_Mkdir_0003
 * @tc.desc: Test function of Mkdir interface for ERROR which parentUri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Mkdir_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Mkdir_0002";
    try {
        Uri newDirUriTest("");
        Uri parentUri("~!@#$%^&*()_");
        int result = g_fah->Mkdir(parentUri, "file_access_helper_Mkdir_0003", newDirUriTest);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Mkdir_0003 result:" << result << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Mkdir_0003";
}

/**
 * @tc.number: user_file_service_file_access_helper_Mkdir_0004
 * @tc.name: file_access_helper_Mkdir_0004
 * @tc.desc: Test function of Mkdir interface for ERROR which displayName is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Mkdir_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Mkdir_0004";
    try {
        Uri newDirUriTest("");
        string displayName = "";
        int result = g_fah->Mkdir(g_newDirUri, displayName, newDirUriTest);
        EXPECT_NE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Mkdir_0004 result:" << result << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Mkdir_0004";
}

void MkdirTdd(shared_ptr<FileAccessHelper> fahs, Uri parent, std::string displayName, Uri newDir)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_Access_helper_MkdirTdd";
    int ret = fahs->Mkdir(parent, displayName, newDir);
    EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
    if (ret < OHOS::FileAccessFwk::ERR_OK) {
        GTEST_LOG_(INFO) << "MkdirTdd get result error, code:" << ret;
    }
    EXPECT_NE(newDir.ToString(), "");
    int result = fahs->Delete(newDir);
    EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_Access_helper_MkdirTdd";
}

/**
 * @tc.number: user_file_service_file_access_helper_Mkdir_0005
 * @tc.name: file_access_helper_Mkdir_0005
 * @tc.desc: Test function of Mkdir interface for SUCCESS which Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Mkdir_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Mkdir_0005";
    try {
        Uri newDirUriTest("");
        std::string displayName[4] = {"test1", "test2", "test3", "test4"};
        for (int j = 0; j < 4; j++) {
            std::thread execthread(MkdirTdd, g_fah, g_newDirUri, displayName[j], newDirUriTest);
            execthread.join();
        }
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Mkdir_0005";
}

/**
 * @tc.number: user_file_service_file_access_helper_Delete_0000
 * @tc.name: file_access_helper_Delete_0000
 * @tc.desc: Test function of Delete interface for SUCCESS which delete file.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Delete_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Delete_0000";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri newFileUri("");
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_Delete_0000.txt", newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Delete(newFileUri);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Delete_0000 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Delete_0000";
}

/**
 * @tc.number: user_file_service_file_access_helper_Delete_0001
 * @tc.name: file_access_helper_Delete_0001
 * @tc.desc: Test function of Delete interface for SUCCESS which delete folder.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Delete_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Delete_0001";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Delete_0001 result:" << result << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Delete_0001";
}

/**
 * @tc.number: user_file_service_file_access_helper_Delete_0002
 * @tc.name: file_access_helper_Delete_0002
 * @tc.desc: Test function of Delete interface for ERROR which selectFileUri is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Delete_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Delete_0002";
    try {
        Uri selectFileUri("");
        int result = g_fah->Delete(selectFileUri);
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Delete_0002 result:" << result << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Delete_0002";
}

/**
 * @tc.number: user_file_service_file_access_helper_Delete_0003
 * @tc.name: file_access_helper_Delete_0003
 * @tc.desc: Test function of Delete interface for ERROR which selectFileUri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Delete_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Delete_0003";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri selectFileUri("storage/media/100/local/files/Download/test");
        result = g_fah->Delete(selectFileUri);
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Delete_0003 result:" << result << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Delete_0003";
}

/**
 * @tc.number: user_file_service_file_access_helper_Delete_0004
 * @tc.name: file_access_helper_Delete_0004
 * @tc.desc: Test function of Delete interface for ERROR which selectFileUri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Delete_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Delete_0004";
    try {
        Uri selectFileUri("!@#$%^&*()");
        int result = g_fah->Delete(selectFileUri);
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Delete_0004 result:" << result << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Delete_0004";
}

void DeleteTdd(shared_ptr<FileAccessHelper> fahs, Uri selectFile)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_Access_helper_DeleteTdd";
    int ret = fahs->Delete(selectFile);
    EXPECT_GE(ret, OHOS::FileAccessFwk::ERR_OK);
    if (ret < OHOS::FileAccessFwk::ERR_OK) {
        GTEST_LOG_(INFO) << "DeleteTdd get result error, code:" << ret;
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_Access_helper_DeleteTdd";
}

/**
 * @tc.number: user_file_service_file_access_helper_Delete_0005
 * @tc.name: file_access_helper_Delete_0005
 * @tc.desc: Test function of Delete interface for SUCCESS which Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Delete_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Delete_0005";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri newFileUri("");
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_Delete_0005.txt", newFileUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri testUri("");
        std::string displayName[4] = {"test1.txt", "test2.txt", "test3.txt", "test4.txt"};
        Uri testUri2("");
        for (int j = 0; j < 4; j++) {
            result = g_fah->CreateFile(newDirUriTest, displayName[j], testUri);
            EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
            std::thread execthread(DeleteTdd, g_fah, testUri);
            execthread.join();
        }

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Delete_0005";
}


/**
 * @tc.number: user_file_service_file_access_helper_Move_0000
 * @tc.name: file_access_helper_Move_0000
 * @tc.desc: Test function of Move interface for SUCCESS which move file.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Move_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Move_0000";
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
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0000 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Move_0000";
}

/**
 * @tc.number: user_file_service_file_access_helper_Move_0001
 * @tc.name: file_access_helper_Move_0001
 * @tc.desc: Test function of Move interface for SUCCESS which move folder.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Move_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Move_0001";
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
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0001 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Move_0001";
}

/**
 * @tc.number: user_file_service_file_access_helper_Move_0002
 * @tc.name: file_access_helper_Move_0002
 * @tc.desc: Test function of Move interface for ERROR which sourceFileUri is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Move_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Move_0002";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri testUri("");
        Uri sourceFileUri("");
        result = g_fah->Move(sourceFileUri, newDirUriTest, testUri);
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0002 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Move_0002";
}

/**
 * @tc.number: user_file_service_file_access_helper_Move_0003
 * @tc.name: file_access_helper_Move_0003
 * @tc.desc: Test function of Move interface for ERROR which sourceFileUri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Move_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Move_0003";
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
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0003 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Move_0003";
}

/**
 * @tc.number: user_file_service_file_access_helper_Move_0004
 * @tc.name: file_access_helper_Move_0004
 * @tc.desc: Test function of Move interface for ERROR which sourceFileUri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Move_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Move_0004";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri testUri("");
        Uri sourceFileUri("~!@#$%^&*()_");
        result = g_fah->Move(sourceFileUri, newDirUriTest, testUri);
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0004 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Move_0004";
}

/**
 * @tc.number: user_file_service_file_access_helper_Move_0005
 * @tc.name: file_access_helper_Move_0005
 * @tc.desc: Test function of Move interface for ERROR which targetParentUri is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Move_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Move_0005";
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
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0005 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Move_0005";
}

/**
 * @tc.number: user_file_service_file_access_helper_Move_0006
 * @tc.name: file_access_helper_Move_0006
 * @tc.desc: Test function of Move interface for ERROR which targetParentUri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Move_0006, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Move_0006";
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
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0006 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Move_0006";
}

/**
 * @tc.number: user_file_service_file_access_helper_Move_0007
 * @tc.name: file_access_helper_Move_0007
 * @tc.desc: Test function of Move interface for ERROR which targetParentUri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Move_0007, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Move_0007";
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
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0007 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Move_0007";
}

/**
 * @tc.number: user_file_service_file_access_helper_Move_0008
 * @tc.name: file_access_helper_Move_0008
 * @tc.desc: Test function of Move interface for SUCCESS which move empty folder.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Move_0008, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Move_0008";
    try {
        Uri newDirUriTest1("");
        Uri newDirUriTest2("");
        int result = g_fah->Mkdir(g_newDirUri, "test1", newDirUriTest1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Mkdir(g_newDirUri, "test2", newDirUriTest2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri testUri2("");
        result = g_fah->Move(newDirUriTest1, newDirUriTest2, testUri2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0008 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Move_0008";
}

/**
 * @tc.number: user_file_service_file_access_helper_Move_0009
 * @tc.name: file_access_helper_Move_0009
 * @tc.desc: Test function of Move interface for SUCCESS which move more file in folder.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Move_0009, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Move_0009";
    try {
        Uri newDirUriTest1("");
        Uri newDirUriTest2("");
        int result = g_fah->Mkdir(g_newDirUri, "test1", newDirUriTest1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Mkdir(g_newDirUri, "test2", newDirUriTest2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri testUri("");
        for (size_t i = 0; i < 2000; i++) {
            string fileName = "test" + ToString(i) + ".txt";
            g_fah->CreateFile(newDirUriTest1, fileName, testUri);
        }

        Uri testUri2("");
        result = g_fah->Move(newDirUriTest1, newDirUriTest2, testUri2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0009 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Move_0009";
}

/**
 * @tc.number: user_file_service_file_access_helper_Move_0010
 * @tc.name: file_access_helper_Move_0010
 * @tc.desc: Test function of Move interface for SUCCESS which move Multilevel directory folder.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Move_0010, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Move_0010";
    try {
        Uri newDirUriTest1("");
        Uri newDirUriTest2("");
        int result = g_fah->Mkdir(g_newDirUri, "test1", newDirUriTest1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Mkdir(g_newDirUri, "test2", newDirUriTest2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri testUri("");
        g_fah->Mkdir(newDirUriTest1, "test", testUri);
        for (size_t i = 0; i < 50; i++) {
            g_fah->Mkdir(testUri, "test", testUri);
        }

        Uri testUri2("");
        result = g_fah->Move(newDirUriTest1, newDirUriTest2, testUri2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Move_0010 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Move_0010";
}

void MoveTdd(shared_ptr<FileAccessHelper> fahs, Uri sourceFile, Uri targetParent, Uri newFile)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_Access_helper_MoveTdd";
    int ret = fahs->Move(sourceFile, targetParent, newFile);
    EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
    if (ret != OHOS::FileAccessFwk::ERR_OK) {
        GTEST_LOG_(INFO) << "MoveTdd get result error, code:" << ret;
    }
    EXPECT_NE(newFile.ToString(), "");
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_Access_helper_MoveTdd";
}

/**
 * @tc.number: user_file_service_file_access_helper_Move_0011
 * @tc.name: file_access_helper_Move_0011
 * @tc.desc: Test function of Move interface for SUCCESS which Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Move_0011, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Move_0011";
    try {
        Uri newDirUriTest1("");
        Uri newDirUriTest2("");
        int result = g_fah->Mkdir(g_newDirUri, "test1", newDirUriTest1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        result = g_fah->Mkdir(g_newDirUri, "test2", newDirUriTest2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri testUri[4] = {Uri(""), Uri(""), Uri(""), Uri("")};
        std::string displayName[4] = {"test1.txt", "test2.txt", "test3.txt", "test4.txt"};

        Uri testUri2("");
        for (int j = 0; j < 4; j++) {
            result = g_fah->CreateFile(newDirUriTest1, displayName[j], testUri[j]);
            EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
            std::thread execthread(MoveTdd, g_fah, testUri[j], newDirUriTest2, testUri2);
            execthread.join();
        }

        result = g_fah->Delete(newDirUriTest1);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->Delete(newDirUriTest2);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Move_0011";
}

/**
 * @tc.number: user_file_service_file_access_helper_Rename_0000
 * @tc.name: file_access_helper_Rename_0000
 * @tc.desc: Test function of Rename interface for SUCCESS which rename file.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Rename_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Rename_0000";
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
        GTEST_LOG_(INFO) << "Rename_0000 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Rename_0000";
}

/**
 * @tc.number: user_file_service_file_access_helper_Rename_0001
 * @tc.name: file_access_helper_Rename_0001
 * @tc.desc: Test function of Rename interface for SUCCESS which rename folder.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Rename_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Rename_0001";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri renameUri("");
        result = g_fah->Rename(newDirUriTest, "testRename", renameUri);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Rename_0001 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Rename_0001";
}

/**
 * @tc.number: user_file_service_file_access_helper_Rename_0002
 * @tc.name: file_access_helper_Rename_0002
 * @tc.desc: Test function of Rename interface for ERROR which sourceFileUri is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Rename_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Rename_0002";
    try {
        Uri renameUri("");
        Uri sourceFileUri("");
        int result = g_fah->Rename(sourceFileUri, "testRename.txt", renameUri);
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Rename_0002 result:" << result << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Rename_0002";
}

/**
 * @tc.number: user_file_service_file_access_helper_Rename_0003
 * @tc.name: file_access_helper_Rename_0003
 * @tc.desc: Test function of Rename interface for ERROR which sourceFileUri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Rename_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Rename_0003";
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
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Rename_0003 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Rename_0003";
}

/**
 * @tc.number: user_file_service_file_access_helper_Rename_0004
 * @tc.name: file_access_helper_Rename_0004
 * @tc.desc: Test function of Rename interface for ERROR which sourceFileUri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Rename_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Rename_0004";
    try {
        Uri renameUri("");
        Uri sourceFileUri("~!@#$%^&*()_");
        int result = g_fah->Rename(sourceFileUri, "testRename.txt", renameUri);
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Rename_0004 result:" << result << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Rename_0004";
}

/**
 * @tc.number: user_file_service_file_access_helper_Rename_0005
 * @tc.name: file_access_helper_Rename_0005
 * @tc.desc: Test function of Rename interface for ERROR which displayName is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Rename_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Rename_0005";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest, "test.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri renameUri("");
        result = g_fah->Rename(testUri, "", renameUri);
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Rename_0005 result:" << result << endl;

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Rename_0005";
}

void RenameTdd(shared_ptr<FileAccessHelper> fahs, Uri sourceFile, std::string displayName, Uri newFile)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_Access_helper_RenameTdd";
    int ret = fahs->Rename(sourceFile, displayName, newFile);
    EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
    if (ret != OHOS::FileAccessFwk::ERR_OK) {
        GTEST_LOG_(INFO) << "RenameTdd get result error, code:" << ret;
    }
    EXPECT_NE(newFile.ToString(), "");
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_Access_helper_RenameTdd";
}

/**
 * @tc.number: user_file_service_file_access_helper_Rename_0006
 * @tc.name: file_access_helper_Rename_0006
 * @tc.desc: Test function of Rename interface for SUCCESS which Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_Rename_0006, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_Rename_0006";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri testUri[4] = {Uri(""), Uri(""), Uri(""), Uri("")};
        std::string displayName[4] = {"test1.txt", "test2.txt", "test3.txt", "test4.txt"};
        std::string displayName1[4] = {"test11.txt", "test22.txt", "test33.txt", "test44.txt"};
        Uri renameUri("");
        for (int j = 0; j < 4; j++) {
            result = g_fah->CreateFile(newDirUriTest, displayName[j], testUri[j]);
            EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
            std::thread execthread(RenameTdd, g_fah, testUri[j], displayName1[j], renameUri);
            execthread.join();
        }

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_Rename_0006";
}

/**
 * @tc.number: user_file_service_file_access_helper_ListFile_0000
 * @tc.name: file_access_helper_ListFile_0000
 * @tc.desc: Test function of ListFile interface for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_ListFile_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_ListFile_0000";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_ListFile_0000.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        FileInfo fileInfo;
        fileInfo.uri = newDirUriTest.ToString();
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        FileFilter filter;
        result = g_fah->ListFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_GT(fileInfoVec.size(), 0);
        GTEST_LOG_(INFO) << "ListFile_0000 result:" << fileInfoVec.size() << endl;

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_ListFile_0000";
}

/**
 * @tc.number: user_file_service_file_access_helper_ListFile_0001
 * @tc.name: file_access_helper_ListFile_0001
 * @tc.desc: Test function of ListFile interface for ERROR which Uri is nullptr.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_ListFile_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_ListFile_0001";
    try {
        Uri sourceFileUri("");
        FileInfo fileInfo;
        fileInfo.uri = sourceFileUri.ToString();
        int64_t offset = 0;
        int64_t maxCount = 1000;
        vector<FileAccessFwk::FileInfo> fileInfoVec;
        FileFilter filter({}, {}, {}, 0, 0, false, false);
        int result = g_fah->ListFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_EQ(fileInfoVec.size(), 0);
        GTEST_LOG_(INFO) << "ListFile_0001 result:" << fileInfoVec.size() << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_ListFile_0001";
}

/**
 * @tc.number: user_file_service_file_access_helper_ListFile_0002
 * @tc.name: file_access_helper_ListFile_0002
 * @tc.desc: Test function of ListFile interface for ERROR which sourceFileUri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_ListFile_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_ListFile_0002";
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
        FileFilter filter({}, {}, {}, 0, 0, false, false);
        result = g_fah->ListFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_LT(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_EQ(fileInfoVec.size(), 0);
        GTEST_LOG_(INFO) << "ListFile_0002 result:" << fileInfoVec.size() << endl;

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_ListFile_0002";
}

/**
 * @tc.number: user_file_service_file_access_helper_ListFile_0003
 * @tc.name: file_access_helper_ListFile_0003
 * @tc.desc: Test function of ListFile interface for ERROR which sourceFileUri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_ListFile_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_ListFile_0003";
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
        EXPECT_EQ(fileInfoVec.size(), 0);
        GTEST_LOG_(INFO) << "ListFile_0003 result:" << fileInfoVec.size() << endl;
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_ListFile_0003";
}

/**
 * @tc.number: user_file_service_file_access_helper_ListFile_0004
 * @tc.name: file_access_helper_ListFile_0004
 * @tc.desc: Test function of ListFile interface for ERROR which add filter.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HB855
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_ListFile_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_ListFile_0004";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri testUri1("");
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_ListFile_0004.txt", testUri1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri2("");
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_ListFile_0004.docx", testUri2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        FileInfo fileInfo;
        fileInfo.uri = newDirUriTest.ToString();
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        FileFilter filter({".txt"}, {}, {}, 0, 0, false, true);
        result = g_fah->ListFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_EQ(fileInfoVec.size(), 1);

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_ListFile_0004";
}

void ListFileTdd(shared_ptr<FileAccessHelper> fahs, FileInfo fileInfo, int offset, int maxCount,
    FileFilter filter, std::vector<FileInfo> fileInfoVec)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_Access_helper_ListFileTdd";
    int ret = fahs->ListFile(fileInfo, offset, maxCount, filter, fileInfoVec);
    EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
    if (ret != OHOS::FileAccessFwk::ERR_OK) {
        GTEST_LOG_(INFO) << "ListFileTdd get result error, code:" << ret;
    }
    EXPECT_EQ(fileInfoVec.size(), 1);
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_Access_helper_ListFileTdd";
}

/**
 * @tc.number: user_file_service_file_access_helper_ListFile_0005
 * @tc.name: file_access_helper_ListFile_0005
 * @tc.desc: Test function of ListFile interface for ERROR which Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_ListFile_0005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_ListFile_0005";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "test", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri testUri1("");
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_ListFile_0005.txt", testUri1);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        Uri testUri2("");
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_ListFile_0005.docx", testUri2);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        FileInfo fileInfo;
        fileInfo.uri = newDirUriTest.ToString();
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        FileFilter filter({".txt"}, {}, {}, 0, 0, false, true);
        for (int j = 0; j < 4; j++) {
            std::thread execthread(ListFileTdd, g_fah, fileInfo, offset, maxCount, filter, fileInfoVec);
            execthread.join();
        }

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_ListFile_0005";
}

/**
 * @tc.number: user_file_service_file_access_helper_ScanFile_0000
 * @tc.name: file_access_helper_ScanFile_0000
 * @tc.desc: Test function of ScanFile interface for SUCCESS which scan root directory with filter.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HB866
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_ScanFile_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_ScanFile_0000";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "Download", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri testUri("");
        FileInfo fileInfo;
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_ScanFile_0000.q1w2e3r4", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_ScanFile_0000.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        fileInfo.uri = "datashare:///media/root";
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        FileFilter filter({".q1w2e3r4"}, {}, {}, 0, 0, false, true);
        result = g_fah->ScanFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_GE(fileInfoVec.size(), 1);
        GTEST_LOG_(INFO) << "ScanFile_0000 result:" << fileInfoVec.size() << endl;

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_ScanFile_0000";
}

/**
 * @tc.number: user_file_service_file_access_helper_ScanFile_0001
 * @tc.name: file_access_helper_ScanFile_0001
 * @tc.desc: Test function of ScanFile interface for SUCCESS which scan root directory with no filter.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HB866
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_ScanFile_0001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_ScanFile_0001";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "Download", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri testUri("");
        FileInfo fileInfo;
        fileInfo.uri = "datashare:///media/root";
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_ScanFile_0001.q1w2e3r4", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_ScanFile_0001.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        FileFilter filter({}, {}, {}, 0, 0, false, false);
        result = g_fah->ScanFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_GE(fileInfoVec.size(), 2);
        GTEST_LOG_(INFO) << "ScanFile_0000 result:" << fileInfoVec.size() << endl;

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_ScanFile_0001";
}

/**
 * @tc.number: user_file_service_file_access_helper_ScanFile_0002
 * @tc.name: file_access_helper_ScanFile_0002
 * @tc.desc: Test function of ScanFile interface for SUCCESS which self created directory with filter.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HB866
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_ScanFile_0002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_ScanFile_0002";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "Download", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_ScanFile_0002.q1w2e3r4", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_ScanFile_0000.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_ScanFile_0000.docx", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        FileInfo fileInfo;
        fileInfo.uri = newDirUriTest.ToString();
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        FileFilter filter({".q1w2e3r4"}, {}, {}, 0, 0, false, true);
        result = g_fah->ScanFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_EQ(fileInfoVec.size(), 1);
        FileFilter filter1({".q1w2e3r4", ".txt"}, {}, {}, 0, 0, false, true);
        result = g_fah->ScanFile(fileInfo, offset, maxCount, filter1, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_EQ(fileInfoVec.size(), 2);

        GTEST_LOG_(INFO) << "ScanFile_0002 result:" << fileInfoVec.size() << endl;
        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_ScanFile_0002";
}

/**
 * @tc.number: user_file_service_file_access_helper_ScanFile_0003
 * @tc.name: file_access_helper_ScanFile_0003
 * @tc.desc: Test function of ScanFile interface for SUCCESS which self created directory with filter.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HB866
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_ScanFile_0003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_ScanFile_0003";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "Download", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri testUri("");
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_ScanFile_0003.q1w2e3r4", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        FileInfo fileInfo;
        fileInfo.uri = newDirUriTest.ToString();
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        FileFilter filter({".q1w2e3r4"}, {}, {}, 0, 0, false, true);
        result = g_fah->ScanFile(fileInfo, offset, maxCount, filter, fileInfoVec);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_EQ(fileInfoVec.size(), 1);
        GTEST_LOG_(INFO) << "ScanFile_0003 result:" << fileInfoVec.size() << endl;

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_ScanFile_0003";
}

void ScanFileTdd(shared_ptr<FileAccessHelper> fahs, FileInfo fileInfo, int offset, int maxCount,
    FileFilter filter, std::vector<FileInfo> fileInfoVec)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_ScanFileTdd";
    int ret = fahs->ScanFile(fileInfo, offset, maxCount, filter, fileInfoVec);
    EXPECT_EQ(ret, OHOS::FileAccessFwk::ERR_OK);
    if (ret != OHOS::FileAccessFwk::ERR_OK) {
        GTEST_LOG_(INFO) << "ScanFileTdd get result error, code:" << ret;
    }
    EXPECT_EQ(fileInfoVec.size(), 1);
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_Access_helper_ScanFileTdd";
}

/**
 * @tc.number: user_file_service_file_access_helper_ScanFile_0004
 * @tc.name: file_access_helper_ScanFile_0004
 * @tc.desc: Test function of ScanFile interface for SUCCESS which scan root directory with Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000HB866
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_ScanFile_0004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_ScanFile_0004";
    try {
        Uri newDirUriTest("");
        int result = g_fah->Mkdir(g_newDirUri, "Download", newDirUriTest);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);

        Uri testUri("");
        FileInfo fileInfo;
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_ScanFile_0000.q1w2e3r4", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        result = g_fah->CreateFile(newDirUriTest, "file_access_helper_ScanFile_0000.txt", testUri);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        fileInfo.uri = "datashare:///media/root";
        int64_t offset = 0;
        int64_t maxCount = 1000;
        std::vector<FileInfo> fileInfoVec;
        FileFilter filter({".q1w2e3r4"}, {}, {}, 0, 0, false, true);
        for (int j = 0; j < 4; j++) {
            std::thread execthread(ScanFileTdd, g_fah, fileInfo, offset, maxCount, filter, fileInfoVec);
            execthread.join();
        }

        result = g_fah->Delete(newDirUriTest);
        EXPECT_GE(result, OHOS::FileAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_ScanFile_0004";
}

/**
 * @tc.number: user_file_service_file_access_helper_GetRoots_0000
 * @tc.name: file_access_helper_GetRoots_0000
 * @tc.desc: Test function of GetRoots interface for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 * @tc.level Level 1
 * @tc.require: SR000H0386
 */
HWTEST_F(FileAccessHelperTest, file_access_helper_GetRoots_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FileAccessHelperTest-begin file_access_helper_GetRoots_0000";
    try {
        uint64_t selfTokenId_ = GetSelfTokenID();

        vector<RootInfo> info;
        int result = g_fah->GetRoots(info);
        EXPECT_EQ(result, OHOS::FileAccessFwk::ERR_OK);
        EXPECT_GT(info.size(), 0);

        if (info.size() > 0) {
            GTEST_LOG_(INFO) << info[0].uri;
            GTEST_LOG_(INFO) << info[0].displayName;
            GTEST_LOG_(INFO) << info[0].deviceType;
            GTEST_LOG_(INFO) << info[0].deviceFlags;
        }

        string uri = "datashare:///media/root";
        string displayName = "LOCAL";

        EXPECT_EQ(info[0].uri, uri);
        EXPECT_EQ(info[0].displayName, displayName);
        EXPECT_EQ(info[0].deviceType, DEVICE_LOCAL_DISK);

        SetSelfTokenID(selfTokenId_);
    } catch (...) {
        GTEST_LOG_(INFO) << "FileAccessHelperTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "FileAccessHelperTest-end file_access_helper_GetRoots_0000";
}
} // namespace