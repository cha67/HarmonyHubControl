#include "csocket.h"
#include <cstring>
#include <iostream>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#endif



#ifndef WIN32
#undef INVALID_SOCKET
#define INVALID_SOCKET (int)(~0)
#endif

#define BLOCK_RETRY_INTERVAL_MSECS 1
#define ERROR_INAPPROPRIATE_STATE -1

#ifdef WIN32
static bool s_bSocketsInitialized = false;
#endif

#define SUCCESS     0
#define FAILURE     1


csocket::csocket() : m_socketState(CLOSED),
					 m_remotePort(0)
{
#ifdef WIN32
	if (!s_bSocketsInitialized)
	{
		WORD socketVersion;
		WSADATA socketData;
		int err;

		socketVersion = MAKEWORD(2, 0);
		err = WSAStartup(socketVersion, &socketData);
	}
#endif

	m_socket = 0;
}


csocket::~csocket()
{
	close();
}


void csocket::close()
{
	 if ( m_socketState != csocket::CLOSED && m_socketState != csocket::ERRORED)
	{
#ifdef WIN32
		closesocket(m_socket);
#else
		::close(m_socket);
#endif
		m_socketState = CLOSED;
		m_socket = 0;
	}
}


int csocket::resolveHost(const std::string& szRemoteHostName, struct sockaddr_in& sa)
{
	if (szRemoteHostName.length() == 0)
		return FAILURE;

#ifdef WIN32
	if (!s_bSocketsInitialized)
	{
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;

		wVersionRequested = MAKEWORD(2, 0);
		err = WSAStartup(wVersionRequested, &wsaData);
	}
#endif

	struct addrinfo *addr;
	if (getaddrinfo(szRemoteHostName.c_str() , "0", 0, &addr) == 0)
	{
		struct sockaddr_in *saddr = (((struct sockaddr_in *)addr->ai_addr));
		sa.sin_family = saddr->sin_family;
		memcpy(&sa, saddr, sizeof(sockaddr_in));
/*
#ifdef WIN32
		char address[INET6_ADDRSTRLEN];
		inet_ntop(saddr->sin_family, &(saddr->sin_addr), address, sizeof(address));
		inet_pton(saddr->sin_family, address, &sa.sin_addr);
#endif
*/
		return SUCCESS;
	}
	else
	{
		sa.sin_family = AF_INET;
		if (inet_pton(sa.sin_family, szRemoteHostName.c_str(), &sa.sin_addr) == 1)
			return SUCCESS;
		sa.sin_family = AF_INET6;
		if (inet_pton(sa.sin_family, szRemoteHostName.c_str(), &sa.sin_addr) == 1)
			return SUCCESS;
	}	

	return FAILURE;
}


int csocket::connect( const char* remoteHost, const unsigned int remotePort )
{
	m_strRemoteHost = remoteHost;
	m_remotePort = remotePort;

	if ( m_socketState != CLOSED ) 
		return ERROR_INAPPROPRIATE_STATE;

	int status = resolveHost(remoteHost, m_remoteSocketAddr);
	if (status == FAILURE )
		return FAILURE;

#ifdef WIN32
	m_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0 , 0 , 0);
#else
	m_socket = socket(AF_INET, SOCK_STREAM, 0);
#endif

	if (m_socket == INVALID_SOCKET)
	        return FAILURE;

	// create local socket
	int iRecvTimeout = 1;
	m_localSocketAddr.sin_family = m_remoteSocketAddr.sin_family;
	m_localSocketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	m_localSocketAddr.sin_port = htons(0);
	setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*) &iRecvTimeout, sizeof(iRecvTimeout));

	status = bind(m_socket, (const sockaddr*)&(m_localSocketAddr), sizeof(struct sockaddr));

	if (status < 0) 
		return FAILURE;

#ifdef WIN32 
	int set = 1;
	setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY,  (char*) &set, sizeof(set) );
#endif

	// connect to remote socket
	m_remoteSocketAddr.sin_port = htons(m_remotePort);

	// do a non-blocking connect and return success or fail after no more than 5 seconds

#ifdef WIN32
	unsigned long nonblock = 1;
	ioctlsocket(m_socket, FIONBIO, &nonblock);
	status = ::connect(m_socket, (const sockaddr*)&(m_remoteSocketAddr), sizeof(sockaddr_in));
	if (status < 0)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
			return FAILURE;

		fd_set fdw, fdr, fde;
		FD_ZERO(&fdw);
		FD_ZERO(&fdr);
		FD_ZERO(&fde);
		FD_SET(m_socket, &fdw);
		FD_SET(m_socket, &fdr);
		FD_SET(m_socket, &fde);

		timeval tv;
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		int rc = select(m_socket + 1, &fdw, &fdr, &fde, &tv);

		if (rc <= 0) // 0 = timeout; less than zero is error (should I do something with state 'errno == EINTR' here?)
			return FAILURE;

		// try to get socket options
		int so_error;
		socklen_t len = sizeof so_error;
		if (getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (char *)&so_error, &len) < 0)
			return FAILURE;

		if (so_error != 0)
			return FAILURE;
	}

	// restore blocking mode on socket
	nonblock = 0;
	ioctlsocket(m_socket, FIONBIO, &nonblock);

	m_socketState = CONNECTED;
	return SUCCESS;
