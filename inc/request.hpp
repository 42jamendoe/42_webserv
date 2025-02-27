#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>
#include "server.hpp"
#include <sstream>
#include <sys/socket.h>
#include <algorithm>

class Request
{
private:
    std::string _rawRequest;
    std::string _method;
    std::string _path;
    std::string _httpVersion;
    std::map<std::string, std::string> _headers;
    std::string _body;
    time_t _lastActivity;
    //bool _isReady;
    int _clientFd;
    
public:

    Request();
    ~Request();

    bool ft_isRequestComplete();
    
    std::string ft_getMethod() const;
    std::string ft_getPath() const;
    std::string ft_getHttpVersion() const;
    std::map<std::string, std::string> ft_getHeaders() const;
    std::string ft_getBody() const;
    void ft_setLastActivity();
    time_t ft_getLastActivity() const;
    
    bool ft_parseRequest();
    std::string ft_getHeader(const std::string &key) const;
    //Response Request::ft_processRequest(int fd, const std::vector<Server>& servers);
    bool ft_appendData(const std::string& data);
    std::string ft_getRawRequest() const;
    bool ft_parseChunkedBody();
    int ft_getClientFd() const;

};

#endif
