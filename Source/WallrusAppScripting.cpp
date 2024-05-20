// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 Chris Roberts


#include "WallrusApp.h"

#include <PropertyInfo.h>
#include <iostream>


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static property_info prop_list[] = {
	{
		"Next",
		{B_EXECUTE_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0},
		"Rotate to next wallpaper",
		0,
		0,
	},
	{
		"Reload",
		{B_EXECUTE_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0},
		"Reload app settings",
		0,
		0,
	},
	{
		"RotateTime",
		{B_GET_PROPERTY, B_SET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0},
		"Get/Set background rotation time(in seconds)",
		0,
		{B_INT32_TYPE},
	},
	{
		"LoggingLevel",
		{B_GET_PROPERTY, B_SET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0},
		"Get/Set logging level for the app",
		0,
		{B_STRING_TYPE},
	},
	{
		"Workspace",
		{B_GET_SUPPORTED_SUITES, 0},
		{B_INDEX_SPECIFIER, 0},
		"Get information for a workspace",
		0,
		0,
	},
	{
		"Workspaces",
		{B_COUNT_PROPERTIES, 0},
		{B_DIRECT_SPECIFIER, 0},
		"Count # of active workspaces",
		0,
		{B_INT32_TYPE},
	},
	{0}};


static property_info ws_prop_list[] = {
	{
		"Background",
		{B_GET_PROPERTY, B_SET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0},
		"Get/Set background for a workspace",
		0,
		{B_STRING_TYPE},
	},
	{
		"TextOutline",
		{B_GET_PROPERTY, B_SET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0},
		"Get/Set text outline for a workspace",
		0,
		{B_BOOL_TYPE},
	},
	{
		"Placement",
		{B_GET_PROPERTY, B_SET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0},
		"Get/Set background placement mode for a workspace",
		0,
		{B_INT32_TYPE},
	},
	{
		"Offset",
		{B_GET_PROPERTY, B_SET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0},
		"Get/Set background offset for a workspace",
		0,
		{B_POINT_TYPE},
	},
	{
		"Color",
		{B_GET_PROPERTY, B_SET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0},
		"Get/Set background color for a workspace",
		0,
		{B_RGB_COLOR_TYPE},
	},
	{
		"Paths",
		{B_COUNT_PROPERTIES, 0},
		{B_DIRECT_SPECIFIER, 0},
		"Count # of rotation paths for this workspace",
		0,
		{B_INT32_TYPE},
	},
	{
		"Path",
		{B_GET_PROPERTY, 0},
		{B_INDEX_SPECIFIER, 0},
		"Get rotation path #X for this workspace",
		0,
		{B_INT32_TYPE},
	},
//	{
//		"Suites",
//		{B_GET_PROPERTY, 0},
//		{B_DIRECT_SPECIFIER, 0},
//		nullptr, 0,
//		{B_PROPERTY_INFO_TYPE}
//	},
	{0}};
#pragma GCC diagnostic pop


status_t
WallrusApp::GetSupportedSuites(BMessage* message)
{
	TRACE

	// message->PrintToStream();

	BPropertyInfo info(prop_list);
	message->AddFlat("messages", &info);
	message->AddString("suites", "suite/x-vnd.cpr.wallrus");

	return BServer::GetSupportedSuites(message);
}


BHandler*
WallrusApp::ResolveSpecifier(BMessage* message, int32 index, BMessage* specifier, int32 what, const char* property)
{
	// TODO format 'what' properly
	TRACEF("..., %" B_PRIi32 ", ..., %" B_PRIi32 "('%.4s'), \"%s\"", index, what, reinterpret_cast<char*>(&what), property)

	// message->PrintToStream();
	// specifier->PrintToStream();

	BPropertyInfo prop_info(prop_list);
	if (prop_info.FindMatch(message, index, specifier, what, property) >= 0)
		return this;

	if (strcmp(property, "Workspace") == 0 && specifier->HasInt32("index")) {
		message->SetCurrentSpecifier(0);
		int32 specIndex = 0;
		int32 specWhat = 0;
		BMessage specMessage;
		const char* specProperty = nullptr;
		message->GetCurrentSpecifier(&specIndex, &specMessage, &specWhat, &specProperty);
		BPropertyInfo ws_prop_info(ws_prop_list);
		if (ws_prop_info.FindMatch(message, specIndex, &specMessage, specWhat, specProperty) >= 0)
			return this;
	}

	return BServer::ResolveSpecifier(message, index, specifier, what, property);
}


