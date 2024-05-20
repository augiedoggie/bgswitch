// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 Chris Roberts


#include <FindDirectory.h>
#include <Path.h>
#include <chrono>
#include <iostream>
#include <sstream>


template<typename... Args>
status_t
WallrusApp::_Log(LogLevel level, const char* message, Args... args)
{
	if (fLogFile.InitCheck() != B_OK)
		return B_ERROR;

	if ((fLogLevel & level) == 0)
		return B_OK;

	off_t size = 0;
	if (fLogFile.GetSize(&size) != B_OK)
		return B_ERROR;

	// check log file size and rotate if needed
	if (size > 1024000) {
		BPath logPath;
		find_directory(B_SYSTEM_LOG_DIRECTORY, &logPath);
		logPath.Append("wallrus.log");
		BEntry entry(logPath.Path());
		entry.Rename("wallrus.log.1", true);
		if (fLogFile.SetTo(logPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_OPEN_AT_END) != B_OK)
			return B_ERROR; // TODO pass on actual error?
	}

	std::time_t now_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::stringstream logStream;
	logStream << std::put_time(std::localtime(&now_t), "%Y/%m/%d %T");
	switch (level) {
		case kLogInfo:
			logStream << " [info] ";
			break;
		case kLogError:
			logStream << " [error] ";
			break;
		case kLogDebug:
			logStream << " [debug] ";
			break;
		case kLogTrace:
			logStream << " [trace] ";
			break;
		default:
			logStream << " [unknown] ";
			break;
	}

	BString msgString;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
	msgString.SetToFormat(message, args...);
#pragma GCC diagnostic pop

	logStream << msgString << "\n";

	std::string logString = logStream.str();
	ssize_t bytesWritten = fLogFile.Write(logString.c_str(), logString.length());
	if (bytesWritten < B_OK || (unsigned)bytesWritten != logString.length())
		return B_ERROR;

	return B_OK;
}
