#include "MCPServerManager.hpp"
#include "GlobalLogger.hpp"
#include <algorithm>
#include <chrono>
#include <thread>
#include <unistd.h> // For fork, execvp, pipe
#include <sys/wait.h> // For waitpid
#include <fcntl.h> // For fcntl
#include <vector> // For std::vector
#include <stdexcept> // For std::runtime_error
#include <cstring> // For strerror
#include <cerrno> // For errno

MCPServerManager::MCPServerManager() {
    get_logger().log(LogLevel::Info, "MCPServerManager initialized");
}

MCPServerManager::~MCPServerManager() {
    disconnect_all();
    get_logger().log(LogLevel::Info, "MCPServerManager destroyed");
}

std::expected<void, MCPServerError> MCPServerManager::initialize(const std::string& config_path) {
    config_ = MCPServerConfig(config_path);
    
    auto load_result = config_.load();
    if (!load_result.has_value()) {
        get_logger().log(LogLevel::Error, std::format("Failed to load MCP configuration: {}", static_cast<int>(load_result.error())));
        return load_result;
    }
    
    get_logger().log(LogLevel::Info, std::format("MCPServerManager initialized with {} servers", config_.servers().size()));
    
    // Log available servers
    for (const auto& [name, server] : config_.servers()) {
        get_logger().log(LogLevel::Info, std::format("  - {} ({}): {}", name, server.enabled ? "enabled" : "disabled", server.description));
    }
    
    return {};
}

std::expected<void, MCPServerError> MCPServerManager::connect_all() {
    auto enabled_servers = config_.get_enabled_servers();
    
    if (enabled_servers.empty()) {
        get_logger().log(LogLevel::Warning, "No enabled MCP servers found in configuration");
        return {};
    }
    
    get_logger().log(LogLevel::Info, std::format("Connecting to {} enabled MCP servers", enabled_servers.size()));
    
    bool any_failed = false;
    for (const auto& server_name : enabled_servers) {
        auto result = connect_server(server_name);
        if (!result.has_value()) {
            get_logger().log(LogLevel::Error, std::format("Failed to connect to MCP server '{}': {}", server_name, static_cast<int>(result.error())));
            any_failed = true;
        }
    }
    
    if (any_failed) {
        return std::unexpected(MCPServerError::ConnectionError);
    }
    
    return {};
}

std::expected<void, MCPServerError> MCPServerManager::connect_server(const std::string& name) {
    auto server_result = config_.get_server(name);
    if (!server_result.has_value()) {
        get_logger().log(LogLevel::Error, std::format("Server '{}' not found in configuration", name));
        return std::unexpected(MCPServerError::ServerNotFound);
    }
    
    const auto& server = server_result.value();
    
    if (!server.enabled) {
        get_logger().log(LogLevel::Info, std::format("Server '{}' is disabled, skipping connection", name));
        return {};
    }
    
    get_logger().log(LogLevel::Info, std::format("Connecting to MCP server: {} ({})", name, server.description));
    
    // Create client
    auto client_result = create_client(server);
    if (!client_result.has_value()) {
        log_connection_status(name, false, "Failed to create client");
        return std::unexpected(client_result.error());
    }
    
    auto client = client_result.value();
    
    // Start server process if needed
    if (server.connection_type == "stdio") {
        auto start_result = start_server_process(server);
        if (!start_result.has_value()) {
            log_connection_status(name, false, "Failed to start server process");
            return std::unexpected(start_result.error());
        }
    }
    
    // Store client and mark as connected
    clients_[name] = client;
    connection_status_[name] = true;
    
    log_connection_status(name, true);
    return {};
}

void MCPServerManager::disconnect_all() {
    get_logger().log(LogLevel::Info, "Disconnecting from all MCP servers");
    
    for (const auto& [name, client] : clients_) {
        disconnect_server(name);
    }
    
    clients_.clear();
    connection_status_.clear();
}

void MCPServerManager::disconnect_server(const std::string& name) {
    auto it = clients_.find(name);
    if (it != clients_.end()) {
        get_logger().log(LogLevel::Info, std::format("Disconnecting from MCP server: {}", name));
        
        // Stop the associated process if it's a stdio server
        stop_server_process(name);

        // MCPClient should handle cleanup in its destructor
        clients_.erase(it);
        connection_status_[name] = false;
        
        log_connection_status(name, false);
    }
}

