#include "../scrpt.h"

#define COMPONENTNAME "Bytecode"

const char* scrpt::OpCodeToString(OpCode code)
{
    switch (code)
    {
        ENUM_CASE_TO_STRING(OpCode::Unknown);
        ENUM_CASE_TO_STRING(OpCode::LoadNull);
        ENUM_CASE_TO_STRING(OpCode::LoadTrue);
        ENUM_CASE_TO_STRING(OpCode::LoadFalse);
        ENUM_CASE_TO_STRING(OpCode::LoadInt);
        ENUM_CASE_TO_STRING(OpCode::LoadFloat);
        ENUM_CASE_TO_STRING(OpCode::LoadString);
        ENUM_CASE_TO_STRING(OpCode::LoadFunc);
        ENUM_CASE_TO_STRING(OpCode::Store);
        ENUM_CASE_TO_STRING(OpCode::StoreIdx);
        ENUM_CASE_TO_STRING(OpCode::Eq);
        ENUM_CASE_TO_STRING(OpCode::Or);
        ENUM_CASE_TO_STRING(OpCode::And);
        ENUM_CASE_TO_STRING(OpCode::Add);
        ENUM_CASE_TO_STRING(OpCode::Sub);
        ENUM_CASE_TO_STRING(OpCode::Mul);
        ENUM_CASE_TO_STRING(OpCode::Div);
        ENUM_CASE_TO_STRING(OpCode::Mod);
        ENUM_CASE_TO_STRING(OpCode::Concat);
        ENUM_CASE_TO_STRING(OpCode::Inc);
        ENUM_CASE_TO_STRING(OpCode::Dec);
        ENUM_CASE_TO_STRING(OpCode::PostInc);
        ENUM_CASE_TO_STRING(OpCode::PostDec);
        ENUM_CASE_TO_STRING(OpCode::Neg);
        ENUM_CASE_TO_STRING(OpCode::LT);
        ENUM_CASE_TO_STRING(OpCode::GT);
        ENUM_CASE_TO_STRING(OpCode::LTE);
        ENUM_CASE_TO_STRING(OpCode::GTE);
        ENUM_CASE_TO_STRING(OpCode::Call);
        ENUM_CASE_TO_STRING(OpCode::Ret);
        ENUM_CASE_TO_STRING(OpCode::RestoreRet);
        ENUM_CASE_TO_STRING(OpCode::BrT);
        ENUM_CASE_TO_STRING(OpCode::BrF);
        ENUM_CASE_TO_STRING(OpCode::Jmp);
        ENUM_CASE_TO_STRING(OpCode::Index);
        ENUM_CASE_TO_STRING(OpCode::MakeList);
        ENUM_CASE_TO_STRING(OpCode::MakeMap);
        ENUM_CASE_TO_STRING(OpCode::Push);
        ENUM_CASE_TO_STRING(OpCode::PopN);

    default:
        AssertFail("Missing case for OpCode");
    }

    return nullptr;
}

std::string DisplayRegisterName(const scrpt::FunctionData& fd, char reg)
{
    std::ostringstream os;
    auto entry = fd.localLookup.find(reg);
    if (entry != fd.localLookup.end())
    {
        os << entry->second << ":" << (int)reg;
    }
    else
    {
        os << (int)reg;
    }

    return os.str();
}

