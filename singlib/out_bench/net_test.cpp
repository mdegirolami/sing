#include "net_test.h"
#include "net.h"
#include "thread.h"
#include "sio.h"
#include "str.h"

//
// Single server connection
// NOTE: work shouldn't assume a single receive gets the full packet.
// if the remote is using stream sockets, the string could need more than one receive 
//
class ServerConnection final : public sing::Runnable {
public:
    virtual void *get__id() const override { return(&id__); };
    bool accept(sing::ListenerSocket *listener);
    virtual void work() override;

    static char id__;

private:
    sing::ConnectedSocket sock_;
};

//
// Server
// NOTE: a real server should recycle closed ServerConnection or allocate them dynamically
// 
class Server final : public sing::Runnable {
public:
    virtual void *get__id() const override { return(&id__); };
    bool init(int32_t port, const sing::IpVersion &ver);
    int32_t getLocalPort() const
    {
        return(listener_.getLocalPort());
    };
    void close()
    {
        listener_.close();
    };
    virtual void work() override;

    static char id__;

private:
    sing::ListenerSocket listener_;
};

//
// Connectionless server
//
class ClessServer final : public sing::Runnable {
public:
    virtual void *get__id() const override { return(&id__); };
    bool init();
    int32_t getLocalPort() const
    {
        return(sock_.getLocalPort());
    };
    virtual void work() override;

    static char id__;

private:
    sing::Socket sock_;
};

static bool testServer(int32_t s_port, int32_t c_port, const sing::IpVersion &ver);
static bool testClessServer();

char ServerConnection::id__;
char Server::id__;
char ClessServer::id__;

bool ServerConnection::accept(sing::ListenerSocket *listener)
{
    return ((*listener).accept(&sock_));
}

void ServerConnection::work()
{
    std::string input;
    while (sock_.rcvString(&input, 100) && sing::len(input.c_str()) > 0) {
        std::string output = sing::formatUnsignedHex((uint64_t)sing::abs(sing::string2int(input)), 20);
        sing::cutLeadingSpaces(&output);
        sock_.sendString(output.c_str());
    }
}

bool Server::init(int32_t port, const sing::IpVersion &ver)
{
    return (listener_.open(port, 5, ver));
}

void Server::work()
{
    for(int32_t count = 1; count < 3; ++count) {
        std::shared_ptr<ServerConnection> connection = std::make_shared<ServerConnection>();
        if (!(*connection).accept(&listener_)) {
            return;
        }
        sing::run(connection);
    }
}

bool ClessServer::init()
{
    return (sock_.open());
}

void ClessServer::work()
{
    std::string input;
    sing::Address src;
    while (sock_.rcvString(&src, &input, 100) && sing::len(input.c_str()) > 0) {
        std::string output = sing::formatUnsignedHex((uint64_t)sing::abs(sing::string2int(input)), 20);
        sing::cutLeadingSpaces(&output);
        sock_.sendString(src, output.c_str());
    }
}

//
// The tester: runs the servers and acts as client
//
bool net_test()
{
    if (!sing::netInit()) {
        return (false);
    }
    if (!testServer(-1, -1, sing::IpVersion::v4)) {
        return (false);
    }
    if (!testServer(7623, 8726, sing::IpVersion::v6)) {
        return (false);
    }
    if (!testClessServer()) {
        return (false);
    }

    // address resolution
    sing::Address google;

    if (!google.set("www.google.com", "https")) {           // https == 443, http = 80
        return (false);
    }

    const std::string gip = google.getIpAddress();
    const int32_t gport = google.getPort();

    if (gip == "" || gport == 0) {
        return (false);
    }

    sing::netShutdown();

    return (true);
}

static bool testServer(int32_t s_port, int32_t c_port, const sing::IpVersion &ver)
{
    std::shared_ptr<Server> srv = std::make_shared<Server>();

    // start the server
    if (!(*srv).init(s_port, ver)) {
        return (false);
    }
    sing::run(srv);

    // get server address
    sing::Address srvadd;
    const int32_t srvport = (*srv).getLocalPort();
    if (ver == sing::IpVersion::v4) {
        srvadd.set(sing::ip4_loopback.c_str(), std::to_string(srvport).c_str());
    } else {
        srvadd.set(sing::ip6_loopback.c_str(), std::to_string(srvport).c_str());
    }

    // connect
    sing::ConnectedSocket cs;

    if (!cs.open(srvadd, c_port)) {
        return (false);
    }

    // make one request
    std::string response;

    cs.sendString("100");
    if (!cs.rcvString(&response, 100)) {
        return (false);
    }
    if (response != "64") {
        return (false);
    }

    // try again with binaries
    std::vector<uint8_t> message;
    message.push_back((uint8_t)49);
    message.push_back((uint8_t)48);
    message.push_back((uint8_t)48);
    cs.send(message, 3);
    if (!cs.rcv(&message, 100, 100, false)) {
        return (false);
    }
    if (message[0] != 54 || cs.timedout()) {
        return (false);
    }

    return (true);
}

static bool testClessServer()
{
    std::shared_ptr<ClessServer> csrv = std::make_shared<ClessServer>();

    // run the server
    if (!(*csrv).init()) {
        return (false);
    }
    sing::run(csrv);

    // get server address
    sing::Address csrvadd;
    const int32_t csrvport = (*csrv).getLocalPort();
    csrvadd.set(sing::ip4_loopback.c_str(), std::to_string(csrvport).c_str());

    // use connectionless datagrams
    sing::Socket cls;
    sing::Address remote;
    std::string response;

    if (!cls.open()) {
        return (false);
    }
    cls.sendString(csrvadd, "100");
    if (!cls.rcvString(&remote, &response, 100)) {
        return (false);
    }
    if (response != "64" || cls.timedout()) {
        return (false);
    }

    // responder should be the server !!
    const std::string remote_ip = remote.getIpAddress();
    const int32_t remote_port = remote.getPort();

    const std::string srv_ip = csrvadd.getIpAddress();
    const int32_t srv_port = csrvadd.getPort();

    if (remote_ip != srv_ip || remote_port != srv_port) {
        return (false);
    }

    // test time out 
    if (cls.rcvString(&remote, &response, 100, 100)) {
        return (false);
    }
    if (!cls.timedout()) {
        return (false);
    }

    return (true);
}
