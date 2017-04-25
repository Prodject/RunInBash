#include "stdafx.h"
#include <malloc.h>
#include <memory>

template <typename T> //for both const and non-const
T *TrimStart(T *string)
{
	while (string[0] == _T(' '))
	{
		++string;
	}

	return string;
}

//There is no CommandLineToArgvA(), so we rely on the caller to pass in argv[]
template <typename T>
const TCHAR *GetArgumentString(const T argv)
{
	const TCHAR *cmdLine = GetCommandLine();
	
	bool escape = cmdLine[0] == '\'' || cmdLine[0] == '\"';
	const TCHAR *skipped = cmdLine + _tcsclen(argv[0]) + (escape ? 1 : 0) * 2;

	return TrimStart(skipped);
}

void PrintHelp()
{
	_tprintf_s(_T("RunInBash by NeoSmart Technologies - Copyright 2017\n"));
	_tprintf_s(_T("Usage: Alias $ to RunInBash.exe and prefix WSL commands with $ to execute. For example:\n"));
	_tprintf_s(_T("$ uname -a\n"));
}

const TCHAR *Escape(const TCHAR *string)
{
	if (!string)
	{
		return nullptr;
	}

	int newLength = 1; //terminating null
	for (size_t i = 0; string[i] != _T('\0'); ++i)
	{
		++newLength;
		if (string[i] == _T('"') || string[i] == _T('\\'))
		{
			++newLength;
		}
	}

	TCHAR *escaped = (TCHAR *) calloc(newLength, sizeof(string[0]));

	for (size_t i = 0, j = 0; string[i] != _T('\0'); ++i, ++j)
	{
		if (string[i] == _T('"') || string[i] == _T('\\'))
		{
			escaped[j++] = _T('\\');
		}
		escaped[j] = string[i];
	}

	return escaped;
}

int _tmain(int argc, TCHAR *argv[])
{
	const TCHAR *argumentStr = GetArgumentString(argv);

	PVOID oldValue;
	Wow64DisableWow64FsRedirection(&oldValue);

	//Search for bash.exe
	TCHAR bash[MAX_PATH] = { 0 };
	ExpandEnvironmentStrings(_T("%windir%\\system32\\bash.exe"), bash, _countof(bash));
	bool found = GetFileAttributes(bash) != INVALID_FILE_ATTRIBUTES;

	if (!found)
	{
		_ftprintf(stderr, _T("Unable to find bash.exe!\n"));
		return -1;
	}

#ifdef _DEBUG
	bool debug = true;
#else
	bool debug = false;
#endif

	//Get whatever the user entered after our EXE as a single string
	auto argument = GetArgumentString(argv);
	if (_tcsclen(argument) == 0)
	{
		PrintHelp();
		ExitProcess(-1);
	}

	//handle possible arguments
	if (_tcsicmp(argv[1], _T("-h")) == 0)
	{
		PrintHelp();
		ExitProcess(0);
	}
	if (_tcsicmp(argv[1], _T("-v")) == 0)
	{
		debug = true;
		argument = TrimStart(argument + 2);
	}

	//Escape it to be placed within double quotes
	auto escaped = Escape(argument);

	TCHAR *format = _T("bash -c \"%s\"");
	int length = _tcsclen(format) + _tcslen(escaped) + 1;

	TCHAR *lpArg = (TCHAR *)alloca(sizeof(TCHAR) * (length + 1));
	ZeroMemory(lpArg, sizeof(TCHAR) * (length + 1));
	_stprintf(lpArg, format, escaped);

	TCHAR currentDir[MAX_PATH] = { 0 };
	GetCurrentDirectory(_countof(currentDir), currentDir);

	if (debug)
	{
		_ftprintf(stderr, _T("> %s\n"), lpArg);
	}

	auto startInfo = STARTUPINFO{ 0 };
	auto pInfo = PROCESS_INFORMATION{ 0 };
	bool success = CreateProcess(bash, lpArg, nullptr, nullptr, true, 0, nullptr, currentDir, &startInfo, &pInfo);

	if (!success)
	{
		_ftprintf(stderr, _T("Failed to create process. Last error: 0x%x\n"), GetLastError());
		return GetLastError();
	}

	WaitForSingleObject(pInfo.hProcess, INFINITE);
	CloseHandle(pInfo.hProcess);

    return 0;
}
