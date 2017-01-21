#pragma once

namespace scrpt
{
    enum class OpCode : unsigned char
    {
        Unknown,
        PushNull, 
        PushInt, // val
        PushFloat, // val
        PushString, // string id
        PushTrue, 
        PushFalse,
        PushIdent, // ident offset
        Pop, 
        Call, // function offset, n params
        AssignI, // ident offset
        PlusEqI, // ident offset
        MinusEqI, // ident offset
        MultEqI, // ident offset
        DivEqI, // ident offset
        ModuloEqI, // ident offset
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
        IncI,
        DecI,
        PostIncI,
        PostDecI,
        BrT, // bytecode location
        BrF, // bytecode location
        Jmp, // bytecode locaiton
        Ret,
    };
    const char* OpCodeToString(OpCode code);

    class Bytecode
    {
    public:

    private:

    };
}