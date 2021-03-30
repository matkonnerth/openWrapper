#include <gtest/gtest.h>
#include <modernOpc/Server.h>
#include <modernOpc/Variant.h>
#include <modernOpc/nodes/VariableNode.h>
#include <modernOpc/types/NodeId.h>
#include <string>
#include <vector>

using opc::NodeId;
using opc::QualifiedName;
using namespace std::string_literals;

TEST(Server, read)
{
    opc::Server s;
    opc::Variant var {};
    ASSERT_TRUE(s.getVariable(NodeId(0, 2255))->read(var));
    ASSERT_TRUE(var.is_a<std::vector<std::string>>());
}

TEST(Server, translate)
{
    opc::Server s;
    std::vector<QualifiedName> path {QualifiedName(0, "DataTypes"), QualifiedName(0, "BaseDataType")};
    NodeId outId;
    ASSERT_EQ(UA_STATUSCODE_GOOD, s.translatePathToNodeId(NodeId(0, 86), path, outId));
    ASSERT_EQ(NodeId(0,24), outId);
}

TEST(Server, translateFail)
{
    opc::Server s;
    std::vector<QualifiedName> path{};
    NodeId outId;
    ASSERT_TRUE(UA_STATUSCODE_GOOD!=
              s.translatePathToNodeId(NodeId(0, 86), path, outId));
}
