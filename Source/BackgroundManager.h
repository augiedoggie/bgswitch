
#include <SupportDefs.h>


class BNode;
class BMessage;
class BPoint;
class BString;


class BackgroundManager {
public:
				BackgroundManager(const char* path);
	virtual		~BackgroundManager();

	status_t	InitCheck();

	status_t	ClearBackground(int32 workspace, bool complete = false, bool verbose = false);

	status_t	GetBackground(int32 workspace, BString& path, int32* mode = NULL, BPoint* offset = NULL, bool* erase = NULL);

	status_t	SetBackground(const char* imagePath, int32 workspace, bool verbose = false);

	status_t	DumpBackground(int32 workspace, bool verbose);

	status_t	SendTrackerMessage();

	void		DumpBackgroundMessage();

private:
	int32		_CreateWorkspaceIndex(int32 workspace);

	int32		_RemoveWorkspaceIndex(int32 workspace);

	int32		_FindWorkspaceIndex(int32 workspace);

	status_t	_WriteMessage();

	BMessage*	fBackgroundMessage;
	BNode*		fFolderNode;
	status_t	fInitStatus;
};
