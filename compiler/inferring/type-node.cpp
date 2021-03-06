// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/type-node.h"

#include <sstream>

#include "compiler/inferring/type-data.h"
#include "compiler/stage.h"

std::string tinf::TypeNode::get_location_text() {
  return stage::to_str(location_);
}

std::string tinf::TypeNode::get_description() {
  std::stringstream ss;
  ss << "as type:" << "  " << "(casted to) " << colored_type_out(type_) << "  " << "at " + get_location_text();
  return ss.str();
}
