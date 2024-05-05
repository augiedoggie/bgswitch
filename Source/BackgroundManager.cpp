// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 Chris Roberts


#include "BackgroundManager.h"

#include <FindDirectory.h>
#include <Messenger.h>
#include <Node.h>
#include <Path.h>
#include <Screen.h>
#include <String.h>
#include <be_apps/Tracker/Background.h>
#include <iostream>
#include <kernel/fs_attr.h>
#include <private/shared/AutoDeleter.h>

#define BACKGROUND_SET "be:bgndimginfoset"


BackgroundManager::BackgroundManager(const char* path)
	:
	fBackgroundMessage(nullptr),
	fFolderNode(nullptr),
	fInitStatus(B_NO_INIT),
	fDirtyMessage(false)
{
	BPath folderPath;
	if (path == nullptr) {
		if (find_directory(B_DESKTOP_DIRECTORY, &folderPath) != B_OK) {
			std::cerr << "Error: unable to find B_DESKTOP_DIRECTORY" << std::endl;
			return;
		}
	} else
		folderPath.SetTo(path);

	fFolderNode = new BNode(folderPath.Path());
	if (fFolderNode->InitCheck() != B_OK) {
		// TODO better message for file not found
		std::cerr << "Error: unable to create BNode for folder" << std::endl;
		return;
	}

	attr_info info;
	if (fFolderNode->GetAttrInfo(B_BACKGROUND_INFO, &info) != B_OK)
		return;

	char* buffer = new char[info.size];
	ArrayDeleter<char> _(buffer);
	ssize_t bytesRead = fFolderNode->ReadAttr(B_BACKGROUND_INFO, B_MESSAGE_TYPE, 0, buffer, info.size);

	if (bytesRead != info.size) {
		std::cerr << "Error: unable to read be:bgndimginfo from node" << std::endl;
		return;
	}

	fBackgroundMessage = new BMessage();
	if (fBackgroundMessage->Unflatten(buffer) != B_OK) {
		std::cerr << "Error: unable to unflatten message" << std::endl;
		return;
	}

	fInitStatus = B_OK;
}


BackgroundManager::~BackgroundManager()
{
	delete fBackgroundMessage;
	delete fFolderNode;
}


status_t
BackgroundManager::InitCheck()
{
	return fInitStatus;
}


status_t
BackgroundManager::_RemoveWorkspaceIndex(int32 workspace)
{
	int32 messageIndex = _FindWorkspaceIndex(workspace, false);
	// we don't allow removing workspace 0
	if (messageIndex < B_OK || workspace == 0) {
		std::cerr << "Error: invalid workspace #" << std::endl;
		return B_BAD_VALUE;
	}

	int32 setWorkspaces = fBackgroundMessage->GetInt32(B_BACKGROUND_WORKSPACES, messageIndex, 0);
	// clear the bit for this workspace
	setWorkspaces &= ~(1 << (workspace - 1));
	if (setWorkspaces != 0) {
		// some other workspace is using this index, keep it around
		fBackgroundMessage->ReplaceInt32(B_BACKGROUND_WORKSPACES, messageIndex, setWorkspaces);
	} else {
		// the index is no longer used, remove it
		fBackgroundMessage->RemoveData(B_BACKGROUND_WORKSPACES, messageIndex);
		fBackgroundMessage->RemoveData(B_BACKGROUND_IMAGE, messageIndex);
		fBackgroundMessage->RemoveData(B_BACKGROUND_MODE, messageIndex);
		fBackgroundMessage->RemoveData(B_BACKGROUND_ORIGIN, messageIndex);
		fBackgroundMessage->RemoveData(B_BACKGROUND_ERASE_TEXT, messageIndex);
		fBackgroundMessage->RemoveData(BACKGROUND_SET, messageIndex);
	}

	if (fBackgroundMessage->FindInt32(B_BACKGROUND_WORKSPACES, 0, &setWorkspaces) != B_OK) {
		std::cerr << "Error: Unable to find B_BACKGROUND_WORKSPACES BMessage" << std::endl;
		return B_ERROR;
	}

	// tell Tracker that this workspace uses the global background
	setWorkspaces |= (1 << (workspace - 1));
	if (fBackgroundMessage->ReplaceInt32(B_BACKGROUND_WORKSPACES, 0, setWorkspaces) != B_OK) {
		std::cerr << "Error: Unable to replace B_BACKGROUND_WORKSPACES BMessage" << std::endl;
		return B_ERROR;
	}

	return B_OK;
}


