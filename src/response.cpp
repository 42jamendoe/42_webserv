#include "response.hpp"

Response::Response(): _clientFd(-1)
{

}

Response::Response(int fd): _clientFd(fd), _statusCode(200), _bytesSent(0)
{

}

Response::~Response()
{

}

void Response::ft_setResponse(int code, const std::string &message, const std::string &contentType)
{
    ft_setStatus(code, message);
    std::stringstream bodyStream;
    bodyStream << "<html><head><title>" << code << " " << message
               << "</title></head><body><h1>" << code << " " << message
               << "</h1></body></html>";

    _body = bodyStream.str();
    ft_setHeader("Content-Type", contentType);
    ft_setHeader("Content-Length", ft_toString(_body.size()));
    ft_generateRawResponse();
}

void Response::ft_setStatus(const int code, const std::string &message)
{
    _statusCode = code;
    std::ostringstream ss;
    ss << "HTTP/1.1 " << code << " " << message;
    _statusLine = ss.str();
}

void Response::ft_setHeader(const std::string &key, const std::string &value)
{
    _headers[key] = value;
}

void Response::ft_generateRawResponse()
{
    if (_headers.find("Content-Length") == _headers.end())
        _headers["Content-Length"] = ft_toString(_body.size());
    std::stringstream responseStream;
    responseStream << _statusLine << "\r\n";
    for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); ++it)
        responseStream << it->first << ": " << it->second << "\r\n";
    responseStream << "\r\n";
    responseStream << _body;
    _rawResponse = responseStream.str();
    _bytesSent = 0;
}

void Response::ft_processResponse(Request &request, Cgi &tmpCgiProcess, const std::vector<Server> &servers, std::unordered_set<pid_t>& pidSet)
{
    if (request.ft_getHttpVersion() != "HTTP/1.1")
    {
        ft_setResponse(505, "HTTP Version Not Supported", "text/html");
        return;
    }
    std::string host = request.ft_getHeader("Host");
    int requestedPort = -1;
    std::string tmp;
    if (host.empty())
    {
        ft_setResponse(400, "Bad Request: Host header is required", "text/html");
        return;
    }
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos)
    {
        tmp = host.substr(0, colonPos);
        const std::string portStr = host.substr(colonPos + 1);
        char *endPtr;
        long portValue = std::strtol(portStr.c_str(), &endPtr, 10);
        if (*endPtr == '\0' && portValue > 0 && portValue <= 65535)
            requestedPort = static_cast<int>(portValue);
    }
    else
        tmp = host;
    const std::string& hostRef = tmp;
    const Server& selectedServer = ft_findServerForRequest(servers, hostRef, requestedPort);
    if (request.ft_getMethod() == "GET")
        ft_handleGET(request, selectedServer);
    else if (request.ft_getMethod() == "POST")
        ft_handlePOST(request, tmpCgiProcess, selectedServer, pidSet);
    else if (request.ft_getMethod() == "DELETE")
        ft_handleDELETE(request, selectedServer);
    else
        ft_setResponse(405, "Method Not Allowed", "text/html");
}

const Server& Response::ft_findServerForRequest(const std::vector<Server>& servers, const std::string& requestedHost, const int requestedPort)
{
    const Server* defaultServer = NULL;

    for (size_t i = 0; i < servers.size(); ++i)
    {
        if (servers[i].ft_getServerName() == requestedHost)
            return servers[i];
        if (servers[i].ft_getIsDefault())
        {
            if (!defaultServer || servers[i].ft_getPort() == requestedPort)
                defaultServer = &servers[i];
        }
    }
    if (defaultServer)
        return *defaultServer;
    return servers[0];
}

