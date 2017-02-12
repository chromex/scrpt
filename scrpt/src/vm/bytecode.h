#pragma once

namespace scrpt
{
    enum class OpCode : unsigned char
    {
        Unknown,
        PushNull, 
        PushTrue, 
        PushFalse,
        Pop, 
        Eq,
        Or,
        And,
        Add,
        Sub,
        Mul,
        Div,
        Mod,
        LT,
        GT,
        LTE,
        GTE,
        Ret,
        RestoreRet,
        PushInt, // int val
        PushFloat, // float val
        PushString, // unsigned int string id
        PushIdent, // int ident offset
        Call, // unsigned int function offset
        AssignI, // int ident offset
        PlusEqI, // int ident offset
        MinusEqI, // int ident offset
        MultEqI, // int ident offset
        DivEqI, // int ident offset
        ModuloEqI, // int ident offset
        IncI, // int ident offset
        DecI, // int ident offset
        PostIncI, // int ident offset
        PostDecI, // int ident offset
        BrT, // unsigned int bytecode location
        BrF, // unsigned int bytecode location
        Jmp, // unsigned int bytecode locaiton
        __Num,
    };
    const char* OpCodeToString(OpCode code);

    struct FunctionData
    {
        std::string name;
        unsigned char nParam;
        unsigned int entry;
        std::map<int, std::string> localLookup;
    };

    struct Bytecode
    {
        std::vector<unsigned char> data;
        std::vector<FunctionData> functions;
        // string table
    };

    void Decompile(const Bytecode& bytecode);
}