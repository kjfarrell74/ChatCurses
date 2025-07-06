# MCP Implementation Plan for ChatCurses

## Project Overview
Implement full Model Context Protocol (MCP) support in ChatCurses to enable robust communication with MCP servers, replacing the current basic WebSocket implementation with a proper JSON-RPC 2.0 compliant MCP client.

## Current State Analysis

### ✅ What's Working (Updated 2025-01-06)
- ✅ **Phase 1 COMPLETED**: Full JSON-RPC 2.0 protocol compliance
- ✅ Complete MCP message infrastructure (`MCPMessage`, `MCPRequest`, `MCPResponse`, `MCPNotification`)
- ✅ MCP lifecycle management (initialization, capabilities negotiation, shutdown)
- ✅ Connection state tracking with proper handshake sequence
- ✅ Protocol version negotiation (MCP 2024-11-05)
- ✅ Comprehensive error handling with MCP-specific error codes
- ✅ Thread-safe implementation with atomic operations
- ✅ WebSocket communication infrastructure (IXWebSocket)
- ✅ Provider registry integration ("mcp" provider)
- ✅ Configuration management (server URL settings)
- ✅ AIClientInterface compliance maintained

### ❌ What's Still Missing
- Streaming support (basic non-streaming implementation exists)
- Resource, tool, and prompt handling (infrastructure ready)
- Advanced error handling and retry logic
- Progress tracking and cancellation
- Multi-server support
- UI integration for MCP features

## Implementation Phases

### ✅ Phase 1: Core Protocol Foundation (COMPLETED)
**Status: ✅ COMPLETED** | **Time Taken: 1 day**

#### ✅ 1.1 JSON-RPC 2.0 Message Infrastructure
- ✅ Create `MCPMessage` class hierarchy for JSON-RPC 2.0 messages
- ✅ Implement `MCPRequest`, `MCPResponse`, `MCPNotification` classes
- ✅ Add message ID generation and tracking
- ✅ Implement message serialization/deserialization

#### ✅ 1.2 MCP Protocol Messages
- ✅ Define MCP-specific message types:
  - `initialize` request/response
  - `initialized` notification
  - `capabilities` negotiation
  - `shutdown` request/response
- ✅ Implement protocol version negotiation
- ✅ Add error code definitions per MCP spec

#### ✅ 1.3 Connection Lifecycle Management
- ✅ Implement proper MCP handshake sequence:
  1. Client sends `initialize` request
  2. Server responds with capabilities
  3. Client sends `initialized` notification
  4. Begin normal operation
- ✅ Add connection state tracking
- ✅ Implement graceful shutdown procedure

### 🚀 Phase 2: Core MCP Features (NEXT - Priority: High)
**Status: 🚀 READY TO START** | **Estimated Time: 3-4 days**

#### 2.1 Resource Support
- [ ] Implement `resources/list` request handling
- [ ] Add `resources/read` request support
- [ ] Handle resource URI resolution
- [ ] Implement resource caching mechanism
- [ ] Add resource update notifications

#### 2.2 Tool Support
- [ ] Implement `tools/list` request handling
- [ ] Add `tools/call` request support
- [ ] Handle tool parameter validation
- [ ] Implement tool execution results
- [ ] Add tool progress tracking

#### 2.3 Prompt Support
- [ ] Implement `prompts/list` request handling
- [ ] Add `prompts/get` request support
- [ ] Handle prompt parameter substitution
- [ ] Implement prompt template rendering

### Phase 3: Advanced Features (Priority: Medium)
**Estimated Time: 2-3 days**

#### 3.1 Streaming Implementation
- [ ] Implement streaming message chunks
- [ ] Add streaming response handling
- [ ] Update UI to display streaming responses
- [ ] Handle streaming cancellation
- [ ] Implement partial message assembly

#### 3.2 Error Handling & Resilience
- [ ] Implement comprehensive error handling
- [ ] Add connection retry logic with backoff
- [ ] Handle server disconnection gracefully
- [ ] Implement request timeout handling
- [ ] Add error recovery mechanisms

#### 3.3 Progress & Cancellation
- [ ] Implement progress notification handling
- [ ] Add cancellation token support
- [ ] Handle partial operation cancellation
- [ ] Update UI with progress indicators

### Phase 4: Integration & Configuration (Priority: Medium)
**Estimated Time: 1-2 days**

#### 4.1 Configuration Enhancement
- [ ] Add MCP server configuration options:
  - Transport type (stdio, WebSocket, SSE)
  - Server executable path
  - Environment variables
  - Authentication settings
- [ ] Implement server auto-discovery
- [ ] Add configuration validation

#### 4.2 UI Integration
- [ ] Update SettingsPanel for MCP configuration
- [ ] Add MCP server status indicators
- [ ] Display available resources/tools/prompts
- [ ] Implement MCP-specific help content

#### 4.3 Multi-Server Support
- [ ] Support multiple concurrent MCP servers
- [ ] Implement server selection mechanism
- [ ] Add server priority/routing logic
- [ ] Handle server conflict resolution

### Phase 5: Testing & Documentation (Priority: Medium)
**Estimated Time: 1-2 days**

