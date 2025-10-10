#pragma once

#include <iostream>

#include "popl.hpp"

#define INPUT_ERROR 1
#define INPUT_OK 0

namespace io {
  int printPoplException(const popl::invalid_option &e);

}
