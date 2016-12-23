#include <Windows.h>
#include <tchar.h>
#include <string>
#include <iostream>

#include <Winsock.h>
#pragma comment(lib,"wsock32.lib")

#include "log_event.h"
#include "hw_api.h"

#define UPDATE_TIME (5000)

using std::string;
using std::wstring;

static BOOL Shutdown = FALSE;
static BOOL PauseFlag = FALSE;
static SERVICE_STATUS ServiceStatus;
static SERVICE_STATUS_HANDLE HandleServiceStatus;

static LPTSTR ServiceName = TEXT("HW_PROFILE_SERVICE");
static LPTSTR LogFileName = TEXT("%APPDATA%\HwProfile.log");
static BOOL ConsoleApp = TRUE;
static BOOL IsService;

enum class SOCKET_ISSUE : USHORT
{
	WSAStartup,
	Socket,
	Connect,
	Send,
};

void SocketHandle(SOCKET_ISSUE Issue)
{
	std::cerr << "Error : ";
	switch (Issue)
	{
	case SOCKET_ISSUE::WSAStartup:
		LogEvent(TEXT("SocketHandle failed : WSAStartup()"), EVENTLOG_SUCCESS);
		std::cerr << "WSAStartup() Failed\n";
		break;
	case SOCKET_ISSUE::Socket:
		LogEvent(TEXT("SocketHandle failed : socket()"), EVENTLOG_SUCCESS);
		std::cerr << "Socket() Failed\n";
		break;
	case SOCKET_ISSUE::Connect:
		LogEvent(TEXT("SocketHandle failed : connect()"), EVENTLOG_SUCCESS);
		std::cerr << "Connect() Failed\n";
		break;
	case SOCKET_ISSUE::Send:
		LogEvent(TEXT("SocketHandle failed : send()"), EVENTLOG_SUCCESS);
		std::cerr << "Send() Failed\n";
		break;
	}

	exit(EXIT_FAILURE);
}

VOID WINAPI ServiceMain(DWORD, LPTSTR[]);
VOID WINAPI ServiceCtrlHandler(DWORD);
INT ServiceHwProfile(INT, LPTSTR[]);
void SetStatus(DWORD, DWORD = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE);

string IP;
USHORT PORT;

VOID _tmain(INT argc, LPTSTR argv[])
{
	if (argc != 3)
	{
		printf("Usage : %s <IP Address> <Port Number>\n", argv[0]);
		return;
	}

	wstring WideCharIP = argv[1];
	IP.assign(WideCharIP.begin(), WideCharIP.end());
	PORT = _ttoi(argv[2]);

	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{ ServiceName, ServiceMain },
		{ NULL, NULL }
	};

	IsService = !ConsoleApp;
	if (!LogInit(LogFileName))
	{
		return;
	}

	if (IsService)
	{
		LogEvent(TEXT("Starting Dispatcher"), EVENTLOG_SUCCESS);
		StartServiceCtrlDispatcher(DispatchTable);
	}
	else
	{
		LogEvent(TEXT("Starting Application"), EVENTLOG_SUCCESS);
		ServiceHwProfile(argc, argv);
	}
}

void WINAPI ServiceMain(DWORD argc, LPTSTR argv[])
{
	LogEvent(TEXT("Entering ServiceMain"), EVENTLOG_SUCCESS);

	HandleServiceStatus = RegisterServiceCtrlHandler
	(
		ServiceName,
		ServiceCtrlHandler
	);
	if (0 == HandleServiceStatus)
	{
		LogEvent(TEXT("Cannot register handler"), EVENTLOG_SUCCESS);
		SetStatus(SERVICE_STOP);
		
		return;
	}

	LogEvent(TEXT("Control handler registered"), EVENTLOG_SUCCESS);
	SetStatus(SERVICE_START_PENDING);
	LogEvent(TEXT("Status SERVICE_START_PENDING"), EVENTLOG_SUCCESS);

	ServiceHwProfile(argc, argv);

	LogEvent(TEXT("Service threads shut down"), EVENTLOG_SUCCESS);
	LogEvent(TEXT("Set SERVICE_STOPPED status"), EVENTLOG_SUCCESS);
	LogEvent(TEXT("Status set to SERVICE_STOPPED"), EVENTLOG_SUCCESS);

	SetStatus(SERVICE_STOP);
	return;
}

