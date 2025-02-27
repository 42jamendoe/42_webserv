#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "request.hpp"
#include "server.hpp"
#include "location.hpp"
#include "defs.hpp"
// #include <iostream>
// #include <fstream>
// #include <map>
// #include <sstream>
// #include <sys/stat.h>
// #include <unistd.h>


class Response
{
    public:
    Server _resServer;
    int _statusCode;
    std::string _statusLine;
    std::map<std::string, std::string> _headers;
    std::string _body;
    std::string _rawResponse;
    size_t _bytesSent;
    bool _readyToSend;

    Response();
    ~Response();
    // void ft_generateResponse(Request &request, const std::vector<Server> &servers);
    void ft_setResServer(const Server& server);
    void ft_setResponse(int code, const std::string &message, const std::string &contentType = "text/html");
    void ft_setStatus(int code, const std::string &message);
    void ft_setHeader(const std::string &key, const std::string &value);
    void ft_handleGET(Request& request, const Server& server);
    void ft_handlePOST(Request& request, const Server& server);
    void ft_handleDELETE(Request& request, const Server& server);
    void ft_generateErrorResponse(int code);
    void ft_processResponse(Request& request, const std::vector<Server>& servers);
    static const Server& ft_findServerForRequest(const std::vector<Server>& servers, const std::string& requestedHost, int requestedPort);
    const Location* ft_findMatchingLocation(const Request& request, const Server& server);
    std::string ft_generateAutoindex(const std::string& directoryPath, const std::string& requestPath);
    std::string ft_getMimeType(const std::string& path);
    void ft_generateRawResponse();
    void ft_setRedirect(const std::string& newLocation);


public:
    std::string ft_getResponseChunk();
    std::string toString();
};
#endif