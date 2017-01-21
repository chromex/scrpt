#include "../scrpt.h"

#define COMPONENTNAME "Bytecode"

const char* scrpt::OpCodeToString(OpCode code)
{
    switch (code)
    {
        ENUM_CASE_TO_STRING(OpCode::Unknown);

    default:
        AssertFail("Missing case for OpCode");
    }

    return nullptr;
}
