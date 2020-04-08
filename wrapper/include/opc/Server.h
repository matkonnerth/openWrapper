#pragma once
#include <memory>
#include <opc/DataSource.h>
#include <opc/NodeMetaInfo.h>
#include <opc/VariableAttributes.h>
#include <opc/methods/Method.h>
#include <opc/methods/MethodWrapper.h>
#include <opc/types/NodeId.h>
#include <opc/types/Types.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config.h>
#include <open62541/types.h>
#include <unordered_map>

struct UA_Server;

namespace opc
{
class BaseEventType;
class MethodNode;
class ObjectNode;

class Server
{
  public:
    Server();
    explicit Server(uint16_t port);
    ~Server();
    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;
    Server(Server &&) = delete;
    Server &operator=(Server &&) = delete;

    void run();
    bool loadNodeset(const std::string &path);

    std::shared_ptr<MethodNode>
    createMethod(const NodeId &objId, const NodeId &methodId,
                 const QualifiedName &browseName,
                 const std::vector<UA_Argument> &in,
                 const std::vector<UA_Argument> &outArgs);

    bool call(void *objectContext, const NodeId &id,
              const std::vector<Variant> &inputArgs,
              std::vector<Variant> &outputArgs);

    template <typename T>
    void addVariableNode(const NodeId &parentId, const NodeId &requestedId,
                         const std::string &browseName, T initialValue,
                         std::unique_ptr<NodeMetaInfo> info)
    {

        UA_VariableAttributes attr = getVariableAttributes(initialValue);
        attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        UA_Server_addVariableNode(
            server, fromNodeId(requestedId), fromNodeId(parentId),
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            UA_QUALIFIEDNAME(1, (char *)browseName.c_str()),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr,
            info.get(), nullptr);

        nodeMetaInfos.emplace_back(std::move(info));

        UA_Server_setVariableNode_dataSource(server, fromNodeId(requestedId),
                                             internalSrc);
    }

    template <typename T>
    void addVariableNode(const NodeId &parentId, const NodeId &requestedId,
                         const std::string &browseName, T initialValue)
    {
        UA_VariableAttributes attr = getVariableAttributes(initialValue);
        attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        UA_Server_addVariableNode(
            server, fromNodeId(requestedId), fromNodeId(parentId),
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            UA_QUALIFIEDNAME(1, (char *)browseName.c_str()),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, nullptr,
            nullptr);
    }

    void registerDataSource(
        const std::string &key,
        std::function<void(const NodeId &id, Variant &var)> read,
        std::function<void(const NodeId &id, Variant &var)> write);

    auto &getDataSources() { return datasources; }

    bool writeValue(const NodeId &id, const Variant &var);
    bool readValue(const NodeId &id, Variant &var) const;

    LocalizedText readDisplayName(const NodeId &id);

    uint16_t getNamespaceIndex(const std::string &uri);

    UA_Server *getUAServer();
    const UA_Server *getUAServer() const;

    void setEvent(const BaseEventType &event, const opc::NodeId &sourceNode);

    std::shared_ptr<ObjectNode> getObject(const NodeId &);
    std::shared_ptr<ObjectNode> createObject(const NodeId &parentId,
                                             const NodeId &requestedId,
                                             const NodeId &typeId,
                                             const QualifiedName &browseName,
                                             void *context);
    std::shared_ptr<ObjectNode> getObjectsFolder();
    std::shared_ptr<MethodNode> getMethod(const NodeId &id);
    std::shared_ptr<MethodNode> createMethod(const NodeId &objectId,
                                             const NodeId &methodId,
                                             QualifiedName browseName);
    void connectMethodCallback(const NodeId&id);

  private:
    UA_Server *server{nullptr};
    UA_DataSource internalSrc{internalRead, internalWrite};
    bool isRunning{false};
    static UA_StatusCode internalMethodCallback(
        UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
        const UA_NodeId *methodId, void *methodContext,
        const UA_NodeId *objectId, void *objectContext, size_t inputSize,
        const UA_Variant *input, size_t outputSize, UA_Variant *output);
    std::vector<std::unique_ptr<DataSource>> datasources{};
    std::vector<std::unique_ptr<NodeMetaInfo>> nodeMetaInfos{};
    void create(uint16_t port);
    static UA_StatusCode
    internalRead(UA_Server *server, const UA_NodeId *sessionId,
                 void *sessionContext, const UA_NodeId *nodeId,
                 void *nodeContext, UA_Boolean includeSourceTimeStamp,
                 const UA_NumericRange *range, UA_DataValue *value);

    static UA_StatusCode
    internalWrite(UA_Server *server, const UA_NodeId *sessionId,
                  void *sessionContext, const UA_NodeId *nodeId,
                  void *nodeContext, const UA_NumericRange *range,
                  const UA_DataValue *value);
    UA_StatusCode setUpEvent(UA_NodeId *outId, const BaseEventType &event);

    UA_StatusCode getNodeIdForPath(const UA_NodeId objectId,
                                   const std::vector<QualifiedName> &qn,
                                   UA_NodeId *outId);

    std::unordered_map<NodeId, std::shared_ptr<ObjectNode>> objects{};
    std::unordered_map<NodeId, std::shared_ptr<MethodNode>> methods{};
};
} // namespace opc