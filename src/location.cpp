#include "../inc/location.hpp"

Location::Location() : _auto_index(false), _uploadAllowed(false)
{

}

Location::~Location()
{

}

void Location::ft_setPath(const std::string& path)
{
    this->_path = path;
}

void Location::ft_setRoot(const std::string& root)
{
    this->_root = root;
}

void Location::ft_setIndex(const std::string& index)
{
    this->_index = index;
}

void Location::ft_addLimitMethods(const std::string& limit_method)
{
    if (this->_limit_methods.find(limit_method) == this->_limit_methods.end())
        this->_limit_methods.insert(limit_method);
}

void Location::ft_setRedirection(const std::string& redirection)
{
    this->_redirection = redirection;
}

void Location::ft_setAutoIndex(const bool& auto_index)
{
    this->_auto_index = auto_index;
}

void Location::ft_setTryFile(const std::string& try_file)
{
    this->_try_file = try_file;
}

void Location::ft_setCgiPath(const std::string& cgi_path)
{
    this->_cgi_path = cgi_path;
}

void Location::ft_setCgiExt(const std::string& cgi_ext)
{
    this->_cgi_ext = cgi_ext;
}

void Location::ft_setUpload(const std::string& upload)
{
    this->_upload = upload;
}

void Location::ft_setUploadAllowed(bool allowed)
{
    _uploadAllowed = allowed;
}

void Location::ft_setUploadPath(const std::string &path)
{
    _upload = path;
}

const std::string& Location::ft_getPath() const
{
    return(_path);
}

const std::string& Location::ft_getRoot() const
{
    return(_root);;
}

const std::string& Location::ft_getIndex() const
{
    return(_index);;
}

const std::set<std::string>& Location::ft_getLimitMethods() const
{
    return(_limit_methods);
}

const std::string& Location::ft_getRedirection() const
{
    return(_redirection);
}

const bool& Location::ft_getAutoIndex() const
{
    return(_auto_index);
}

const std::string& Location::ft_getTryFile() const
{
    return(_try_file);
}

const std::string& Location::ft_getCgiPath() const
{
    return(_cgi_path);
}

const std::string& Location::ft_getCgiExt() const
{
    return(_cgi_ext);
}

const std::string& Location::ft_getUpload() const
{
    return(_upload);
}


bool Location::ft_isUploadAllowed() const
{
 return _uploadAllowed;
}

const std::string& Location::ft_getUploadPath() const
{
 return _upload;
}

bool Location::ft_getDeleteAllowed() const
{
    return _deleteAllowed;
}