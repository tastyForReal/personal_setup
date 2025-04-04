#include "command_executor.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <string>

// Ensure Windows headers are included correctly
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace setup {

// Helper function to convert UTF-8 std::string to std::wstring
std::wstring utf8_to_wstring(const std::string& str)
{
    if(str.empty())
    {
        return std::wstring();
    }
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    if(size_needed <= 0)
    {
        throw std::runtime_error("MultiByteToWideChar failed to calculate size.");
    }
    std::wstring wstr_to(size_needed, 0);
    int bytes_converted = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr_to[0], size_needed);
    if(bytes_converted <= 0)
    {
        throw std::runtime_error("MultiByteToWideChar failed to convert string.");
    }
    return wstr_to;
}

// Helper function to quote a single argument according to CRT rules
void quote_argument(const std::wstring& argument, std::wstringstream& command_line)
{
    // Check if quoting is necessary: empty, contains space/tab, or contains quote
    bool needs_quoting = argument.empty() || argument.find_first_of(L" \t\"") != std::wstring::npos;

    if(!needs_quoting)
    {
        command_line << argument;
        return;
    }

    command_line << L'"';
    for(auto it = argument.begin();; ++it)
    {
        unsigned int backslash_count = 0;
        // Count consecutive backslashes before the current position
        while(it != argument.end() && *it == L'\\')
        {
            ++backslash_count;
            ++it;
        }

        if(it == argument.end())
        {
            // End of string: append 2*n backslashes
            command_line << std::wstring(backslash_count * 2, L'\\');
            break;
        }

        if(*it == L'"')
        {
            // Escape all backslashes and the quote: append (2*n + 1) backslashes, then \"
            command_line << std::wstring(backslash_count * 2 + 1, L'\\');
            command_line << L'\"'; // Append the literal quote
        }
        else
        {
            // Non-quote character: append n backslashes, then the character
            command_line << std::wstring(backslash_count, L'\\');
            command_line << *it;
        }
    }
    command_line << L'"';
}


std::wstring build_command_line_string(const std::vector<std::string>& args)
{
    if(args.empty())
    {
        return L""; // Or throw an exception, depending on desired behavior for empty input
    }

    std::wstringstream command_line_stream;
    bool first_arg = true;

    for(const auto& narrow_arg : args)
    {
        if(!first_arg)
        {
            command_line_stream << L' ';
        }
        first_arg = false;

        try
        {
            std::wstring wide_arg = utf8_to_wstring(narrow_arg);
            quote_argument(wide_arg, command_line_stream);
        }
        catch(const std::runtime_error& e)
        {
            // Log the error or rethrow a more specific exception
            std::wcerr << L"[ERROR] Failed to convert argument to wide string: " << e.what() << std::endl;
            // Rethrow or return an indicator of failure; here we rethrow
            // to signal the build process failed. The caller (execute_command)
            // will catch this.
            throw;
        }
    }
    return command_line_stream.str();
}

exec_result execute_command_line(const std::wstring& command_line)
{
    if(command_line.empty())
    {
        std::cerr << "[ERROR] Cannot execute an empty command line." << std::endl;
        return exec_result::FAILURE_EMPTY_COMMAND;
    }

    std::wcout << L"[INFO] Executing command line: " << command_line << std::endl;

    // CreateProcessW requires a mutable buffer for the command line argument (lpCommandLine).
    // Create a copy.
    std::vector<wchar_t> command_line_buffer(command_line.begin(), command_line.end());
    command_line_buffer.push_back(L'\0'); // Null-terminate

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Create the child process.
    // Pass NULL for lpApplicationName, relying on lpCommandLine to specify the executable.
    // Pass the mutable buffer command_line_buffer.data().
    if(!CreateProcessW(NULL,           // No module name (use command line)
           command_line_buffer.data(), // Command line (mutable)
           NULL,                       // Process handle not inheritable
           NULL,                       // Thread handle not inheritable
           FALSE,                      // Set handle inheritance to FALSE
           0,                          // No creation flags (e.g., CREATE_NO_WINDOW)
           NULL,                       // Use parent's environment block
           NULL,                       // Use parent's starting directory
           &si,                        // Pointer to STARTUPINFO structure
           &pi)                        // Pointer to PROCESS_INFORMATION structure
    )
    {
        DWORD error_code = GetLastError();
        std::cerr << "[ERROR] CreateProcessW failed with error code: " << error_code << std::endl;
        // Optionally, format the error message using FormatMessageW
        return exec_result::FAILURE_CREATE_PROCESS;
    }

    std::cout << "[INFO] Process created with ID: " << pi.dwProcessId << std::endl;

    // Wait until child process exits.
    DWORD wait_result = WaitForSingleObject(pi.hProcess, INFINITE);

    if(wait_result == WAIT_FAILED)
    {
        DWORD error_code = GetLastError();
        std::cerr << "[ERROR] WaitForSingleObject failed with error code: " << error_code << std::endl;
        // Close handles even if wait failed, as process *was* created
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return exec_result::FAILURE_WAIT;
    }

    // Optional: Get and log the exit code
    DWORD exit_code = 0;
    if(GetExitCodeProcess(pi.hProcess, &exit_code))
    {
        std::cout << "[INFO] Process " << pi.dwProcessId << " exited with code: " << exit_code << std::endl;
    }
    else
    {
        std::cerr << "[WARN] Failed to get process exit code. Error: " << GetLastError() << std::endl;
    }


    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::cout << "[INFO] Command execution finished." << std::endl;
    return exec_result::SUCCESS;
}


exec_result execute_command(const std::vector<std::string>& args)
{
    if(args.empty())
    {
        std::cerr << "[ERROR] Cannot execute command: argument list is empty." << std::endl;
        return exec_result::FAILURE_EMPTY_COMMAND;
    }

    std::wstring command_line;
    try
    {
        command_line = build_command_line_string(args);
    }
    catch(const std::runtime_error& e)
    {
        std::cerr << "[ERROR] Failed to build command line string: " << e.what() << std::endl;
        return exec_result::FAILURE_BUILD_COMMAND;
    }

    return execute_command_line(command_line);
}


const char* get_result_message(exec_result result)
{
    switch(result)
    {
        case exec_result::SUCCESS: return "Command executed successfully.";
        case exec_result::FAILURE_EMPTY_COMMAND: return "Execution failed: The command or argument list was empty.";
        case exec_result::FAILURE_BUILD_COMMAND:
            return "Execution failed: Could not build the command line string (e.g., encoding error).";
        case exec_result::FAILURE_CREATE_PROCESS:
            return "Execution failed: CreateProcessW failed. Check system error logs.";
        case exec_result::FAILURE_WAIT: return "Execution failed: Waiting for the child process failed.";
        default: return "Unknown execution result.";
    }
}

} // namespace setup