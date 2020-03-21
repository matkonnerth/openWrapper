#include "nodesetLoader.h"
#include "value.h"
#include <open62541/server.h>
#include <string>
#include <vector>

static UA_NodeId getNodeIdFromChars(TNodeId id)
{
    if (!id.id)
    {
        return UA_NODEID_NULL;
    }
    auto nsidx = static_cast<UA_UInt16>(id.nsIdx);

    switch (id.id[0])
    {
    // integer
    case 'i':
    {
        auto nodeId = static_cast<UA_UInt32>(atoi(&id.id[2]));
        return UA_NODEID_NUMERIC(nsidx, nodeId);
        break;
    }
    case 's':
    {
        return UA_NODEID_STRING_ALLOC(nsidx, &id.id[2]);
        break;
    }
    }
    return UA_NODEID_NULL;
}

static UA_NodeId getTypeDefinitionIdFromChars2(const TNode *node)
{
    Reference *ref = node->nonHierachicalRefs;
    while (ref)
    {
        if (!strcmp("HasTypeDefinition", ref->refType.idString))
        {
            return getNodeIdFromChars(ref->target);
        }
        ref = ref->next;
    }
    return UA_NODEID_NULL;
}

static UA_NodeId getReferenceTypeId(const Reference *ref)
{
    if (!ref)
    {
        return UA_NODEID_NULL;
    }
    if (!strcmp(ref->refType.idString, "HasProperty"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    }
    else if (!strcmp(ref->refType.idString, "HasComponent"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    }
    else if (!strcmp(ref->refType.idString, "Organizes"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    }
    else if (!strcmp(ref->refType.idString, "HasTypeDefinition"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    }
    else if (!strcmp(ref->refType.idString, "HasSubtype"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    }
    else if (!strcmp(ref->refType.idString, "HasEncoding"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASENCODING);
    }
    else
    {
        return getNodeIdFromChars(ref->refType);
    }
    return UA_NODEID_NULL;
}

static UA_NodeId getReferenceTarget(const Reference *ref)
{
    if (!ref)
    {
        return UA_NODEID_NULL;
    }
    return getNodeIdFromChars(ref->target);
}

static Reference *getHierachicalInverseReference(const TNode *node);

static Reference *getHierachicalInverseReference(const TNode *node)
{

    Reference *hierachicalRef = node->hierachicalRefs;
    while (hierachicalRef)
    {
        if (!hierachicalRef->isForward)
        {
            return hierachicalRef;
        }
        hierachicalRef = hierachicalRef->next;
    }
    return nullptr;
}

static UA_NodeId getParentId(const TNode *node, UA_NodeId &parentRefId)
{
    UA_NodeId parentId = UA_NODEID_NULL;
    if (node->nodeClass == NODECLASS_OBJECT)
    {
        parentId =
            getNodeIdFromChars(((const TObjectNode *)node)->parentNodeId);
    }
    else if (node->nodeClass == NODECLASS_VARIABLE)
    {
        parentId =
            getNodeIdFromChars(((const TVariableNode *)node)->parentNodeId);
    }
    Reference *ref = getHierachicalInverseReference((const TNode *)node);
    parentRefId = getReferenceTypeId(ref);
    if (UA_NodeId_equal(&parentId, &UA_NODEID_NULL))
    {
        parentId = getReferenceTarget(ref);
    }
    return parentId;
}

static void handleObjectNode(const TObjectNode *node, UA_NodeId &id,
                      const UA_NodeId &parentId,
                      const UA_NodeId &parentReferenceId, const UA_LocalizedText& lt, const UA_QualifiedName& qn, UA_Server *server)
{
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = lt;

    UA_NodeId typeDefId = getTypeDefinitionIdFromChars2((const TNode *)node);

    UA_Server_addObjectNode(server, id, parentId, parentReferenceId,
                            qn, typeDefId,
                            oAttr, nullptr, nullptr);
}

static void handleMethodNode(const TMethodNode *node, UA_NodeId &id,
                             const UA_NodeId &parentId,
                             const UA_NodeId &parentReferenceId,
                             const UA_LocalizedText &lt,
                             const UA_QualifiedName &qn, UA_Server *server)
{
    UA_MethodAttributes attr = UA_MethodAttributes_default;
    attr.executable = true;
    attr.userExecutable = true;
    attr.displayName = lt;

    UA_Server_addMethodNode(server, id, parentId, parentReferenceId,
                            qn, attr,
                            nullptr, 0, nullptr, 0, nullptr, nullptr, nullptr);
}

static std::vector<uint32_t> getArrayDimensions(const char* s)
{
    std::string dim{s};
    std::vector<uint32_t> arrDims;
    if(dim.length()==0)
    {
        return arrDims;
    }    
    size_t idx = 0;
    //add the first one
    int val = atoi(s);
    arrDims.push_back(static_cast<uint32_t>(val));
    while(idx < dim.length())
    {
        size_t cur = dim.find(';', idx);
        if(cur!= std::string::npos)
        {
            int val = atoi(s+cur+1);
            arrDims.push_back(static_cast<uint32_t>(val));
            idx=cur+1;
        }
        else
        {
            break;
        }
    }
    return arrDims;
}

static void handleVariableNode(const TVariableNode *node, UA_NodeId &id,
                               const UA_NodeId &parentId,
                               const UA_NodeId &parentReferenceId,
                               const UA_LocalizedText &lt,
                               const UA_QualifiedName &qn, UA_Server *server)
{
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = lt;
    attr.dataType = getNodeIdFromChars(node->datatype);
    attr.valueRank = atoi(node->valueRank);
    std::vector<uint32_t> arrDims = getArrayDimensions(node->arrayDimensions);
    attr.arrayDimensionsSize = arrDims.size();
    attr.arrayDimensions = arrDims.data();
    
    if (node->value)
    {
        if (node->value->isArray)
        {
            UA_Variant_setArray(&attr.value, node->value->value,
                                node->value->arrayCnt, node->value->datatype);
            //todo: is this really necessary??
            if(!attr.arrayDimensions)
            {
                attr.arrayDimensions = UA_UInt32_new();
                *attr.arrayDimensions = (UA_UInt32)node->value->arrayCnt;
                attr.arrayDimensionsSize = 1;
            }
        }
        else
        {
            UA_Variant_setScalar(&attr.value, node->value->value,
                                 node->value->datatype);
        }
    }
    UA_NodeId typeDefId = getTypeDefinitionIdFromChars2((const TNode *)node);
    UA_Server_addVariableNode(server, id, parentId, parentReferenceId,
                                  qn,
                                  typeDefId, attr, nullptr, nullptr);

    // value is copied in addVariableNode
    Value_delete(&((TVariableNode *)node)->value);
}

static void handleObjectTypeNode(const TObjectTypeNode *node, UA_NodeId &id,
                                 const UA_NodeId &parentId,
                                 const UA_NodeId &parentReferenceId,
                                 const UA_LocalizedText &lt,
                                 const UA_QualifiedName &qn, UA_Server *server)
{
    UA_ObjectTypeAttributes oAttr = UA_ObjectTypeAttributes_default;
    oAttr.displayName = lt;

    UA_Server_addObjectTypeNode(server, id, parentId, parentReferenceId,
                                qn, oAttr,
                                nullptr, nullptr);
}

static void handleReferenceTypeNode(const TReferenceTypeNode *node,
                                    UA_NodeId &id, const UA_NodeId &parentId,
                                    const UA_NodeId &parentReferenceId,
                                    const UA_LocalizedText &lt,
                                    const UA_QualifiedName &qn,
                                    UA_Server *server)
{
    UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;
    attr.symmetric = true;
    attr.displayName = lt;

    UA_Server_addReferenceTypeNode(server, id, parentId, parentReferenceId,
                                   qn, attr,
                                   nullptr, nullptr);
}

static void handleVariableTypeNode(const TVariableTypeNode *node, UA_NodeId &id,
                                   const UA_NodeId &parentId,
                                   const UA_NodeId &parentReferenceId,
                                   const UA_LocalizedText &lt,
                                   const UA_QualifiedName &qn,
                                   UA_Server *server)
{
    UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;
    attr.displayName = lt;
    attr.valueRank = atoi(node->valueRank);
    attr.isAbstract = node->isAbstract;
    if (attr.valueRank >= 0)
    {
        if (!strcmp(node->arrayDimensions, ""))
        {
            attr.arrayDimensionsSize = 1;
            UA_UInt32 arrayDimensions[1];
            arrayDimensions[0] = 0;
            attr.arrayDimensions = &arrayDimensions[0];
        }
    }

    UA_NodeId typeDefId = getTypeDefinitionIdFromChars2((const TNode *)node);

    UA_Server_addVariableTypeNode(server, id, parentId, parentReferenceId,
                                  qn,
                                  typeDefId, attr, nullptr, nullptr);
}

static void handleDataTypeNode(const TDataTypeNode *node, UA_NodeId &id,
                               const UA_NodeId &parentId,
                               const UA_NodeId &parentReferenceId,
                               const UA_LocalizedText &lt,
                               const UA_QualifiedName &qn, UA_Server *server)
{
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = lt;

    UA_Server_addDataTypeNode(server, id, parentId, parentReferenceId,
                              qn, attr,
                              nullptr, nullptr);
}

void importNodesCallback(void *userContext, const TNode *node)
{
    auto *server = static_cast<UA_Server *>(userContext);
    UA_NodeId id = getNodeIdFromChars(node->id);
    UA_NodeId parentReferenceId = UA_NODEID_NULL;
    UA_NodeId parentId = getParentId(node, parentReferenceId);
    UA_LocalizedText lt = UA_LOCALIZEDTEXT((char *)"", node->displayName);
    UA_QualifiedName qn = UA_QUALIFIEDNAME(node->browseName.nsIdx, node->browseName.name);
    switch (node->nodeClass)
    {
    case NODECLASS_OBJECT:
        handleObjectNode((const TObjectNode *)node, id, parentId,
                         parentReferenceId, lt, qn, server);
        break;

    case NODECLASS_METHOD:
        handleMethodNode((const TMethodNode *)node, id, parentId,
                         parentReferenceId, lt, qn, server);
        break;

    case NODECLASS_OBJECTTYPE:
        handleObjectTypeNode((const TObjectTypeNode *)node, id, parentId,
                             parentReferenceId, lt, qn, server);
        break;

    case NODECLASS_REFERENCETYPE:
        handleReferenceTypeNode((const TReferenceTypeNode *)node, id, parentId,
                                parentReferenceId, lt, qn, server);
        break;

    case NODECLASS_VARIABLETYPE:
        handleVariableTypeNode((const TVariableTypeNode *)node, id, parentId,
                               parentReferenceId, lt, qn, server);
        break;

    case NODECLASS_VARIABLE:
        handleVariableNode((const TVariableNode *)node, id, parentId,
                           parentReferenceId, lt, qn, server);
        break;
    case NODECLASS_DATATYPE:
        handleDataTypeNode((const TDataTypeNode *)node, id, parentId,
                           parentReferenceId, lt, qn, server);
    }
}

int addNamespaceCallback(void *userContext, const char *namespaceUri)
{
    int idx =
        (int)UA_Server_addNamespace((UA_Server *)userContext, namespaceUri);
    return idx;
}