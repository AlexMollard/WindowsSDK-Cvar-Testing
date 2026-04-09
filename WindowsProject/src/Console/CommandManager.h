#pragma once

#include <initializer_list>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include "Command.h"

class CommandManager
{
public:
	struct CommandRegistration
	{
		std::string_view name;
		Command command;
		std::initializer_list<std::string_view> aliases;
	};

	CommandManager();

	void registerCommand(std::string_view name, const Command& command,
	                     std::initializer_list<std::string_view> aliases = {});
	void registerCommands(std::initializer_list<CommandRegistration> commands);
	void emplace(std::string_view name, const Command& command);
	void executeCommand(std::string_view commandName, const std::string& value) const;

	std::vector<std::string> getCommandNames() const;
	std::vector<std::string> getCommandCompletions(const std::string& partialCommand) const;

private:
	std::map<std::string, Command> basicCommands;
	std::map<std::string, Command> userCommands;

	void registerBasicCommand(std::string_view name, const Command& command,
	                          std::initializer_list<std::string_view> aliases = {});
	void printCommands() const;
	void printCommandMap(const std::string& title, const std::map<std::string, Command>& commandMap) const;
};