int32
BackgroundManager::_FindWorkspaceIndex(int32 workspace, bool create)
{
	if (workspace < 0 || workspace > 32) {
		std::cerr << "Error: invalid workspace #" << std::endl;
		return B_BAD_VALUE;
	}

	// we're adjusting the global/default background
	if (workspace == 0)
		return 0;

	int32 countFound = 0;
	fBackgroundMessage->GetInfo(B_BACKGROUND_WORKSPACES, nullptr, &countFound);
	for (int32 x = 1; x < countFound; x++) {
		int32 workspaces = 0;
		if (fBackgroundMessage->FindInt32(B_BACKGROUND_WORKSPACES, x, &workspaces) != B_OK) {
			std::cerr << "Error: Unable to find B_BACKGROUND_WORKSPACES BMessage" << std::endl;
			return B_ERROR;
		}

		// check if our workspace bit is set in this index
		if ((1 << (workspace - 1)) & workspaces)
			return x;
	}

	// return an error if we didn't find it because we have a global background or a single workspace or ...
	if (!create)
		return B_ERROR;

	// we're switching from global to a locally set background, find the next free index
	return _CreateWorkspaceIndex(workspace);
}


int32
BackgroundManager::_CreateWorkspaceIndex(int32 workspace)
{
	// start at 1 because we don't allow creating workspace 0
	if (workspace < 1 || workspace > 32) {
		std::cerr << "Error: invalid workspace #" << std::endl;
		return B_BAD_VALUE;
	}

	int32 setWorkspaces = 0;
	if (fBackgroundMessage->FindInt32(B_BACKGROUND_WORKSPACES, 0, &setWorkspaces) != B_OK) {
		std::cerr << "Error: Unable to find B_BACKGROUND_WORKSPACES BMessage" << std::endl;
		return B_ERROR;
	}

	// TODO verify the new workspace is in setWorkspaces before trying to clear the bit

	// tell Tracker that this workspace has a custom background
	setWorkspaces &= ~(1 << (workspace - 1));
	if (fBackgroundMessage->ReplaceInt32(B_BACKGROUND_WORKSPACES, 0, setWorkspaces) != B_OK) {
		std::cerr << "Error: Unable to replace B_BACKGROUND_WORKSPACES BMessage" << std::endl;
		return B_ERROR;
	}

	int32 countFound = B_ERROR;
	fBackgroundMessage->GetInfo(B_BACKGROUND_WORKSPACES, nullptr, &countFound);
	// start at 2 because there should already be at least one index for workspace 0
	if (countFound < 2)
		return B_ERROR;

	// get our default values from index 0
	fBackgroundMessage->AddInt32(B_BACKGROUND_WORKSPACES, (1 << (workspace - 1)));
	fBackgroundMessage->AddString(B_BACKGROUND_IMAGE, fBackgroundMessage->GetString(B_BACKGROUND_IMAGE, 0, ""));
	fBackgroundMessage->AddInt32(B_BACKGROUND_MODE, fBackgroundMessage->GetInt32(B_BACKGROUND_MODE, 0, B_BACKGROUND_MODE_SCALED));
	fBackgroundMessage->AddPoint(B_BACKGROUND_ORIGIN, fBackgroundMessage->GetPoint(B_BACKGROUND_ORIGIN, 0, BPoint(0, 0)));
	fBackgroundMessage->AddBool(B_BACKGROUND_ERASE_TEXT, fBackgroundMessage->GetBool(B_BACKGROUND_ERASE_TEXT, 0, true));
	fBackgroundMessage->AddInt32(BACKGROUND_SET, 0);

	// countFound will be our new message index
	return countFound;
}


