// Copyright (c) 2012- PPSSPP Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official git repository and contact information can be found at
// https://github.com/hrydgard/ppsspp and http://www.ppsspp.org/.

#pragma once

#include <string>
#include <vector>
#include <map>

#include "ui/screen.h"
#include "ui/ui.h"
#include "file/file_util.h"

class LogoScreen : public Screen
{
public:
	LogoScreen(const std::string &bootFilename)
		: bootFilename_(bootFilename), frames_(0) {}
	void update(InputState &input);
	void render();

private:
	std::string bootFilename_;
	int frames_;
};


class MenuScreen : public Screen
{
public:
	MenuScreen() : frames_(0) {}
	void update(InputState &input);
	void render();

private:
	int frames_;
};

// Dialog box, meant to be pushed
class InGameMenuScreen : public Screen
{
public:
	void update(InputState &input);
	void render();
};


class SettingsScreen : public Screen
{
public:
	void update(InputState &input);
	void render();
};


class DeveloperScreen : public Screen
{
public:
	void update(InputState &input);
	void render();
};


struct FileSelectScreenOptions {
	const char* filter;  // Enforced extension filter. Case insensitive, extensions separated by ":".
	bool allowChooseDirectory;
	int folderIcon;
	std::map<std::string, int> iconMapping;
};


class FileSelectScreen : public Screen
{
public:
	FileSelectScreen(const FileSelectScreenOptions &options);
	void update(InputState &input);
	void render();

	// Override these to for example write the current directory to a config file.
	virtual void onSelectFile() {}
	virtual void onCancel() {}

private:
	void updateListing();

	FileSelectScreenOptions options_;
	UIList list_;
	std::string currentDirectory_;
	std::vector<FileInfo> listing_;
};


class CreditsScreen : public Screen
{
public:
	CreditsScreen() : frames_(0) {}
	void update(InputState &input);
	void render();
private:
	int frames_;
};


// Dialog box, meant to be pushed
class ErrorScreen : public Screen
{
public:
	ErrorScreen(const std::string &errorTitle, const std::string &errorMessage) : errorTitle_(errorTitle), errorMessage_(errorMessage) {}
	void update(InputState &input);
	void render();

private:
	std::string errorTitle_;
	std::string errorMessage_;
};


void DrawWatermark();
