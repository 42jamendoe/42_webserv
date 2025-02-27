#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "defs.hpp"
#include "request.hpp"
#include "response.hpp"

class Connection
{
    private:
    std::vector<struct pollfd> _poll_fds;
    std::set<int> _listen_sockets;
    std::map<int, Request> _clientRequests;
    std::map<int, Response> _responses;
    std::set<int> _readyRequests;
    std::set<int> _readyResponses; 

    public:

    void ft_addListenSockets(const std::vector<int>& listen_sockets);
    void ft_webservRun(const std::vector<Server>& servers);
    void ft_processReadEvent(int fd, const std::vector<Server>& servers);
    void ft_processWriteEvent(int fd);
    void ft_removeClient(int fd);
    void ft_cleanup();
    void ft_acceptNewConnection(int fd);
    void ft_handleClientData(int fd, const std::vector<Server>& servers);
    void ft_processResponse(int fd, const std::vector<Server> &servers);
    bool ft_sendResponse(int fd);
    static void ft_setPollOut(int fd, std::vector<pollfd>& _poll_fds);
    void ft_timeOutRes(int fd);
};

#endif