INT ServiceHwProfile(INT argc, LPTSTR argv[])
{
	SetStatus(SERVICE_RUNNING);
	LogEvent(TEXT("Status update. Service running"), EVENTLOG_SUCCESS);
	LogEvent(TEXT("Starting main service loop"), EVENTLOG_SUCCESS);

	PSOCKADDR_IN Server(new SOCKADDR_IN);
	PSOCKADDR_IN Client(new SOCKADDR_IN);
	WSADATA WinSock;
	SOCKET ConnectSocket;
	CHAR DataToSend[1024];
	CHAR UserName[0x100];
	DWORD UserNameLen = sizeof(UserName) / sizeof(CHAR) + 1;
	GetUserNameA(UserName, &UserNameLen);

	if (WSAStartup(MAKEWORD(1, 1), &WinSock) != NO_ERROR)
	{
		SocketHandle(SOCKET_ISSUE::WSAStartup);
	}
	LogEvent(TEXT("Socket successed : WSAStartup"), EVENTLOG_SUCCESS);

	ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(INVALID_SOCKET == ConnectSocket)
	{
		SocketHandle(SOCKET_ISSUE::Socket);
	}
	LogEvent(TEXT("Socket successed : socket"), EVENTLOG_SUCCESS);

	Server->sin_addr.s_addr = inet_addr(IP.c_str());
	Server->sin_family = AF_INET;
	Server->sin_port = htons(PORT);

	if (connect(ConnectSocket, (struct sockaddr*)Server, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		Beep(500, 3000);
		SocketHandle(SOCKET_ISSUE::Connect);
	}
	LogEvent(TEXT("Socket successed : connect"), EVENTLOG_SUCCESS);

	if (ERROR_SUCCESS != PdhOpenQuery(NULL, NULL, &CPUQuery)); 
	if (ERROR_SUCCESS != PdhAddCounter(CPUQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &CPUTotal));

	do
	{
		ZeroMemory(DataToSend, sizeof(DataToSend));
		sprintf(DataToSend,
			"%s\n%lf\n%lf\n%lf\n",
			UserName,
			GetCPUCycle(),
			GetPhysicalMemory(),
			GetHardDiskUsage("C:\\"));
		if (send(ConnectSocket, DataToSend, (INT)strlen(DataToSend), 0) == SOCKET_ERROR)
		{
			LogEvent(TEXT("Send failed with error"), EVENTLOG_SUCCESS);
			SocketHandle(SOCKET_ISSUE::Send);
		}
		printf("%s\n", DataToSend);

		Sleep(UPDATE_TIME);
	} while (!Shutdown);

	if (Server)
	{
		delete Server;
	}
	if (Client)
	{
		delete Client;
	}

	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}

VOID WINAPI ServiceCtrlHandler(DWORD DwControl)
{
	switch (DwControl)
	{
	case SERVICE_CONTROL_SHUTDOWN:
	case SERVICE_CONTROL_STOP:
		Shutdown = true;
		SetStatus(SERVICE_STOP_PENDING, 0);
		break;

	case SERVICE_CONTROL_PAUSE:
		SetStatus(SERVICE_PAUSE_PENDING, 0);
		PauseFlag = TRUE;
		SetStatus(SERVICE_PAUSED);
		break;

	case SERVICE_CONTROL_CONTINUE:
		SetStatus(SERVICE_CONTINUE_PENDING, 0);
		PauseFlag = FALSE;
		SetStatus(SERVICE_RUNNING);
		break;

	case SERVICE_CONTROL_INTERROGATE:
		break;
		
	default:
		if (DwControl > 127 && DwControl < 256)
		{
			break;
		}
	}

	return;
}

void SetStatus(DWORD DwState, DWORD DwAccept)
{
	SERVICE_STATUS ServiceStatus;
	ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ServiceStatus.dwCurrentState = DwState;
	ServiceStatus.dwControlsAccepted = DwAccept;
	ServiceStatus.dwWin32ExitCode = 0;
	ServiceStatus.dwServiceSpecificExitCode = 0;
	ServiceStatus.dwCheckPoint = 0;
	ServiceStatus.dwWaitHint = 0;

	SetServiceStatus(HandleServiceStatus, &ServiceStatus);
}