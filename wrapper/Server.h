
#include <open62541/server.h>
#include <Method.hpp>
#include "DataSource.h"
#include "NodeId.h"

struct UA_Server;
namespace opc {

class Server {
  public:
    Server();
    ~Server();
    void
    run();
    template <typename M>
    void
    addMethod(const std::string &name, const M &callback) {
        std::vector<UA_Argument> inputArgs = MethodTraits<M>::getInputArguments();

        UA_Argument *data = inputArgs.data();

        UA_MethodAttributes methAttr = UA_MethodAttributes_default;
        methAttr.executable = true;
        methAttr.userExecutable = true;

        UA_Server_addMethodNode(
            server, UA_NODEID_NULL, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            UA_QUALIFIEDNAME(1, const_cast<char *>(name.c_str())), methAttr, nullptr,
            MethodTraits<M>::getNumArgs(), data, 0, nullptr, nullptr, nullptr);
    }

    template <typename T>
    void
    addVariableNode(const NodeId &parentId, const std::string &browseName,
                    T initialValue) {
        UA_VariableAttributes attr = TypeConverter::getVariableAttributes(initialValue);
        attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        UA_Server_addVariableNode(server, UA_NODEID_NULL, parentId.toUA_NodeId(),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(1, (char *)browseName.c_str()),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  attr, nullptr, nullptr);
    }

    template <typename T>
    void
    addVariableNode(const NodeId &parentId, const NodeId &requestedId,
                    const std::string &browseName, T initialValue) {
        UA_VariableAttributes attr = TypeConverter::getVariableAttributes(initialValue);
        attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        UA_Server_addVariableNode(
            server, requestedId.toUA_NodeId(), parentId.toUA_NodeId(),
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            UA_QUALIFIEDNAME(1, (char *)browseName.c_str()),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, nullptr, nullptr);
    }

    void
    setDataSource(const NodeId &id, DataSource &src) {
        UA_Server_setNodeContext(server, id.toUA_NodeId(), &src);
        UA_Server_setVariableNode_dataSource(server, id.toUA_NodeId(),
                                             src.getRawSource());
    }

  private:
    UA_Server *server;
    bool isRunning;
};
}  // namespace opc