#pragma once

#include <vector>
#include <string>

// Define WIN32_LEAN_AND_MEAN and NOMINMAX before including windows.h
// to speed up compilation and avoid potential conflicts.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h> // Required for Windows types, but keep it out of general headers if possible

// Forward declare to avoid including windows.h directly in user code if they only need the enum/messages
// However, the implementation *will* need it. For simplicity here, we include it.
// If strict header hygiene was paramount, we might hide Windows types behind opaque pointers or PIMPL.

namespace setup {

/**
 * @brief Enumerates the possible results of command execution attempts.
 */
enum class exec_result
{
    SUCCESS,                // Command executed successfully (process launched and waited for).
    FAILURE_EMPTY_COMMAND,  // The provided argument list was empty.
    FAILURE_BUILD_COMMAND,  // Failed to build the command line string (e.g., encoding issues).
    FAILURE_CREATE_PROCESS, // The CreateProcessW Windows API call failed.
    FAILURE_WAIT            // Waiting for the child process to complete failed.
};

/**
 * @brief Translates a vector of arguments into a Windows command-line string.
 *
 * This function implements the quoting rules used by the Microsoft C/C++ runtime library (CRT).
 * See: https://docs.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments
 *
 * @param args A vector of strings representing the command and its arguments.
 * @return A std::wstring suitable for use with CreateProcessW.
 * @throws std::runtime_error if string conversion fails.
 */
std::wstring build_command_line_string(const std::vector<std::string>& args);

/**
 * @brief Executes a command specified by a list of arguments.
 *
 * Builds the command line string using build_command_line_string and then
 * executes it using CreateProcessW, waiting for the child process to finish.
 *
 * @param args A vector of strings representing the command and its arguments.
 *             The first element should be the executable path/name.
 * @return An exec_result indicating the outcome.
 */
exec_result execute_command(const std::vector<std::string>& args);

/**
 * @brief Executes a command specified by a pre-built command line string.
 *
 * Executes the command using CreateProcessW, waiting for the child process to finish.
 * Use this if you have already constructed the command line string according to CRT rules.
 *
 * @param command_line The command line string (must follow CRT parsing rules).
 * @return An exec_result indicating the outcome.
 */
exec_result execute_command_line(const std::wstring& command_line);


/**
 * @brief Retrieves a human-readable message corresponding to an exec_result.
 *
 * @param result The exec_result value.
 * @return A const char* describing the result.
 */
const char* get_result_message(exec_result result);

} // namespace setup