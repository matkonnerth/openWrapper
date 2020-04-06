#pragma once
#include <iostream>
#include <opc/Variant.h>
#include <string>
#include <vector>

namespace opc
{
namespace types
{
class NodeId;
}

class Variant;
} // namespace opc

class MyDataSource
{
  public:
    MyDataSource(const std::string &key) : key(key) {}

    void read(const opc::types::NodeId &id, opc::Variant &var)
    {
        std::cout << "datasource: " << key << " read called" << std::endl;
        var(std::vector{1, 2, 3, 4, 54, 56});
    }

    void write(const opc::types::NodeId &id, opc::Variant &var) {}

    const std::string &getKey() { return key; }

  private:
    std::string key;
};