std::vector<std::string> MCPServerManager::get_connected_servers() const {
    std::vector<std::string> connected;
    
    for (const auto& [name, status] : connection_status_) {
        if (status) {
            connected.push_back(name);
        }
    }
    
    return connected;
}

std::vector<std::string> MCPServerManager::get_available_servers() const {
    std::vector<std::string> available;
    
    for (const auto& [name, server] : config_.servers()) {
        available.push_back(name);
    }
    
    return available;
}

std::expected<MCPServerConfiguration, MCPServerError> MCPServerManager::get_server_info(const std::string& name) const {
    return config_.get_server(name);
}

std::shared_ptr<MCPClient> MCPServerManager::get_client(const std::string& name) const {
    auto it = clients_.find(name);
    if (it != clients_.end()) {
        return it->second;
    }
    return nullptr;
}

bool MCPServerManager::is_connected(const std::string& name) const {
    auto it = connection_status_.find(name);
    return it != connection_status_.end() && it->second;
}

std::expected<void, MCPServerError> MCPServerManager::reload_config() {
    get_logger().log(LogLevel::Info, "Reloading MCP configuration");
    
    // Disconnect all current connections
    disconnect_all();
    
    // Reload configuration
    auto load_result = config_.load();
    if (!load_result.has_value()) {
        get_logger().log(LogLevel::Error, "Failed to reload MCP configuration");
        return load_result;
    }
    
    get_logger().log(LogLevel::Info, "MCP configuration reloaded successfully");
    return {};
}

void MCPServerManager::health_check() {
    get_logger().log(LogLevel::Debug, std::format("Performing health check on {} connected servers", clients_.size()));
    
    for (const auto& [name, client] : clients_) {
        if (client) {
            // Basic health check - could be extended with ping/status requests
            bool is_healthy = true; // client->is_healthy(); // Would need to implement this
            
            if (!is_healthy) {
                get_logger().log(LogLevel::Warning, std::format("MCP server '{}' failed health check", name));
                connection_status_[name] = false;
            }
        }
    }
}

std::expected<std::shared_ptr<MCPClient>, MCPServerError> MCPServerManager::create_client(const MCPServerConfiguration& server) {
    try {
        auto client = std::make_shared<MCPClient>();
        
        // Configure client based on server info
        if (server.connection_type == "websocket" && !server.url.empty()) {
            client = std::make_shared<MCPClient>(server.url);
            get_logger().log(LogLevel::Info, std::format("Creating WebSocket MCP client for: {}", server.url));
        } else if (server.connection_type == "stdio") {
            // Retrieve the process info for this server
            auto it = stdio_processes_.find(server.name);
            if (it == stdio_processes_.end()) {
                get_logger().log(LogLevel::Error, std::format("STDIO process info not found for server: {}", server.name));
                return std::unexpected(MCPServerError::InitializationError);
            }
            MCPProcessInfo& proc_info = it->second;
            client = std::make_shared<MCPClient>(proc_info.stdin_fd, proc_info.stdout_fd);
            get_logger().log(LogLevel::Info, std::format("Creating stdio MCP client for command: {}", server.command));
        } else {
            get_logger().log(LogLevel::Error, std::format("Unsupported connection type: {}", server.connection_type));
            return std::unexpected(MCPServerError::InitializationError);
        }
        
        return client;
        
    } catch (const std::exception& e) {
        get_logger().log(LogLevel::Error, std::format("Failed to create MCP client: {}", e.what()));
        return std::unexpected(MCPServerError::InitializationError);
    }
}

