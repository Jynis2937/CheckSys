#pragma once
#include <exception>
#include <string>

class SocketException : std::exception
{
public:
	enum class SOCKET_ISSUE_FOR_SERVER
	{
		WSAStartup,
		Connect,
		Bind,
		Listen,
		Accept,
		Full,
		Closed,
	};

private:
	SOCKET_ISSUE_FOR_SERVER SocketIssue;
	std::string StlIssueMessage;

public:
	SocketException::SocketException(void) = delete;
	SocketException::SocketException(SOCKET_ISSUE_FOR_SERVER);

public:
	const char* SocketException::what(void);
	SOCKET_ISSUE_FOR_SERVER SocketException::GetErrorCode(void);
};