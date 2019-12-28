#include "container.hpp"

#include <ostream>
#include <cstdio>

namespace opc {
bool Container::open(const char *filename, ContainerOpenMode openMode)
{
  m_container = opcContainerOpen(BAD_CAST(filename), openMode, nullptr, nullptr);
}

bool Container::dump(std::ostream &out)
{
  auto *tmp = std::tmpfile();
  opcContainerDump(m_container, tmp);
  std::string result;

  fseek(tmp, 0L, SEEK_END);
  auto size = ftell(tmp);
  rewind(tmp);
  result.resize(size);
  fread((void *)result.data(), 1, size, tmp);
  fclose(tmp);
  out << result;
  return true;
}

Container::~Container() {
  close();
}

void Container::close() {
  if (m_container) {
    opcContainerClose(m_container, OPC_CLOSE_NOW);
  }
}
}