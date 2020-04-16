#include <stdio.h>
#include <stdlib.h>
#include "hidapi.h"
#include "bootloader.h"
#include <iostream>

#define MAX_STR 255

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		std::cout << "Argument error, usage: blapp <path_to_hex_file>" << std::endl;
		exit(EXIT_FAILURE);
	}

	Bootloader bl;
	std::string pathToHexFile(argv[1]);
	if (!bl.Initialize(pathToHexFile))
	{
		std::cout << "Initialize failed, aborting program" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::cout << "Querying bootloader info..." << std::endl;
	if (!bl.GetInfo())
	{
		std::cout << "Getting bootloader info failed, aborting program" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::cout << "Erasing..." << std::endl;
	if (!bl.Erase())
	{
		std::cout << "Erasing application flash failed, aborting program" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::cout << "Programming..." << std::endl;
	if (!bl.Program())
	{
		std::cout << "Programming application flash failed, aborting program" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::cout << "Verifying..." << std::endl;
	if (!bl.Verify())
	{
		std::cout << "Verifying application flash failed, aborting program" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::cout << "Jumping to app..." << std::endl;
	bl.JumpToApp();
	std::cout << "Complete" << std::endl;

	exit(EXIT_SUCCESS);
}