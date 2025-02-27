#ifndef COLORS_HPP
#define COLORS_HPP

#include <iostream>
#include <cstring>
#include <exception>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <set>
#include <map>
#include <vector>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <csignal>
#include <algorithm>
#include <cerrno>
#include <sys/stat.h>
#include <dirent.h>
#include <ctime>

#define TIMEOUT 60000
#define BUFFER_SIZE 2048
#define MAX_CONNECTIONS 100
#define DEFAULT_CONFIG "Configuration/default.conf"
#define MAX_HEADER_SIZE 8192
#define MAX_BODY_SIZE 10485760
#define CONNECTION_TIMEOUT 20

#define RESET   "\033[0m"
#define BLACK   "\033[30m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define BOLD_BLACK   "\033[1m\033[30m"
#define BOLD_RED     "\033[1m\033[31m"
#define BOLD_GREEN   "\033[1m\033[32m"
#define BOLD_YELLOW  "\033[1m\033[33m"
#define BOLD_BLUE    "\033[1m\033[34m"
#define BOLD_MAGENTA "\033[1m\033[35m"
#define BOLD_CYAN    "\033[1m\033[36m"
#define BOLD_WHITE   "\033[1m\033[37m"

template <typename T>
std::string ft_toString(const T& value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

#endif
