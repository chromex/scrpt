#pragma once

namespace scrpt
{
    enum OpCode
    {
        Push,
        Pop,
        Load,
        Call,
        Assign,
        Unknown,
    };
    const char* OpCodeToString(OpCode code);

    class Bytecode
    {
    public:

    private:

    };
}