#pragma once
#include <Windows.h>
#include <tchar.h>
#include <cstdio>
#include <ctime>

static FILE* Logger = NULL;

VOID LogEvent(LPCTSTR UserMessage, WORD Type)
{
	TCHAR TimeString[0x20] = TEXT("");
	time_t CurrentTime = time(NULL);

	_tcsncat(TimeString, _tctime(&CurrentTime), sizeof(TimeString) / sizeof(TCHAR));
	TimeString[_tcslen(TimeString) - sizeof(TCHAR)] = TEXT('\0');
	_ftprintf(Logger, TEXT("%s. "), TimeString);

	if (EVENTLOG_SUCCESS == Type || EVENTLOG_INFORMATION_TYPE == Type)
	{
		_ftprintf(Logger, TEXT("%s"), TEXT("Information. "));
	}
	else if (EVENTLOG_ERROR_TYPE == Type)
	{
		_ftprintf(Logger, TEXT("%s"), TEXT("Error. "));
	}
	else if (EVENTLOG_WARNING_TYPE == Type)
	{
		_ftprintf(Logger, TEXT("%s"), TEXT("Unknown. "));
	}

	_ftprintf(Logger, TEXT("%s\n"), UserMessage);
	fflush(Logger);

	return;
}
BOOL LogInit(LPTSTR Name)
{
	Logger = _tfopen(Name, TEXT("a+"));
	if (nullptr != Logger)
	{
		LogEvent(TEXT("Initialized Logging"), EVENTLOG_SUCCESS);
	}

	return (nullptr != Logger);
}
VOID LogClose(void)
{
	LogEvent(TEXT("Closing Log"), EVENTLOG_SUCCESS);
	return;
}
