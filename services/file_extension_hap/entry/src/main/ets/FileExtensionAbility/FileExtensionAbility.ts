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
import Extension from '@ohos.application.FileExtensionAbility'
import fileio from '@ohos.fileio'
import hilog from '@ohos.hilog'
import { init, delVolumeInfo, getVolumeInfoList } from './VolumeManager'
import { onReceiveEvent } from './Subcriber'
import fileExtensionInfo from "@ohos.fileExtensionInfo"
import appManager from '@ohos.application.appManager';

const FLAG = fileExtensionInfo.FLAG;
const BUNDLE_NAME = 'com.ohos.UserFile.ExternalFileManager';

export default class FileExtAbility extends Extension {
    onCreate(want) {
        init();
        onReceiveEvent(function (data) {
            if (data.event == 'usual.event.data.VOLUME_MOUNTED') {
                appManager.killProcessesByBundleName(BUNDLE_NAME);
            } else {
                delVolumeInfo(data.parameters.id);
            }
        });
    }

    getPath(uri) {
        let sep = '://';
        let arr = uri.split(sep);
        if (arr.length < 2) {
            return uri;
        }
        let path = uri.replace(arr[0] + sep, '');
        if (arr[1].indexOf('/') > 0) {
            path = path.replace(arr[1].split('/')[0], '');
        }
        return path;
    }

    genNewFileUri(uri, displayName) {
        let arr = uri.split('/');
        let newFileUri = uri;
        if (arr.pop() == '') {
            newFileUri += displayName;
        } else {
            newFileUri += '/' + displayName;
        }
        return newFileUri;
    }

    getFileName(uri) {
        let arr = uri.split('/');
        let name = arr.pop();
        if (name == '') {
            name = arr.pop();
        }
        return name;
    }

    renameUri(uri, displayName) {
        let arr = uri.split('/');
        let newFileUri = '';
        if (arr.pop() == '') {
            arr.pop();
            arr.push(displayName)
            arr.push('');
        } else {
            arr.push(displayName)
        }
        for (var index = 0; index < arr.length; index++) {
            if (arr[index] == '') {
                newFileUri += '/';
            } else if (index == arr.length - 1) {
                newFileUri += arr[index];
            } else {
                newFileUri += arr[index] + '/';
            }
        }
        return newFileUri;
    }

    listDir(path, cb) {
        try {
            let stat = fileio.statSync(path);
            if (stat.isDirectory()) {
                let dir = fileio.opendirSync(path);
                let hasNextFile = true;
                while (hasNextFile) {
                    try {
                        let dirent = dir.readSync();
                        this.listDir(path + '/' + dirent.name, cb);
                    } catch (e) {
                        hilog.debug(0x0001, 'jsserver', 'listDir dir.readSync catch' + e);
                        hasNextFile = false;
                        cb(path, true);
                    }
                }
            } else {
                cb(path, false);
            }
        } catch (e) {
            hilog.debug(0x0001, 'jsserver', 'listDir catch ' + e + ' ' + path);
            cb(path, true);
        }
    }

    openFile(sourceFileUri, flags) {
        let fd = 0;
        try {
            let path = this.getPath(sourceFileUri);
            fd = fileio.openSync(path, flags, 0o666);
        } catch (e) {
            hilog.debug(0x0001, 'jsserver', 'openFile catch' + e);
            fd = -1;
        }

        return fd;
    }

    closeFile(fd, uri) {
        try {
            fileio.closeSync(fd);
            return true;
        } catch (e) {
            hilog.debug(0x0001, 'jsserver', 'closeFile catch' + e);
            return false;
        }
    }

    createFile(parentUri, displayName) {
        try {
            let newFileUri = this.genNewFileUri(parentUri, displayName);
            let path = this.getPath(newFileUri);
            fileio.openSync(path, 0o100, 0o666);
            return newFileUri;
        } catch (e) {
            hilog.debug(0x0001, 'jsserver', 'createFile catch' + e);
            return '';
        }
    }

