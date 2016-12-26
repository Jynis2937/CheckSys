#include "main.h"

SocketException::SocketException(SOCKET_ISSUE_FOR_SERVER Issue)
	: SocketIssue(Issue)
{
	;
}
const char* SocketException::what(void)
{
	switch (SocketIssue)
	{
	case SOCKET_ISSUE_FOR_SERVER::WSAStartup:
		StlIssueMessage = "������ ���� �ʱ�ȭ ����";
		break;

	case SOCKET_ISSUE_FOR_SERVER::Bind:
		StlIssueMessage = "���ε� ����";
		break;

	case SOCKET_ISSUE_FOR_SERVER::Accept:
		StlIssueMessage = "������ ���� Ŭ���̾�Ʈ ���� ����";
		break;

	case SOCKET_ISSUE_FOR_SERVER::Listen:
		StlIssueMessage = "���� ��� ����";
		break;

	default:
		StlIssueMessage = "�� �� ���� ����";
	}

	return StlIssueMessage.c_str();
}
SocketException::SOCKET_ISSUE_FOR_SERVER SocketException::GetErrorCode(void)
{
	return SocketIssue;
}

System::Void MarshalString(System::String^ SystemString, string& RefStlString)
{
	using namespace System::Runtime::InteropServices;
	LPCSTR PtrString = (LPCSTR)(Marshal::StringToHGlobalAnsi(SystemString)).ToPointer();
	RefStlString = PtrString;
	Marshal::FreeHGlobal(System::IntPtr((LPVOID)PtrString));
}
System::Void MarshalString(System::String^ SystemString, wstring& RefStlString)
{
	using namespace System::Runtime::InteropServices;
	LPCWSTR PtrWstring = (LPCWSTR)(Marshal::StringToHGlobalUni(SystemString)).ToPointer();
	RefStlString = PtrWstring;
	Marshal::FreeHGlobal(System::IntPtr((LPVOID)PtrWstring));
}