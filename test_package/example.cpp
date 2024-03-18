#include "Modbus_Technology_Adapter/ModbusTechnologyAdapter.hpp"

void use_library(bool actually_run) {
  if (actually_run) {
    Technology_Adapter::ModbusTechnologyAdapter adapter("path to config file");
  }
}

int main(int /*argc*/, char const* /*argv*/[]) {
  use_library(false);
}
