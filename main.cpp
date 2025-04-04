#include "command_executor.hpp"
#include "downloader.hpp"
#include <cstdio>
#include <fileapi.h>
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>

bool download_and_run(setup::dl_params params, std::vector<std::string> args = {})
{
    std::cout << "Downloading file from URL: " << params.url << std::endl;

    setup::downloader dl(params);
    auto result = dl.download(
        [&]()
        {
            std::cout << "Download finished." << std::endl;
            std::vector<std::string> command = {
                (std::filesystem::path(params.output_dir) / std::filesystem::path(params.filename)).string()};
            for(const auto& arg : args)
            {
                command.emplace_back(arg);
            }
            setup::execute_command(command);
        });

    if(result != setup::dl_result::SUCCESS)
    {
        std::cerr << "Download failed: " << setup::downloader::get_result_message(result) << std::endl;
        return false;
    }

    return true;
}

int main()
{
    setup::execute_command({"powershell", "/c",
        "Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser -Verbose; Invoke-RestMethod -Uri "
        "https://get.scoop.sh | Invoke-Expression"});

    setup::execute_command({"cmd", "/c", "scoop update *"});
    setup::execute_command({"cmd", "/c", "scoop install ", "7zip", "adb", "cloc", "dotnet-sdk", "fastfetch", "ffmpeg",
        "gh", "git", "imagemagick", "jq", "nodejs", "python"});
    setup::execute_command({"cmd", "/c", "scoop cache rm *"});
    setup::execute_command({"cmd", "/c", "scoop cleanup *"});

    download_and_run({.url = "https://vscode.download.prss.microsoft.com/dbazure/download/stable/"
                             "4437686ffebaf200fa4a6e6e67f735f3edf24ada/VSCodeUserSetup-x64-1.99.0.exe",
                         .output_dir = ".",
                         .filename = "VSCodeUserSetup-x64-1.99.0.exe",
                         .show_progress = true},
        {"/SP-", "/SILENT", "/SUPPRESSMSGBOXES", "/NORESTART", "/mergetasks=!runcode"});

    return 0;
}