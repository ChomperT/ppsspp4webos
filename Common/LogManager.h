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

#pragma once

#include "Log.h"
#include "StringUtils.h"
#include "Thread.h"
#include "FileUtil.h"
#include "file/ini_file.h" 

#include <set>
#include <string.h>

#define	MAX_MESSAGES 8000   
#define MAX_MSGLEN  1024

extern const char *hleCurrentThreadName;

// pure virtual interface
class LogListener
{
public:
	virtual ~LogListener() {}

	virtual void Log(LogTypes::LOG_LEVELS, const char *msg) = 0;
};

class FileLogListener : public LogListener
{
public:
	FileLogListener(const char *filename);

	void Log(LogTypes::LOG_LEVELS, const char *msg);

	bool IsValid() { if (!m_logfile) return false; else return true; }
	bool IsEnabled() const { return m_enable; }
	void SetEnable(bool enable) { m_enable = enable; }

	const char* GetName() const { return "file"; }

private:
	std::mutex m_log_lock;
	std::ofstream m_logfile;
	bool m_enable;
};

class DebuggerLogListener : public LogListener
{
public:
	void Log(LogTypes::LOG_LEVELS, const char *msg);
};

class LogContainer
{
public:
	LogContainer(const char* shortName, const char* fullName, bool enable = false);
	
	const char* GetShortName() const { return m_shortName; }
	const char* GetFullName() const { return m_fullName; }

	void AddListener(LogListener* listener);
	void RemoveListener(LogListener* listener);

	void Trigger(LogTypes::LOG_LEVELS, const char *msg);

	bool IsEnabled() const { return m_enable; }
	void SetEnable(bool enable) { m_enable = enable; }

	LogTypes::LOG_LEVELS GetLevel() const { return m_level;	}

	void SetLevel(LogTypes::LOG_LEVELS level) {	m_level = level; }

	bool HasListeners() const { return !m_listeners.empty(); }

private:
	char m_fullName[128];
	char m_shortName[32];
	bool m_enable;
	LogTypes::LOG_LEVELS m_level;
	std::mutex m_listeners_lock;
	std::set<LogListener*> m_listeners;
};

class ConsoleListener;

class LogManager : NonCopyable
{
private:
	LogContainer* m_Log[LogTypes::NUMBER_OF_LOGS];
	FileLogListener *m_fileLog;
	ConsoleListener *m_consoleLog;
	DebuggerLogListener *m_debuggerLog;
	static LogManager *m_logManager;  // Singleton. Ugh.
	std::mutex m_log_lock;

	LogManager();
	~LogManager();
public:

	static u32 GetMaxLevel() { return MAX_LOGLEVEL;	}

	void Log(LogTypes::LOG_LEVELS level, LogTypes::LOG_TYPE type, 
			 const char *file, int line, const char *fmt, va_list args);

	void SetLogLevel(LogTypes::LOG_TYPE type, LogTypes::LOG_LEVELS level)
	{
		m_Log[type]->SetLevel(level);
	}

	LogTypes::LOG_LEVELS GetLogLevel(LogTypes::LOG_TYPE type)
	{
		return m_Log[type]->GetLevel();
	}

	void SetEnable(LogTypes::LOG_TYPE type, bool enable)
	{
		m_Log[type]->SetEnable(enable);
	}

	bool IsEnabled(LogTypes::LOG_TYPE type) const
	{
		return m_Log[type]->IsEnabled();
	}

	const char* GetShortName(LogTypes::LOG_TYPE type) const
	{
		return m_Log[type]->GetShortName();
	}

	const char* GetFullName(LogTypes::LOG_TYPE type) const
	{
		return m_Log[type]->GetFullName();
	}

	void AddListener(LogTypes::LOG_TYPE type, LogListener *listener)
	{
		m_Log[type]->AddListener(listener);
	}

	void RemoveListener(LogTypes::LOG_TYPE type, LogListener *listener)
	{
		m_Log[type]->RemoveListener(listener);
	}

	FileLogListener *GetFileListener() const
	{
		return m_fileLog;
	}

	ConsoleListener *GetConsoleListener() const
	{
		return m_consoleLog;
	}

	DebuggerLogListener *GetDebuggerListener() const
	{
		return m_debuggerLog;
	}

	static LogManager* GetInstance()
	{
		return m_logManager;
	}

	static void SetInstance(LogManager *logManager)
	{
		m_logManager = logManager;
	}

	static void Init();
	static void Shutdown();

	void ChangeFileLog(const char *filename);

  void SaveConfig(IniFile::Section *section);
  void LoadConfig(IniFile::Section *section);
};
