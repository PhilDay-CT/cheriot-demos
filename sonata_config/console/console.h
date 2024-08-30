// Copyright Configured Things Ltd and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <compartment.h>
#include <stdint.h>

#include "../sonata_lcd/lcd.hh"

namespace console {

struct Config
{
	char header[30];
};

/**
 * A simple console interaface to the
 * Sonata LCD 
 */
int __cheri_compartment("console")
  header(const char *header);

int __cheri_compartment("console")
  print(const char *line);

int __cheri_compartment("console")
  error(const char *line, const char *error);

} // console
