#pragma once

#include <iostream>
#include <sstream>
#include <filesystem>
#include <list>
#include <stdexcept>
namespace fs = std::experimental::filesystem;

#include "util/trace.h"
#include "util/fileio.h"
#include "compiler/lexer.h"
#include "compiler/ast.h"
#include "compiler/parser.h"
#include "compiler/error.h"
#include "compiler/bytecodegen.h"
#include "vm/bytecode.h"

#define ENUM_CASE_TO_STRING(s) case s: return #s
