#include "NamedPipeServer.h"
#include <windows.h>
#include <string>
#include <thread>
#include <memory>
#include <algorithm>

class NamedPipeServer::PipeImpl {
public:
    explicit PipeImpl(const std::wstring& pipe_name, DatabaseManager* db)
        : pipe_name_(L"\\\\.\\pipe\\" + pipe_name), db_(db) {}

    void start() { running_ = true; server_thread_ = std::thread([this]() { run_loop(); }); }
    ~PipeImpl() { if (running_) { CloseHandle(pipe_handle_); server_thread_.join(); } }

private:
    std::wstring pipe_name_; DatabaseManager* db_ = nullptr; bool running_ = false;
    HANDLE pipe_handle_ = INVALID_HANDLE_VALUE; std::thread server_thread_;

    void run_loop() {
        while (running_) {
            pipe_handle_ = CreateNamedPipeW(pipe_name_.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 10, 4096, 4096, 1000, nullptr);
            if (pipe_handle_ == INVALID_HANDLE_VALUE) { Sleep(1000); continue; }
            ConnectNamedPipe(pipe_handle_, nullptr);
            char buffer[4096]; DWORD bytes_read = 0;
            while (ReadFile(pipe_handle_, buffer, sizeof(buffer), &bytes_read, nullptr) && bytes_read > 0) {
                std::string command(buffer, bytes_read);
                if (command == "SCAN" || command == "GET_RESULTS") {
                    auto results = db_->get_cached_files();
                    DWORD bytes_written; WriteFile(pipe_handle_, serialize_results(results).c_str(), static_cast<DWORD>(serialize_results(results).size()), &bytes_written, nullptr);
                }
                if (command == "SCAN") break;
            }
            DisconnectNamedPipe(pipe_handle_);
        }
    }

    std::string serialize_results(const std::vector<FileInfo>& files) {
        std::string json = "{\n  \"results\": [\n"; size_t count = 0;
        for (const auto& f : files) {
            std::string path_str = PathUtils::wide_to_utf8(f.path), escaped;
            for (char c : path_str) escaped += (c == '\\') ? "\\\\" : c;
            json += "    {\n      \"path\": \"" + escaped + "\",\n      \"size\": " + std::to_string(f.size) + ",\n";
            json += "      \"xxhash32\": " + std::to_string(static_cast<uint32_t>(f.xxhash)) + "\n" + (++count < files.size() ? "    },\n" : "    }\n");
        }
        return json + "  ]\n}";
    }
};

NamedPipeServer::NamedPipeServer(const std::wstring& pipe_name, DatabaseManager* db) { impl_ = std::make_unique<PipeImpl>(pipe_name, db); }
void NamedPipeServer::start() { if (impl_) impl_->start(); }
NamedPipeServer::~NamedPipeServer() = default;