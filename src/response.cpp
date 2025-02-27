#include "response.hpp"



Response::Response(): _statusCode(200), _bytesSent(0), _readyToSend(false)
{

}

Response::~Response()
{

}

void Response::ft_setResServer(const Server& server)
{
    _resServer = server;
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
    //_readyToSend = true;
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

void Response::ft_processResponse(Request& request, const std::vector<Server>& servers)
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
    const std::string& hostRef = tmp;
    const Server& selectedServer = ft_findServerForRequest(servers, hostRef, requestedPort);
    if (request.ft_getMethod() == "GET")
        ft_handleGET(request, selectedServer);
    else if (request.ft_getMethod() == "POST")
        ft_handlePOST(request, selectedServer);
    else if (request.ft_getMethod() == "DELETE")
        ft_handleDELETE(request, selectedServer);
    else
        ft_setResponse(405, "Method Not Allowed", "text/html");
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
    std::string fullPath = root + path;
    char resolvedPath[PATH_MAX] = "\0";
    if (realpath(fullPath.c_str(), resolvedPath) == NULL)
    {
        ft_setResponse(403, "Forbidden: Path traversal detected", "text/html");
        return;
    }
    fullPath = resolvedPath;
    struct stat pathStat;
    if (stat(fullPath.c_str(), &pathStat) != 0)
    {
        ft_setResponse(404, "Not Found: Resource not found", "text/html");
        return;
    }
    bool autoindex = location && location->ft_getAutoIndex();
    if (S_ISDIR(pathStat.st_mode))
    {
        if (fullPath[fullPath.size() - 1] != '/')
            fullPath += "/";
        std::string indexPath = fullPath + "index.html";
        if (stat(indexPath.c_str(), &pathStat) == 0 && !S_ISDIR(pathStat.st_mode))
        {
            fullPath = indexPath;
        }
        else if (autoindex)
        {
            std::string autoindexHtml = ft_generateAutoindex(fullPath, path);
            if (autoindexHtml.empty())
            {
                ft_setResponse(500, "Internal Server Error: Failed to generate directory listing", "text/html");
                return;
            }
            _body = autoindexHtml;
            ft_setStatus(200, "OK");
            ft_setHeader("Content-Length", ft_toString(_body.size()));
            ft_setHeader("Content-Type", "text/html");
            _readyToSend = true;
            return;
        }
        else
        {
            ft_setResponse(403, "Forbidden: Directory listing is not allowed", "text/html");
            return;
        }
    }
    std::ifstream file(fullPath.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        ft_setResponse(403, "Forbidden: Cannot open file", "text/html");
        return;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    _body = buffer.str();
    file.close();
    ft_setStatus(200, "OK");
    ft_setHeader("Content-Length", ft_toString(_body.size()));
    ft_setHeader("Content-Type", ft_getMimeType(fullPath));
    ft_generateRawResponse();
    _readyToSend = true;
}

void Response::ft_handlePOST(Request& request, const Server& server)
{
    size_t maxBodySize = server.ft_getClientMaxBodySize();

    if (request.ft_getBody().size() > maxBodySize)
    {
        ft_setResponse(413, "Payload Too Large", "text/html");
        return;
    }
    const Location* location = server.ft_findLocation(request.ft_getPath());
    if (!location || location->ft_getUpload().empty())
    {
        ft_setResponse(403, "Forbidden: No upload directory configured", "text/html");
        return;
    }
    std::string uploadPath = location->ft_getUpload();
    struct stat info;
    if (stat(uploadPath.c_str(), &info) != 0 || !S_ISDIR(info.st_mode) || access(uploadPath.c_str(), W_OK) != 0)
    {
        ft_setResponse(403, "Forbidden: Upload directory is not writable", "text/html");
        return;
    }
    std::string filePath = uploadPath + "/upload_" + ft_toString(request.ft_getClientFd()) + "_" + ft_toString(time(NULL)) + ".tmp";
    std::ofstream outFile(filePath.c_str(), std::ios::binary);
    if (!outFile.is_open())
    {
        ft_setResponse(500, "Internal Server Error: Cannot create file", "text/html");
        return;
    }
    outFile.write(request.ft_getBody().c_str(), request.ft_getBody().size());
    if (!outFile.good())
    {
        ft_setResponse(500, "Internal Server Error: Failed to write file", "text/html");
        return;
    }
    outFile.close();
    ft_setResponse(201, "Created: File uploaded successfully at " + filePath, "text/html");
}


void Response::ft_handleDELETE(Request& request, const Server& server)
{
    const Location* location = server.ft_findLocation(request.ft_getPath());
    if (!location || !location->ft_getDeleteAllowed())
    {
        ft_setResponse(403, "Forbidden: DELETE is not allowed for this location", "text/html");
        return;
    }
    std::string root = location->ft_getRoot();
    std::string path = request.ft_getPath();
    if (!root.empty() && root[root.size() - 1] == '/')
        root.erase(root.size() - 1, 1);
    if (!path.empty() && path[0] != '/')
        path = "/" + path;
    std::string fullPath = root + path;
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
    }
    ft_setResponse(204, "No Content", "text/html");
}


