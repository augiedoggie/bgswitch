// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 Chris Roberts

#include "WallrusApp.h"
#include "toml.hpp"

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <MessageRunner.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <experimental/random>
#include <iomanip>
#include <iostream>


// TODO add scripting commands
enum {
	kRotateWhat = 'ROT8',
	kRunnerWhat = 'MRT8'
};


WallrusApp::WallrusApp() :
	BServer("application/x-vnd.cpr.wallrus", true, nullptr),
	fRotateTime(-1),
	fRotateRunner(nullptr),
	fLogLevel(kLogError)
{
	if (_LoadSettings() != B_OK)
		_Log(kLogError, "Error loading settings!");

	if (fBackgroundManager.InitCheck() != B_OK) {
		_Log(kLogError, "Error intializing background manager!");
		return;
	}

	_Log(kLogTrace, "%s() finished", __FUNCTION__);
}


WallrusApp::~WallrusApp()
{
	TRACE

	_ResetMaps();
}


void
WallrusApp::MessageReceived(BMessage* message)
{
	// TODO log messages?
	switch (message->what) {
		case B_NODE_MONITOR:
			if (message->GetInt32("fields", 0) & B_STAT_MODIFICATION_TIME)
				_LoadSettings();
			break;
		case kRotateWhat:
			_ResetMessageRunner();
			[[fallthrough]];
		case kRunnerWhat:
			_RotateBackgrounds();
			break;
		case B_COUNT_PROPERTIES:
		case B_EXECUTE_PROPERTY:
		case B_GET_PROPERTY:
		case B_SET_PROPERTY:
		case B_GET_SUPPORTED_SUITES:
			if (message->what == B_GET_SUPPORTED_SUITES || message->HasSpecifiers())
				_ScriptReceived(message);
			break;
		default:
			BServer::MessageReceived(message);
	}
}


void
WallrusApp::ReadyToRun()
{
	TRACE

	_RotateBackgrounds();
}


status_t
WallrusApp::_ResetMaps()
{
	TRACE

	// delete fWorkspaceFileMap/fSettingsFolderMap contents
	auto iterator = fWorkspaceFileMap.GetIterator();
	while (iterator.HasNext()) {
		const auto& entry = iterator.Next();
		BObjectList<BString>* list = entry.value;
		list->MakeEmpty();
		delete list;
	}
	fWorkspaceFileMap.Clear();

	iterator = fSettingsFolderMap.GetIterator();
	while (iterator.HasNext()) {
		const auto& entry = iterator.Next();
		BObjectList<BString>* list = entry.value;
		list->MakeEmpty();
		delete list;
	}
	fSettingsFolderMap.Clear();

	return B_OK;
}


status_t
WallrusApp::_ResetMessageRunner()
{
	TRACE

	delete fRotateRunner;

	if (fRotateTime > 0) {
		BMessage rotateMessage(kRunnerWhat);
		fRotateRunner = new BMessageRunner(this, &rotateMessage, fRotateTime * 1000 * 1000);
	}

	return B_OK;
}


status_t
WallrusApp::_RotateBackgrounds()
{
	TRACE

	// iterate through the map and pick a random wallpaper
	auto iterator = fWorkspaceFileMap.GetIterator();
	while (iterator.HasNext()) {
		const auto& entry = iterator.Next();
		// if list is empty then rescan
		BObjectList<BString>* list = entry.value;
		if (list->CountItems() == 0)
			if (_RescanDirectories(entry.key.value) != B_OK)
				continue;

		int32 rand = std::experimental::randint(0, static_cast<int>(list->CountItems() - 1));
		BString* bgString = list->ItemAt(rand);
		// verify file exists
		if (BEntry(bgString->String()).IsFile()) {
			// change background
			fBackgroundManager.SetBackground(bgString->String(), entry.key.value);
			_Log(kLogInfo, "Workspace %" B_PRIi32 " [%" B_PRIi32 " left] %s", entry.key.value, list->CountItems() - 1, bgString->String());
		}
		list->RemoveItemAt(rand);
	}

	fBackgroundManager.Flush();

	return B_OK;
}


status_t
WallrusApp::_RescanDirectories(int32 workspace)
{
	TRACEF("%" B_PRIi32, workspace);

	if (!fSettingsFolderMap.ContainsKey(HashKey32<int32>(workspace)))
		return B_ERROR;

	BObjectList<BString>* folderList = fSettingsFolderMap.Get(HashKey32<int32>(workspace));

	if (folderList->CountItems() == 0)
		return B_ERROR;

	status_t result = B_ERROR;
	for (int32 x = 0; x < folderList->CountItems(); x++) {
		if (_ScanDirectory(workspace, folderList->ItemAt(x)->String(), false) == B_OK)
			// we found at least one valid directory with files
			result = B_OK;
	}

	return result;
}


