#include "../inc/config.hpp"


Config::Config()
{

}

Config::Config(const std::string& configPath)
{
    ft_openConfig(configPath);
    ft_validateConfig();
    ft_defineDefaultServers();
}

Config::~Config()
{

}

void Config::ft_openConfig(const std::string& configPath)
{
    std::ifstream file(configPath.c_str());
    if (!file.is_open())
        throw std::runtime_error("Failed to open configuration file!");
    ft_parseConfig(file);           
}

void Config::ft_parseConfig(std::ifstream& file)
{
    Server currentServer;
    Location currentLocation;
    std::string line;
    bool inServerBlock = false;
    bool inLocationBlock = false;

    while (std::getline(file, line))
    {
        ft_cleanLine(line);
        if (line.empty())
            continue;
        if (ft_handleServerBlock(line, currentServer, inServerBlock))
            continue;
        if (ft_handleLocationBlock(line, currentLocation, inServerBlock, inLocationBlock))
            continue;
        if (ft_closeBlocks(line, currentServer, currentLocation, inServerBlock, inLocationBlock))
            continue;
        if (inServerBlock && !inLocationBlock)
            ft_parseServer(line, currentServer);
        else if (inLocationBlock)
            ft_parseLocation(line, currentLocation);
        else
            throw std::invalid_argument("Invalid configuration outside of a block: " + line);
    }
}

void Config::ft_cleanLine(std::string& line)
{
    size_t hashPos = line.find('#');
    if (hashPos != std::string::npos)
        line.erase(hashPos);
    line.erase(0, line.find_first_not_of("  "));
    line.erase(line.find_last_not_of("  ") + 1);
    if (std::count(line.begin(), line.end(), ';') > 1)
        throw std::invalid_argument("Multiple ';' found in line: " + line);
    if (line.empty())
        return ;
    if (line[line.length() - 1] != ';')
    {
        if (line.find("server ") == 0 || line == "server" || line.find("location") == 0 || line.find('}') == 0)
            return ;
        throw std::invalid_argument("Line should end with ';': " + line);
    }
    line.erase(line.length() - 1);
}

bool Config::ft_handleServerBlock(const std::string& line, Server& currentServer, bool& inServerBlock)
{
    if (line.find("server") == 0 && (line.length() == 6 || line[6] == ' '))
    {
        if (inServerBlock)
            throw std::invalid_argument("Cannot start a new server block before closing the previous one");
        if (line.find('{') == 7)
        {
            inServerBlock = true;
            currentServer = Server();
            return true;
        }
        else
            throw std::invalid_argument("Invalid server block declaration: expected 'server {'");
    }
    return false;
}

bool Config::ft_handleLocationBlock(const std::string& line, Location& currentLocation, bool inServerBlock, bool& inLocationBlock)
{
    if (line.find("location") == 0)
    {
        if (!inServerBlock)
            throw std::invalid_argument("Location block must be inside a server block");
        if (inLocationBlock)
            throw std::invalid_argument("Cannot start a new location block before closing the previous one");
        if (line.find('{') != std::string::npos)
        {
            inLocationBlock = true;
            currentLocation = Location();
            ft_parseLocationPath(line, currentLocation);
            return true;
        }
        else
            throw std::invalid_argument("Invalid location block syntax: expected 'location <path> {'");
    }
    return false;
}

void Config::ft_parseLocationPath(const std::string &line, Location& location)
{
    size_t start = line.find("location") + 8;
    size_t end = line.find('{');
    if (end == std::string::npos)
        throw std::runtime_error("Invalid location block format");
    std::string path = line.substr(start, end - start);
    path.erase(0, path.find_first_not_of("  "));
    path.erase(path.find_last_not_of("  ") + 1);
    location.ft_setPath(path);
}

