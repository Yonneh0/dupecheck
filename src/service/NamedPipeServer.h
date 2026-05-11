#pragma once
#include <string>
#include <memory>
#include "database/DatabaseManager.h"

class NamedPipeServer {
public:
    explicit NamedPipeServer(const std::wstring& pipe_name, DatabaseManager* db);

    void start();

    ~NamedPipeServer();

private:
    class PipeImpl;
    std::unique_ptr<PipeImpl> impl_;
};