status_t
WallrusApp::_ScanDirectory(int32 workspace, const char* path, bool cachePath)
{
	TRACEF("%" B_PRIi32 ", \"%s\", %d", workspace, path, cachePath)

	// TODO better sanity check on path
	if (workspace < 1 || workspace > 32 || path == nullptr)
		return B_ERROR;

	BDirectory dir(path);
	if (dir.InitCheck() != B_OK)
		return B_ERROR;

	if (cachePath) {
		// store configured paths in fSettingsFolderMap so we can rescan
		if (!fSettingsFolderMap.ContainsKey(workspace))
			fSettingsFolderMap.Put(workspace,
#if B_HAIKU_VERSION > B_HAIKU_VERSION_1_BETA_5
				new BObjectList<BString>(20));
#else
				new BObjectList<BString>(20, true));
#endif

		BObjectList<BString>* folderList = fSettingsFolderMap.Get(workspace);
		folderList->AddItem(new BString(path));
	}

	if (!fWorkspaceFileMap.ContainsKey(workspace))
		fWorkspaceFileMap.Put(workspace,
#if B_HAIKU_VERSION > B_HAIKU_VERSION_1_BETA_5
			new BObjectList<BString>(20));
#else
			new BObjectList<BString>(20, true));
#endif

	BEntry entry;
	while (dir.GetNextEntry(&entry, true) != B_ENTRY_NOT_FOUND) {
		BPath subPath;
		entry.GetPath(&subPath);

		if (entry.IsDirectory()) {
			// recurse
			_ScanDirectory(workspace, subPath.Path(), cachePath);
			continue;
		}

		// TODO filter out non-images in addition to dot files
		if (subPath.Leaf()[0] == '.')
			continue;

		// add paths to rotation list
		BObjectList<BString>* fileList = fWorkspaceFileMap.Get(workspace);
		fileList->AddItem(new BString(subPath.Path()));

		_Log(kLogDebug, "Workspace %" B_PRIi32 " adding %s", workspace, subPath.Path());
	}

	return B_OK;
}


status_t
WallrusApp::_LoadSettings()
{
	TRACE

	BPath settingsPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath) != B_OK)
		return B_ERROR;

	settingsPath.Append("wallrus.toml");

	BFile settingsFile(settingsPath.Path(), B_READ_ONLY);
	if (settingsFile.InitCheck() != B_OK)
		return B_ERROR;

	// add node monitor to settings file
	stop_watching(this);
	node_ref ref;
	settingsFile.GetNodeRef(&ref);
	// TODO watch for newly created settings files too
	if (watch_node(&ref, B_WATCH_STAT, this) != B_OK)
		_Log(kLogError, "Wallrus: Unable to start node monitoring");

	toml::table tbl;
	try {
		tbl = toml::parse_file(settingsPath.Path());

		std::optional<std::string> logVal = tbl["log_level"].value<std::string>();
		if (logVal.has_value()) {
			if (logVal.value() == "trace")
				fLogLevel = kLogError | kLogInfo | kLogDebug | kLogTrace;
			else if (logVal.value() == "debug")
				fLogLevel = kLogError | kLogInfo | kLogDebug;
			else if (logVal.value() == "info")
				fLogLevel = kLogError | kLogInfo;
			else if (logVal.value() == "none")
				fLogLevel = kLogNone;
			else
				fLogLevel = kLogError;
		}

		// open our log file if needed
		if (fLogLevel != kLogNone && fLogFile.InitCheck() == B_NO_INIT) {
			BPath logPath;
			find_directory(B_SYSTEM_LOG_DIRECTORY, &logPath);
			logPath.Append("wallrus.log");
			if (fLogFile.SetTo(logPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_OPEN_AT_END) == B_OK) {
				_Log(kLogInfo, "Wallrus starting up...");
				TRACE
			} else
				std::cerr << "Error opening log file!" << std::endl;
		} else if (fLogLevel == kLogNone)
			// close log file if needed
			fLogFile.Unset();

		std::optional<int64_t> rtVal = tbl["rotate_time"].value<int64_t>();
		// only reset the message runner if the time has actually changed
		if (rtVal.has_value() && rtVal.value() != fRotateTime) {
			fRotateTime = rtVal.value();
			if (fRotateTime > 0)
				_ResetMessageRunner();
			else {
				// no auto rotate with negative or zero time, disable the runner
				delete fRotateRunner;
				fRotateRunner = nullptr;
			}
		} else if (!rtVal.has_value()) {
			// no auto rotate if there is no time setting, disable the runner
			delete fRotateRunner;
			fRotateRunner = nullptr;
		}

		// clear fWorkspaceFileMap and fSettingsFolderMap
		_ResetMaps();

		toml::table* workspacesTable = tbl["workspaces"].as_table();
		if (workspacesTable != nullptr) {
			workspacesTable->for_each([this](const toml::key& workspace, auto&& paths) {
				if (paths.is_string())
					_ScanDirectory(atol(workspace.data()), paths.as_string()->get().c_str(), true);
				else if (paths.is_array()) {
					toml::array* pathArray = paths.as_array();
					for (auto&& pathElement: *pathArray) {
						if (pathElement.is_string())
							_ScanDirectory(atol(workspace.data()), pathElement.value<std::string>().value().c_str(), true);
					}
				}
			});
		}
	} catch (const toml::parse_error& err) {
		// TODO log exact error message
		_Log(kLogError, "Failed to parse settings file");
		std::cerr << "Parsing failed:" << std::endl;
		std::cerr << err << std::endl;
		return B_ERROR;
	}

	return B_OK;
}


int
main(int /*argc*/, char** /*argv*/)
{
	WallrusApp app;
	app.Run();

	return 0;
}
