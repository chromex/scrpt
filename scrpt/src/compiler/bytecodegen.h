#pragma once

namespace scrpt
{
    class BytecodeGen
    {
    public:
        BytecodeGen();

        void Consume(const AstNode& ast);
        void DumpBytecode();

        // Add string
        // Add func: name, n args
        // Add instr

    private:
        void CompileFunction(const AstNode& node);
        void CompileStatement(const AstNode& node);
        bool CompileExpression(const AstNode& node);
        void CompileFor(const AstNode& node);
        void CompileWhile(const AstNode& node);
        void CompileIf(const AstNode& node);
        void CompileCall(const AstNode& node);
        inline void Verify(const AstNode& node, Symbol sym) const;
    };

    enum class BytecodeGenErr
    {
        NoError,
        UnexpectedToken,
    };
    const char* BytecodeGenErrToString(BytecodeGenErr err);
}