std::string Response::ft_generateAutoindex(const std::string& directoryPath, const std::string& requestPath)
{
    DIR* dir = opendir(directoryPath.c_str());
    if (!dir)
    {
        std::cerr << "Error: Cannot open directory " << directoryPath << std::endl;
        return "<html><head><title>500 Internal Server Error</title></head><body><h1>Internal Server Error</h1><p>Failed to generate directory listing.</p></body></html>";
    }
    std::stringstream html;
    html << "<html><head><title>Index of " << requestPath << "</title></head>"
         << "<body><h1>Index of " << requestPath << "</h1><ul>";
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == ".")
            continue;
        std::string href = requestPath;
        if (href[href.size() - 1] != '/')
            href += '/';
        href += name;
        html << "<li><a href=\"" << href << "\">" << name << "</a></li>";
    }
    closedir(dir);
    html << "</ul></body></html>";
    return html.str();
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

std::string Response::ft_getResponseChunk()
{
    if (_bytesSent >= _rawResponse.size())
        return "";

    size_t chunkSize = BUFFER_SIZE;
    if (_bytesSent + chunkSize > _rawResponse.size())
        chunkSize = _rawResponse.size() - _bytesSent;

    return _rawResponse.substr(_bytesSent, chunkSize);
}

void Response::ft_generateErrorResponse(int code)
{
    std::string message;

    switch (code)
    {
    case 400:
        message = "Bad Request";
        break;
    case 403:
        message = "Forbidden";
        break;
    case 404:
        message = "Not Found";
        break;
    case 405:
        message = "Method Not Allowed";
        break;
    case 500:
        message = "Internal Server Error";
        break;
    case 505:
        message = "HTTP Version Not Supported";
        break;
    default:
        message = "Unknown Error";
        break;
    }

    ft_setResponse(code, message);
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

const Location* Response::ft_findMatchingLocation(const Request& request, const Server& server)
{
    const std::vector<Location>& locations = server.ft_getLocations();
    const Location* matchedLocation = NULL;

    for (size_t i = 0; i < locations.size(); i++)
    {
        if (request.ft_getPath().find(locations[i].ft_getPath()) == 0)
        {
            if (!matchedLocation || locations[i].ft_getPath().size() > matchedLocation->ft_getPath().size())
            {
                matchedLocation = &locations[i];
            }
        }
    }

    return matchedLocation;
}

void Response::ft_generateRawResponse()
{
    if (_headers.find("Content-Length") == _headers.end())
        _headers["Content-Length"] = ft_toString(_body.size());
    _headers["Connection"] = "close";
    std::stringstream responseStream;
    responseStream << _statusLine << "\r\n";
    for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); ++it)
        responseStream << it->first << ": " << it->second << "\r\n";
    responseStream << "\r\n";
    responseStream << _body;
    _rawResponse = responseStream.str();
    _bytesSent = 0;
}

void Response::ft_setRedirect(const std::string& newLocation)
{
    _statusCode = 301;
    _statusLine = "Moved Permanently";
    _headers["Location"] = newLocation;
    _headers["Content-Length"] = "0";
    _body.clear();
    //_readyToSend = true;//esta porcaria nao deve estar bem
}