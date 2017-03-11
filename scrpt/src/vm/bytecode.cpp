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
        ENUM_CASE_TO_STRING(OpCode::RestoreRet);

    default:
        AssertFail("Missing case for OpCode");
    }

    return nullptr;
}

void scrpt::Decompile(const Bytecode& bytecode)
{
    // TODO: Dump string table

    std::cout << "[FUNCTIONS]" << std::endl;
    std::map<unsigned int, unsigned int> functionEntryMap;
    for (unsigned int idx = 0; idx < bytecode.functions.size(); ++idx)
    {
        const FunctionData& func = bytecode.functions[idx];
        std::cout << func.name << "/" << (int)func.nParam;
        if (func.external)
            std::cout << " external" << std::endl;
        else
            std::cout << " entry: " << func.entry << std::endl;
        functionEntryMap[func.entry] = idx;
    }

    std::cout << "[BYTECODE]" << std::endl;
    if (bytecode.data.size() > 0)
    {
        const unsigned char* data = &bytecode.data[0];
        unsigned int currentFunction = -1;
        for (unsigned int idx = 0; idx < bytecode.data.size(); ++idx)
        {
            auto entry = functionEntryMap.find(idx);
            if (entry != functionEntryMap.end())
            {
                currentFunction = entry->second;
                std::cout << "; begin " << bytecode.functions[entry->second].name << std::endl;
            }

            std::cout << std::setfill('0') << std::setw(4) << idx << " ";

            if (bytecode.data[idx] >= (unsigned char)OpCode::__Num)
            {
                std::cout << "<< invalid opcode >>" << std::endl;
                continue;
            }

            OpCode op = (OpCode)(bytecode.data[idx]);

            std::cout << std::string(OpCodeToString(op)).substr(8);

            float floatVal = *(float *)(data + idx + 1);
            int intVal = *(int *)(data + idx + 1);
            unsigned int uintVal = *(unsigned int *)(data + idx + 1);

            switch (op)
            {
            case OpCode::PushFloat:
                std::cout << " " << floatVal;
                idx += 4;
                break;

            case OpCode::PushInt:
            case OpCode::PushIdent:
            case OpCode::AssignI:
            case OpCode::PlusEqI:
            case OpCode::MinusEqI:
            case OpCode::MultEqI:
            case OpCode::DivEqI:
            case OpCode::ModuloEqI:
            case OpCode::IncI:
            case OpCode::DecI:
            case OpCode::PostIncI:
            case OpCode::PostDecI:
                std::cout << " " << intVal;
                idx += 4;
                break;

            case OpCode::PushString:
            case OpCode::Call:
            case OpCode::BrT:
            case OpCode::BrF:
            case OpCode::Jmp:
                std::cout << " " << uintVal;
                idx += 4;
                break;
            }

            switch (op)
            {
            case OpCode::Call:
                std::cout << " ; " << bytecode.functions[uintVal].name;
                break;

            case OpCode::PushIdent:
            case OpCode::AssignI:
            case OpCode::PlusEqI:
            case OpCode::MinusEqI:
            case OpCode::MultEqI:
            case OpCode::DivEqI:
            case OpCode::ModuloEqI:
            case OpCode::IncI:
            case OpCode::DecI:
            case OpCode::PostIncI:
            case OpCode::PostDecI:
                {
                    const scrpt::FunctionData& fd = bytecode.functions[currentFunction];
                    auto local = fd.localLookup.find(intVal);
                    if (local != fd.localLookup.end())
                    {
                        std::cout << " ; " << local->second;
                    }
                    break;
                }
            }

            std::cout << std::endl;
        }
    }
}
