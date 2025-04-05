#include "command_executor.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

fs::path get_downloads_dir()
{
    return fs::path(std::getenv("USERPROFILE")).append("Downloads").string();
}

fs::path get_scoop_app_path(const std::string& app_name)
{
    auto path = fs::path(std::getenv("USERPROFILE")).append("scoop").append("apps").append(app_name).append("current");

    if(fs::exists(path) && fs::is_directory(path))
    {
        for(const auto& entry : fs::directory_iterator(path))
        {
            if(entry.is_directory() && entry.path().filename() == "bin")
            {
                path = entry.path();
                break;
            }
        }
    }

    return path;
}

fs::path get_scoop_file_path()
{
    return get_scoop_app_path("scoop").append("scoop.ps1");
}

int main()
{
    const std::string scoop_path = get_scoop_file_path().string();

    if(fs::exists(scoop_path) && fs::is_regular_file(scoop_path))
    {
        setup::execute_command({"powershell", "/c", scoop_path, "update", "*"});
    }
    else
    {
        setup::execute_command({"powershell", "/c",
            "Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser -Force -Verbose; Invoke-RestMethod "
            "-Uri "
            "https://get.scoop.sh -Verbose | Invoke-Expression -Verbose"});
    }

    const std::vector<std::string> scoop_apps = {"7zip", "adb", "cloc", "curl", "dotnet-sdk", "fastfetch", "ffmpeg",
        "gh", "git", "imagemagick", "jq", "msys2", "nodejs", "python"};

    for(const auto& app : scoop_apps)
    {
        const auto app_path = get_scoop_app_path(app);
        if(fs::exists(app_path) && fs::is_directory(app_path))
        {
            continue;
        }

        setup::execute_command({"powershell", "/c", scoop_path, "install", app});
    }

    setup::execute_command({"powershell", "/c", scoop_path, "cache", "rm", "*"});
    setup::execute_command({"powershell", "/c", scoop_path, "cleanup", "*"});

    const std::string curl_path = get_scoop_app_path("curl").append("curl.exe").string();
    const std::string firefox_dl_path = get_downloads_dir().append("firefox_setup.exe").string();
    const std::string vscode_dl_path = get_downloads_dir().append("vscode_setup.exe").string();

    setup::execute_command({curl_path, "-L", "-o", firefox_dl_path,
        "https://download.mozilla.org/?product=firefox-latest-ssl&os=win64&lang=en-US"});
    setup::execute_command({firefox_dl_path, "/S", "/PreventRebootRequired=true"});

    setup::execute_command({curl_path, "-L", "-o", vscode_dl_path,
        "https://code.visualstudio.com/sha/download?build=stable&os=win32-x64-user"});
    setup::execute_command(
        {vscode_dl_path, "/SP-", "/SILENT", "/SUPPRESSMSGBOXES", "/NORESTART", "/mergetasks=!runcode"});

    fs::remove(firefox_dl_path);
    fs::remove(vscode_dl_path);

    return 0;
}