NAME = webserv

CXX = c++
CXXFLAGS = -g #-Wall -Wextra -Werror -g -std=c++98 -g
RM = rm -rf

SRCDIR = ./src
OBJDIR = ./obj
INCDIR = ./inc

SRC = main.cpp config.cpp server.cpp location.cpp webserv.cpp connection.cpp request.cpp response.cpp cgi.cpp
OBJ = $(addprefix $(OBJDIR)/, $(SRC:.cpp=.o))

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

 .PHONY: all libft clean fclean re