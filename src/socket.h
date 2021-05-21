#ifndef SOCKET_H
#define SOCKET_H

#ifdef WIN32
#include <winsock2.h>
#include <errno.h>

#ifdef EADDRINUSE
#undef EADDRINUSE
#endif
#ifdef EADDRNOTAVAIL
#undef EADDRNOTAVAIL
#endif
#ifdef EAFNOSUPPORT
#undef EAFNOSUPPORT
#endif
#ifdef EALREADY
#undef EALREADY
#endif
#ifdef ECONNABORTED
#undef ECONNABORTED
#endif
#ifdef ECONNREFUSED
#undef ECONNREFUSED
#endif
#ifdef ECONNRESET
#undef ECONNRESET
#endif
#ifdef EDESTADDRREQ
#undef EDESTADDRREQ
#endif
#ifdef EDQUOT
#undef EDQUOT
#endif
#ifdef EHOSTDOWN
#undef EHOSTDOWN
#endif
#ifdef EHOSTUNREACH
#undef EHOSTUNREACH
#endif
#ifdef EINPROGRESS
#undef EINPROGRESS
#endif
#ifdef EISCONN
#undef EISCONN
#endif
#ifdef ELOOP
#undef ELOOP
#endif
#ifdef EMSGSIZE
#undef EMSGSIZE
#endif
#ifdef ENETDOWN
#undef ENETDOWN
#endif
#ifdef ENETRESET
#undef ENETRESET
#endif
#ifdef ENETUNREACH
#undef ENETUNREACH
#endif
#ifdef ENOBUFS
#undef ENOBUFS
#endif
#ifdef ENOPROTOOPT
#undef ENOPROTOOPT
#endif
#ifdef ENOTCONN
#undef ENOTCONN
#endif
#ifdef ENOTSOCK
#undef ENOTSOCK
#endif
#ifdef EOPNOTSUPP
#undef EOPNOTSUPP
#endif
#ifdef EPFNOSUPPORT
#undef EPFNOSUPPORT
#endif
#ifdef EPROTONOSUPPORT
#undef EPROTONOSUPPORT
#endif
#ifdef EPROTOTYPE
#undef EPROTOTYPE
#endif
#ifdef EREMOTE
#undef EREMOTE
#endif
#ifdef ESHUTDOWN
#undef ESHUTDOWN
#endif
#ifdef ESOCKTNOSUPPORT
#undef ESOCKTNOSUPPORT
#endif
#ifdef ESTALE
#undef ESTALE
#endif
#ifdef ETIMEDOUT
#undef ETIMEDOUT
#endif
#ifdef ETOOMANYREFS
#undef ETOOMANYREFS
#endif
#ifdef EUSERS
#undef EUSERS
#endif
#ifdef EWOULDBLOCK
#undef EWOULDBLOCK
#endif
#define EADDRINUSE WSAEADDRINUSE
#define EADDRNOTAVAIL WSAEADDRNOTAVAIL
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#define EALREADY WSAEALREADY
#define ECONNABORTED WSAECONNABORTED
#define ECONNREFUSED WSAECONNREFUSED
#define ECONNRESET WSAECONNRESET
#define EDESTADDRREQ WSAEDESTADDRREQ
#define EDQUOT WSAEDQUOT
#define EHOSTDOWN WSAEHOSTDOWN
#define EHOSTUNREACH WSAEHOSTUNREACH
#define EINPROGRESS WSAEINPROGRESS
#define EISCONN WSAEISCONN
#define ELOOP WSAELOOP
#define EMSGSIZE WSAEMSGSIZE
#define ENETDOWN WSAENETDOWN
#define ENETRESET WSAENETRESET
#define ENETUNREACH WSAENETUNREACH
#define ENOBUFS WSAENOBUFS
#define ENOPROTOOPT WSAENOPROTOOPT
#define ENOTCONN WSAENOTCONN
#define ENOTSOCK WSAENOTSOCK
#define EOPNOTSUPP WSAEOPNOTSUPP
#define EPFNOSUPPORT WSAEPFNOSUPPORT
#define EPROTONOSUPPORT WSAEPROTONOSUPPORT
#define EPROTOTYPE WSAEPROTOTYPE
#define EREMOTE WSAEREMOTE
#define ESHUTDOWN WSAESHUTDOWN
#define ESOCKTNOSUPPORT WSAESOCKTNOSUPPORT
#define ESTALE WSAESTALE
#define ETIMEDOUT WSAETIMEDOUT
#define ETOOMANYREFS WSAETOOMANYREFS
#define EUSERS WSAEUSERS
#define EWOULDBLOCK WSAEWOULDBLOCK

