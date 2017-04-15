#pragma once

#include <iostream>
#include <sstream>
#include <filesystem>
#include <list>
#include <stdexcept>
#include <vector>
#include <map>
#include <functional>
#include <iomanip>
#include <bitset>
namespace fs = std::experimental::filesystem;

namespace scrpt { class VM; }
#include "vm/stack.h"
#include "util/trace.h"
#include "util/fileio.h"
#include "compiler/lexer.h"
#include "compiler/ast.h"
#include "compiler/parser.h"
#include "vm/bytecode.h"
#include "compiler/bytecodegen.h"
#include "vm/vm.h"
#include "vm/stdlib.h"

#include "compiler/error.h"

#define ENUM_CASE_TO_STRING(s) case s: return #s
