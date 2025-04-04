#include "downloader.hpp"
#include <curl/curl.h>
#include <windows.h>
#include <shlobj.h>
#include <cctype>

#ifdef _WIN32

namespace setup {

downloader::downloader(const dl_params& params) : m_params(params)
{
    if(m_params.output_dir.empty())
    {
        m_params.output_dir = get_default_download_dir();
    }

    // Sanitize all paths
    m_params.output_dir = sanitize_path(m_params.output_dir);
    if(!m_params.filename.empty())
    {
        m_params.filename = sanitize_path(m_params.filename);
    }
}

downloader::~downloader()
{
    curl_global_cleanup();
}

dl_result downloader::download() const
{
    return download(nullptr);
}

dl_result downloader::download(std::function<void()> callback) const
{
    CURL* curl = curl_easy_init();
    if(!curl)
    {
        return dl_result::EASY_INIT_FAILED;
    }

    const std::string output_path = get_output_path();
    if(output_path.empty())
    {
        return dl_result::INVALID_OUTPUT_PATH;
    }

    FILE* file;
    if(fopen_s(&file, output_path.c_str(), "wb") != 0)
    {
        return dl_result::FILE_CREATION_FAILED;
    }

    curl_easy_setopt(curl, CURLOPT_URL, m_params.url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, m_params.show_progress ? 0L : 1L);
    if(m_params.show_progress)
    {
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, nullptr);
    }

    CURLcode res = curl_easy_perform(curl);
    fclose(file);

    if(res != CURLE_OK)
    {
        std::remove(output_path.c_str());
        return dl_result::PERFORM_FAILED;
    }

    if(callback)
    {
        callback();
    }

    curl_easy_cleanup(curl);
    return dl_result::SUCCESS;
}

std::string downloader::get_result_message(dl_result result)
{
    switch(result)
    {
        case dl_result::SUCCESS: return "Download completed successfully";
        case dl_result::CURL_INIT_FAILED: return "Failed to initialize libcurl";
        case dl_result::EASY_INIT_FAILED: return "Failed to initialize curl easy handle";
        case dl_result::EASY_SETOPT_FAILED: return "Failed to set curl option";
        case dl_result::PERFORM_FAILED: return "Download failed";
        case dl_result::FILE_CREATION_FAILED: return "Failed to create output file";
        case dl_result::INVALID_OUTPUT_PATH: return "Invalid output path";
        default: return "Unknown error";
    }
}

size_t downloader::write_callback(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    return fwrite(ptr, size, nmemb, stream);
}

int downloader::progress_callback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    (void)clientp;
    (void)ultotal;
    (void)ulnow;
    if(dltotal > 0.0)
    {
        double percentage = (dlnow / dltotal) * 100.0;
        printf("\rDownloading: %.0f%% (%.0f / %.0f bytes)", percentage, dlnow, dltotal);
        fflush(stdout);
    }
    else if(dlnow > 0.0)
    {
        printf("\rDownloading: %.0f bytes", dlnow);
        fflush(stdout);
    }
    return 0;
}

std::string downloader::sanitize_path(const std::string& path) const
{
    static const std::string illegal_chars = "<>:\"/\\|?*";
    std::string sanitized = path;

    for(char& c : sanitized)
    {
        if(illegal_chars.find(c) != std::string::npos)
        {
            c = '_';
        }
    }

    return sanitized;
}

std::string downloader::get_default_download_dir() const
{
    char path[MAX_PATH];
    if(SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_PROFILE, nullptr, 0, path)))
    {
        std::string downloads(path);
        downloads += "\\Downloads\\";
        return downloads;
    }
    return "C:\\Users\\Public\\Downloads\\";
}

std::string downloader::get_output_path() const
{
    std::string path = m_params.output_dir;

    // Ensure directory ends with a backslash
    if(!path.empty() && path.back() != '\\')
    {
        path += '\\';
    }

    // Create directory if it doesn't exist
    CreateDirectoryA(path.c_str(), nullptr);

    // Determine filename
    std::string filename;
    if(!m_params.filename.empty())
    {
        filename = m_params.filename;
    }
    else
    {
        // Extract filename from URL
        size_t last_slash = m_params.url.find_last_of('/');
        if(last_slash != std::string::npos && last_slash + 1 < m_params.url.length())
        {
            filename = m_params.url.substr(last_slash + 1);
        }
        else
        {
            filename = "downloaded_file";
        }
    }

    return path + filename;
}

} // namespace setup

#endif // _WIN32