#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

typedef int SOCKET;

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

#define closesocket close

extern int GetLastError( );
#endif

#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifdef WIN32
#define SHUT_RDWR 2
#endif

//
// CSocket
//

class CSocket
{
protected:
	SOCKET m_Socket;
	struct sockaddr_in m_SIN;
	bool m_HasError;
	int m_Error;
    std::string m_CachedHostName;

public:
	CSocket( );
	CSocket( SOCKET nSocket, struct sockaddr_in nSIN );
	~CSocket( );

	virtual BYTEARRAY GetPort( );
	virtual BYTEARRAY GetIP( );
    virtual std::string GetIPString( );
	virtual bool HasError( )
	{
		return m_HasError;
	}
	virtual int GetError( )
	{
		return m_Error;
	}
    virtual std::string GetErrorString( );
	virtual void SetFD( fd_set *fd, fd_set *send_fd, int *nfds );
	virtual void Allocate( int type );
	virtual void Reset( );
    virtual std::string GetHostName( );
};

//
// CTCPSocket
//

class CTCPSocket : public CSocket
{
protected:
    bool m_Connected;

private:
    std::string m_RecvBuffer;
    std::string m_SendBuffer;
	uint32_t m_LastRecv;
	uint32_t m_LastSend;

public:
	CTCPSocket( );
	CTCPSocket( SOCKET nSocket, struct sockaddr_in nSIN );
	virtual ~CTCPSocket( );

    virtual void Reset( );
	virtual bool GetConnected( )
	{
		return m_Connected;
	}
    virtual std::string *GetBytes( )
	{
		return &m_RecvBuffer;
	}
    virtual void PutBytes( std::string bytes );
	virtual void PutBytes( BYTEARRAY bytes );
	virtual void ClearRecvBuffer( )
	{
		m_RecvBuffer.clear( );
	}
	virtual void ClearSendBuffer( )
	{
		m_SendBuffer.clear( );
	}
	virtual uint32_t GetLastRecv( )
	{
		return m_LastRecv;
	}
	virtual uint32_t GetLastSend( )
	{
		return m_LastSend;
	}
    virtual void DoRecv( fd_set *fd, std::string logmessage );
    virtual void DoSend( fd_set *send_fd, std::string logmessage );
	virtual void Disconnect( );
    virtual void SetNoDelay( bool noDelay );
};

//
// CTCPClient
//

class CTCPClient : public CTCPSocket
{
protected:
	bool m_Connecting;

public:
	CTCPClient( );
	virtual ~CTCPClient( );

	virtual void Reset( );
	virtual void Disconnect( );
	virtual bool GetConnecting( )
	{
		return m_Connecting;
	}
    virtual void Connect( std::string localaddress, std::string address, uint16_t port, std::string logmessage );
	virtual bool CheckConnect( );
};

//
// CTCPServer
//

class CTCPServer : public CTCPSocket
{
public:
	CTCPServer( );
	virtual ~CTCPServer( );

    virtual bool Listen( std::string address, uint16_t port, std::string logmessage );
	virtual CTCPSocket *Accept( fd_set *fd );
};

//
// CUDPSocket
//

class CUDPSocket : public CSocket
{
protected:
	struct in_addr m_BroadcastTarget;
public:
	CUDPSocket( );
	virtual ~CUDPSocket( );

	virtual bool SendTo( struct sockaddr_in sin, BYTEARRAY message );
    virtual bool SendTo( std::string address, uint16_t port, BYTEARRAY message, std::string logmessage );
    virtual bool Broadcast( uint16_t port, BYTEARRAY message, std::string logmessage );
    virtual void SetBroadcastTarget( std::string subnet, std::string logmessage );
	virtual void SetDontRoute( bool dontRoute );
};

//
// CUDPServer
//

class CUDPServer : public CUDPSocket
{
public:
	CUDPServer( );
	virtual ~CUDPServer( );

	virtual bool Bind( struct sockaddr_in sin );
    virtual bool Bind( std::string address, uint16_t port );
    virtual void RecvFrom( fd_set *fd, struct sockaddr_in *sin, std::string *message );
};
#endif
