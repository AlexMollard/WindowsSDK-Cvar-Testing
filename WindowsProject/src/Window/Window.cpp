#include "Window.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>

HWND Window::m_hwnd = nullptr;

Window::Window(HINSTANCE hInstance, int nCmdShow)
{
	// Allocate a console for input and output
	AllocConsole();
	freopen_s(reinterpret_cast<FILE**>(stdin), "CONIN$", "r", stdin);
	freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
	freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);

	// Enable ANSI escape sequence processing for colored/styled console output.
	const HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (outputHandle != INVALID_HANDLE_VALUE)
	{
		DWORD outputMode = 0;
		if (GetConsoleMode(outputHandle, &outputMode))
		{
			outputMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			if (!SetConsoleMode(outputHandle, outputMode))
			{
				std::cerr << "Warning: Failed to enable virtual terminal processing: " << GetLastError() << '\n';
			}
		}
		else
		{
			std::cerr << "Warning: Failed to query console mode: " << GetLastError() << '\n';
		}
	}

	// Get the current console screen buffer info
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

	// Register signal handler function
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
	{
		std::cerr << "The Control Handler is installed.\n";
	}
	else
	{
		std::cerr << "SetConsoleCtrlHandler failed: " << GetLastError() << '\n';
		return;
	}

	// Create a window
	const wchar_t* className = L"HelloWorldWindowClass";
	WNDCLASS wc{};
	wc.lpfnWndProc   = WndProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = className;

	if (!RegisterClass(&wc))
	{
		std::cerr << "RegisterClass failed: " << GetLastError() << '\n';
		return;
	}

	m_hwnd = CreateWindow(className, L"Hello World!", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1000, 800, nullptr,
	                      nullptr, hInstance, nullptr);

	if (m_hwnd == nullptr)
	{
		std::cerr << "CreateWindow failed: " << GetLastError() << '\n';
		return;
	}

	// Display the window
	ShowWindow(m_hwnd, nCmdShow);

	// Create and register commands using lambdas
	AddExtraWindowCommands(m_commandManager, m_hwnd);

	// Start the console input thread
	m_consoleInputThread = std::make_unique<ConsoleInputThread>(m_commandManager);
	m_consoleThread      = std::thread(&ConsoleInputThread::run, m_consoleInputThread.get());
}

Window::~Window()
{
	m_consoleInputThread->SetIsRunning(false); // signal the console input thread to stop
	PostMessage(m_hwnd, WM_CLOSE, 0, 0);       // post a close message to exit the message loop
	m_consoleThread.join();
	FreeConsole();
}

void Window::runMessageLoop()
{
	MSG msg{};
	while (m_isRunning && GetMessage(&msg, nullptr, 0, 0))
	{
		if (msg.message == WM_CLOSE)
		{
			m_isRunning = false; // signal the message loop to stop
			break;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE: DestroyWindow(hwnd); break;
	case WM_DESTROY: PostQuitMessage(0); break;
	default: return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
}

BOOL Window::CtrlHandler(DWORD dwCtrlType)
{
	std::cout << "CtrlHandler called" << std::endl;
	if (dwCtrlType == CTRL_C_EVENT)
	{
		std::cout << "Ctrl+C signal received" << std::endl;
		PostQuitMessage(0); // post a quit message to exit the message loop
		return TRUE;        // indicate that the signal has been handled
	}
	return FALSE; // indicate that the signal has not been handled
}

void Window::AddExtraWindowCommands(CommandManager& commandManager, HWND& window)
{
	const std::string backgroundColorDesc =
	    "Changes the main window's background color, takes a color name (e.g. red) or hex value (e.g. #FF0000) as "
	    "parameter, or -h to display a list of available colors";

	commandManager.registerCommands({
	    { "title", Command(ChangeTitle, "Changes the main window's title") },
	    { "background-color", Command(ChangeBackgroundFn, backgroundColorDesc), { "bgc" } },
	    { "time", Command(PrintTime, "Shows the current local date and time"), { "now" } },
	});
}

void Window::ChangeTitle(const std::string_view value)
{
	// Convert the std::string_view to a wide string
	int wchars_num = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.length()), nullptr, 0);
	auto wstr      = new wchar_t[wchars_num + 1];
	MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.length()), wstr, wchars_num);
	wstr[wchars_num] = L'\0';

	// Set the window text using the wide string
	SetWindowText(m_hwnd, wstr);

	// Free memory
	delete[] wstr;

	std::cout << "Successfully set new title: " << value << "\n";
}

