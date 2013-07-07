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
#include <list>

#include "ui/screen.h"
#include "Common/KeyMap.h"

class EmuScreen : public Screen
{
public:
	EmuScreen(const std::string &filename);
	~EmuScreen();

	virtual void update(InputState &input);
	virtual void render();
	virtual void deviceLost();
	virtual void dialogFinished(const Screen *dialog, DialogResult result);
	virtual void sendMessage(const char *msg, const char *value);

	virtual void touch(const TouchInput &touch);
	virtual void key(const KeyInput &key);
	virtual void axis(const AxisInput &axis);

private:
	void onVKeyDown(int virtualKeyCode);
	void onVKeyUp(int virtualKeyCode);

	// Something invalid was loaded, don't try to emulate
	bool invalid_;
	std::string errorMessage_;

	// For the virtual touch buttons, that currently can't send key events.
	InputState fakeInputState;

	// Analog is still buffered.
	struct {float x, y;} analog_[2];

	// To track mappable virtual keys. We can have as many as we want.
	bool virtKeys[VIRTKEY_COUNT];
};
