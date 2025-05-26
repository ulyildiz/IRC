#include "../include/IRC_Protocol.hpp"
#include <cstdlib>

void	input_check(char *port, char *password)
{
	int i = 0;
	int j = 0;

	while (port[i])
	{
		if (port[i] < '0' || port[i] > '9')
		{
			std::cerr << "Port must be a number." << std::endl;
			exit(1);
		}
		i++;
	}

	while (password[j])
	{
		if (password[j] < 32 || password[j] > 126)
		{
			std::cerr << "Password must be printable characters." << std::endl;
			exit(1);
		}
		j++;
	}

}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
		return (1);
	}
	input_check(argv[1], argv[2]);
	try {
    	IRC_Protocol irc(atoi(argv[1]), argv[2]);
    	irc.run();
	}
	catch (const std::exception &e) {
		std::cout << e.what() << std::endl;
	}
	return (0);
}
//TryCatch Maybe ??