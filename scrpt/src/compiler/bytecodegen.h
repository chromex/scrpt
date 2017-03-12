#pragma once

namespace scrpt
{
    class BytecodeGen
    {
    public:
        BytecodeGen();

        void AddExternFunc(const char* name, unsigned char nParam, const std::function<void(VM*)>& func);
        void Consume(const AstNode& ast);
        Bytecode GetBytecode();

    private:
        void RecordFunction(const AstNode& node);
        void CompileFunction(const AstNode& node);
        void CompileStatement(const AstNode& node);
        bool CompileExpression(const AstNode& node);
        void CompileFor(const AstNode& node);
        void CompileWhile(const AstNode& node);
        void CompileDo(const AstNode& node);
        void CompileIf(const AstNode& node);
        void CompileCall(const AstNode& node);
        void CompileList(const AstNode& node);
        size_t AddOp(OpCode op);
        size_t AddOp(OpCode op, int p0);
        size_t AddOp(OpCode op, unsigned int p0);
        size_t AddOp(OpCode op, float p0);
        size_t AddOp(OpCode op, unsigned char* p0);
        void SetOpOperand(size_t opIdx, unsigned int p0);
        void SetOpOperand(size_t opIdx, unsigned char* p0);
        inline void Verify(const AstNode& node, Symbol sym) const;
        OpCode MapBinaryOp(Symbol sym) const;
        OpCode MapUnaryAssignOp(Symbol sym) const;

        void PushScope();
        void PopScope();
        int AddParam(const AstNode& node);
        int AddLocal(const char* ident);
        bool LookupIdentOffset(const char* ident, int* id) const;
        int LookupIdentOffset(const AstNode& node) const;

        std::vector<unsigned char> _byteBuffer;
        std::vector<FunctionData> _functions;
        std::map<std::string, unsigned int> _functionLookup;
        std::vector<std::string> _strings;
        std::map<std::string, unsigned int> _stringLookup;
        std::vector< std::map<std::string, int> > _scopeStack;
        int _paramOffset;
        int _localOffset;
        FunctionData* _fd;
    };

    enum class BytecodeGenErr
    {
        NoError,
        UnexpectedToken,
        FunctionRedefinition,
        ParamCountExceeded,
        NoSuchFunction,
        IncorrectArity,
        NoSuchIdent,
        DuplicateParameter,
    };
    const char* BytecodeGenErrToString(BytecodeGenErr err);
}