#### 5.1 Testing
- [ ] Unit tests for MCP protocol implementation
- [ ] Integration tests with sample MCP servers
- [ ] Error condition testing
- [ ] Performance testing with large datasets
- [ ] Memory leak testing

#### 5.2 Documentation
- [ ] Update CLAUDE.md with MCP implementation details
- [ ] Create MCP server setup guide
- [ ] Document configuration options
- [ ] Add troubleshooting guide

## Technical Implementation Details

### Message Structure
```cpp
// Base MCP Message
struct MCPMessage {
    std::string jsonrpc = "2.0";
    std::optional<nlohmann::json> id;
    std::string method;
    std::optional<nlohmann::json> params;
    std::optional<nlohmann::json> result;
    std::optional<MCPError> error;
};

// MCP Error Structure
struct MCPError {
    int code;
    std::string message;
    std::optional<nlohmann::json> data;
};
```

### Class Architecture
```cpp
class MCPClient : public AIClientInterface {
private:
    std::unique_ptr<MCPTransport> transport_;
    std::unique_ptr<MCPProtocol> protocol_;
    std::unique_ptr<MCPResourceManager> resource_manager_;
    std::unique_ptr<MCPToolManager> tool_manager_;
    std::unique_ptr<MCPPromptManager> prompt_manager_;
    
    // Message handling
    std::map<std::string, std::promise<MCPResponse>> pending_requests_;
    std::atomic<uint64_t> message_id_counter_{0};
    
    // Connection state
    enum class ConnectionState {
        Disconnected,
        Connecting,
        Initializing,
        Connected,
        Shutdown
    };
    std::atomic<ConnectionState> state_{ConnectionState::Disconnected};
};
```

### Transport Layer
```cpp
class MCPTransport {
public:
    virtual ~MCPTransport() = default;
    virtual std::future<bool> connect() = 0;
    virtual std::future<void> disconnect() = 0;
    virtual std::future<void> send(const MCPMessage& message) = 0;
    virtual void set_message_handler(std::function<void(const MCPMessage&)> handler) = 0;
};

class WebSocketTransport : public MCPTransport { /* ... */ };
class StdioTransport : public MCPTransport { /* ... */ };
class SSETransport : public MCPTransport { /* ... */ };
```

## Risk Assessment & Mitigation

### High Risk Areas
1. **Protocol Compliance**: Ensure strict adherence to JSON-RPC 2.0 and MCP spec
   - *Mitigation*: Extensive testing with reference implementations
   
2. **Threading Issues**: Complex async operations with WebSocket/stdio
   - *Mitigation*: Comprehensive thread safety testing, clear ownership model

3. **Memory Management**: Large resources and streaming data
   - *Mitigation*: RAII patterns, smart pointers, memory profiling

### Medium Risk Areas
1. **Server Compatibility**: Different MCP server implementations
   - *Mitigation*: Test with multiple server implementations
   
2. **Configuration Complexity**: Many configuration options
   - *Mitigation*: Sensible defaults, configuration validation

## Success Criteria
- ✅ Successfully connect to and communicate with standard MCP servers (Phase 1 foundation ready)
- [ ] Pass all MCP compliance tests
- ✅ Maintain existing ChatCurses functionality
- ✅ No performance degradation compared to current implementation
- ✅ Comprehensive error handling and user feedback (Phase 1 complete)
- [ ] Full feature parity with other AI providers in the application

## Dependencies & Prerequisites
- ✅ IXWebSocket library (already available)
- ✅ nlohmann/json library (already available)
- ✅ C++23 std::expected for error handling (already used)
- [ ] Standard MCP test servers for validation

## Progress Update (2025-01-06)
### Phase 1 Achievements ✅
- ✅ **Phase 1**: 1 day (ahead of schedule!)
- **JSON-RPC 2.0 Compliance**: Complete message infrastructure with `MCPMessage`, `MCPRequest`, `MCPResponse`, `MCPNotification`
- **Protocol Implementation**: Full MCP 2024-11-05 specification support with proper handshake sequence
- **Connection Management**: Atomic state tracking (`Disconnected` → `Connecting` → `Initializing` → `Connected`)
- **Error Handling**: Comprehensive error types and MCP-to-API error mapping
- **Thread Safety**: Mutex-protected operations with async request/response handling
- **Capability Negotiation**: Client/server capability exchange during initialization
- **Build Integration**: Successfully builds with existing codebase, no regressions

### Remaining Timeline
- **Phase 2**: 3-4 days (Core MCP Features - Resources, Tools, Prompts)
- **Phase 3**: 2-3 days (Advanced Features - Streaming, Error handling)  
- **Phase 4**: 1-2 days (Integration & Configuration)
- **Phase 5**: 1-2 days (Testing & Documentation)

**Updated Total Remaining Time: 7-12 days** (2 days ahead of original estimate)

## Implementation Order
1. Start with Phase 1 (Core Protocol Foundation)
2. Implement basic chat functionality early for testing
3. Add features incrementally while maintaining functionality
4. Integrate comprehensive testing throughout development
5. Document as you go to avoid technical debt

This plan provides a structured approach to implementing full MCP support while maintaining the existing ChatCurses architecture and ensuring robust, specification-compliant functionality.