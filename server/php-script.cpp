// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/php-script.h"

#include <cassert>
#include <cstring>
#include <map>
#include <string>

#include "common/dl-utils-lite.h"

static std::map<std::string, script_t *> scripts;

script_t *get_script(const char *name) {
  auto i = scripts.find(name);

  if (i != scripts.end()) {
    return i->second;
  }

  return nullptr;
}

void set_script(const char *name, void (*run)(), void (*clear)()) {
  static int cnt = 0;

  auto script = new script_t{run, clear};
  bool inserted __attribute__((unused)) = scripts.insert({name, script}).second;
  assert(inserted);

  inserted = scripts.insert({"#" + std::to_string(cnt++), script}).second;
  assert(inserted);
}