status_t
BackgroundManager::_WriteMessage()
{
	char* flat = new char[fBackgroundMessage->FlattenedSize()];
	ArrayDeleter<char> _(flat);

	if (fBackgroundMessage->Flatten(flat, fBackgroundMessage->FlattenedSize()) != B_OK) {
		std::cerr << "Error: unable to flatten new background message" << std::endl;
		return B_ERROR;
	}

	if (fFolderNode->WriteAttr(B_BACKGROUND_INFO, B_MESSAGE_TYPE, 0, flat, fBackgroundMessage->FlattenedSize()) < B_OK) {
		std::cerr << "Error: unable to write message to node" << std::endl;
		return B_ERROR;
	}
	return B_OK;
}


status_t
BackgroundManager::GetWorkspaceInfo(int32 workspace, BString& path, int32* mode, BPoint* offset, bool* erase, rgb_color* color)
{
	int32 messageIndex = _FindWorkspaceIndex(workspace);
	// use the global defaults if we're asked for info of a workspace without customization
	if (messageIndex < 0)
		messageIndex = 0;

	if (fBackgroundMessage->FindString(B_BACKGROUND_IMAGE, messageIndex, &path) != B_OK) {
		// message index has been deleted?
		std::cerr << "File: Index not found in BMessage!" << std::endl;
		return B_ERROR;
	}

	if (mode != nullptr && fBackgroundMessage->FindInt32(B_BACKGROUND_MODE, messageIndex, mode) != B_OK)
		std::cerr << "Background Mode: Not found, using default!" << std::endl;

	if (offset != nullptr && fBackgroundMessage->FindPoint(B_BACKGROUND_ORIGIN, messageIndex, offset) != B_OK)
		std::cerr << "Offset: Not found, using default!" << std::endl;

	if (erase != nullptr && fBackgroundMessage->FindBool(B_BACKGROUND_ERASE_TEXT, messageIndex, erase) != B_OK)
		std::cerr << "Text Outline: Not found, using default!" << std::endl;

	if (workspace != 0 && color != nullptr && BScreen().IsValid())
		*color = BScreen().DesktopColor(workspace - 1);

	return B_OK;
}


status_t
BackgroundManager::PrintBackgroundToStream(int32 workspace, bool verbose)
{
	BString path;
	int32 mode = B_BACKGROUND_MODE_SCALED;
	BPoint offset;
	bool erase = true;
	rgb_color color;

	if (GetWorkspaceInfo(workspace, path, &mode, &offset, &erase, &color) != B_OK)
		return B_ERROR;

	if (verbose) {
		std::cout << "Workspace: " << workspace;
		if (workspace == 0)
			std::cout << " *Global Default*";
		else if (_FindWorkspaceIndex(workspace) < B_OK)
			std::cout << " **Warning: No custom settings.  Showing Global Defaults**";
		std::cout << std::endl;
	} else {
		// mark the global workspace and workspaces which are using it with an asterisk
		std::cout << workspace;
		if (workspace == 0 || _FindWorkspaceIndex(workspace) < B_OK)
			std::cout << "*";
		std::cout << ": ";
	}

	if (path.IsEmpty()) {
		if (verbose)
			std::cout << "File: No background set!" << std::endl;
		else {
			std::cout << "<none>" << std::endl;
			return B_OK;
		}
	} else if (!verbose) {
		std::cout << path << std::endl;
		return B_OK;
	} else {
		std::cout << "File: " << path << std::endl;
	}

	switch (mode) {
		case B_BACKGROUND_MODE_USE_ORIGIN:
			std::cout << "Mode: Use Origin" << std::endl;
			break;
		case B_BACKGROUND_MODE_CENTERED:
			std::cout << "Mode: Centered" << std::endl;
			break;
		case B_BACKGROUND_MODE_SCALED:
			std::cout << "Mode: Scaled" << std::endl;
			break;
		case B_BACKGROUND_MODE_TILED:
			std::cout << "Mode: Tiled" << std::endl;
			break;
		default:
			std::cout << "Mode: Unknown" << std::endl;
			break;
	}

	std::cout << "Offset: X=" << offset.x << " Y=" << offset.y << std::endl;

	std::cout << "Text Outline: " << (erase ? "true" : "false") << std::endl;

	if (workspace != 0)
		std::cout << "Background Color: {r:" << +color.red << ",g:" << +color.green << ",b:" << +color.blue << "}" << std::endl;

	std::cout << std::endl;

	return B_OK;
}


