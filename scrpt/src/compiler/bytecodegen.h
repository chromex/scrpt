#pragma once

namespace scrpt
{
    class BytecodeGen
    {
    public:
        BytecodeGen();

        void Consume(AstNode* ast);
        void DumpBytecode();
    };
}