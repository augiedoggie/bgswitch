#include "BackgroundManager.h"

#include <FindDirectory.h>
#include <Messenger.h>
#include <Node.h>
#include <Path.h>
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
	fInitStatus(B_ERROR)
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
	int32 setWorkspaces = 0;
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
BackgroundManager::_FindWorkspaceIndex(int32 workspace)
{
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

		if ((1 << (workspace - 1)) & workspaces)
			return x;
	}
	// return an error if we didn't find it because we have a global background or a single workspace or ...
	return B_ERROR;
}


int32
BackgroundManager::_CreateWorkspaceIndex(int32 workspace)
{
	int32 countFound = 0;
	fBackgroundMessage->GetInfo(B_BACKGROUND_WORKSPACES, nullptr, &countFound);

	int32 setWorkspaces = 0;
	if (fBackgroundMessage->FindInt32(B_BACKGROUND_WORKSPACES, 0, &setWorkspaces) != B_OK) {
		std::cerr << "Error: Unable to find B_BACKGROUND_WORKSPACES BMessage" << std::endl;
		return B_ERROR;
	}

	// tell Tracker that this workspace has a custom background
	setWorkspaces &= ~(1 << (workspace - 1));
	if (fBackgroundMessage->ReplaceInt32(B_BACKGROUND_WORKSPACES, 0, setWorkspaces) != B_OK) {
		std::cerr << "Error: Unable to replace B_BACKGROUND_WORKSPACES BMessage" << std::endl;
		return B_ERROR;
	}
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
BackgroundManager::GetBackground(int32 workspace, BString& path, int32* mode, BPoint* offset, bool* erase)
{
	int32 messageIndex = _FindWorkspaceIndex(workspace);
	if (messageIndex < 0) {
		std::cerr << "Warning: No background for this specific workspace.  Showing global defaults." << std::endl;
		messageIndex = 0;
	}

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

	return B_OK;
}


status_t
BackgroundManager::DumpBackground(int32 workspace, bool verbose)
{
	BString path;
	int32 mode = B_BACKGROUND_MODE_SCALED;
	BPoint offset;
	bool erase = true;

	if (GetBackground(workspace, path, &mode, &offset, &erase) != B_OK)
		return B_ERROR;

	if (verbose)
		std::cout << "Workspace: " << workspace << std::endl;
	else
		std::cout << workspace << ":";

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
	}

	std::cout << "Offset: X=" << offset.x << " Y=" << offset.y << std::endl;

	std::cout << "Text Outline: " << (erase ? "true" : "false") << std::endl;

	return B_OK;
}


status_t
BackgroundManager::ResetBackground(int32 workspace, bool verbose)
{
	if (verbose)
		std::cout << "Resetting workspace " << workspace << " to global default" << std::endl;

	int32 messageIndex = _FindWorkspaceIndex(workspace);
	if (messageIndex < 0)
		return B_ERROR;

	if (_RemoveWorkspaceIndex(workspace) != B_OK)
		return B_ERROR;

	fBackgroundMessage->RemoveData(B_BACKGROUND_WORKSPACES, messageIndex);
	fBackgroundMessage->RemoveData(B_BACKGROUND_IMAGE, messageIndex);
	fBackgroundMessage->RemoveData(B_BACKGROUND_MODE, messageIndex);
	fBackgroundMessage->RemoveData(B_BACKGROUND_ORIGIN, messageIndex);
	fBackgroundMessage->RemoveData(B_BACKGROUND_ERASE_TEXT, messageIndex);
	fBackgroundMessage->RemoveData(BACKGROUND_SET, messageIndex);

	return _WriteMessage();
}


status_t
BackgroundManager::SetBackground(const char* imagePath, int32 workspace, bool verbose)
{
	if (verbose)
		std::cout << "Setting workspace " << workspace << " to " << (imagePath == nullptr ? "<none>" : imagePath) << std::endl;

	int32 messageIndex = _FindWorkspaceIndex(workspace);
	if (messageIndex < 0) {
		// we're switching from global to a locally set background, find the next free index
		messageIndex = _CreateWorkspaceIndex(workspace);

		if (messageIndex == B_ERROR)
			return B_ERROR;

		// TODO we should get these default values from index 0
		fBackgroundMessage->AddInt32(B_BACKGROUND_WORKSPACES, (1 << (workspace - 1)));
		fBackgroundMessage->AddString(B_BACKGROUND_IMAGE, "");
		fBackgroundMessage->AddInt32(B_BACKGROUND_MODE, B_BACKGROUND_MODE_SCALED);
		fBackgroundMessage->AddPoint(B_BACKGROUND_ORIGIN, BPoint(0, 0));
		fBackgroundMessage->AddBool(B_BACKGROUND_ERASE_TEXT, true);
		fBackgroundMessage->AddInt32(BACKGROUND_SET, 0);
	}

	// verify the file exists if we were given a non-empty path
	if (imagePath != nullptr && strcmp(imagePath, "") != 0) {
		BEntry newWallEntry(imagePath);
		if (newWallEntry.InitCheck() != B_OK || !newWallEntry.Exists()) {
			std::cerr << "Error: unable to find source wallpaper" << std::endl;
			return B_ERROR;
		}
	}

	if (fBackgroundMessage->ReplaceString(B_BACKGROUND_IMAGE, messageIndex, imagePath == nullptr ? "" : imagePath) != B_OK) {
		std::cerr << "Error: unable to replace background image path" << std::endl;
		return B_ERROR;
	}

	return _WriteMessage();
}


status_t
BackgroundManager::SendTrackerMessage()
{
	BMessenger tracker("application/x-vnd.Be-TRAK");
	return tracker.SendMessage(B_RESTORE_BACKGROUND_IMAGE);
}


void
BackgroundManager::DumpBackgroundMessage()
{
	fBackgroundMessage->PrintToStream();
}
