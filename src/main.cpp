#include "defs.hpp"
#include "../inc/config.hpp"
#include "../inc/webserv.hpp"
#include "../inc/connection.hpp"

const char* ft_checkFileAccess(const int argc, char **argv)
{
	if (argc > 2)
		throw std::invalid_argument("webserv usage: ./webserv <Valid_config_path/config_name.conf>");
	const char* tmp;
	if (argc == 2)
		tmp = argv[1];
	else
		tmp = DEFAULT_CONFIG;
	if (access(tmp, R_OK) != 0)
        throw std::runtime_error("webserv error: " + std::string(tmp) + ": " + strerror(errno));
	std::cout << BOLD_BLUE << std::endl << "Configuration: " << RESET << std::endl;
	std::cout << MAGENTA << "Using configuration file: " << RESET << tmp << std::endl << std::endl;
	return tmp;
}

int main(const int argc, char **argv)
{
    try
	{
    	const std::string configPath(ft_checkFileAccess(argc, argv));
		const Config config(configPath);
		std::cout << BOLD_GREEN << "Configuration loaded with success!" << RESET << std::endl;
		config.ft_printConfigurations();
		const Webserv setup(config.ft_getServerConfigurations());
		Connection con_setup;
		con_setup.ft_addListenSockets(setup.ft_getServSock());
		con_setup.ft_webservRun(config.ft_getServerConfigurations());
    }
	catch (const std::exception &e)
	{
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
