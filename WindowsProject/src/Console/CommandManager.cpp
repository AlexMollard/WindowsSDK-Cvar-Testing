#include "CommandManager.h"

#include "../Algorithms.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>

CommandManager::CommandManager()
{
	registerBasicCommand(
	    "help",
	    Command([this](const std::string_view& value) { this->printCommands(); }, "Displays a list of available commands"));

	registerBasicCommand("close", Command([](const std::string_view& value) { std::exit(0); }, "Exits the program"),
	                     { "quit", "exit" });

	registerBasicCommand(
	    "clear", Command([](const std::string_view& value) { std::cout << "\x1B[2J\x1B[H"; }, "Clears the console"),
	    { "cls" });
}

void CommandManager::registerCommand(std::string_view name, const Command& command,
	                                  std::initializer_list<std::string_view> aliases)
{
	userCommands.emplace(std::string(name), command);
	for (const auto alias : aliases)
	{
		userCommands.emplace(std::string(alias), command);
	}
}

void CommandManager::registerCommands(std::initializer_list<CommandRegistration> commands)
{
	for (const auto& command : commands)
	{
		registerCommand(command.name, command.command, command.aliases);
	}
}

void CommandManager::emplace(std::string_view name, const Command& command)
{
	registerCommand(name, command);
}

void CommandManager::executeCommand(std::string_view commandName, const std::string& value) const
{
	const auto it1 = basicCommands.find(std::string(commandName));
	if (it1 != basicCommands.end())
	{
		it1->second.execute(value);
		std::cout << "\n";
		return;
	}

	const auto it2 = userCommands.find(std::string(commandName));
	if (it2 != userCommands.end())
	{
		it2->second.execute(value);
		std::cout << "\n";
		return;
	}

	const auto reset     = "\033[0m";
	const auto bold      = "\033[1m";
	const auto red       = "\033[31m";
	const auto cyan      = "\033[36m";
	const auto underline = "\033[4m";

	// Find the closest matching command using Levenshtein distance
	std::string closestMatch;
	int minDistance = std::numeric_limits<int>::max();
	for (const auto& cmd : basicCommands)
	{
		int distance = Algorithms::levenshteinDistance(commandName, cmd.first);
		if (distance < minDistance)
		{
			closestMatch = std::string(cmd.first);
			minDistance  = distance;
		}
	}
	for (const auto& cmd : userCommands)
	{
		int distance = Algorithms::levenshteinDistance(commandName, cmd.first);
		if (distance < minDistance)
		{
			closestMatch = std::string(cmd.first);
			minDistance  = distance;
		}
	}

	if (minDistance < 3)
	{
		std::cout << red << bold << underline << "Error:" << reset << " Unknown command \"" << bold << commandName << reset
		          << "\".";
		if (!closestMatch.empty())
		{
			std::cout << " Did you mean \"" << bold << cyan << closestMatch << reset << "\"?" << std::endl << std::endl;
		}
		else
		{
			std::cout << " Type \"" << bold << cyan << "help" << reset << "\" for a list of available commands." << std::endl
			          << std::endl;
		}
	}
	else
	{
		std::cout << red << bold << underline << "Error:" << reset << " Unknown command \"" << bold << commandName << reset
		          << "\". Type \"" << bold << cyan << "help" << reset << "\" for a list of available commands." << std::endl
		          << std::endl;
	}
}

std::vector<std::string> CommandManager::getCommandNames() const
{
	std::vector<std::string> commandNames;
	for (const auto& cmd : basicCommands)
	{
		commandNames.emplace_back(cmd.first.data());
	}
	for (const auto& cmd : userCommands)
	{
		commandNames.emplace_back(cmd.first.data());
	}
	return commandNames;
}

void CommandManager::registerBasicCommand(std::string_view name, const Command& command,
	                                      std::initializer_list<std::string_view> aliases)
{
	basicCommands.emplace(std::string(name), command);
	for (const auto alias : aliases)
	{
		basicCommands.emplace(std::string(alias), command);
	}
}

void CommandManager::printCommandMap(const std::string& title, const std::map<std::string, Command>& commandMap) const
{
	const auto reset = "\033[0m";
	const auto bold  = "\033[1m";
	const auto blue  = "\033[34m";
	const auto green = "\033[32m";

	static bool longDesc = false;

	std::cout << bold << title << reset << std::endl;
	for (const auto& [name, command] : commandMap)
	{
		std::cout << "   - " << bold << blue << std::setw(20) << std::left << name << reset;
		if (!command.getDescription().empty())
		{
			std::cout << " : ";
			std::string_view description = command.getDescription();
			size_t len                   = description.length();
			size_t pos                   = 0;
			bool firstLine               = true;
			while (pos < len)
			{
				std::string_view line = description.substr(pos, 60);
				// Avoid cutting words at the end of the line
				if (pos + line.length() < len && !std::isspace(description[pos + line.length()]))
				{
					size_t lastSpace = line.find_last_of(" \t\n");
					if (lastSpace != std::string_view::npos)
					{
						// Move the cursor to the last space
						line = line.substr(0, lastSpace + 1);
					}
				}
				if (firstLine)
				{
					std::cout << bold << green << std::setw(60) << line << reset << std::endl;
					firstLine = false;
				}
				else
				{
					std::cout << std::setw(28) << ' ' << bold << green << std::setw(60) << line << reset << std::endl;
					longDesc = true;
				}
				pos += line.length();
			}
		}

		if (longDesc)
			std::cout << "\n";

		longDesc = false;
	}
}

void CommandManager::printCommands() const
{
	std::cout << "Available commands:" << std::endl;
	printCommandMap("Basic commands:", basicCommands);
	printCommandMap("User-defined commands:", userCommands);
}

std::vector<std::string> CommandManager::getCommandCompletions(const std::string& partialCommand) const
{
	std::vector<std::string> completions;

	// Check if the partial command matches any basic commands
	for (const auto& cmd : basicCommands)
	{
		if (cmd.first.find(partialCommand) == 0) // Command starts with partialCommand
		{
			completions.push_back(cmd.first);
		}
	}

	// Check if the partial command matches any user commands
	for (const auto& cmd : userCommands)
	{
		if (cmd.first.find(partialCommand) == 0) // Command starts with partialCommand
		{
			completions.push_back(cmd.first);
		}
	}

	return completions;
}
