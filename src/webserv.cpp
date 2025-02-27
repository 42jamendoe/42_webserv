#include "../inc/webserv.hpp"

Webserv::Webserv(const std::vector<Server>& servers)
{
    for (size_t i = 0; i < servers.size(); i++)
    {
        if (!servers[i].ft_getIsDefault())
            continue;
        struct addrinfo hints;
        struct addrinfo *res = NULL;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        std::string host = servers[i].ft_getHost();
        if (host.empty())
            host = "0.0.0.0";
        std::string port = ft_toString(servers[i].ft_getPort());
        if (getaddrinfo(host.c_str(), port.c_str(), &hints, &res) != 0)
        {
            std::cerr << "Error at getaddrinfo to " << host << ":" << port << std::endl;
            continue;
        }
        int server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (server_socket == -1)
        {
            std::cerr << "Error creating socket at port " << servers[i].ft_getPort() << ": " << strerror(errno) << std::endl;
            freeaddrinfo(res);
            continue;
        }
        if (ft_socketOptions(res, server_socket))
            continue;
        if (ft_bindSocket(servers, i, res, server_socket))
            continue;
        if (ft_listenSocket(servers, i, res, server_socket))
            continue;
        if (ft_nbSocket(res, server_socket))
            continue;
        _listen_sockets.push_back(server_socket);
        std::string showName;
        if (servers[i].ft_getServerName().empty())
            showName = "Unnamed";
        else
            showName = servers[i].ft_getServerName();
        std::cout << "Server " << showName << " configured at port " << servers[i].ft_getPort() << std::endl;
        freeaddrinfo(res);
    }
}

Webserv::~Webserv()
{
    ft_closeSockets();
}

bool Webserv::ft_socketOptions(struct addrinfo *res, int server_socket)
{
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        std::cerr << "Error configuring socket options " << server_socket << ": " << strerror(errno) << std::endl;
        close(server_socket);
        freeaddrinfo(res);
        return true;
    }
    return false;
}

bool Webserv::ft_bindSocket(const std::vector<Server> &servers, size_t i, struct addrinfo *res, int server_socket)
{
    if (bind(server_socket, res->ai_addr, res->ai_addrlen) == -1)
    {
        std::cerr << "Error binding port " << servers[i].ft_getPort() << ": " << strerror(errno) << std::endl;
        close(server_socket);
        freeaddrinfo(res);
        return true;
    }
    return false;
}

bool Webserv::ft_listenSocket(const std::vector<Server> &servers, size_t i, struct addrinfo *res, int server_socket)
{
    if (listen(server_socket, SOMAXCONN) == -1)
    {
        std::cerr << "Error listening socket at port " << servers[i].ft_getPort() << ": " << strerror(errno) << std::endl;
        close(server_socket);
        freeaddrinfo(res);
        return true;
    }
    return false;
}

bool Webserv::ft_nbSocket(struct addrinfo *res, int server_socket)
{
    if (fcntl(server_socket, F_SETFL, O_NONBLOCK) == -1)
    {
        std::cerr << "Error configuring non-blocking socket: " << strerror(errno)  << std::endl;
        close(server_socket);
        freeaddrinfo(res);
        return true;
    }
    return false;
}

const std::vector<int>& Webserv::ft_getServSock() const
{
    return(_listen_sockets);
}

void Webserv::ft_closeSockets()
{
    for (std::vector<int>::iterator it = _listen_sockets.begin(); it != _listen_sockets.end(); ++it)
    {
        close(*it);
        std::cout << "Closing socket " << *it << std::endl;
    }
    _listen_sockets.clear();
}
