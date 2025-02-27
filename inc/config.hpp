#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "defs.hpp"
#include "server.hpp"
#include "location.hpp"

class Config
{
private:
    std::vector<Server> _servers;
    
    void ft_openConfig(const std::string &configPath);
    void ft_parseConfig(std::ifstream& file);
    static void ft_cleanLine(std::string& line);
    static bool ft_handleServerBlock(const std::string& line, Server& currentServer, bool& inServerBlock);
    static bool ft_handleLocationBlock(const std::string& line, Location& currentLocation, bool inServerBlock, bool& inLocationBlock);
    static void ft_parseLocationPath(const std::string &line, Location &location);
    bool ft_closeBlocks(const std::string& line, Server& currentServer, const Location& currentLocation, bool& inServerBlock, bool& inLocationBlock);
    static void ft_parseServer(const std::string &line, Server &server);
    static void ft_parseLocation(const std::string &line, Location &location);
    void ft_validateConfig() const;
    void ft_defineDefaultServers();

public:
    Config();
    explicit Config(const std::string& configPath);
    ~Config();

    void ft_printConfigurations() const;
    const std::vector<Server>& ft_getServerConfigurations() const;
};

#endif
