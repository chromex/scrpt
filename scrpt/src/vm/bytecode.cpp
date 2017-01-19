#include "../scrpt.h"

#define COMPONENTNAME "Bytecode"

const char* scrpt::BytecodeToString(Bytecode code)
{
    switch (code)
    {
        ENUM_CASE_TO_STRING(Bytecode::Unknown);

    default:
        AssertFail("Missing case for Bytecode");
    }

    return nullptr;
}
