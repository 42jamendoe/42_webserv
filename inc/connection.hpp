#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "defs.hpp"
#include "request.hpp"
#include "response.hpp"
#include "cgi.hpp"

class Server;

class Connection
{
    private:
    std::vector<struct pollfd> _pollFds;
    std::set<int> _listenSockets;
    std::map<int, Cgi> _cgiP;
    std::map<int, Request> _reqP;
    std::map<int, Response> _resP;
    std::unordered_set<pid_t> _pidsRunning;

    public:
    Connection();
    ~Connection();
    void ft_addListenSockets(const std::vector<int>& listen_sockets);
    void ft_webservRun(const std::vector<Server>& servers);
    void ft_monitorRequests();
    void ft_monitorCgi();
    void ft_timeOutRes(int fd);
    void ft_removeClient(int fd);
    void ft_processReadEvent(int fd, const std::vector<Server>& servers, std::unordered_set<pid_t>& pidSet);
    void ft_acceptNewConnection(int fd);
    void ft_receiveCgiResponse(Cgi &cgiProcess);
    void ft_handleClientData(int fd, const std::vector<Server>& servers, std::unordered_set<pid_t>& pidSet);
    static void ft_setPollOut(int fd, std::vector<pollfd> &_poll_fds);
    void ft_processWriteEvent(int fd);
    bool ft_sendResponse(int fd);
    void ft_sendCgi(Cgi &cgiProcess);
    void ft_cleanup();

};

#endif