#else
	fcntl(m_socket, F_SETFL, O_NONBLOCK);
	status = ::connect(m_socket, (const sockaddr*)&(m_remoteSocketAddr), sizeof(sockaddr_in));

	if (status < 0)
	{
		if (errno != EINPROGRESS)
			return FAILURE;

		struct pollfd fds;
		fds.fd = m_socket;
		fds.events = POLLERR | POLLOUT;
		fds.revents = 0;
		int rc = poll(&fds, 1, 3000);

		if (rc <= 0) // 0 = timeout; less than zero is error (should I do something with state 'errno == EINTR' here?)
			return FAILURE;

		// try to get socket options
		int so_error;
		socklen_t len = sizeof so_error;
		if (getsockopt(m_socket, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0)
			return FAILURE;

		if (so_error != 0)
			return FAILURE;
	}

	// restore blocking mode on socket
	long socketMode = fcntl(m_socket, F_GETFL, NULL);
	socketMode &= (~O_NONBLOCK);
	fcntl(m_socket, F_SETFL, socketMode);

	m_socketState = CONNECTED;
	return SUCCESS;
#endif
}


int csocket::canRead(bool* readyToRead, float waitTime)
{
	if (m_socketState != CONNECTED)
	{
		return ERROR_INAPPROPRIATE_STATE;
	}

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(m_socket, &fds);

	timeval timeout;

	if (waitTime <= 0.0f)
	{
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
	}
	else
	{
		timeout.tv_sec = (int)(waitTime);
		timeout.tv_usec = (int)(1000000.0f * (waitTime - (float)timeout.tv_sec));
	}

	int n = select(sizeof(fds)*8, &fds, NULL, NULL, &timeout);
	if (n < 0)
	{
		m_socketState = ERRORED;
		return FAILURE;
	}

	if ((n > 0) && (FD_ISSET(m_socket, &fds)))
	{
		*readyToRead = true;
		return SUCCESS;
	}

	*readyToRead = false;
	return SUCCESS;
}


int csocket::read(char* pDataBuffer, unsigned int numBytesToRead, bool bReadAll)
{
	if (m_socketState != CONNECTED)
	{
		return ERROR_INAPPROPRIATE_STATE;
	}

	int numBytesRemaining = numBytesToRead;
	int numBytesRead = 0;

	do
	{
#ifdef WIN32
		numBytesRead = recv(m_socket, pDataBuffer, numBytesRemaining, 0);
#else
		numBytesRead = static_cast<int>(::read(m_socket, pDataBuffer, numBytesRemaining));
#endif

		if (numBytesRead < 0)
		{
			if (bReadAll)
			{
#ifdef WIN32
				Sleep(BLOCK_RETRY_INTERVAL_MSECS);
#else
				usleep(BLOCK_RETRY_INTERVAL_MSECS * 1000);
#endif
			}
			else
			{
				return numBytesRead;
			}
		}

		if (numBytesRead > 0)
		{
			numBytesRemaining -= numBytesRead;
			pDataBuffer += numBytesRead;
		}

		if (numBytesRead == 0)
		{
			break;
		}

	} while ((numBytesRemaining > 0) && (bReadAll));

	return (numBytesToRead - numBytesRemaining);
}


int csocket::write(const char* pDataBuffer, unsigned int numBytesToWrite)
{
	if (m_socketState != CONNECTED)
	{
		return ERROR_INAPPROPRIATE_STATE;
	}

	int numBytesRemaining = numBytesToWrite;
	int numBytestWritten = 0;

	while (numBytesRemaining > 0)
	{
#ifdef WIN32
		numBytestWritten= static_cast<int>(::send(m_socket, pDataBuffer, numBytesRemaining, 0));
#else
		numBytestWritten= ::write(m_socket, pDataBuffer, numBytesRemaining);
#endif

		if (numBytestWritten < 0)
		{
			m_socketState = ERRORED;
			return numBytestWritten;
		}

		numBytesRemaining -= numBytestWritten;
		pDataBuffer += numBytestWritten;
	}

	return (numBytesToWrite - numBytesRemaining);
}

csocket::SocketState csocket::getState(void) const
{
	return m_socketState;
}
