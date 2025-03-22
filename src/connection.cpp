#include "../inc/connection.hpp"

int g_running = 1;

void ft_handleSignal(int signal)
{
    (void)signal;
    g_running = 0;
    std::cout << "\n🔴 Server shutting down... 🔴\n" << std::endl;
}

Connection::Connection()
{

}

Connection::~Connection()
{

}

void Connection::ft_addListenSockets(const std::vector<int>& listen_sockets)
{
    for (size_t i = 0; i < listen_sockets.size(); i++)
    {
        struct pollfd pFd = {listen_sockets[i], POLLIN, 0};
        _pollFds.push_back(pFd);
        _listenSockets.insert(listen_sockets[i]);
    }
}

void Connection::ft_webservRun(const std::vector<Server>& servers)
{
    std::signal(SIGINT, ft_handleSignal);
    std::signal(SIGPIPE, SIG_IGN);
    std::cout << GREEN << "🟢 Webserv is running! Waiting for connections..." << RESET << std::endl;

    while (g_running)
    {
        if (_pollFds.empty())
            continue;
        ft_monitorRequests();
        const int event_count = poll(&_pollFds[0], _pollFds.size(), TIMEOUT);
        if (event_count == -1)
        {
            if (errno == EINTR)
                break;
            std::cerr << "Error at poll(): " << strerror(errno) << std::endl;
            continue;
        }
        if (event_count == 0)
            continue;
        for (int i = static_cast<int>(_pollFds.size()) - 1; i >= 0; i--)
        {
            const int fd = _pollFds[i].fd;
            const short revents = _pollFds[i].revents;
            if (revents & POLLIN)
                ft_processReadEvent(fd, servers,_pidsRunning);
            if (revents & POLLOUT)
                ft_processWriteEvent(fd);
            if (revents & (POLLHUP | POLLERR | POLLNVAL))
                ft_removeClient(fd);
        }
    }
    ft_cleanup();
}

void Connection::ft_monitorRequests()
{
    int status;
    if (!_pidsRunning.empty())
    {
        for (std::unordered_set<pid_t>::iterator it = _pidsRunning.begin(); it != _pidsRunning.end(); )
        {
            const pid_t pid = *it;
            int ret = waitpid(pid, &status, WNOHANG);
            if (ret > 0)
            {
                std::cerr << "CGI Process " << pid << " finalized." << std::endl;
                it = _pidsRunning.erase(it);  // Remove do set
            }
            else
                ++it;
        }
    }
    time_t now = time(NULL);
    for (std::map<int, Request>::iterator it = _reqP.begin(); it != _reqP.end(); )
    {
        if (now - it->second.ft_getLastActivity() > CONNECTION_TIMEOUT)
        {
            std::cerr << "Request timeout. Closing connection " << it->first << std::endl;
            ft_timeOutRes(it->first);
            it = _reqP.erase(it);  // Remove request e avança
        }
        else
            ++it;
    }
}

void Connection::ft_timeOutRes(int const fd)
{
    std::string const timeoutResponse =
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

void Connection::ft_removeClient(const int fd)
{
    std::map<int, Cgi>::iterator it_cgi = _cgiP.find(fd);
    if (it_cgi != _cgiP.end())
    {
        const pid_t cgi_pid = it_cgi->second.ft_getPid();
        close(it_cgi->second.ft_getPipeInReadFd());  // pipeIn[1] (escrita do pai)
        close(it_cgi->second.ft_getPipeOutWriteFd());
        kill(cgi_pid, SIGKILL);
        _cgiP.erase(fd);
        _pidsRunning.erase(cgi_pid);
    }
    std::cout << "The socket " << fd << " timed out and will be closed." << std::endl;
    close(fd);
    _reqP.erase(fd);
    _resP.erase(fd);
    for (std::vector<struct pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); )
    {
        if (it->fd == fd)
        {
            _pollFds.erase(it);
            break;
        }
        else
            ++it;
    }
}//verificar que estou a remover os pipes e a fazer kill do pid

void Connection::ft_processReadEvent(const int fd, const std::vector<Server>& servers, std::unordered_set<pid_t>& pidSet)
{
    if (_listenSockets.find(fd) != _listenSockets.end())
        ft_acceptNewConnection(fd);
    else if (_cgiP.find(fd) != _cgiP.end())
        ft_receiveCgiResponse(_cgiP[fd]);
    else
        ft_handleClientData(fd, servers, pidSet);
}

void Connection::ft_acceptNewConnection(int listen_fd)
{
    struct sockaddr_in client_addr = {};
    socklen_t addr_len = sizeof(client_addr);
    if (_pollFds.size() >= MAX_CONNECTIONS)
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
    _pollFds.push_back(client_pfd);
    _reqP[client_fd] = Request(client_fd);
    std::cout << "New connection accepted at socket " << client_fd << std::endl;
}

void Connection::ft_receiveCgiResponse(Cgi &cgiProcess)
{
    int pipeFd = cgiProcess.ft_getPipeInReadFd();
    if (pipeFd == -1)
        return;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = read(pipeFd, buffer, sizeof(buffer) - 1);
    if (bytes_received == -1)
    {
        std::cerr << "❌ Erro ao ler do pipe do CGI" << std::endl;
        close(pipeFd);
        for (std::vector<pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it) {
            if (it->fd == pipeFd) {
                _pollFds.erase(it);
                break;
            }
        }
        _cgiP.erase(pipeFd);
        return;
    }
    if (bytes_received == 0)
    {

        close(pipeFd);
        for (std::vector<pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it)
        {
            if (it->fd == pipeFd)
            {
                _pollFds.erase(it);
                break;
            }
        }
        _cgiP.erase(pipeFd);
        return;
    }
    buffer[bytes_received] = '\0';
    int clientFd = cgiProcess.ft_getClientFd();
    _resP[clientFd] = Response(clientFd);
    _resP[clientFd].ft_setResponse(201, std::string(buffer, bytes_received), "text/plain");
}