void scrpt::Decompile(const Bytecode& bytecode, std::ostream& os)
{
    os << "[STRINGS]" << std::endl;
    for (unsigned int idx = 0; idx < bytecode.strings.size(); ++idx)
    {
        os << idx << ": " << bytecode.strings[idx] << std::endl;
    }

#define DISPLAY_FUNC(Func) { os << "Func: " << Func.name << " nParam: " << (int)Func.nParam; if (Func.external) os << " external" << std::endl; else os << " entry: " << Func.entry << std::endl; }

    std::map<unsigned int, const FunctionData*> functionEntryMap;

    os << "[FUNCTIONS]" << std::endl;
    for (const FunctionData& func : bytecode.functions)
    {
        DISPLAY_FUNC(func);
        functionEntryMap[func.entry] = &func;
    }

    os << "[CLASSES]" << std::endl;
    for (const ClassData& cd : bytecode.classes)
    {
        os << "Class: " << cd.name << " members: " << cd.nMembers << std::endl;

        for (const FunctionData& func : cd.methods)
        {
            DISPLAY_FUNC(func);
            functionEntryMap[func.entry] = &func;
        }

        for (std::pair<unsigned int, const FunctionData&> ctorPair : cd.ctors)
        {
            const FunctionData& func = ctorPair.second;
            DISPLAY_FUNC(func);
            functionEntryMap[func.entry] = &func;
        }
    }

    os << "[BYTECODE]" << std::endl;
    os << std::setfill('0') << std::setw(4);
    if (bytecode.data.size() > 0)
    {
        const unsigned char* data = &bytecode.data[0];
        const FunctionData* currentFunction = nullptr;
        for (unsigned int idx = 0; idx < bytecode.data.size(); ++idx)
        {
            auto entry = functionEntryMap.find(idx);
            if (entry != functionEntryMap.end())
            {
                currentFunction = entry->second;
                os << "; begin " << currentFunction->name << std::endl;
            }

            os << idx << " ";

            if (bytecode.data[idx] >= (unsigned char)OpCode::__Num)
            {
                os << "<< invalid opcode >>" << std::endl;
                continue;
            }

            OpCode op = (OpCode)(bytecode.data[idx]);

            os << std::string(OpCodeToString(op)).substr(8);

            char reg0 = *(char*)(data + idx + 1);
            char reg1 = *(char*)(data + idx + 2);
            char reg2 = *(char*)(data + idx + 3);

#define DISPLAY_ONE_REG os << " " << DisplayRegisterName(*currentFunction, reg0); idx += 1;
#define DISPLAY_TWO_REG os << " " << DisplayRegisterName(*currentFunction, reg0) << " " << DisplayRegisterName(*currentFunction, reg1); idx += 2;
#define DISPLAY_THREE_REG os << " " << DisplayRegisterName(*currentFunction, reg0) << " " << DisplayRegisterName(*currentFunction, reg1) << " " << DisplayRegisterName(*currentFunction, reg2); idx += 3;
#define DISPLAY_ONE_REG_UINT os << " " << DisplayRegisterName(*currentFunction, reg0) << " " << *(unsigned int *)(data + idx + 2); idx += 5;
#define DISPLAY_ONE_REG_INT os << " " << DisplayRegisterName(*currentFunction, reg0) << " " << *(int *)(data + idx + 2); idx += 5;
#define DISPLAY_ONE_REG_FLOAT os << " " << DisplayRegisterName(*currentFunction, reg0) << " " << *(float *)(data + idx + 2); idx += 5;
#define DISPLAY_ONE_REG_CHAR os << " " << DisplayRegisterName(*currentFunction, reg0) << " " << (int)*(char *)(data + idx + 2); idx += 2;
#define DISPLAY_UINT os << " " << *(unsigned int *)(data + idx + 1); idx += 4;
#define DISPLAY_CHAR os << " " << (int)reg0; idx += 1; 

            switch (op)
            {
            case OpCode::LoadNull: DISPLAY_ONE_REG; break;
            case OpCode::LoadTrue: DISPLAY_ONE_REG; break;
            case OpCode::LoadFalse: DISPLAY_ONE_REG; break;
            case OpCode::LoadInt: DISPLAY_ONE_REG_INT; break;
            case OpCode::LoadFloat: DISPLAY_ONE_REG_FLOAT; break;
            case OpCode::LoadString: DISPLAY_ONE_REG_UINT; break;
            case OpCode::LoadFunc:
                DISPLAY_ONE_REG_UINT;
                os << " ; " << bytecode.functions[*(unsigned int *)(data + idx - 3)].name;
                break;
            case OpCode::Store: DISPLAY_TWO_REG; break;
            case OpCode::StoreIdx: DISPLAY_THREE_REG; break;
            case OpCode::Eq: DISPLAY_THREE_REG; break;
            case OpCode::Or: DISPLAY_THREE_REG; break;
            case OpCode::And: DISPLAY_THREE_REG; break;
            case OpCode::Add: DISPLAY_THREE_REG; break;
            case OpCode::Sub: DISPLAY_THREE_REG; break;
            case OpCode::Mul: DISPLAY_THREE_REG; break;
            case OpCode::Div: DISPLAY_THREE_REG; break;
            case OpCode::Mod: DISPLAY_THREE_REG; break;
            case OpCode::Concat: DISPLAY_THREE_REG; break;
            case OpCode::Inc: DISPLAY_ONE_REG; break;
            case OpCode::Dec: DISPLAY_ONE_REG; break;
            case OpCode::PostInc: DISPLAY_ONE_REG; break;
            case OpCode::PostDec: DISPLAY_ONE_REG; break;
            case OpCode::Neg: DISPLAY_TWO_REG; break;
            case OpCode::LT: DISPLAY_THREE_REG; break;
            case OpCode::GT: DISPLAY_THREE_REG; break;
            case OpCode::LTE: DISPLAY_THREE_REG; break;
            case OpCode::GTE: DISPLAY_THREE_REG; break;
            case OpCode::Ret: DISPLAY_ONE_REG; break;
            case OpCode::RestoreRet: DISPLAY_ONE_REG; break;
            case OpCode::BrT: DISPLAY_ONE_REG_UINT; break;
            case OpCode::BrF: DISPLAY_ONE_REG_UINT; break;
            case OpCode::Jmp: DISPLAY_UINT; break;
            case OpCode::Index: DISPLAY_THREE_REG; break;
            case OpCode::MakeList: DISPLAY_ONE_REG_UINT; break;
            case OpCode::MakeMap: DISPLAY_ONE_REG_UINT; break;
            case OpCode::Call: DISPLAY_ONE_REG_CHAR; break;
            case OpCode::Push: DISPLAY_ONE_REG; break;
            case OpCode::PopN: DISPLAY_CHAR; break;
            }

            os << std::endl;
        }
    }
}
