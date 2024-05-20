// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 Chris Roberts


#include "BackgroundManager.h"

#include <File.h>
#include <ObjectList.h>
#include <private/app/Server.h>
#include <private/shared/HashMap.h>


#define TRACE _Log(kLogTrace, "%s()", __FUNCTION__);
#define TRACEF(fmt, args...) _Log(kLogTrace, "%s(" fmt ")", __FUNCTION__, args);


enum LogLevel {
	kLogNone = 0,
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

	status_t GetSupportedSuites(BMessage* message);
	BHandler* ResolveSpecifier(BMessage* message, int32 index, BMessage* specifier, int32 what, const char* property);

private:
	status_t _ResetMaps();
	status_t _ResetMessageRunner();
	status_t _RotateBackgrounds();
	status_t _RescanDirectories(int32 workspace);
	status_t _ScanDirectory(int32 workspace, const char* path, bool cachePath);
	status_t _LoadSettings();

	void _ScriptReceived(BMessage* message);
	void _HandleScriptGet(BMessage* message, const char* property);
	void _HandleScriptSet(BMessage* message, const char* property);
	void _HandleScriptCount(BMessage* message, const char* property);
	void _HandleScriptSuites(BMessage* message);

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

#include "WallrusAppImpl.h"
