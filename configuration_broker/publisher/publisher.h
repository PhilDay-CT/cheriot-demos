// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <compartment.h>

void __cheri_compartment("publisher")
  updateConfig(const char *topic, const char *message);
