#define _WIN32_WINNT  0x600     // vista
#include "net.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    //#include <netdb.h>
    #include <sys/socket.h>

    #define closesocket(a) close(a) 
#endif

namespace sing {

#ifdef _WIN32

void utf8_to_16(const char *src, std::vector<wchar_t> *dst);

static int getaddrinfoL(const char *nodename, const char *servname, const struct addrinfo *hints, struct addrinfo **res)
{
    if (nodename != nullptr) {
        std::vector<wchar_t> wname;
        std::vector<wchar_t> wservice;
        utf8_to_16(nodename, &wname);
        utf8_to_16(servname, &wservice);
        return(GetAddrInfoW(wname.data(), wservice.data(), (const ADDRINFOW*)hints, (PADDRINFOW*)res));
    }
    return(::getaddrinfo(nodename, servname, hints, res));
}

#else

typedef int SOCKET
static inline closesocket(int a) { ::close(a); }
static const int INVALID_SOCKET = -1;
static inline int getaddrinfoL(const char *nodename, const char *servname, const struct addrinfo *hints, struct addrinfo **res)
{
    return(::getaddrinfo(nodename, servname, hints, res));
}

#endif

///////////////////
//
// Utilities
//
//////////////////
const std::string ip4_loopback = "127.0.0.1";
const std::string ip6_loopback = "::1";

static bool createBoundSocketIp4(int32_t port, Sock *out_socket, int socket_type)
{
    struct sockaddr_in addr;

    memset(addr.sin_zero, 0, sizeof(addr.sin_zero));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    SOCKET socket = ::socket(addr.sin_family, socket_type, 0);
    if (socket == INVALID_SOCKET) {
        return(false);
    }
    if (bind(socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(socket);
        *out_socket = (Sock)INVALID_SOCKET;
        return(false);
    }
    *out_socket = (Sock)socket;
    return(true);
}

static bool createBoundSocketIp6(int32_t port, Sock *out_socket, int socket_type)
{
    struct sockaddr_in6 addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(port);

    SOCKET socket = ::socket(addr.sin6_family, socket_type, 0);
    if (socket == INVALID_SOCKET) {
        return(false);
    }
    if (bind(socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(socket);
        *out_socket = (Sock)INVALID_SOCKET;
        return(false);
    }
    *out_socket = (Sock)socket;
    return(true);
}

static bool createBoundSocket(int32_t port, Sock *socket, int socket_type, IpVersion ipv)
{
    if (port < 0) port = 0;
    if (ipv == IpVersion::v4) {
        return(createBoundSocketIp4(port, socket, socket_type));
    } else if (ipv == IpVersion::v6) {
        return(createBoundSocketIp6(port, socket, socket_type));
    }
    return(false);
}

static int32_t getLocalPortFromSocket(SOCKET socket) 
{   
    Address addr;

    socklen_t len = sizeof(addr.add_);
    if (getsockname(socket, (struct sockaddr *)&addr.add_[0], &len) == 0) {
        addr.len_ = len;
        return(addr.getPort());
    }
    return(-1);
}

//
// returns false for error/tout
//
static bool waitForSocket(int32_t tout, SOCKET socket, bool *timedout)
{
    fd_set fds;
    struct timeval tv;

    tout = std::min(tout, 2000000);
    tout = std::max(tout, 0);
    tv.tv_sec = 0;
    tv.tv_usec = tout * 1000;
    int nn;
    do {
        FD_ZERO(&fds);
        FD_SET(socket, &fds);
        nn = select(socket+1, &fds, nullptr, nullptr, &tv);
    } while (nn == -1 && errno == EINTR);
    *timedout = nn == 0;
    return(nn == 1);
}

///////////////////
//
// Address implementation
//
//////////////////
Address::Address()
{
    len_ = 0;
}

bool Address::set(const char *ip_address, const char *port)
{
    struct addrinfo *servinfo; // will point to the results
    bool result = false;

    if (ip_address == nullptr || ip_address[0] == 0) return(false);
    if (port == nullptr || port[0] == 0) return(false);
    if (getaddrinfoL(ip_address, port, nullptr, &servinfo) != 0) {
        return(false);
    }
    for(struct addrinfo *ptr=servinfo; ptr != nullptr; ptr=ptr->ai_next) {
        if (ptr->ai_family == AF_INET || ptr->ai_family == AF_INET6) {
            if (ptr->ai_addrlen < sizeof(add_)) {
                len_ = ptr->ai_addrlen;
                memcpy(&add_[0], ptr->ai_addr, len_);
                result = true;
                break;
            }
        }
    }
    freeaddrinfo(servinfo);
    return(result);
}

std::string Address::getIpAddress() const
{
    char print_buffer[INET6_ADDRSTRLEN];

    if (len_ == 0) return("");
    short family = ((sockaddr_in*)&add_[0])->sin_family;
    if (family == AF_INET) {
        if (inet_ntop(AF_INET, &((sockaddr_in*)&add_[0])->sin_addr, print_buffer, INET6_ADDRSTRLEN) != nullptr) {
            return(print_buffer);
        }
    } else if (family == AF_INET6) {
        if (inet_ntop(AF_INET6, &((sockaddr_in6*)&add_[0])->sin6_addr, print_buffer, INET6_ADDRSTRLEN) != nullptr) {
            return(print_buffer);
        }
    }
    return("");
}

int32_t Address::getPort() const
{
    if (len_ == 0) return(-1);
    short family = ((sockaddr_in*)&add_[0])->sin_family;
    if (family == AF_INET) {
        return(ntohs(((sockaddr_in*)&add_[0])->sin_port));
    } else if (family == AF_INET6) {
        return(ntohs(((sockaddr_in6*)&add_[0])->sin6_port));
    }
    return(-1);
}

///////////////////
//
// ListenerSocket implementation
//
//////////////////
ListenerSocket::ListenerSocket()
{
    sock_ = INVALID_SOCKET;
    tout_ = false;
}

ListenerSocket::~ListenerSocket()
{
    close();
}

bool ListenerSocket::open(int32_t localport, int32_t queue_length, const IpVersion &ipv)
{
    if (!createBoundSocket(localport, &sock_, SOCK_STREAM, ipv)) {
        return(false);
    }
    if (listen((SOCKET)sock_, std::max(queue_length, 1)) == SOCKET_ERROR) {
        close();
        return(false);
    }
    return(true);
}

void ListenerSocket::close()
{
    if (sock_ != INVALID_SOCKET) {
        closesocket((SOCKET)sock_);
        sock_ = INVALID_SOCKET;
    }
}

bool ListenerSocket::accept(ConnectedSocket *socket, int32_t ms_timeout)
{
    Address remote;
    SOCKET newsock;

    if (ms_timeout >= 0 && !waitForSocket(ms_timeout, (SOCKET)sock_, &tout_)) {
        return(false);
    }
    socklen_t len;
    do {
        len = sizeof(remote.add_);
        newsock = ::accept((SOCKET)sock_, (sockaddr*)&remote.add_[0], &len);
    } while (newsock == INVALID_SOCKET && errno == EINTR);
    if (newsock == INVALID_SOCKET) {
        return(false);
    }
    remote.len_ = len;
    socket->setSock(newsock);
    socket->setRemote(remote);
    return(true);
}
  
int32_t ListenerSocket::getLocalPort() const
{
    return(getLocalPortFromSocket((SOCKET)sock_));
}

bool ListenerSocket::timedout() const
{
    return(tout_);
}

///////////////////
//
// ConnectedSocket implementation
//
//////////////////
ConnectedSocket::ConnectedSocket()
{
    sock_ = INVALID_SOCKET;
    tout_ = false;
}

ConnectedSocket::~ConnectedSocket()
{
    close();
}

bool ConnectedSocket::open(const Address &addr, int32_t localport)
{
    short family = ((sockaddr_in*)&addr.add_[0])->sin_family;
    IpVersion ipv = family == AF_INET ? IpVersion::v4 : IpVersion::v6;
    if (!createBoundSocket(localport, &sock_, SOCK_STREAM, ipv)) {
        return(false);
    }
    int retvalue;
    do {
        retvalue = connect((SOCKET)sock_, (sockaddr*)&addr.add_[0], addr.len_);
    } while (retvalue == SOCKET_ERROR && errno == EINTR);
    if (retvalue == SOCKET_ERROR) {
        close();
        return(false);
    }
    remote_ = addr;
    return(true);
}

void ConnectedSocket::close()
{
    if (sock_ != INVALID_SOCKET) {
        closesocket((SOCKET)sock_);
        sock_ = INVALID_SOCKET;
    }
}

int32_t ConnectedSocket::getLocalPort() const
{
    return(getLocalPortFromSocket((SOCKET)sock_));
}

void ConnectedSocket::getRemoteAddress(Address *addr) const
{
    *addr = remote_;
}

bool ConnectedSocket::timedout() const
{
    return(tout_);
}

bool ConnectedSocket::rcvString(std::string *value, int64_t maxbytes, int32_t ms_timeout)
{
    if (ms_timeout >= 0 && !waitForSocket(ms_timeout, (SOCKET)sock_, &tout_)) {
        return(false);
    }
    value->resize(maxbytes);
    int received;
    do {
        received = recv((SOCKET)sock_, (char*)value->data(), maxbytes, 0);
    } while (received == -1 && errno == EINTR);
    value->resize(std::max(received, 0));

    // 0 means 'connection closed'
    return(received > 0);
}

bool ConnectedSocket::rcv(std::vector<uint8_t> *dst, int64_t count, int32_t ms_timeout, bool append)
{
    if (ms_timeout >= 0 && !waitForSocket(ms_timeout, (SOCKET)sock_, &tout_)) {
        return(false);
    }
    if (!append) dst->clear();
    int orig_size = dst->size();
    dst->resize(orig_size + count);
    uint8_t *pdst = dst->data() + orig_size;
    int received;
    do {
        received = recv((SOCKET)sock_, (char*)pdst, count, 0);
    } while (received == -1 && errno == EINTR);

    // 0 means 'connection closed'
    if (received > 0) {
        dst->resize(orig_size + received);
        return(true);
    }
    dst->resize(orig_size);
    return(false);
}

bool ConnectedSocket::sendString(const char *value)
{
    const char *src = value;
    int len = strlen(value);
    while (len > 0) {
        int sent = ::send((SOCKET)sock_, src, len, 0);
        if (sent <= 0) {
            if (errno != EINTR) return(false);
            sent = 0;
        }
        src += sent;
        len -= sent;
    }
    return(true);
}

bool ConnectedSocket::send(const std::vector<uint8_t> &src, int64_t count, int64_t from)
{
    const uint8_t *psrc = src.data();
    int64_t len = (int64_t)src.size();
    if (from >= len) return(true);
    psrc += from;
    len -= from;
    len = std::min(len, count);
    while (len > 0) {
        int sent = ::send((SOCKET)sock_, (const char*)psrc, len, 0);
        if (sent <= 0) {
            if (errno != EINTR) return(false);
            sent = 0;
        }
        psrc += sent;
        len -= sent;
    }
    return(true);
}

// internal use
void ConnectedSocket::setSock(int64_t v)
{
    sock_ = v;
}

void ConnectedSocket::setRemote(const Address &remote)
{
    remote_ = remote;
}

///////////////////
//
// unconnected Sockets
//
//////////////////
Socket::Socket()
{
    sock_ = INVALID_SOCKET;
    tout_ = false;
}

Socket::~Socket()
{
    close();
}

bool Socket::open(int32_t localport, const IpVersion &ipv)
{
    return(createBoundSocket(localport, &sock_, SOCK_DGRAM, ipv));
}

void Socket::close()
{
    if (sock_ != INVALID_SOCKET) {
        closesocket((SOCKET)sock_);
        sock_ = INVALID_SOCKET;
    }
}

int32_t Socket::getLocalPort() const
{
    return(getLocalPortFromSocket((SOCKET)sock_));
}

bool Socket::timedout() const
{
    return(tout_);
}

bool Socket::rcvString(Address *add, std::string *value, int64_t maxbytes, int32_t ms_timeout)
{
    if (ms_timeout >= 0 && !waitForSocket(ms_timeout, (SOCKET)sock_, &tout_)) {
        return(false);
    }
    value->resize(maxbytes);
    socklen_t len;
    int received;
    do {
        len = sizeof(add->add_);
        received = recvfrom((SOCKET)sock_, (char*)value->data(), maxbytes, 0, (sockaddr*)&add->add_[0], &len);
    } while (received == -1 && errno == EINTR);
    if (received >= 0) add->len_ = len;
    value->resize(std::max(received, 0));

    // 0 is a legal datagram length
    return(received >= 0);
}

bool Socket::rcv(Address *add, std::vector<uint8_t> *dst, int64_t count, int32_t ms_timeout, bool append)
{
    if (ms_timeout >= 0 && !waitForSocket(ms_timeout, (SOCKET)sock_, &tout_)) {
        return(false);
    }
    if (!append) dst->clear();
    int orig_size = dst->size();
    dst->resize(orig_size + count);
    uint8_t *pdst = dst->data() + orig_size;
    socklen_t len;
    int received;
    do {
        len = sizeof(add->add_);
        received = recvfrom((SOCKET)sock_, (char*)pdst, count, 0, (sockaddr*)&add->add_[0], &len);
    } while (received == -1 && errno == EINTR);

    // 0 is a legal datagram length
    if (received >= 0) {
        add->len_ = len;
        dst->resize(orig_size + received);
        return(true);
    }
    dst->resize(orig_size);
    return(false);
}

bool Socket::sendString(const Address &add, const char *value)
{
    const char *src = value;
    int len = strlen(value);
    while (len > 0) {
        int sent = ::sendto((SOCKET)sock_, src, len, 0, (sockaddr*)&add.add_[0], add.len_);
        if (sent <= 0) {
            if (errno != EINTR) return(false);
            sent = 0;
        }
        src += sent;
        len -= sent;
    }
    return(true);
}

bool Socket::send(const Address &add, const std::vector<uint8_t> &src, int64_t count, int64_t from)
{
    const uint8_t *psrc = src.data();
    int64_t len = (int64_t)src.size();
    if (from >= len) return(true);
    psrc += from;
    len -= from;
    len = std::min(len, count);
    while (len > 0) {
        int sent = ::sendto((SOCKET)sock_, (const char*)psrc, len, 0, (sockaddr*)&add.add_[0], add.len_);
        if (sent <= 0) {
            if (errno != EINTR) return(false);
            sent = 0;
        }
        psrc += sent;
        len -= sent;
    }
    return(true);
}

///////////////////
//
// free functions implementation
//
//////////////////

#ifdef _WIN32

bool netInit()
{
    WSADATA wsaData;
    return(WSAStartup(MAKEWORD(2,2), &wsaData) == 0);
}

void netShutdown()
{
    WSACleanup();
}

#else

bool netInit()
{
    return(true);
}

void netShutdown()
{
}

#endif

} // namespace
