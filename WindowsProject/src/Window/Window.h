#pragma once
#include "../Console/ConsoleInputThread.h"
#include <Windows.h>
#include <codecvt>
#include <iostream>
#include <string_view>
#include <thread>

class Window
{
public:
	Window(HINSTANCE hInstance, int nCmdShow);
	~Window();

	void runMessageLoop();

private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CtrlHandler(DWORD dwCtrlType);

	void AddExtraWindowCommands(CommandManager& commandManager, HWND& window);

	// Command functions
	static void ChangeTitle(const std::string_view value);
	static void ChangeBackgroundFn(const std::string_view value);
	static void PrintTime(const std::string_view value);


	static HWND m_hwnd;

	CommandManager m_commandManager;

	std::unique_ptr<ConsoleInputThread> m_consoleInputThread;
	std::thread m_consoleThread;

	bool m_isRunning{ true };
};
