#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include "defs.hpp"
#include "server.hpp"

class Webserv
{
    private:
    std::vector<int> _listen_sockets;
    static bool ft_socketOptions(struct addrinfo *res, int server_socket);
    static bool ft_bindSocket(const std::vector<Server> &servers, size_t i, struct addrinfo *res, int server_socket);
    static bool ft_listenSocket(const std::vector<Server> &servers, size_t i, struct addrinfo *res, int server_socket);
    static bool ft_nbSocket(struct addrinfo *res, int server_socket);
    public:

    explicit Webserv(const std::vector<Server>& servers);
    ~Webserv();

    const std::vector<int>& ft_getServSock() const;
    void ft_closeSockets();
};

#endif