bool Config::ft_closeBlocks(const std::string& line, Server& currentServer, \
    const Location& currentLocation, bool& inServerBlock, bool& inLocationBlock)
{
    if (line == "}")
    {
        if (inLocationBlock)
        {
            inLocationBlock = false;
            currentServer.ft_addLocation(currentLocation);
            return true;
        }
        if (inServerBlock)
        {
            inServerBlock = false;
            _servers.push_back(currentServer);
            return true;
        }
        throw std::invalid_argument("Unexpected closing brace: no open server or location block to close");
    }
    return false;
}

void Config::ft_parseServer(const std::string &line, Server& server)
{
    std::istringstream iss(line);
    std::string key;
    iss >> key;
    if (key == "port")
    {
        int tmp;
        if (!(iss >> tmp))
            throw std::runtime_error("Invalid port number");
        server.ft_setPort(tmp);
    }
    else if (key == "client_max_body_size")
    {
        std::string size;
        iss >> size;
        unsigned long tmpSize = 0;
        std::stringstream sizeStream(size);
        sizeStream >> tmpSize;
        if (sizeStream.fail())
            throw std::runtime_error("Invalid client_max_body_size value");
        server.ft_setClientMaxBodySize(tmpSize);
    }
    else
    {
        std::string value;
        iss >> value;
        if (key == "host")
            server.ft_setHost(value);
        else if (key == "root")
            server.ft_setRoot(value);
        else if (key == "index")
            server.ft_setIndex(value);
        else if (key == "server_name")
            server.ft_setServerName(value);
        else if (key == "error_page")
            server.ft_setErrorPage(value);
        else
            throw std::runtime_error("Unknown key in server config: " + key);
    }
    std::string extra;
    if (iss >> extra)
        throw std::runtime_error("Unexpected extra values in server directive: " + line);
}

void Config::ft_parseLocation(const std::string &line, Location& location)
{
    std::istringstream iss(line);
    std::string key;
    std::string value;
    iss >> key;

    if (key == "limit_methods")
    {
        while (iss >> value)
            location.ft_addLimitMethods(value);
    }
    else
    {
        iss >> value;
        if (key == "root")
            location.ft_setRoot(value);
        else if (key == "index")
            location.ft_setIndex(value);
        else if (key == "return")
            location.ft_setRedirection(value);
        else if (key == "auto_index")
        {
            if (value == "on")
                location.ft_setAutoIndex(true);
            else
                location.ft_setAutoIndex(false);
        }
        else if (key == "try_file")
            location.ft_setTryFile(value);
        else if (key == "cgi_path")
            location.ft_setCgiPath(value);
        else if (key == "cgi_ext")
            location.ft_setCgiExt(value);
        else if (key == "upload")
            location.ft_setUpload(value);
        else
            throw std::runtime_error("Unknown key in location config: " + key);
    }
    std::string extra;
    if (iss >> extra)
        throw std::runtime_error("Unexpected extra values in location directive: " + line);
}

void Config::Config::ft_validateConfig() const
{
    if (_servers.empty())
        throw std::runtime_error("Configuration must have at least one server block");
    std::set<int> used_ports;
    for (std::vector<Server>::const_iterator it = _servers.begin(); it != _servers.end(); ++it)
    {
        if (it->ft_getPort() < 1 || it->ft_getPort() > 65535)
            throw std::runtime_error("Invalid port number: " + ft_toString(it->ft_getPort()));
        for (size_t i = 0; i < it->ft_getLocationCount(); ++i)
        {
            const Location& loc = it->ft_getLocation(i);
            if (loc.ft_getPath().empty())
                throw std::runtime_error("Location block must specify a valid path");
            if (loc.ft_getPath()[0] != '/')
                throw std::runtime_error("Location path must start with '/'");
            if (loc.ft_getAutoIndex() != true && loc.ft_getAutoIndex() != false)
                throw std::runtime_error("Invalid value for auto_index");
            // if (loc.ft_getRoot().empty())
            //     throw std::runtime_error("Location block must specify a root path");
            for (std::set<std::string>::const_iterator itl = loc.ft_getLimitMethods().begin(); itl != loc.ft_getLimitMethods().end(); ++itl)
            {
                const std::string& method = *itl;
                if (method != "GET" && method != "POST" && method != "DELETE")
                    throw std::runtime_error("Invalid HTTP method: " + method);
            }
            if (!loc.ft_getCgiExt().empty() && loc.ft_getCgiPath().empty())
                throw std::runtime_error("CGI extension defined but no CGI path provided");
        }
    }
}

