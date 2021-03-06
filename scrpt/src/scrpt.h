#pragma once

#include <iostream>
#include <sstream>
#include <filesystem>
#include <list>
#include <stdexcept>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <iomanip>
#include <bitset>
namespace fs = std::experimental::filesystem;

namespace scrpt { class VM; class Token; }
#include "compiler/error.h"
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

#define ENUM_CASE_TO_STRING(s) case s: return #s
