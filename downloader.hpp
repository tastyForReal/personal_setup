#pragma once

#include <string>
#include <functional>

#ifdef _WIN32

namespace setup {

enum class dl_result
{
    SUCCESS,
    CURL_INIT_FAILED,
    EASY_INIT_FAILED,
    EASY_SETOPT_FAILED,
    PERFORM_FAILED,
    FILE_CREATION_FAILED,
    INVALID_OUTPUT_PATH
};

struct dl_params
{
    std::string url;
    std::string output_dir;
    std::string filename;
    bool show_progress = false;
};

class downloader
{
public:
    explicit downloader(const dl_params& params);
    ~downloader();

    dl_result download() const;
    dl_result download(std::function<void()> callback) const;

    static std::string get_result_message(dl_result result);

private:
    dl_params m_params;
    static size_t write_callback(void* ptr, size_t size, size_t nmemb, FILE* stream);
    static int progress_callback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);

    std::string sanitize_path(const std::string& path) const;
    std::string get_default_download_dir() const;
    std::string get_output_path() const;
};

} // namespace setup

#endif // _WIN32