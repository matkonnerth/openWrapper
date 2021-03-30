#include <opc/Variant.h>

namespace opc
{

Variant::Variant()
{
    variant = UA_Variant_new();
    owned = true;
}

Variant::Variant(UA_Variant *var, bool owner)
{
    variant = var;
    owned = owner;
}

Variant::~Variant()
{
    if (owned)
    {
        UA_Variant_delete(variant);
    }
}

Variant::Variant(Variant &&other) noexcept
{
    owned = other.owned;
    variant = other.variant;
    other.owned = false;
    other.variant = nullptr;
}

Variant &Variant::operator=(Variant &&other) noexcept
{
    if (owned)
    {
        UA_Variant_delete(variant);
    }
    owned = other.owned;
    variant = other.variant;
    other.owned = false;
    other.variant = nullptr;
    return *this;
}

bool Variant::isScalar() { return UA_Variant_isScalar(variant); }
bool Variant::isEmpty() const { return UA_Variant_isEmpty(variant); }

} // namespace opc