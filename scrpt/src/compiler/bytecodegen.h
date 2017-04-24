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
        std::tuple<bool, char> CompileExpression(const AstNode& node);
        void CompileFor(const AstNode& node);
        void CompileWhile(const AstNode& node);
        void CompileDo(const AstNode& node);
        void CompileIf(const AstNode& node);
        void CompileReturn(const AstNode& node);
        char CompileCall(const AstNode& node);
        char CompileList(const AstNode& node);
        char CompileMap(const AstNode& node);
        size_t AddOp(OpCode op);
        size_t AddOp(OpCode op, char reg0);
        size_t AddOp(OpCode op, char reg0, char reg1);
        size_t AddOp(OpCode op, char reg0, char reg1, char reg2);
        size_t AddOp(OpCode op, char reg0, unsigned int data);
        size_t AddOp(OpCode op, char reg0, float data);
        size_t AddOp(OpCode op, char reg0, int data);
        size_t AddOp(OpCode op, unsigned int data);
        void AddData(unsigned char* data);
        void SetOpOperand(size_t opIdx, int offset, unsigned int p0);
        void SetOpOperand(size_t opIdx, int offset, unsigned char* p0);
        inline void Verify(const AstNode& node, Symbol sym) const;
        OpCode MapBinaryOp(Symbol sym) const;
        OpCode MapUnaryAssignOp(Symbol sym) const;
        OpCode MapUnaryAssignIdxOp(Symbol sym) const;

        void PushScope();
        void PopScope();
        char ClaimRegister(const AstNode& node, bool lock = false);
        void ReleaseRegister(char reg, bool unlock = false);
        static char GetRegResult(const std::tuple<bool, char>& result);
        char AddParam(const AstNode& node);
        char AddLocal(const AstNode& node);
        bool LookupIdentOffset(const char* ident, char* id) const;
        char LookupIdentOffset(const AstNode& node) const;
        unsigned int GetStringId(const char* str);

        std::vector<unsigned char> _byteBuffer;
        std::vector<FunctionData> _functions;
        std::map<std::string, unsigned int> _functionLookup;
        std::vector<std::string> _strings;
        std::map<std::string, unsigned int> _stringLookup;
        std::vector< std::map<std::string, char> > _scopeStack;
        std::bitset<128> _registers;
        std::bitset<128> _lockedRegisters;
        int _paramOffset;
        FunctionData* _fd;
    };

    enum class BytecodeGenErr
    {
        NoError,
        UnexpectedToken,
        FunctionRedefinition,
        ParameterCountExceeded,
        UndeclaredFunctionReference,
        IncorrectCallArity,
        UndeclaredIdentifierReference,
        DuplicateParameterName,
        InsufficientRegisters,
    };
    const char* BytecodeGenErrToString(BytecodeGenErr err);
}
