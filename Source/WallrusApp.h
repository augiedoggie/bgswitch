// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 Chris Roberts


#include "BackgroundManager.h"

#include <File.h>
#include <ObjectList.h>
#include <private/app/Server.h>
#include <private/shared/HashMap.h>


enum LogLevel {
	kLogError = 1,
	kLogInfo = 2,
	kLogDebug = 4,
	kLogTrace = 8
};


class WallrusApp : public BServer {
public:
	WallrusApp();
	~WallrusApp();
	virtual void MessageReceived(BMessage* message);
	virtual void ReadyToRun();

private:
	status_t _ResetMaps();
	status_t _ResetMessageRunner();
	status_t _RotateBackgrounds();
	status_t _RescanDirectories(int32 workspace);
	status_t _ScanDirectory(int32 workspace, const char* path, bool cachePath);
	status_t _LoadSettings();

	template<typename... Args>
	status_t _Log(LogLevel level, const char* message, Args...);

	BackgroundManager fBackgroundManager;
	bigtime_t fRotateTime;
	BMessageRunner* fRotateRunner;
	HashMap<HashKey32<int32>, BObjectList<BString>*> fWorkspaceFileMap;
	HashMap<HashKey32<int32>, BObjectList<BString>*> fSettingsFolderMap;
	BFile fLogFile;
	int32 fLogLevel;
};
