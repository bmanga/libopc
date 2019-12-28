#ifndef LIBOPC_V2_CONTAINER_HPP
#define LIBOPC_V2_CONTAINER_HPP

#include <opc/container.h>
#include <opc/internal.h>

#include <iosfwd>

namespace opc {
using ContainerOpenMode = opcContainerOpenMode;

class Container
{
 public:
  Container(const char *filename, ContainerOpenMode openMode)
  {
    open(filename, openMode);
  }
  ~Container();

  bool is_open() const { return m_container != nullptr; }
  bool open(const char *filename, ContainerOpenMode openMode);
  void close();
  bool dump(std::ostream &out);

 private:
  opcContainer *m_container = nullptr;
};
};

#endif  // LIBOPC_V2_CONTAINER_HPP