status_t
BackgroundManager::ResetWorkspace(int32 workspace)
{
	int32 messageIndex = _FindWorkspaceIndex(workspace);
	if (messageIndex < B_OK)
		return messageIndex;

	if (status_t status = _RemoveWorkspaceIndex(workspace) != B_OK)
		return status;

	fDirtyMessage = true;
	return B_OK;
}


status_t
BackgroundManager::SetBackground(const char* imagePath, int32 workspace)
{
	int32 messageIndex = _FindWorkspaceIndex(workspace, true);
	if (messageIndex < B_OK)
		return messageIndex;

	// verify the file exists if we were given a non-empty path
	if (imagePath != nullptr && strcmp(imagePath, "") != 0) {
		BEntry newWallEntry(imagePath);
		if (newWallEntry.InitCheck() != B_OK || !newWallEntry.Exists() || !newWallEntry.IsFile()) {
			std::cerr << "Error: invalid file path" << std::endl;
			return B_ERROR;
		}
	}

	if (fBackgroundMessage->ReplaceString(B_BACKGROUND_IMAGE, messageIndex, imagePath == nullptr ? "" : imagePath) != B_OK) {
		std::cerr << "Error: unable to replace background image path in BMessage" << std::endl;
		return B_ERROR;
	}

	fDirtyMessage = true;
	return B_OK;
}


status_t
BackgroundManager::SetPlacement(int32 mode, int32 workspace)
{
	// verify we were given a valid mode
	if (mode != B_BACKGROUND_MODE_USE_ORIGIN && mode != B_BACKGROUND_MODE_CENTERED
		&& mode != B_BACKGROUND_MODE_SCALED && mode != B_BACKGROUND_MODE_TILED) {
		std::cerr << "Error: invalid placement mode" << std::endl;
		return B_BAD_VALUE;
	}

	int32 messageIndex = _FindWorkspaceIndex(workspace, true);
	if (messageIndex < B_OK)
		return messageIndex;

	if (fBackgroundMessage->ReplaceInt32(B_BACKGROUND_MODE, messageIndex, mode) != B_OK) {
		std::cerr << "Error: unable to replace background mode in BMessage" << std::endl;
		return B_ERROR;
	}

	fDirtyMessage = true;
	return B_OK;
}


status_t
BackgroundManager::SetOutline(bool enabled, int32 workspace)
{
	int32 messageIndex = _FindWorkspaceIndex(workspace, true);
	if (messageIndex < B_OK)
		return messageIndex;

	if (fBackgroundMessage->ReplaceBool(B_BACKGROUND_ERASE_TEXT, messageIndex, enabled) != B_OK) {
		std::cerr << "Error: unable to replace outline mode in BMessage" << std::endl;
		return B_ERROR;
	}

	fDirtyMessage = true;
	return B_OK;
}


status_t
BackgroundManager::SetOffset(int32 x, int32 y, int32 workspace)
{
	int32 messageIndex = _FindWorkspaceIndex(workspace, true);
	if (messageIndex < B_OK)
		return messageIndex;

	if (fBackgroundMessage->ReplacePoint(B_BACKGROUND_ORIGIN, messageIndex, BPoint(x, y)) != B_OK) {
		std::cerr << "Error: unable to replace offset in BMessage" << std::endl;
		return B_ERROR;
	}

	fDirtyMessage = true;
	return B_OK;
}


status_t
BackgroundManager::SetColor(rgb_color color, int32 workspace)
{
	if (workspace < 1 || workspace > 32) {
		std::cerr << "Error: invalid workspace #" << std::endl;
		return B_BAD_VALUE;
	}
	if (!BScreen().IsValid()) {
		std::cerr << "Error: unable to get BScreen" << std::endl;
		return B_ERROR;
	}

	BScreen().SetDesktopColor(color, (uint32)(workspace - 1));

	return B_OK;
}


status_t
BackgroundManager::Flush()
{
	if (fDirtyMessage && _WriteMessage() != B_OK)
		return B_ERROR;

	fDirtyMessage = false;

	return BMessenger("application/x-vnd.Be-TRAK").SendMessage(B_RESTORE_BACKGROUND_IMAGE);
}


void
BackgroundManager::PrintToStream()
{
	fBackgroundMessage->PrintToStream();
}
