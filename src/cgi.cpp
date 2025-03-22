#include "../inc/cgi.hpp"



Cgi::Cgi()
{

}

Cgi::Cgi(int Fd) : _isCgi(false), _clientFd(Fd), _pid(-1)
{

}

Cgi::~Cgi()
{

}

void Cgi::ft_validateCgiSetup(const Location& location)
{
    if (location.ft_getLimitMethods().find("POST") == location.ft_getLimitMethods().end())
        throw std::runtime_error("Method POST is not allowed in this location.");
    std::string fullCgiPath = location.ft_getRoot() + '/' + location.ft_getCgiPath();
    struct stat dirStat;
    if (stat(location.ft_getRoot().c_str(), &dirStat) == -1 || !S_ISDIR(dirStat.st_mode))
        throw std::runtime_error("Root directory is invalid or does not exist.");
    if (stat(fullCgiPath.c_str(), &dirStat) == -1 || !S_ISDIR(dirStat.st_mode))
        throw std::runtime_error("CGI directory is invalid or does not exist.");
    std::string fullScriptPath = fullCgiPath + "/" + ft_getScriptName();
    struct stat scriptStat;
    if (stat(fullScriptPath.c_str(), &scriptStat) == -1 || !S_ISREG(scriptStat.st_mode))
        throw std::runtime_error("Script file does not exist or is not a regular file.");
    size_t dotPos = ft_getScriptName().find_last_of('.');
    if (dotPos == std::string::npos)
        throw std::runtime_error("Script does not have an extension.");
    std::string scriptExt = ft_getScriptName().substr(dotPos);
    if (scriptExt != location.ft_getCgiExt())
        throw std::runtime_error("Script extension is not allowed in this location.");
    if (!location.ft_getUpload().empty())
    {
        struct stat uploadStat;
    	std::string fullUpload = location.ft_getRoot() + location.ft_getUpload();
        if (stat(fullUpload.c_str(), &uploadStat) == -1 || !S_ISDIR(uploadStat.st_mode))
            throw std::runtime_error("Upload directory is invalid or does not exist.");
    }
}




void Cgi::ft_setupCgi(const Location& location, Request& request)
{
	_env.clear();
	_envChar.clear();
	_args.clear();
	_argChar.clear();

	_isCgi = true;
	_env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	_env.push_back("SERVER_PROTOCOL=HTTP/1.1");
	_env.push_back("REQUEST_METHOD=" + request.ft_getMethod());
	_env.push_back("SCRIPT_NAME=" + ft_getScriptName());
	_env.push_back("PATH_INFO=" + request.ft_getPath());
	_env.push_back("ROOT=" + location.ft_getRoot());
	_env.push_back("UPLOAD_DIR=" + location.ft_getUploadPath());
	_env.push_back("CONTENT_TYPE=" + request.ft_getHeader("Content-Type"));
	_env.push_back("CONTENT_LENGTH=" + request.ft_getHeader("Content-Length"));
	_env.push_back("HTTP_COOKIE=" + request.ft_getHeader("Cookie"));
	_args.push_back("/usr/bin/python3");
	_args.push_back(ft_getScriptName());
	for (size_t i = 0; i < _env.size(); ++i)
		_envChar.push_back(const_cast<char*>(_env[i].c_str()));
	_envChar.push_back(NULL);
	for (size_t i = 0; i < _args.size(); ++i)
		_argChar.push_back(const_cast<char*>(_args[i].c_str()));
	_argChar.push_back(NULL);
	if (pipe(_pipeIn) == -1 || pipe(_pipeOut) == -1)
		throw std::runtime_error("Error creating pipes to CGI");
	// fcntl(_pipeOut[1], F_SETFL, O_NONBLOCK);
	// fcntl(_pipeIn[0], F_SETFL, O_NONBLOCK);
}

void Cgi::ft_executeCgi(const Location& location, std::unordered_set<pid_t>& pidSet)
{
	_pid = fork();
	if (_pid < 0)
		throw std::runtime_error("Erro ao criar processo CGI");
	if (_pid == 0)
	{
		std::string cgiDir = location.ft_getRoot() + '/' + location.ft_getCgiPath();
		if (chdir(cgiDir.c_str()) == -1)
		{
			std::cout << "chdir failed: " << strerror(errno) << std::endl;
			exit(1);
		}
		close(_pipeOut[1]);
		close(_pipeIn[0]);
		dup2(_pipeOut[0], STDIN_FILENO);
		dup2(_pipeIn[1], STDOUT_FILENO);
		close(_pipeIn[1]);
		close(_pipeOut[0]);
		execve(_argChar[0], &_argChar[0], &_envChar[0]);
		std::cout << "execve failed" << std::endl;
		exit(1);
	}
	pidSet.insert(_pid);
	close(_pipeIn[1]);
	close(_pipeOut[0]);
	fcntl(_pipeOut[1], F_SETFL, O_NONBLOCK);
	fcntl(_pipeIn[0], F_SETFL, O_NONBLOCK);
}

void Cgi::ft_setCgiServer(const Server& server)
{
	_cgiServer = server;
}

int Cgi::ft_getClientFd() const
{
	return _clientFd;
}

int Cgi::ft_getPipeInReadFd() const
{
	return _pipeIn[0];
}

int Cgi::ft_getPipeOutWriteFd() const
{
	return _pipeOut[1];
}

pid_t Cgi::ft_getPid() const
{
	return _pid;
}

std::string Cgi::ft_getCgiMessage() const
{
	return _cgiMessage;
}

std::string Cgi::ft_getScriptDir() const
{
	return _scriptDir;
}
std::string Cgi::ft_getScriptName() const
{
	return _scriptName;
}
void Cgi::ft_setScriptDir(const std::string& dir)
{
	_scriptDir = dir;
}
void Cgi::ft_setScriptName(const std::string& name)
{
	_scriptName = name;
}

bool Cgi::ft_getIsCgi() const
{
	return _isCgi;
}
