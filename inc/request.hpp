#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "defs.hpp"
#include "server.hpp"

class Request
{
private:
    time_t _lastActivity;
    int _clientFd;
    std::string _rawRequest;
    std::string _method;
    std::string _path;
    std::string _httpVersion;
    std::map<std::string, std::string> _headers;
    std::string _body;

public:
    Request();
    Request(int fd);
    ~Request();

    time_t ft_getLastActivity() const;
    void ft_setLastActivity();
    bool ft_appendData(const std::string& data);
    bool ft_isRequestComplete();
    bool ft_isBodyComplete(size_t headerEnd);
    bool ft_parseRequest();
    bool ft_parseChunkedBody();
    std::string ft_getHttpVersion() const;
    std::string ft_getHeader(const std::string &key) const;
    std::string ft_getMethod() const;
    std::string ft_getPath() const;
};

#endif
