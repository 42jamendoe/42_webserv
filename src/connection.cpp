#include "../inc/connection.hpp"

volatile sig_atomic_t g_running = 1;

void ft_handleSignal(int signal)
{
    (void)signal;
    g_running = 0;
    std::cout << "\n🔴 Server shutting down... 🔴" << std::endl;
}

void Connection::ft_addListenSockets(const std::vector<int>& listen_sockets)
{
    for (size_t i = 0; i < listen_sockets.size(); i++)
    {
        struct pollfd pfd = {listen_sockets[i], POLLIN, 0};
        _poll_fds.push_back(pfd);
        _listen_sockets.insert(listen_sockets[i]);
    }
}

void Connection::ft_webservRun(const std::vector<Server>& servers)
{
    std::signal(SIGINT, ft_handleSignal);
    std::cout << GREEN << "🟢 Webserv is running! Waiting for connections..." << RESET << std::endl;

    while (g_running)
    {
        if (_poll_fds.empty())
            continue;
        const int event_count = poll(&_poll_fds[0], _poll_fds.size(), TIMEOUT);
        if (event_count == -1)
        {
            if (errno == EINTR)
                break;
            std::cerr << "Error at poll(): " << strerror(errno) << std::endl;
            continue;
        }
        if (event_count == 0)
            continue;
        for (int i = static_cast<int>(_poll_fds.size()) - 1; i >= 0; i--)
        {
            const int fd = _poll_fds[i].fd;
            const short revents = _poll_fds[i].revents;
            if (revents & POLLIN)
                ft_processReadEvent(fd, servers);            
            if (revents & POLLOUT)
                ft_processWriteEvent(fd);
            if (revents & (POLLHUP | POLLERR | POLLNVAL))
                ft_removeClient(fd);
        }
    }
    ft_cleanup();
}

void Connection::ft_processReadEvent(const int fd, const std::vector<Server>& servers)
{
    if (_listen_sockets.find(fd) != _listen_sockets.end())
    {
        ft_acceptNewConnection(fd);
    }
    else
    {
        ft_handleClientData(fd, servers);
        const time_t lastActivity = _clientRequests[fd].ft_getLastActivity();
        const time_t now = time(NULL);
        const time_t timeOutReq = now - lastActivity;
        if (timeOutReq > CONNECTION_TIMEOUT)
        {
            std::cerr << "⏳ Connection " << fd << " timed out. Sending timeout response and closing." << std::endl;
            ft_timeOutRes(fd);
        }
    }
}

void Connection::ft_processWriteEvent(const int fd)
{
    std::map<int, Response>::iterator it = _responses.find(fd);
    if (it != _responses.end())
        ft_sendResponse(fd);
}

void Connection::ft_removeClient(const int fd)
{
    close(fd);
    _clientRequests.erase(fd);
    _responses.erase(fd);

    for (std::vector<struct pollfd>::iterator it = _poll_fds.begin(); it != _poll_fds.end(); ++it)
    {
        if (it->fd == fd)
        {
            _poll_fds.erase(it);
            break;
        }
    }
    std::cout << "Closed connection (FD: " << fd << ")" << std::endl;
}

void Connection::ft_cleanup()
{
    std::cout << "\n🧹 Cleaning up server resources... 🧹" << std::endl;
    _poll_fds.clear();
    _listen_sockets.clear();
    _clientRequests.clear();
    _responses.clear();
}

void Connection::ft_acceptNewConnection(int listen_fd)
{
    struct sockaddr_in client_addr = {};
    socklen_t addr_len = sizeof(client_addr);
    if (_poll_fds.size() >= MAX_CONNECTIONS)
    {
        std::cerr << "Maximum connections reached. Cannot accept new clients." << std::endl;
        return;
    }
    int client_fd = accept(listen_fd, reinterpret_cast<struct sockaddr *>(&client_addr), &addr_len);
    if (client_fd == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        std::cerr << "Error at accept(): " << strerror(errno) << std::endl;
        return;
    }
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1)
    {
        std::cerr << "Error setting client_fd to non-blocking mode: " << strerror(errno) << std::endl;
        close(client_fd);
        return;
    }   
    const struct pollfd client_pfd = {client_fd, POLLIN, 0};
    _poll_fds.push_back(client_pfd);
    _clientRequests[client_fd] = Request();
    std::cout << "New connection accepted at socket " << client_fd << std::endl;
    _clientRequests[client_fd].ft_setLastActivity();
}

void Connection::ft_handleClientData(int fd, const std::vector<Server>& servers)
{
    char buffer[BUFFER_SIZE];
    const ssize_t bytes_read = recv(fd, buffer, BUFFER_SIZE, 0);
sleep(30);
    if (bytes_read > 0)
    {
        
        if (_clientRequests[fd].ft_appendData(std::string(buffer, bytes_read)))
        {
            _responses[fd] = Response();
            _responses[fd].ft_setResponse(400, "Bad Request", "text/html");
            ft_setPollOut(fd, _poll_fds);
            _clientRequests.erase(fd);
            return ;
        }
        if (_clientRequests[fd].ft_isRequestComplete())
        {
            _responses[fd] = Response();
            if (_clientRequests[fd].ft_parseRequest())
                _responses[fd].ft_processResponse(_clientRequests[fd], servers);
            else
                _responses[fd].ft_setResponse(400, "Bad Request", "text/html");
            ft_setPollOut(fd, _poll_fds);
            _clientRequests.erase(fd);
        }
    }
    else if (bytes_read == 0)
    {
        if (_responses.find(fd) == _responses.end())
            ft_removeClient(fd);
    }
    else if (bytes_read == -1)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            std::cerr << "Error reading from socket " << fd << ": " << strerror(errno) << std::endl;
            ft_removeClient(fd);
        }
    }
}


bool Connection::ft_sendResponse(int fd)
{
    if (_responses.find(fd) == _responses.end())
        return false;
    Response &response = _responses[fd];
    std::string chunk = response.ft_getResponseChunk();
    if (chunk.empty())
    {
        ft_removeClient(fd);
        return true;
    }
    ssize_t bytes_sent = send(fd, chunk.c_str(), chunk.size(), 0);
    if (bytes_sent == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return true;
        std::cerr << "Error sending to socket " << fd << ": " << strerror(errno) << std::endl;
        ft_removeClient(fd);
        return false;
    }
    if (bytes_sent > 0)
        response._bytesSent += bytes_sent;
    if (response._bytesSent >= response._rawResponse.size())
    {
        ft_removeClient(fd);
        return true;
    }

    return true;
}

void Connection::ft_setPollOut(const int fd, std::vector<pollfd>& _poll_fds)
{
    for (size_t i = 0; i < _poll_fds.size(); i++)
    {
        if (_poll_fds[i].fd == fd)
        {
            _poll_fds[i].events |= POLLOUT;
            break;
        }
    }
}

void Connection::ft_timeOutRes(int fd)
{
    std::string timeoutResponse =
        "HTTP/1.1 408 Request Timeout\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 31\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Request took too long. Closing.";

    send(fd, timeoutResponse.c_str(), timeoutResponse.size(), 0);

    std::cerr << "❌ Connection " << fd << " closed due to timeout." << std::endl;

    ft_removeClient(fd);
}