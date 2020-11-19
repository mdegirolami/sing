#pragma once

#include <sing.h>

namespace sing {

class ConnectedSocket;

typedef int64_t Sock;
enum class IpVersion {v4, v6};

class Address final {
public:
    Address();
    bool set(const char *ip_address, const char *port);
    std::string getIpAddress() const;
    int32_t getPort() const;

    // for internal use
    sing::array<int64_t, 5> add_;
    int32_t len_;
};

class ListenerSocket final {
public:
    ListenerSocket();
    ~ListenerSocket();
    bool open(int32_t localport, int32_t queue_length, IpVersion ipv = IpVersion::v4);
    void close();
    bool accept(ConnectedSocket *socket, int32_t ms_timeout = -1);
    int32_t getLocalPort() const;
    bool timedout() const;

private:
    Sock sock_;
    bool tout_;
};

class ConnectedSocket final {
public:
    ConnectedSocket();
    ~ConnectedSocket();
    bool open(const Address &addr, int32_t localport = -1);
    void close();

    int32_t getLocalPort() const;
    void getRemoteAddress(Address *addr) const;
    bool timedout() const;

    bool rcvString(std::string *value, int64_t maxbytes, int32_t ms_timeout = -1);
    bool rcv(std::vector<uint8_t> *dst, int64_t count, int32_t ms_timeout = -1, bool append = true);

    bool sendString(const char *value);
    bool send(const std::vector<uint8_t> &src, int64_t count, int64_t from = 0);

    // internal use
    void setSock(int64_t v);
    void setRemote(const Address &remote);

private:
    Address remote_;
    Sock sock_;
    bool tout_;
};

class Socket final {
public:
    Socket();
    ~Socket();
    bool open(int32_t localport = -1, IpVersion ipv = IpVersion::v4);
    void close();

    int32_t getLocalPort() const;
    bool timedout() const;

    bool rcvString(Address *add, std::string *value, int64_t maxbytes, int32_t ms_timeout = -1);
    bool rcv(Address *add, std::vector<uint8_t> *dst, int64_t count, int32_t ms_timeout = -1, bool append = true);

    bool sendString(const Address &add, const char *value);
    bool send(const Address &add, const std::vector<uint8_t> &src, int64_t count, int64_t from = 0);

private:
    Sock sock_;
    bool tout_;
};

bool netInit();
void netShutdown();

extern const std::string ip4_loopback;
extern const std::string ip6_loopback;

}   // namespace