    mkdir(parentUri, displayName) {
        try {
            let newFileUri = this.genNewFileUri(parentUri, displayName);
            let path = this.getPath(newFileUri);
            fileio.mkdirSync(path);
            return newFileUri;
        } catch (e) {
            hilog.debug(0x0001, 'jsserver', 'mkdir catch' + e);
            return '';
        }
    }

    delete(selectFileUri) {
        let path = this.getPath(selectFileUri);
        let code = 0;
        this.listDir(path, function (filePath, isDirectory) {
            try {
                if (isDirectory) {
                    fileio.rmdirSync(filePath);
                } else {
                    fileio.unlinkSync(filePath);
                }
            } catch (e) {
                hilog.debug(0x0001, 'jsserver', 'delete catch' + e);
                code = -1;
            }
        });

        return code;
    }

    move(sourceFileUri, targetParentUri) {
        try {
            let displayName = this.getFileName(sourceFileUri);
            let newFileUri = this.genNewFileUri(targetParentUri, displayName);
            let oldPath = this.getPath(sourceFileUri);
            let newPath = this.getPath(newFileUri);
            fileio.renameSync(oldPath, newPath);
            return newFileUri;
        } catch (e) {
            hilog.debug(0x0001, 'jsserver', 'rename catch' + e);
            return '';
        }
    }

    rename(sourceFileUri, displayName) {
        try {
            let newFileUri = this.renameUri(sourceFileUri, displayName);
            let oldPath = this.getPath(sourceFileUri);
            let newPath = this.getPath(newFileUri);
            fileio.renameSync(oldPath, newPath);
            return newFileUri;
        } catch (e) {
            hilog.debug(0x0001, 'jsserver', 'rename catch' + e);
            return '';
        }
    }

    query(sourceFileUri) {
        try {
            let path = this.getPath(sourceFileUri);
            let stat = fileio.statSync(path);
            return {
                uri: sourceFileUri,
                fileName: this.getFileName(sourceFileUri),
                mode: '' + stat.mode,
                size: stat.size,
                mtime: stat.mtime,
                mimeType: '',
            };
        } catch (e) {
            hilog.debug(0x0001, 'jsserver', 'query catch' + e);
            return null;
        }
    }

    listFile(sourceFileUri) {
        let infos = [];
        try {
            let path = this.getPath(sourceFileUri);
            let dir = fileio.opendirSync(path);
            let hasNextFile = true;
            while (hasNextFile) {
                try {
                    let dirent = dir.readSync();
                    let stat = fileio.statSync(path + '/' + dirent.name);
                    infos.push({
                        uri: this.genNewFileUri(sourceFileUri, dirent.name),
                        fileName: dirent.name,
                        mode: '' + stat.mode,
                        size: stat.size,
                        mtime: stat.mtime,
                        mimeType: '',
                    });
                } catch (e) {
                    hilog.debug(0x0001, 'jsserver', 'dir.readSync catch' + e);
                    hasNextFile = false;
                }
            }
        } catch (e) {
            hilog.debug(0x0001, 'jsserver', 'listFile catch' + e);
        }

        return infos;
    }

    getRoots() {
        let roots = getVolumeInfoList().concat({
            uri: 'fileAccess:///data/storage/el1/bundle/storage_daemon',
            displayName: 'storage_daemon',
            deviceId: '',
            flags: FLAG.SUPPORTS_WRITE | FLAG.SUPPORTS_DELETE | FLAG.SUPPORTS_RENAME | FLAG.SUPPORTS_COPY
                | FLAG.SUPPORTS_MOVE | FLAG.SUPPORTS_REMOVE | FLAG.DIR_SUPPORTS_CREATE | FLAG.DIR_PREFERS_LAST_MODIFIED,
        });
        return roots;
    }
};