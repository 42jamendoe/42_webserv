#ifndef LOCATION_HPP
#define LOCATION_HPP

#include "defs.hpp"

class Location
{
    private:
        std::string _path;
        std::string _root;
        std::string _index;
        std::set<std::string> _limit_methods;
        std::string _redirection;
        bool _auto_index;
        std::string _try_file;
        std::string _cgi_path;
        std::string _cgi_ext;
        std::string _upload;
        std::string _uploadPath;


    public:

        Location();
        ~Location();

        //setters devem faltar porque acrescentei variaveis
        void ft_setPath(const std::string& path);
        void ft_setRoot(const std::string& root);
        void ft_setIndex(const std::string& index);
        void ft_addLimitMethods(const std::string& limit_method);
        void ft_setRedirection(const std::string& redirection);
        void ft_setAutoIndex(const bool& auto_index);
        void ft_setTryFile(const std::string& try_file);
        void ft_setCgiPath(const std::string& cgi_path);
        void ft_setCgiExt(const std::string& cgi_ext);
        void ft_setUpload(const std::string& upload);

        //getters
        const std::string& ft_getPath() const;
        const std::string& ft_getRoot() const;
        const std::string& ft_getIndex() const;
        const std::set<std::string>& ft_getLimitMethods() const;
        const std::string& ft_getRedirection() const;
        const bool& ft_getAutoIndex() const;
        const std::string& ft_getTryFile() const;
        const std::string& ft_getCgiPath() const;
        const std::string& ft_getCgiExt() const;
        const std::string& ft_getUpload() const;
        void ft_setUploadPath(const std::string &path);
        const std::string& ft_getUploadPath() const;
        bool ft_isMethodAllowed(const std::string& method) const;
        bool ft_isRedirect() const;
        std::string ft_getRedirectUrl() const;
};

#endif
