#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "request.hpp"
#include "server.hpp"
#include "cgi.hpp"
#include "defs.hpp"

class Response
{
private:
    int _clientFd;
    int _statusCode;
    std::string _statusLine;
    std::string _body;
    std::map<std::string, std::string> _headers;
    std::string _rawResponse;
    size_t _bytesSent;

public:
    Response();
    Response(int fd);
    ~Response();

    void ft_setResponse(int code, const std::string &message, const std::string &contentType);
    void ft_setStatus(int code, const std::string &message);
    void ft_setHeader(const std::string &key, const std::string &value);
    void ft_generateRawResponse();
    void ft_processResponse(Request &request, Cgi &tmpCgiProcess, const std::vector<Server> &servers, std::unordered_set<pid_t>& pidSet);
    static const Server& ft_findServerForRequest(const std::vector<Server>& servers, const std::string& requestedHost, int requestedPort);
    void ft_handleGET(Request& request, const Server& server);
    std::vector<std::string> ft_getDirectoryListing(const std::string& directoryPath);
    void ft_setBody(const std::string& bodyContent);
    std::string ft_getMimeType(const std::string& path);
    void ft_handlePOST(Request& request, Cgi &tmpCgiProcess, const Server& server, std::unordered_set<pid_t>& pidSet);
    void ft_setResServer(const Server& server);
    void ft_handleDELETE(Request& request, const Server& server);
    std::string ft_getResponseChunk();
    void ft_addBytesSent(size_t bytes);
    size_t ft_getBytesSent() const;
    std::string ft_getRawResponse() const;
};
#endif