void
WallrusApp::_ScriptReceived(BMessage* message)
{
	TRACE

	// message->PrintToStream();

	// TODO is specifier required in GetCurrentSpecifier()?
	BMessage specifier;
	const char* property;
	if (message->GetCurrentSpecifier(nullptr, &specifier, nullptr, &property) != B_OK)
		// a getsuites request for the main app can be passed on
		return BServer::MessageReceived(message);

	switch (message->what) {
		case B_COUNT_PROPERTIES:
			_HandleScriptCount(message, property);
			break;
		case B_GET_PROPERTY:
			_HandleScriptGet(message, property);
			break;
		case B_GET_SUPPORTED_SUITES:
			_HandleScriptSuites(message);
			break;
		case B_EXECUTE_PROPERTY:
		{
			_Log(kLogInfo, "Executing '%s' command...", property);
			// check for individual commands to execute
			if (strcmp(property, "Next") == 0) {
				_ResetMessageRunner();
				_RotateBackgrounds();
			} else if (strcmp(property, "Reload") == 0) {
				// TODO reset all settings to defaults before loading
				_LoadSettings();
				// TODO switch to next wallpaper?
			}
			BMessage reply(B_REPLY);
			reply.AddInt32("error", B_OK);
			message->SendReply(&reply);
		} break;
		case B_SET_PROPERTY:
			_HandleScriptSet(message, property);
			break;
		default:
			return BServer::MessageReceived(message);
	}
}


void
WallrusApp::_HandleScriptSet(BMessage* message, const char* property)
{
	TRACEF("..., \"%s\"", property)

	message->PrintToStream();
}


void
WallrusApp::_HandleScriptGet(BMessage* message, const char* property)
{
	TRACEF("..., \"%s\"", property)

	BMessage specifier;
	if (message->GetCurrentSpecifier(nullptr, &specifier, nullptr, nullptr) != B_OK)
		return;

	BMessage reply(B_REPLY);
	// handle top level application script requests
	if (strcmp(property, "RotateTime") == 0) {
		reply.AddInt32("result", fRotateTime);
		reply.AddInt32("error", B_OK);
		message->SendReply(&reply);
		return;
	} else if (strcmp(property, "LoggingLevel") == 0) {
		if (fLogLevel & kLogTrace)
			reply.AddString("result", "trace");
		else if (fLogLevel & kLogError)
			reply.AddString("result", "error");
		else if (fLogLevel & kLogInfo)
			reply.AddString("result", "info");
		else if (fLogLevel & kLogDebug)
			reply.AddString("result", "debug");
		else if (fLogLevel & kLogTrace)
			reply.AddString("result", "trace");

		reply.AddInt32("error", B_OK);
		message->SendReply(&reply);
		return;
	}

	// handle script requests for 'Workspace XX'

	message->SetCurrentSpecifier(1);
	if (message->GetCurrentSpecifier(nullptr, &specifier, nullptr, nullptr) != B_OK)
		return;

	int32 workspace = specifier.GetInt32("index", -1);
	// TODO better check if workspace is valid
	if (workspace < 0)
		// TODO reply with index error?
		return;

	BString bgPath;
	int32 bgMode = -1;
	BPoint bgOffset;
	bool bgErase = false;
	rgb_color bgColor = {0, 0, 0, 0};
	// TODO check for errors from GetWorkspaceInfo()
	fBackgroundManager.GetWorkspaceInfo(workspace, bgPath, &bgMode, &bgOffset, &bgErase, &bgColor);

	if (strcmp(property, "Background") == 0) {
		reply.AddString("result", bgPath);
	} else if (strcmp(property, "TextOutline") == 0) {
		reply.AddBool("result", bgErase);
	} else if (strcmp(property, "Placement") == 0) {
		reply.AddInt32("result", bgMode);
	} else if (strcmp(property, "Offset") == 0) {
		reply.AddPoint("result", bgOffset);
	} else if (strcmp(property, "Color") == 0) {
		reply.AddColor("result", bgColor);
	} else {
		message->PrintToStream();
		return;
	}

	reply.AddInt32("error", B_OK);
	message->SendReply(&reply);
}


