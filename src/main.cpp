/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ysarac <ysarac@student.42kocaeli.com>	    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/28 15:44:47 by ysarac            #+#    #+#             */
/*   Updated: 2025/03/04 01:38:28 by ysarac           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/IRC_Protocol.h"
#include <iostream>
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
    IRC_Protocol irc(atoi(argv[1]), argv[2]);
    irc.start();
	return (0);
}
//TryCatch Maybe ??