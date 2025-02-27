#include "../inc/server.hpp"


Server::Server() : _port(0), _client_max_body_size(0), _isDefault(true)
{

}

Server::~Server()
{

}

void Server::ft_setPort(int port)
{
    this->_port = port;
}

void Server::ft_setHost(const std::string& host)
{
    this->_host = host;
}

void Server::ft_setRoot(const std::string& root)
{
    this->_root = root;
}

void Server::ft_setIndex(const std::string& index)
{
    this->_index = index;
}

void Server::ft_setServerName(const std::string& server_name) {
    this->_server_name = server_name;
}

void Server::ft_setErrorPage(const std::string& error_page) {
    this->_error_page = error_page;
}

void Server::ft_setClientMaxBodySize(size_t client_max_body_size)
{
    this->_client_max_body_size = client_max_body_size;
}

void Server::ft_addLocation(const Location& location)
{
    _location.push_back(location);
}

void Server::ft_removeLocation(size_t index)
{
    if (index >= _location.size())
        throw std::out_of_range("Index out of range");
    _location[index] = _location[_location.size() - 1];
    _location.pop_back();
}

void Server::ft_setIsDefault(bool value)
{
    this->_isDefault = value;
}

int Server::ft_getPort() const
{
    return _port;
}

const std::string& Server::ft_getHost() const
{
    return _host;
}

const std::string& Server::ft_getRoot() const
{
    return _root;
}

const std::string& Server::ft_getIndex() const
{
    return _index;
}

const std::string& Server::ft_getServerName() const
{
    return _server_name;
}

const std::string& Server::ft_getErrorPage() const
{
    return _error_page;
}

size_t Server::ft_getClientMaxBodySize() const
{
    return _client_max_body_size;
}

const Location& Server::ft_getLocation(size_t index) const
{
    if (index >= _location.size())
        throw std::out_of_range("Index out of range");
    return _location[index];
}

size_t Server::ft_getLocationCount() const
{
    return _location.size();
}

bool Server::ft_getIsDefault() const
{
    return _isDefault;
}

const std::vector<Location>& Server::ft_getLocations() const
{
    return _location;
}

const Location* Server::ft_findLocation(const std::string& path) const
{
    const Location* bestMatch = NULL;
    size_t bestMatchLength = 0;

    for (size_t i = 0; i < _location.size(); i++)
    {
        const Location& loc = _location[i];
        const std::string& locPath = loc.ft_getPath();

        std::string locPathNormalized = locPath;
        if (!locPathNormalized.empty() && locPathNormalized[locPathNormalized.size() - 1] != '/')
            locPathNormalized += "/";
        if (path.find(locPathNormalized) == 0)
        {
            const size_t matchLength = locPathNormalized.size();
            if (matchLength > bestMatchLength)
            {
                bestMatch = &loc;
                bestMatchLength = matchLength;
            }
        }
    }
    return bestMatch;
}