void
WallrusApp::_HandleScriptSuites(BMessage* message)
{
	TRACE

	BPropertyInfo info(ws_prop_list);
	BMessage reply(B_REPLY);
	if (reply.AddFlat("messages", &info) == B_OK)
		message->SendReply(&reply);
}


void
WallrusApp::_HandleScriptCount(BMessage* message, const char* property)
{
	TRACEF("..., \"%s\"", property)

	// handle application level script requests
	BMessage reply(B_REPLY);
	if (strcmp(property, "Workspaces") == 0) {
		reply.AddInt32("error", B_OK);
		reply.AddInt32("result", count_workspaces());
		message->SendReply(&reply);
		return;
	}

	// handle workspace level script requests
	BMessage specMessage;
	message->SetCurrentSpecifier(1);
	if (message->GetCurrentSpecifier(nullptr, &specMessage, nullptr, nullptr) != B_OK)
		return;

	int32 workspace = specMessage.GetInt32("index", -1);
	// TODO better check if workspace is valid
	if (workspace < 0)
		// TODO reply with index error?
		return;

	if (strcmp(property, "Paths") == 0) {
		if (fSettingsFolderMap.ContainsKey(HashKey32<int32>(workspace)))
			reply.AddInt32("result", fSettingsFolderMap.Get(HashKey32<int32>(workspace))->CountItems());
		// TODO else reply with error
	} else {
		reply.what = B_MESSAGE_NOT_UNDERSTOOD;
		reply.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
	}
	message->SendReply(&reply);
}


#if 0
void
WallrusApp::_GetClipInformation(BMessage* msg)
{
	BMessage reply(B_REPLY), specMsg;

	BString propName;
	if (msg->GetCurrentSpecifier(NULL, &specMsg) != B_OK || specMsg.FindString("property", &propName) != B_OK) {
		reply.what = B_MESSAGE_NOT_UNDERSTOOD;
		reply.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
		msg->SendReply(&reply);
		return;
	}

	bool favorite = false;
	if (propName == "Favorite")
		favorite = true;

	if (specMsg.HasInt32("index")) {
		int32 clipIndex = 0;
		specMsg.FindInt32("index", &clipIndex);

		fMainWindow->Lock();
		BListItem* clipItem = favorite
								? fMainWindow->fFavorites->ItemAt(clipIndex)
								: fMainWindow->fHistory->ItemAt(clipIndex);

		if (clipItem == NULL) {
			reply.what = B_MESSAGE_NOT_UNDERSTOOD;
			reply.AddString("message", "Index out of range");
			reply.AddInt32("error", B_BAD_INDEX);
			msg->SendReply(&reply);
			fMainWindow->Unlock();
			return;
		}

		BString clipString = favorite
								? dynamic_cast<FavItem*>(clipItem)->GetClip()
								: dynamic_cast<ClipItem*>(clipItem)->GetClip();

		fMainWindow->Unlock();

		reply.AddString("result", clipString);

	} else if (specMsg.HasString("name")) {
		BString clipName;
		specMsg.FindString("name", &clipName);


		//TODO search for clip, return an error for now
//		reply.AddString("result", clipString);
		reply.what = B_MESSAGE_NOT_UNDERSTOOD;
		reply.AddString("message", "Name not found");
		reply.AddInt32("error", B_NAME_NOT_FOUND);
		msg->SendReply(&reply);
		return;
	} else {
		reply.what = B_MESSAGE_NOT_UNDERSTOOD;
		reply.AddString("message", "No index or name found");
		reply.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
		msg->SendReply(&reply);
		return;
	}

	reply.AddInt32("error", B_OK);
	msg->SendReply(&reply);
}
#endif
