#include <modernopc/Variant.h>
#include <modernopc/types/QualifiedName.h>
#include <modernopc/types/String.h>
#include <ostream>

namespace modernopc
{

template <>
QualifiedName Variant::get<QualifiedName>() const
{
    return fromUAQualifiedName(static_cast<UA_QualifiedName *>(variant->data));
}


const UA_QualifiedName fromQualifiedName(const QualifiedName &qn)
{
    return UA_QUALIFIEDNAME(qn.namespaceIndex(),
                            const_cast<char *>(qn.name().c_str()));
}

QualifiedName fromUAQualifiedName(const UA_QualifiedName *qn)
{
    return QualifiedName(qn->namespaceIndex, fromUAString(qn->name));
}

void toUAVariantImpl(const QualifiedName &qn, UA_Variant *var)
{
    UA_QualifiedName n = fromQualifiedName(qn);
    UA_Variant_setScalarCopy(var, &n, modernopc::getDataType<QualifiedName>());
    var->storageType = UA_VariantStorageType::UA_VARIANT_DATA;
}

std::ostream &operator<<(std::ostream &os, const QualifiedName &qn)
{
    os << "ns=" << qn.namespaceIndex() << ";name=" << qn.name();
    return os;
}

} // namespace modernopc
