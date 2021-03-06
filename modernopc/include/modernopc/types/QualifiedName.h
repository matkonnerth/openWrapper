#pragma once
#include <modernopc/DataType.h>
#include <open62541/types.h>
#include <string>

namespace modernopc
{
class QualifiedName
{
  public:
    QualifiedName() = default;
    QualifiedName(uint16_t namespaceIndex, const std::string &name)
        : nsIdx{namespaceIndex}, n{name}
    {
    }

    QualifiedName(const QualifiedName&)=default;
    QualifiedName& operator=(const QualifiedName&)=default;
    QualifiedName(QualifiedName&&)=default;
    QualifiedName& operator=(QualifiedName&&)=default;

    uint16_t namespaceIndex() const { return nsIdx; }
    const std::string &name() const { return n; }

  private:
    uint16_t nsIdx{};
    std::string n{};

    friend std::ostream &operator<<(std::ostream &os, const QualifiedName &qn);
    friend void toUAVariantImpl(const QualifiedName &qn,
                                       UA_Variant *var);
    friend const UA_QualifiedName fromQualifiedName(const QualifiedName &qn);
};

template <>
inline const UA_DataType *getDataType<QualifiedName>()
{
    return &UA_TYPES[UA_TYPES_QUALIFIEDNAME];
}

QualifiedName fromUAQualifiedName(const UA_QualifiedName *qn);
}