void Response::ft_handleGET(Request& request, const Server& server)
{
    std::string root = server.ft_getRoot();
    std::string path = request.ft_getPath();

    if (!root.empty() && root[root.size() - 1] == '/')
        root.erase(root.size() - 1, 1);
    if (!path.empty() && path[0] != '/')
        path = "/" + path;
    const Location* location = server.ft_findLocation(path);
    if (location)
        root = location->ft_getRoot();
    if (root.empty())
    {
        ft_setResponse(500, "Internal Server Error: No root directory configured", "text/html");
        return;
    }
    if (!location->ft_isMethodAllowed("GET"))
    {
        ft_setResponse(405, "Method Not Allowed", "text/html");
        return;
    }
    if (location->ft_isRedirect())
    {
        ft_setResponse(301, "Moved Permanently", "text/html");
        return;
    }
    bool isDirectory = (path[path.size() - 1] == '/');
    if (isDirectory && !server.ft_getIndex().empty())
        path += server.ft_getIndex();
    std::string fullPath = root + path;
    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) == -1)
    {
        ft_setResponse(404, "Not Found", "text/html");
        return;
    }
    if (S_ISDIR(fileStat.st_mode))
    {
        if (location->ft_getAutoIndex())
        {
            std::vector<std::string> files = ft_getDirectoryListing(fullPath);
            if (!files.empty())
            {
                std::stringstream listContent;
                for (size_t i = 0; i < files.size(); ++i)
                    listContent << "<a href='" << path + files[i] << "'>" << files[i] << "</a><br>";
                std::string fileContent = listContent.str();
                ft_setResponse(200, "OK", "text/html");
                ft_setHeader("Content-Length", ft_toString(fileContent.size()));
                ft_setBody(fileContent);
                ft_generateRawResponse();
                return;
            }
        }
        ft_setResponse(404, "Not Found", "text/html");
        return;
    }
    if (S_ISREG(fileStat.st_mode))
    {
        int file_fd = open(fullPath.c_str(), O_RDONLY | O_NONBLOCK);
        if (file_fd < 0)
        {
            ft_setResponse(500, "Internal Server Error: File open failed", "text/html");
            return;
        }
        std::string mimeType = ft_getMimeType(fullPath);
        if (mimeType.empty())
            mimeType = "application/octet-stream";
        char buffer[8192];
        std::string fileContent;
        ssize_t bytesRead;
        while ((bytesRead = read(file_fd, buffer, sizeof(buffer))) > 0)
            fileContent.append(buffer, bytesRead);
        close(file_fd);
        if (bytesRead < 0)
        {
            ft_setResponse(500, "Internal Server Error: File read failed", "text/html");
            return;
        }
        ft_setResponse(200, "OK", mimeType);
        ft_setHeader("Content-Length", ft_toString(fileContent.size()));
        ft_setBody(fileContent);
        ft_generateRawResponse();
        return;
    }
    ft_setResponse(404, "Not Found", "text/html");
}

std::vector<std::string> Response::ft_getDirectoryListing(const std::string& directoryPath)
{
    std::vector<std::string> files;
    DIR* dir = opendir(directoryPath.c_str());
    if (dir)
    {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_name[0] != '.')
                files.push_back(entry->d_name);
        }
        closedir(dir);
    }
    return files;
}

void Response::ft_setBody(const std::string& bodyContent)
{
    _body = bodyContent;
}

std::string Response::ft_getMimeType(const std::string& path)
{
    static std::map<std::string, std::string> mimeTypes;

    mimeTypes[".html"] = "text/html";
    mimeTypes[".htm"] = "text/html";
    mimeTypes[".css"] = "text/css";
    mimeTypes[".js"] = "application/javascript";
    mimeTypes[".json"] = "application/json";
    mimeTypes[".png"] = "image/png";
    mimeTypes[".jpg"] = "image/jpeg";
    mimeTypes[".jpeg"] = "image/jpeg";
    mimeTypes[".gif"] = "image/gif";
    mimeTypes[".txt"] = "text/plain";
    mimeTypes[".pdf"] = "application/pdf";

    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos)
        return "application/octet-stream";
    std::string ext = path.substr(dotPos);
    for (size_t i = 0; i < ext.size(); ++i)
        ext[i] = std::tolower(ext[i]);
    std::map<std::string, std::string>::const_iterator it = mimeTypes.find(ext);
    if (it != mimeTypes.end())
        return it->second;
    return "application/octet-stream";
}