void Connection::ft_handleClientData(int fd, const std::vector<Server>& servers, std::unordered_set<pid_t>& pidSet)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    const ssize_t bytes_read = recv(fd, buffer, BUFFER_SIZE, 0);

    if (bytes_read > 0)
    {
        if (_reqP[fd].ft_appendData(std::string(buffer, bytes_read)))
        {
            _resP[fd] = Response(fd);
            _resP[fd].ft_setResponse(999, "payload to large", "text/html");
            ft_setPollOut(fd, _pollFds);// Aqui tb devo tirar o fd do map requests e eventualmente do cgi
            return ;
        }
        if (_reqP[fd].ft_isRequestComplete())
        {
            _resP[fd] = Response(fd);
            if (_reqP[fd].ft_parseRequest())
            {
                Cgi tmpCgiProcess(fd);
                _resP[fd].ft_processResponse(_reqP[fd], tmpCgiProcess, servers, pidSet);
                if (tmpCgiProcess.ft_getIsCgi() == true)
                {
                    ft_setPollOut(tmpCgiProcess.ft_getClientFd(), _pollFds);
                    struct pollfd firstPipe = {tmpCgiProcess.ft_getPipeOutWriteFd(), POLLOUT, 0};
                    struct pollfd secondPipe = {tmpCgiProcess.ft_getPipeInReadFd(), POLLIN, 0};
                    _pollFds.push_back(firstPipe);
                    _pollFds.push_back(secondPipe);
                    _cgiP[tmpCgiProcess.ft_getPipeOutWriteFd()] = tmpCgiProcess;
                    _cgiP[tmpCgiProcess.ft_getPipeInReadFd()] = tmpCgiProcess;
                }
                else
                    ft_setPollOut(fd, _pollFds);
            }
            else
            {
                _resP[fd].ft_setResponse(400, "Bad Request", "text/html");
                ft_setPollOut(fd, _pollFds);
            }
        }
    }
    else if (bytes_read == 0)
    {
        if (_resP.find(fd) == _resP.end())
        {
            //std::cerr << "so pa testar outro" << std::endl;
            //ft_removeClient(fd); //aqui devo tirar de todos os mapas onde está (não vale a pena enviar responta pq o cliente ja nao está

            return ;
        }
    }
    else if (bytes_read == -1)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            //std::cerr << "Error reading from socket " << fd << ": " << strerror(errno) << std::endl;
            ft_removeClient(fd);//aqui devo tirar de todos os mapas onde está e lançar um erro ver nginx numero
        }
    }
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

void Connection::ft_processWriteEvent(const int fd)
{
    std::map<int, Response>::iterator it = _resP.find(fd);
    if (it != _resP.end())
    {
        ft_sendResponse(fd);
        return ;
    }
    std::map<int, Cgi>::iterator itCgi = _cgiP.find(fd);
    if (itCgi != _cgiP.end())
        ft_sendCgi(_cgiP[fd]);
}

bool Connection::ft_sendResponse(int fd)
{
    if (_resP.find(fd) == _resP.end())
        return false;
    Response &response = _resP[fd];
    std::string chunk = response.ft_getResponseChunk();
    if (chunk.empty())
    {
        if (_reqP[fd].ft_getHeader("Connection") == "keep-alive")
            _reqP[fd].ft_setLastActivity();
        else
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
        response.ft_addBytesSent(bytes_sent);
    if (response.ft_getBytesSent() >= response.ft_getRawResponse().size())
    {
        if (_reqP[fd].ft_getHeader("Connection") == "keep-alive")
        {
            _resP.erase(fd);
            _reqP[fd] = Request(fd);
            _reqP[fd].ft_setLastActivity();
        }
        else
            ft_removeClient(fd);
        return true;
    }
    return true;
}

void Connection::ft_sendCgi(Cgi& tmpCgiProcess)
{
    int pipeFd = tmpCgiProcess.ft_getPipeOutWriteFd();

    if (pipeFd == -1)
        return;

    if (!tmpCgiProcess.ft_getCgiMessage().empty())
    {
        ssize_t bytes_sent = write(pipeFd, tmpCgiProcess.ft_getCgiMessage().c_str(), tmpCgiProcess.ft_getCgiMessage().size());

        if (bytes_sent == -1)
        {
            std::cerr << "❌ Erro ao escrever no pipe do CGI" << std::endl;
            close(pipeFd);
            for (std::vector<pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it)
            {
                if (it->fd == pipeFd)
                {
                    _pollFds.erase(it);
                    break;
                }
            }
            _cgiP.erase(pipeFd);
            return;
        }
        tmpCgiProcess.ft_getCgiMessage().erase(0, bytes_sent);
    }
    if (tmpCgiProcess.ft_getCgiMessage().empty())
    {
        close(pipeFd);
        for (std::vector<pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it)
        {
            if (it->fd == pipeFd)
            {
                _pollFds.erase(it);
                break;
            }
        }
        _cgiP.erase(pipeFd);
    }
}

void Connection::ft_cleanup()
{
    std::cout << "\n🧹 Cleaning up server resources... 🧹" << std::endl;
    _pollFds.clear();
    _listenSockets.clear();
    _reqP.clear();
    _resP.clear();
    _cgiP.clear();
}