void Window::ChangeBackgroundFn(const std::string_view value)
{
	static std::unordered_map<std::string, COLORREF> colors = {
		{ "black", RGB(0, 0, 0) },      { "white", RGB(255, 255, 255) }, { "red", RGB(255, 0, 0) },
		{ "lime", RGB(0, 255, 0) },     { "blue", RGB(0, 0, 255) },      { "yellow", RGB(255, 255, 0) },
		{ "cyan", RGB(0, 255, 255) },   { "magenta", RGB(255, 0, 255) }, { "silver", RGB(192, 192, 192) },
		{ "gray", RGB(128, 128, 128) }, { "maroon", RGB(128, 0, 0) },    { "olive", RGB(128, 128, 0) },
		{ "green", RGB(0, 128, 0) },    { "purple", RGB(128, 0, 128) },  { "teal", RGB(0, 128, 128) },
		{ "navy", RGB(0, 0, 128) }
	};

	if (value == "-h")
	{
		const std::ios::fmtflags previousFlags = std::cout.flags();
		const char previousFill                = std::cout.fill();

		// Define text colors for the table
		const std::string COLOR_RESET = "\033[0m";
		const std::string COLOR_NAME  = "\033[38;5;208m"; // Light orange
		const std::string COLOR_HEX   = "\033[38;5;111m"; // Light green

		// Define a function to print a color row
		auto print_color = [&](const std::string& name, int value)
		{
			std::cout << "  " << COLOR_NAME << std::setfill(' ') << std::setw(8) << std::left << name << ": " << COLOR_HEX
			          << std::setw(6) << std::right << std::setfill('0') << std::hex << value << COLOR_RESET << "\n";
		};

		// Print the example colors
		std::cout << "Example colors:\n";
		for (const auto& color : colors)
		{
			print_color(color.first, color.second);
		}
		std::cout << "\n";
		std::cout.flags(previousFlags);
		std::cout.fill(previousFill);

		return;
	}

	COLORREF bgColor;
	if (value.empty())
	{
		std::cout << "You need to define a color?" << value << "\n";
		return;
	}

	if (value.size() > 7)
	{
		std::cout << "Invalid color value: " << value << "\n";
		return;
	}

	// Check if input is a color name
	std::string colorStr = std::string(value);
	std::transform(colorStr.begin(), colorStr.end(), colorStr.begin(), ::tolower);
	auto it = colors.find(colorStr);
	if (it != colors.end())
	{
		bgColor = it->second;
	}
	else
	{
		if (value[0] != '#' || value.size() < 4 || value.size() > 7)
		{
			std::cout << "Invalid color value: " << value << "\n";
			return;
		}
		try
		{
			bgColor = static_cast<COLORREF>(std::stoul(colorStr.substr(1), nullptr, 16));
		}
		catch (...)
		{
			std::cout << "Error converting color value: " << value << "\n";
			return;
		}
	}

	// Create a new brush with the specified color
	HBRUSH hBrush = CreateSolidBrush(bgColor);

	// Set the window background brush
	SetClassLongPtr(m_hwnd, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(hBrush));

	// Redraw the window
	RedrawWindow(m_hwnd, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE);

	std::cout << "Successfully set new background color: " << value << "\n";
}

void Window::PrintTime(const std::string_view value)
{
	(void)value;

	const auto now       = std::chrono::system_clock::now();
	const std::time_t tt = std::chrono::system_clock::to_time_t(now);

	std::tm localTime{};
	localtime_s(&localTime, &tt);

	std::cout << "Local time: " << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "\n";
}
