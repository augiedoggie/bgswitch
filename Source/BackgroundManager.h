// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 Chris Roberts


#include <SupportDefs.h>


class BNode;
class BMessage;
class BPoint;
class BString;
struct rgb_color;


class BackgroundManager {
public:
	BackgroundManager(const char* path = nullptr);
	virtual ~BackgroundManager();

	status_t InitCheck();

	status_t ResetWorkspace(int32 workspace);

	status_t GetWorkspaceInfo(int32 workspace, BString& path, int32* mode = nullptr, BPoint* offset = nullptr, bool* erase = nullptr, rgb_color* = nullptr);

	status_t SetBackground(const char* imagePath, int32 workspace);

	status_t PrintBackgroundToStream(int32 workspace, bool verbose = false);

	status_t SetPlacement(int32 mode, int32 workspace);

	status_t SetOutline(bool enabled, int32 workspace);

	status_t SetOffset(int32 x, int32 y, int32 workspace);

	status_t SetColor(rgb_color color, int32 workspace);

	status_t Flush();

	void PrintToStream();

private:
	int32 _CreateWorkspaceIndex(int32 workspace);

	status_t _RemoveWorkspaceIndex(int32 workspace);

	int32 _FindWorkspaceIndex(int32 workspace, bool create = false);

	status_t _WriteMessage();

	BMessage* fBackgroundMessage;
	BNode* fFolderNode;
	status_t fInitStatus;
	bool fDirtyMessage;
};
