#include "opc.hpp"

#include <opc/opc.h>

namespace opc {
void initLibrary()
{
  opcInitLibrary();
}

void freeLibrary()
{
  opcFreeLibrary();
}
}
