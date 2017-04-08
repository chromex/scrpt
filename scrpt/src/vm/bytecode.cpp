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
		ENUM_CASE_TO_STRING(OpCode::PlusEq); 
		ENUM_CASE_TO_STRING(OpCode::PlusEqIdx); 
		ENUM_CASE_TO_STRING(OpCode::MinusEq);  
		ENUM_CASE_TO_STRING(OpCode::MinusEqIdx); 
		ENUM_CASE_TO_STRING(OpCode::MultEq); 
		ENUM_CASE_TO_STRING(OpCode::MultEqIdx); 
		ENUM_CASE_TO_STRING(OpCode::DivEq); 
		ENUM_CASE_TO_STRING(OpCode::DivEqIdx); 
		ENUM_CASE_TO_STRING(OpCode::ModuloEq); 
		ENUM_CASE_TO_STRING(OpCode::ModuloEqIdx);
		ENUM_CASE_TO_STRING(OpCode::ConcatEq); 
		ENUM_CASE_TO_STRING(OpCode::ConcatEqIdx);
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

    default:
        AssertFail("Missing case for OpCode");
    }

    return nullptr;
}

void scrpt::Decompile(const Bytecode& bytecode)
{
    std::cout << "[STRINGS]" << std::endl;
    for (unsigned int idx = 0; idx < bytecode.strings.size(); ++idx)
    {
        std::cout << idx << ": " << bytecode.strings[idx] << std::endl;
    }

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

            //float floatVal = *(float *)(data + idx + 1);
            //int intVal = *(int *)(data + idx + 1);
            //unsigned int uintVal = *(unsigned int *)(data + idx + 1);

            switch (op)
            {
            //case OpCode::PushFloat:
            //    std::cout << " " << floatVal;
            //    idx += 4;
            //    break;

            //case OpCode::PushInt:
            //case OpCode::PushIdent:
            //case OpCode::AssignI:
            //case OpCode::AssignIdxI:
            //case OpCode::PlusEqI:
            //case OpCode::PlusEqIdxI:
            //case OpCode::MinusEqI:
            //case OpCode::MinusEqIdxI:
            //case OpCode::MultEqI:
            //case OpCode::MultEqIdxI:
            //case OpCode::DivEqI:
            //case OpCode::DivEqIdxI:
            //case OpCode::ModuloEqI:
            //case OpCode::ModuloEqIdxI:
            //case OpCode::ConcatEqI:
            //case OpCode::ConcatEqIdxI:
            //case OpCode::IncI:
            //case OpCode::DecI:
            //case OpCode::PostIncI:
            //case OpCode::PostDecI:
            //    std::cout << " " << intVal;
            //    idx += 4;
            //    break;

            //case OpCode::PushString:
            //case OpCode::Call:
            //case OpCode::BrT:
            //case OpCode::BrF:
            //case OpCode::Jmp:
            //case OpCode::MakeList:
            //    std::cout << " " << uintVal;
            //    idx += 4;
            //    break;
            //}

            //switch (op)
            //{
            //case OpCode::Call:
            //    std::cout << " ; " << bytecode.functions[uintVal].name;
            //    break;

            //case OpCode::PushIdent:
            //case OpCode::AssignI:
            //case OpCode::AssignIdxI:
            //case OpCode::PlusEqI:
            //case OpCode::PlusEqIdxI:
            //case OpCode::MinusEqI:
            //case OpCode::MinusEqIdxI:
            //case OpCode::MultEqI:
            //case OpCode::MultEqIdxI:
            //case OpCode::DivEqI:
            //case OpCode::DivEqIdxI:
            //case OpCode::ModuloEqI:
            //case OpCode::ModuloEqIdxI:
            //case OpCode::ConcatEqI:
            //case OpCode::ConcatEqIdxI:
            //case OpCode::IncI:
            //case OpCode::DecI:
            //case OpCode::PostIncI:
            //case OpCode::PostDecI:
            //    {
            //        const scrpt::FunctionData& fd = bytecode.functions[currentFunction];
            //        auto local = fd.localLookup.find(intVal);
            //        if (local != fd.localLookup.end())
            //        {
            //            std::cout << " ; " << local->second;
            //        }
            //        break;
            //    }
            //}

            std::cout << std::endl;
        }
    }
}