std::expected<void, MCPServerError> MCPServerManager::start_server_process(const MCPServerConfiguration& server) {
    if (server.connection_type != "stdio") {
        return {}; // No process to start for non-stdio connections
    }

    if (stdio_processes_.count(server.name)) {
        get_logger().log(LogLevel::Warning, std::format("MCP server process for '{}' already running.", server.name));
        return {};
    }
    
    get_logger().log(LogLevel::Info, std::format("Starting MCP server process: {} {}", server.command, 
                      [&]() {
                          std::string args_str;
                          for (const auto& arg : server.args) {
                              args_str += arg + " ";
                          }
                          return args_str;
                      }()));

    int stdin_pipe[2];  // parent_write_fd, child_read_fd
    int stdout_pipe[2]; // child_write_fd, parent_read_fd

    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
        get_logger().log(LogLevel::Error, std::format("Failed to create pipes: {}", strerror(errno)));
        return std::unexpected(MCPServerError::ProcessSpawnError);
    }

    pid_t pid = fork();

    if (pid == -1) {
        get_logger().log(LogLevel::Error, std::format("Failed to fork process: {}", strerror(errno)));
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        return std::unexpected(MCPServerError::ProcessSpawnError);
    } else if (pid == 0) { // Child process
        // Redirect child's stdin to read end of stdin_pipe
        if (dup2(stdin_pipe[0], STDIN_FILENO) == -1) {
            get_logger().log(LogLevel::Error, std::format("Child: Failed to dup2 stdin: {}", strerror(errno)));
            _exit(EXIT_FAILURE);
        }
        // Redirect child's stdout to write end of stdout_pipe
        if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1) {
            get_logger().log(LogLevel::Error, std::format("Child: Failed to dup2 stdout: {}", strerror(errno)));
            _exit(EXIT_FAILURE);
        }

        // Close all pipe file descriptors in the child process
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);

        // Prepare arguments for execvp
        std::vector<char*> args_c_str;
        args_c_str.push_back(const_cast<char*>(server.command.c_str()));
        for (const auto& arg : server.args) {
            args_c_str.push_back(const_cast<char*>(arg.c_str()));
        }
        args_c_str.push_back(nullptr); // Null-terminate the argument list

        // Set environment variables
        for (const auto& [key, value] : server.env) {
            setenv(key.c_str(), value.c_str(), 1);
        }

        // Execute the command
        execvp(server.command.c_str(), args_c_str.data());
        
        // If execvp returns, an error occurred
        get_logger().log(LogLevel::Error, std::format("Child: Failed to execute command '{}': {}", server.command, strerror(errno)));
        _exit(EXIT_FAILURE); // Exit child process with failure
    } else { // Parent process
        // Close child's ends of the pipes
        close(stdin_pipe[0]);  // Close child's read end of stdin pipe
        close(stdout_pipe[1]); // Close child's write end of stdout pipe

        // Store process info
        stdio_processes_[server.name] = {pid, stdin_pipe[1], stdout_pipe[0]};
        get_logger().log(LogLevel::Info, std::format("Started MCP server process '{}' with PID {}", server.name, pid));
    }
    
    return {};
}

void MCPServerManager::stop_server_process(const std::string& name) {
    auto it = stdio_processes_.find(name);
    if (it != stdio_processes_.end()) {
        MCPProcessInfo& proc_info = it->second;
        get_logger().log(LogLevel::Info, std::format("Stopping MCP server process '{}' with PID {}", name, proc_info.pid));

        // Close pipe file descriptors
        close(proc_info.stdin_fd);
        close(proc_info.stdout_fd);

        // Send SIGTERM to the process
        if (kill(proc_info.pid, SIGTERM) == -1) {
            get_logger().log(LogLevel::Error, std::format("Failed to send SIGTERM to PID {}: {}", proc_info.pid, strerror(errno)));
        }

        // Wait for the process to terminate
        int status;
        pid_t result = waitpid(proc_info.pid, &status, 0);
        if (result == -1) {
            get_logger().log(LogLevel::Error, std::format("Failed to wait for PID {}: {}", proc_info.pid, strerror(errno)));
        } else if (WIFEXITED(status)) {
            get_logger().log(LogLevel::Info, std::format("Process PID {} exited with status {}", proc_info.pid, WEXITSTATUS(status)));
        } else if (WIFSIGNALED(status)) {
            get_logger().log(LogLevel::Info, std::format("Process PID {} terminated by signal {}", proc_info.pid, WTERMSIG(status)));
        }

        stdio_processes_.erase(it);
    }
}

void MCPServerManager::log_connection_status(const std::string& name, bool connected, const std::string& error) {
    if (connected) {
        get_logger().log(LogLevel::Info, std::format("✓ MCP server '{}' connected successfully", name));
    } else {
        if (error.empty()) {
            get_logger().log(LogLevel::Info, std::format("✗ MCP server '{}' disconnected", name));
        } else {
            get_logger().log(LogLevel::Error, std::format("✗ MCP server '{}' connection failed: {}", name, error));
        }
    }
}