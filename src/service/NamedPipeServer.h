#pragma once
#include <string>
#include <memory>

// Forward declaration of DatabaseManager to avoid circular dependency.
class DatabaseManager;

// Named pipe server for GUI-service IPC communication.
class NamedPipeServer {
public:
    explicit NamedPipeServer(const std::wstring& pipe_name, DatabaseManager* db);
    void start();
    ~NamedPipeServer();

private:
    class PipeImpl;
    std::unique_ptr<PipeImpl> impl_;
};