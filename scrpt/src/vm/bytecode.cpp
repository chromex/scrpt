#include "../scrpt.h"

#define COMPONENTNAME "Bytecode"

const char* scrpt::OpCodeToString(OpCode code)
{
    switch (code)
    {
        ENUM_CASE_TO_STRING(OpCode::Unknown);
        ENUM_CASE_TO_STRING(OpCode::PushNull);
        ENUM_CASE_TO_STRING(OpCode::PushInt);
        ENUM_CASE_TO_STRING(OpCode::PushFloat);
        ENUM_CASE_TO_STRING(OpCode::PushString);
        ENUM_CASE_TO_STRING(OpCode::PushTrue);
        ENUM_CASE_TO_STRING(OpCode::PushFalse);
        ENUM_CASE_TO_STRING(OpCode::PushIdent);
        ENUM_CASE_TO_STRING(OpCode::Pop);
        ENUM_CASE_TO_STRING(OpCode::Call);
        ENUM_CASE_TO_STRING(OpCode::AssignI);
        ENUM_CASE_TO_STRING(OpCode::PlusEqI);
        ENUM_CASE_TO_STRING(OpCode::MinusEqI);
        ENUM_CASE_TO_STRING(OpCode::MultEqI);
        ENUM_CASE_TO_STRING(OpCode::DivEqI);
        ENUM_CASE_TO_STRING(OpCode::ModuloEqI);
        ENUM_CASE_TO_STRING(OpCode::Eq);
        ENUM_CASE_TO_STRING(OpCode::Or);
        ENUM_CASE_TO_STRING(OpCode::And);
        ENUM_CASE_TO_STRING(OpCode::Add);
        ENUM_CASE_TO_STRING(OpCode::Sub);
        ENUM_CASE_TO_STRING(OpCode::Mul);
        ENUM_CASE_TO_STRING(OpCode::Div);
        ENUM_CASE_TO_STRING(OpCode::Mod);
        ENUM_CASE_TO_STRING(OpCode::LT);
        ENUM_CASE_TO_STRING(OpCode::GT);
        ENUM_CASE_TO_STRING(OpCode::LTE);
        ENUM_CASE_TO_STRING(OpCode::GTE);
        ENUM_CASE_TO_STRING(OpCode::IncI);
        ENUM_CASE_TO_STRING(OpCode::DecI);
        ENUM_CASE_TO_STRING(OpCode::PostIncI);
        ENUM_CASE_TO_STRING(OpCode::PostDecI);
        ENUM_CASE_TO_STRING(OpCode::BrT);
        ENUM_CASE_TO_STRING(OpCode::BrF);
        ENUM_CASE_TO_STRING(OpCode::Jmp);
        ENUM_CASE_TO_STRING(OpCode::Ret);

    default:
        AssertFail("Missing case for OpCode");
    }

    return nullptr;
}