void Response::ft_handlePOST(Request& request, Cgi &tmpCgiProcess, const Server& server, std::unordered_set<pid_t>& pidSet)
{
    try
    {
        const Location* location = server.ft_findLocation(request.ft_getPath());
        if (!location)
        {
            ft_setResponse(404, "Not Found","text/html");
            return;
        }
        Location tempLocation = *location;
        if (tempLocation.ft_getRoot().empty())
            tempLocation.ft_setRoot(server.ft_getRoot());
        size_t lastSlashPos = request.ft_getPath().find_last_of('/');
        if (lastSlashPos == std::string::npos) {
            tmpCgiProcess.ft_setScriptDir("./");
            tmpCgiProcess.ft_setScriptName(request.ft_getPath());
        }
        else
        {
            tmpCgiProcess.ft_setScriptDir(request.ft_getPath().substr(0, lastSlashPos));
            tmpCgiProcess.ft_setScriptName(request.ft_getPath().substr(lastSlashPos + 1));
        }
        tmpCgiProcess.ft_validateCgiSetup(tempLocation);
        tmpCgiProcess.ft_setupCgi(tempLocation, request);
        tmpCgiProcess.ft_executeCgi(tempLocation, pidSet);
        tmpCgiProcess.ft_setCgiServer(server);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error at CGI: " << e.what() << std::endl;
        ft_setResponse(500, "Internal Server Error", "text/html");
    }
}

void Response::ft_handleDELETE(Request& request, const Server& server)
{
    const Location* location = server.ft_findLocation(request.ft_getPath());
    if (!location || !location->ft_isMethodAllowed(request.ft_getMethod()))
    {
        ft_setResponse(403, "Forbidden: DELETE is not allowed for this location", "text/html");
        return;
    }
    std::string root = location->ft_getRoot();
    if (root.empty())
        root = server.ft_getRoot();
    if (root.empty())
    {
        ft_setResponse(500, "Internal Server Error: No root directory configured", "text/html");
        return;
    }
    std::string path = request.ft_getPath();
    if (!root.empty() && root[root.size() - 1] == '/')
        root.erase(root.size() - 1, 1);
    if (!path.empty() && path[0] != '/')
        path = "/" + path;
    std::string relativePath = path.substr(location->ft_getPath().size());
    if (!relativePath.empty() && relativePath[0] == '/')
        relativePath.erase(0, 1);
    std::string fullPath = root + '/' + relativePath;
    if (fullPath.find("..") != std::string::npos)
    {
        ft_setResponse(403, "Forbidden: Path traversal detected", "text/html");
        return;
    }
    struct stat pathStat;
    if (stat(fullPath.c_str(), &pathStat) != 0)
    {
        ft_setResponse(404, "Not Found: File or directory does not exist", "text/html");
        return;
    }
    if (S_ISDIR(pathStat.st_mode))
    {
        DIR* dir = opendir(fullPath.c_str());
        if (!dir)
        {
            ft_setResponse(500, "Internal Server Error: Cannot open directory", "text/html");
            return;
        }
        struct dirent* entry;
        bool isEmpty = true;
        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            {
                isEmpty = false;
                break;
            }
        }
        closedir(dir);
        if (!isEmpty)
        {
            ft_setResponse(403, "Forbidden: Cannot delete a non-empty directory", "text/html");
            return;
        }
        if (rmdir(fullPath.c_str()) != 0)
        {
            ft_setResponse(500, "Internal Server Error: Failed to delete directory", "text/html");
            return;
        }
    }
    else
    {
        if (unlink(fullPath.c_str()) != 0)
        {
            ft_setResponse(500, "Internal Server Error: Failed to delete file", "text/html");
            return;
        }
        else
        {
            ft_setResponse(200, "The file was deleted!", "text/html");
            return;
        }
    }
    ft_setResponse(204, "No Content", "text/html");
}

std::string Response::ft_getResponseChunk()
{
    if (_bytesSent >= _rawResponse.size())
        return "";

    size_t chunkSize = BUFFER_SIZE;
    if (_bytesSent + chunkSize > _rawResponse.size())
        chunkSize = _rawResponse.size() - _bytesSent;

    return _rawResponse.substr(_bytesSent, chunkSize);
}

void Response::ft_addBytesSent(size_t bytes)
{
    _bytesSent += bytes;
}

size_t Response::ft_getBytesSent() const
{
    return _bytesSent;
}

std::string Response::ft_getRawResponse() const
{
    return _rawResponse;
}
