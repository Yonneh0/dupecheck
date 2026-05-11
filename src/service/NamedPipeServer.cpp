#include "NamedPipeServer.h"
#include <windows.h>
#include <string>
#include <thread>
#include <memory>

class PipeImpl {
public:
    explicit PipeImpl(const std::wstring& pipe_name, DatabaseManager* db)
        : pipe_name_(L"\\\\.\\pipe\\" + pipe_name), db_(db) {}

    void start() {
        running_ = true;
        server_thread_ = std::thread([this]() { run_loop(); });
    }

    ~PipeImpl() {
        if (running_) {
            CloseHandle(pipe_handle_);
            server_thread_.join();
        }
    }

private:
    std::wstring pipe_name_;
    DatabaseManager* db_ = nullptr;
    bool running_ = false;
    HANDLE pipe_handle_ = INVALID_HANDLE_VALUE;
    std::thread server_thread_;

    void run_loop() {
        while (running_) {
            pipe_handle_ = CreateNamedPipeW(
                pipe_name_.c_str(),
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                10,
                4096,
                4096,
                1000,
                nullptr);

            if (pipe_handle_ == INVALID_HANDLE_VALUE) {
                Sleep(1000);
                continue;
            }

            if (!ConnectNamedPipe(pipe_handle_, nullptr)) {
                DisconnectNamedPipe(pipe_handle_);
                continue;
            }

            char buffer[4096];
            DWORD bytes_read = 0;

            while (ReadFile(pipe_handle_, buffer, sizeof(buffer), &bytes_read, nullptr) && bytes_read > 0) {
                std::string command(buffer, bytes_read);

                if (command == "SCAN" || command == "GET_RESULTS") {
                    auto results = db_->get_cached_files();
                    std::string response = serialize_results(results);
                    DWORD bytes_written;
                    WriteFile(pipe_handle_, response.c_str(), static_cast<DWORD>(response.size()), &bytes_written, nullptr);
                }

                if (command == "SCAN") break;
            }

            DisconnectNamedPipe(pipe_handle_);
        }
    }

    std::string serialize_results(const std::vector<FileInfo>& files) {
        std::string json = "{\n  \"results\": [\n";

        for (size_t i = 0; i < files.size(); ++i) {
            const auto& f = files[i];
            std::string path = PathUtils::wide_to_utf8(f.path);

            std::string escaped_path;
            escaped_path.reserve(path.size() * 2);
            for (char c : path) {
                if (c == '\\') escaped_path += "\\\\";
                else escaped_path += c;
            }

            json += "    {\n";
            json += "      \"path\": \"" + escaped_path + "\",\n";
            json += "      \"size\": " + std::to_string(f.size) + ",\n";
            json += "      \"xxhash32\": " + std::to_string(static_cast<uint32_t>(f.xxhash)) + "\n";

            if (i < files.size() - 1) {
                json += "    },\n";
            } else {
                json += "    }\n";
            }
        }

        json += "  ]\n}";

        return json;
    }
};

NamedPipeServer::NamedPipeServer(const std::wstring& pipe_name, DatabaseManager* db) {
    impl_ = std::make_unique<PipeImpl>(pipe_name, db);
}

void NamedPipeServer::start() {
    if (impl_) impl_->start();
}

NamedPipeServer::~NamedPipeServer() = default;