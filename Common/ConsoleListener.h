// Copyright (C) 2003 Dolphin Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official SVN repository and contact information can be found at
// http://code.google.com/p/dolphin-emu/

#ifndef _CONSOLELISTENER_H
#define _CONSOLELISTENER_H

#include "LogManager.h"

#ifdef _WIN32
#include <windows.h>
#endif

class ConsoleListener : public LogListener
{
public:
	ConsoleListener();
	~ConsoleListener();

	void Open(bool Hidden = false, int Width = 200, int Height = 100, const char * Name = "DebugConsole (PPSSPP)");
	void UpdateHandle();
	void Close();
	bool IsOpen();
	void LetterSpace(int Width, int Height);
	void BufferWidthHeight(int BufferWidth, int BufferHeight, int ScreenWidth, int ScreenHeight, bool BufferFirst);
	void PixelSpace(int Left, int Top, int Width, int Height, bool);
#ifdef _WIN32
	COORD GetCoordinates(int BytesRead, int BufferWidth);
#endif
	void Log(LogTypes::LOG_LEVELS, const char *Text);
	void ClearScreen(bool Cursor = true);

	void Show(bool bShow);
	bool Hidden() const { return bHidden; }
private:
#ifdef _WIN32
	HWND GetHwnd(void);
	HANDLE hConsole;

	static DWORD WINAPI RunThread(LPVOID lpParam);
	void LogWriterThread();
	void SendToThread(LogTypes::LOG_LEVELS Level, const char *Text);
	void WriteToConsole(LogTypes::LOG_LEVELS Level, const char *Text, size_t Len);

	HANDLE hThread;
	HANDLE hTriggerEvent;
	CRITICAL_SECTION criticalSection;

	char *logPending;
	volatile u32 logPendingReadPos;
	volatile u32 logPendingWritePos;
#endif
	bool bHidden;
	bool bUseColor;
};

#endif  // _CONSOLELISTENER_H