void Config::ft_defineDefaultServers()
{
    for (std::vector<Server>::iterator main = this->_servers.begin(); main != this->_servers.end(); ++main)
    {
        main->ft_setIsDefault(true);
        for (std::vector<Server>::iterator second = main + 1; second != this->_servers.end(); ++second)
        {
            if (main->ft_getHost() == second->ft_getHost() && main->ft_getPort() == second->ft_getPort())
            {
                second->ft_setIsDefault(false);
                if ((main->ft_getServerName().empty() && second->ft_getServerName().empty())
                    || (!main->ft_getServerName().empty() && !second->ft_getServerName().empty() &&
                    main->ft_getServerName() == second->ft_getServerName()))
                    throw std::runtime_error("Duplicate servers");
            }
        }
    }
}

void Config::ft_printConfigurations() const
{
    int l = 1;
    for (std::vector<Server>::const_iterator it = _servers.begin(); it != _servers.end(); ++it)
    {
        const Server& config = *it;
        std::cout << std::endl << BLUE << "***********************************************************" << RESET << std::endl << std::endl;
        std::cout << BOLD_GREEN << "SERVER: #" << l << RESET << std::endl << std::endl;
        std::cout << MAGENTA << "Port: " << CYAN << config.ft_getPort() << RESET << std::endl;
        std::cout << MAGENTA << "Host: " << CYAN << config.ft_getHost() << RESET << std::endl;
        std::cout << MAGENTA << "Root: " << CYAN << config.ft_getRoot() << RESET << std::endl;
        std::cout << MAGENTA << "Index: " << CYAN << config.ft_getIndex() << RESET << std::endl;
        std::cout << MAGENTA << "Server name: " << CYAN << config.ft_getServerName() << RESET << std::endl;
        std::cout << MAGENTA << "Error page: " << CYAN << config.ft_getErrorPage() << RESET << std::endl;
        std::cout << MAGENTA << "Client max body size: " << CYAN << config.ft_getClientMaxBodySize() << RESET << std::endl;
        std::cout << MAGENTA << "Default: " << CYAN << config.ft_getIsDefault() << RESET << std::endl;

        for (size_t i = 0; i < config.ft_getLocationCount(); ++i)
        {
            const Location& loc = config.ft_getLocation(i);

            std::cout << std::endl << BOLD_GREEN << "Path: " << loc.ft_getPath() << RESET << std::endl;
            std::cout << "Root: " << loc.ft_getRoot() << std::endl;
            std::cout << "Index: " << loc.ft_getIndex() << std::endl;
            for (std::set<std::string>::const_iterator itl = loc.ft_getLimitMethods().begin(); itl != loc.ft_getLimitMethods().end(); ++itl)
            {
                std::cout << "Limit methods: " << *itl << std::endl;
            }
            std::cout << "Redirection: " << loc.ft_getRedirection() << std::endl;
            std::cout << "Auto index: " << loc.ft_getAutoIndex() << std::endl;
            std::cout << "Try file: " << loc.ft_getTryFile() << std::endl;
            std::cout << "Cgi path: " << loc.ft_getCgiPath() << std::endl;
            std::cout << "Cgi ext: " << loc.ft_getCgiExt() << std::endl;
            std::cout << "Upload: " << loc.ft_getUpload() << std::endl;
        }
        std::cout << std::endl << BLUE << "***********************************************************" << RESET << std::endl << std::endl;
        l++;
    }
}

const std::vector<Server> &Config::ft_getServerConfigurations() const
{
    return _servers;
}
