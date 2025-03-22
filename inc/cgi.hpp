#ifndef CGI_HPP
#define CGI_HPP

#include "defs.hpp"
#include "request.hpp"

class Cgi
{
private:
	bool _isCgi;
	int _clientFd;
	std::vector<std::string> _env;
	std::vector<char*> _envChar;
	std::vector<std::string> _args;
	std::vector<char*> _argChar;
	int _pipeIn[2];
	int _pipeOut[2];
	pid_t _pid;
	Server _cgiServer;
	std::string _cgiMessage;
	std::string _scriptDir;
	std::string _scriptName;
public:
	Cgi();
	Cgi(int Fd);
    ~Cgi();

	void ft_validateCgiSetup(const Location& location);
	void ft_setupCgi(const Location& location, Request& request);
	void ft_executeCgi(const Location& location, std::unordered_set<pid_t>& pidSet);
	void ft_setCgiServer(const Server& server);
	int ft_getClientFd() const;
	int ft_getPipeInReadFd() const;
	int ft_getPipeOutWriteFd() const;
	pid_t ft_getPid() const;
	std::string ft_getCgiMessage() const;
	std::string ft_getScriptDir() const;
	std::string ft_getScriptName() const;
	void ft_setScriptDir(const std::string& dir);
	void ft_setScriptName(const std::string& name);
	bool ft_getIsCgi() const;
};

#endif
