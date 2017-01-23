#pragma once

namespace scrpt
{
    enum class OpCode : unsigned char
    {
        Unknown,
        PushNull, 
        PushInt, // int val
        PushFloat, // float val
        PushString, // unsigned int string id
        PushTrue, 
        PushFalse,
        PushIdent, // int ident offset
        Pop, 
        Call, // unsigned int function offset
        AssignI, // int ident offset
        PlusEqI, // int ident offset
        MinusEqI, // int ident offset
        MultEqI, // int ident offset
        DivEqI, // int ident offset
        ModuloEqI, // int ident offset
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
        IncI, // int ident offset
        DecI, // int ident offset
        PostIncI, // int ident offset
        PostDecI, // int ident offset
        BrT, // unsigned int bytecode location
        BrF, // unsigned int bytecode location
        Jmp, // unsigned int bytecode locaiton
        Ret,
        __Num,
    };
    const char* OpCodeToString(OpCode code);

    struct FunctionData
    {
        std::string name;
        unsigned char nParam;
        unsigned int entry;
    };

    struct Bytecode
    {
        std::vector<unsigned char> data;
        std::vector<FunctionData> functions;
        // string table
    };

    void Decompile(Bytecode* bytecode);
}