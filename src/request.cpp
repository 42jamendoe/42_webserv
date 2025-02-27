#include "../inc/request.hpp"


Request::Request()
{
    ft_setLastActivity();
}

Request::~Request()
{

}

bool Request::ft_isRequestComplete()
{
    size_t headerEnd = _rawRequest.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return false;

    if (_rawRequest.find("Transfer-Encoding: chunked") != std::string::npos)
    {
        if(_rawRequest.rfind("\r\n0\r\n\r\n") == std::string::npos)
            return false;
        return true;
    }
    size_t contentLengthPos = _rawRequest.find("Content-Length:");
    if (contentLengthPos != std::string::npos && contentLengthPos < headerEnd)
    {
        size_t start = contentLengthPos + 16;
        size_t end = _rawRequest.find("\r\n", start);
        if (end != std::string::npos)
        {
            char *endPtr;
            long contentLength = std::strtol(_rawRequest.substr(start, end - start).c_str(), &endPtr, 10);
            if (*endPtr != '\0' || contentLength < 0)
                return false;

            size_t bodyStart = headerEnd + 4;
            if (_rawRequest.size() < bodyStart + static_cast<size_t>(contentLength))
                return false;
        }
    }
    return true;
}

bool Request::ft_parseRequest()
{
    std::istringstream stream(_rawRequest);
    std::string line;

    if (!std::getline(stream, line) || line.empty())
        return false;
    std::istringstream firstLine(line);
    if (!(firstLine >> _method >> _path >> _httpVersion))
        return false;
    while (std::getline(stream, line) && line != "\r")
    {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        size_t colon = line.find(": ");
        if (colon != std::string::npos)
        {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 2);
            if (_headers.count(key) > 0)
            {
                std::cerr << "Error: Duplicate header " << key << "!" << std::endl;
                return false;
            }
            _headers[key] = value;
        }
    }
    std::cout << "Method: " << ft_getMethod() << std::endl;
    std::cout << "Path: " << ft_getPath() << std::endl;
    std::cout << "HTTP Version: " << ft_getHttpVersion() << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
    {
        std::cout << it->first << ": " << it->second << std::endl;
    }
    std::cout << "Body: " << ft_getBody() << std::endl;

    if (_method == "GET" || _method == "DELETE")
        return true;

    std::map<std::string, std::string>::iterator te = _headers.find("Transfer-Encoding");
    if (te != _headers.end() && te->second == "chunked")
        return ft_parseChunkedBody();
    std::map<std::string, std::string>::iterator cl = _headers.find("Content-Length");
    if (cl != _headers.end())
    {
        char *endPtr = NULL;
        long contentLengthValue = std::strtol(cl->second.c_str(), &endPtr, 10);
        if (*endPtr != '\0' || contentLengthValue < 0 )
        {
            std::cerr << "Error: Invalid Content-Length!" << std::endl;
            return false;
        }
        _body.resize(contentLengthValue);
        stream.read(&_body[0], contentLengthValue);
        if (stream.gcount() != contentLengthValue)
        {
            std::cerr << "Error: uncompleted body!" << std::endl;
            return false;
        }
    }

    if (!_method.empty() && !_path.empty() && !_httpVersion.empty())
        return true;
    return false;
}

std::string Request::ft_getMethod() const
{
    return _method;
}

std::string Request::ft_getPath() const
{
    return _path;
}

std::string Request::ft_getHttpVersion() const
{
    return _httpVersion;
}

std::map<std::string, std::string> Request::ft_getHeaders() const
{
    return _headers;
}

std::string Request::ft_getBody() const
{
    return _body;
}

std::string Request::ft_getHeader(const std::string &key) const
{
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);
    if (it != _headers.end())
        return it->second;
    return "";
}

// void Request::ft_appendData(const std::string& data)
// {
//     _rawRequest += data;
// }

bool Request::ft_appendData(const std::string& data)
{
    if (_rawRequest.size() + data.size() > MAX_HEADER_SIZE)
    {
        std::cerr << "Request exceeded maximum allowed size (" << MAX_HEADER_SIZE << " bytes)." << std::endl;
        return true;
    }
    _rawRequest += data;
    return false;
}

std::string Request::ft_getRawRequest() const
{
    return _rawRequest;
}

bool Request::ft_parseChunkedBody()
{
    size_t pos = 0;
    while (pos < _rawRequest.size())
    {
        size_t chunkSizeEnd = _rawRequest.find("\r\n", pos);
        if (chunkSizeEnd == std::string::npos)
            return false;
        std::istringstream hexStream(_rawRequest.substr(pos, chunkSizeEnd - pos));
        int chunkSize;
        hexStream >> std::hex >> chunkSize;
        if (chunkSize == 0)
            return true;
        pos = chunkSizeEnd + 2;
        if (_rawRequest.size() < pos + chunkSize + 2)
            return false;
        _body.append(_rawRequest.substr(pos, chunkSize));
        pos += chunkSize + 2;
    }
    return true;
}

int Request::ft_getClientFd() const
{
    return _clientFd;
}

void Request::ft_setLastActivity()
{
    const time_t timestamp = time(NULL);
    _lastActivity = timestamp;
}

time_t Request::ft_getLastActivity() const
{
    return _lastActivity;
}