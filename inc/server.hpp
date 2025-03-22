#ifndef SERVER_HPP
#define SERVER_HPP

#include "defs.hpp"
#include "location.hpp"

class Server
{
    private:
	    int _port;
		std::string _host;
		std::string _root;
		std::string _index;
		std::map<int, std::string> _server_name;
		std::string _error_page;
		size_t 	_client_max_body_size;
		std::vector<Location> _location;
	    bool _isDefault;
    
    public:
        Server();
        ~Server();

        //setters faltam os que acrescentei depois
        void ft_setPort(int port);
        void ft_setHost(const std::string& host);
        void ft_setRoot(const std::string& root);
        void ft_setIndex(const std::string& index);
        void ft_setServerName(const std::string& server_name);
        void ft_setErrorPage(const std::string& error_page);
        void ft_setClientMaxBodySize(size_t client_max_body_size);
        void ft_addLocation(const Location& location);
        void ft_removeLocation(size_t index);
        void ft_setIsDefault(bool value);

        //getters
        int ft_getPort() const;
        const std::string& ft_getHost() const;
        const std::string& ft_getRoot() const;
        const std::string& ft_getIndex() const;
        const std::string& ft_getServerName() const;
        const std::string& ft_getErrorPage() const;
        size_t ft_getClientMaxBodySize() const;
        const Location& ft_getLocation(size_t index) const;
        size_t ft_getLocationCount() const;
        bool ft_getIsDefault() const;

		const Server& findServerForRequest(const std::vector<Server>& servers, const std::string& requestedHost) const;
        const std::vector<Location>& ft_getLocations() const;
		const Location* ft_findLocation(const std::string& path) const;

};

#endif
