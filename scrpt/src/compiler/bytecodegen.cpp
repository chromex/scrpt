#include "../scrpt.h"

#define COMPONENTNAME "BytecodeGen"

namespace scrpt
{
    BytecodeGen::BytecodeGen()
        : _fd(nullptr)
    {
    }

    void BytecodeGen::Consume(const AstNode& ast)
    {
        for (auto node : ast.GetChildren())
        {
            this->RecordFunction(node);
        }

        for (auto node : ast.GetChildren())
        {
            this->CompileFunction(node);
        }
    }

    void BytecodeGen::DumpBytecode()
    {
        _byteBuffer.resize(_byteBuffer.size());
        Bytecode bytecode;
        bytecode.data = std::move(_byteBuffer);
        bytecode.functions = std::move(_functions);
        Decompile(&bytecode);
    }

    void BytecodeGen::RecordFunction(const AstNode& node)
    {
        // Check the node
        this->Verify(node, Symbol::Func);
        auto children = node.GetChildren();
        Assert(children.size() >= 2, "Func node is missing minimum children of ident and block");

        // Get the name
        auto ident = children.front();
        this->Verify(ident, Symbol::Ident);
        std::string name = ident.GetToken()->GetString();

        if (_functionLookup.find(name) != _functionLookup.end())
        {
            CreateBytecodeGenEx(BytecodeGenErr::FunctionRedefinition, ident.GetToken());
        }

        size_t nParam = children.size() - 2;
        if (nParam > 255)
        {
            CreateBytecodeGenEx(BytecodeGenErr::ParamCountExceeded, ident.GetToken());
        }

        _functionLookup[name] = (unsigned int)_functions.size();
        _functions.push_back(FunctionData{ ident.GetToken()->GetString(), (unsigned char)nParam, 0xFFFFFFFF });
    }

    void BytecodeGen::CompileFunction(const AstNode& node)
    {
        auto ident = node.GetFirstChild();

        // Update function table with bytecode offset
        _fd = &_functions[_functionLookup[ident.GetToken()->GetString()]];
        _fd->entry = (unsigned int)_byteBuffer.size();

        // Start function scope
        this->PushScope();

        // Get the params
        auto children = node.GetChildren();
        size_t nParam = children.size() - 2;
        size_t nParamAdded = 0;
        for (auto paramIter = ++children.rbegin(); nParamAdded < nParam; ++paramIter)
        {
            this->Verify(*paramIter, Symbol::Ident);
            int offset = this->AddParam(*paramIter);
            _fd->localLookup[offset] = paramIter->GetToken()->GetString();
            ++nParamAdded;
        }
        
        auto block = node.GetLastChild();
        this->Verify(block, Symbol::LBracket);

        for (auto statement : block.GetChildren())
        {
            this->CompileStatement(statement);
        }

        // Add an implicit return if none there was no explicit one
        if (block.GetLastChild().GetSym() != Symbol::Return)
        {
            this->AddOp(OpCode::PushNull);
            this->AddOp(OpCode::Ret);
        }

        this->PopScope();

        _fd = nullptr;
    }

    void BytecodeGen::CompileStatement(const AstNode& node)
    {
        switch (node.GetSym())
        {
        case Symbol::For:
            this->CompileFor(node);
            break;

        case Symbol::While:
            this->CompileWhile(node);
            break;

        case Symbol::Do:
            this->CompileDo(node);
            break;

        case Symbol::If:
            this->CompileIf(node);
            break;

        case Symbol::Return:
            Assert(node.GetChildren().size() < 2, "Unexpected number of children");
            if (node.GetChildren().size() > 0)
            {
                this->CompileExpression(node.GetFirstChild());
            }
            else
            {
                this->AddOp(OpCode::PushNull);
            }
            this->AddOp(OpCode::Ret);
            break;

        case Symbol::LBracket:
            this->PushScope();
            for (auto child : node.GetChildren())
            {
                this->CompileStatement(child);
            }
            this->PopScope();
            break;

        default:
            if (this->CompileExpression(node))
            {
                this->AddOp(OpCode::Pop);
            }
            else
            {
                AssertFail("Unhandled Symbol in statement compilation: " << SymbolToString(node.GetToken()->GetSym()));
            }
        }
    }

    // TODO: Unary -
    // TODO: Need to special case all assignments when target operand is not an ident (including ++, --)
    bool BytecodeGen::CompileExpression(const AstNode& node)
    {
        bool success = true;
        switch (node.GetSym())
        {
        case Symbol::Int:
            this->AddOp(OpCode::PushInt, node.GetToken()->GetInt());
            break;

        case Symbol::Float:
            this->AddOp(OpCode::PushFloat, node.GetToken()->GetFloat());
            break;

        case Symbol::Ident:
            this->AddOp(OpCode::PushIdent, this->LookupIdentOffset(node));
            break;

        case Symbol::Terminal:
            // TODO: Need string table id
            this->AddOp(OpCode::PushString, unsigned int(0xFFFFFFFF));
            break;

        case Symbol::True:
            this->AddOp(OpCode::PushTrue);
            break;

        case Symbol::False:
            this->AddOp(OpCode::PushFalse);
            break;

        case Symbol::Assign:
        case Symbol::PlusEq:
        case Symbol::MinusEq:
        case Symbol::MultEq:
        case Symbol::DivEq:
        case Symbol::ModuloEq:
            Assert(node.GetFirstChild().GetSym() == Symbol::Ident, "Non ident assignment not yet supported");

            Assert(node.GetChildren().size() == 2, "Unexpected number of children");
            this->CompileExpression(node.GetSecondChild());
            this->AddOp(this->MapUnaryAssignOp(node.GetSym()), this->AddLocal(node.GetFirstChild().GetToken()->GetString()));
            break;

        case Symbol::Eq:
        case Symbol::Or:
        case Symbol::And:
        case Symbol::Plus:
        case Symbol::Minus:
        case Symbol::Mult:
        case Symbol::Div:
        case Symbol::Modulo:
        case Symbol::LessThan:
        case Symbol::LessThanEq:
        case Symbol::GreaterThan:
        case Symbol::GreaterThanEq:
            Assert(node.GetChildren().size() == 2, "Unexpected number of children");
            this->CompileExpression(node.GetFirstChild());
            this->CompileExpression(node.GetSecondChild());
            this->AddOp(this->MapBinaryOp(node.GetSym()));
            break;

        case Symbol::PlusPlus:
            Assert(node.GetFirstChild().GetSym() == Symbol::Ident, "Non ident assignment not yet supported");

            Assert(node.GetChildren().size() == 1, "Unexpected number of children");
            this->AddOp(node.IsPostfix() ? OpCode::PostIncI : OpCode::IncI, this->LookupIdentOffset(node.GetFirstChild()));
            break;

        case Symbol::MinusMinus:
            Assert(node.GetFirstChild().GetSym() == Symbol::Ident, "Non ident assignment not yet supported");

            Assert(node.GetChildren().size() == 1, "Unexpected number of children");
            this->AddOp(node.IsPostfix() ? OpCode::PostDecI : OpCode::DecI, this->LookupIdentOffset(node.GetFirstChild()));
            break;

        case Symbol::LParen:
            if (node.IsPostfix())
            {
                this->CompileCall(node);
            }
            else
            {
                // TODO: tuples?
            }
            break;

        default:
            AssertFail("Unhandled Symbol in expression compilation: " << SymbolToString(node.GetToken()->GetSym()));
            success = false;
        }

        return success;
    }

    void BytecodeGen::CompileFor(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::For, "Unexpected node");
        Assert(node.GetChildren().size() == 4, "Unexpected child count on For node");

        auto beginExpr = node.GetFirstChild();
        auto checkExpr = node.GetSecondChild();
        auto endExpr = node.GetThirdChild();
        auto blockStatement = node.GetLastChild();

        this->PushScope();

        // TODO: Add note to lessons about bytecode variety

        // Begin
        if (!beginExpr.IsEmpty())
        {
            this->CompileExpression(beginExpr);
            this->AddOp(OpCode::Pop);
        }

        // Check
        unsigned int reentry = (unsigned int)_byteBuffer.size();
        size_t brIdx = 0;
        if (!checkExpr.IsEmpty())
        {
            this->CompileExpression(checkExpr);
            brIdx = this->AddOp(OpCode::BrF, unsigned int(0xFFFFFFFF));
        }

        // Block
        this->PushScope();
        this->CompileStatement(blockStatement);
        this->PopScope();

        // End 
        if (!endExpr.IsEmpty())
        {
            this->CompileExpression(endExpr);
        }
        this->AddOp(OpCode::Jmp, reentry);
        if (!checkExpr.IsEmpty())
        {
            this->SetOpOperand(brIdx, (unsigned int)_byteBuffer.size());
        }

        this->PopScope();
    }

    void BytecodeGen::CompileWhile(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::While, "Unexpected node");
        Assert(node.GetChildren().size() == 2, "Unexpected child count on While node");

        auto checkExpr = node.GetFirstChild();
        auto blockStatement = node.GetLastChild();

        this->PushScope();

        unsigned int reentry = (unsigned int)_byteBuffer.size();
        this->CompileExpression(checkExpr);
        size_t brIdx = this->AddOp(OpCode::BrF, unsigned int(0xFFFFFFFF));
        this->PushScope();
        this->CompileStatement(blockStatement);
        this->PopScope();
        this->AddOp(OpCode::Jmp, reentry);
        this->SetOpOperand(brIdx, (unsigned int)_byteBuffer.size());

        this->PopScope();
    }

    void BytecodeGen::CompileDo(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::Do, "Unexpected node");
        Assert(node.GetChildren().size() == 2, "Unexpected child count on Do node");

        this->PushScope();

        unsigned int reentry = (unsigned int)_byteBuffer.size();
        this->PushScope();
        this->CompileStatement(node.GetFirstChild());
        this->PopScope();
        this->CompileExpression(node.GetSecondChild());
        this->AddOp(OpCode::BrT, reentry);

        this->PopScope();
    }

    void BytecodeGen::CompileIf(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::If, "Unexpected node");
        Assert(node.GetChildren().size() >= 2, "Unexpected child count on If node");

        // TODO: ElIf and Else support

        auto checkExpr = node.GetFirstChild();
        auto blockStatement = node.GetSecondChild();

        this->PushScope();

        this->CompileExpression(checkExpr);
        size_t brIdx = this->AddOp(OpCode::BrF, unsigned int(0xFFFFFFFF));
        this->PushScope();
        this->CompileStatement(blockStatement);
        this->PopScope();
        this->SetOpOperand(brIdx, (unsigned int)_byteBuffer.size());

        this->PopScope();
    }

    void BytecodeGen::CompileCall(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::LParen, "Unexpected node");
        Assert(node.GetChildren().size() >= 1, "Unexpected child count on Call node");

        auto ident = node.GetFirstChild();
        this->Verify(ident, Symbol::Ident);
        size_t nParam = node.GetChildren().size() - 1;

        auto funcIter = _functionLookup.find(node.GetFirstChild().GetToken()->GetString());
        if (funcIter == _functionLookup.end())
        {
            throw CreateBytecodeGenEx(BytecodeGenErr::NoSuchFunction, node.GetToken());
        }

        unsigned int funcId = funcIter->second;

        if ((unsigned char)nParam != _functions[funcId].nParam)
        {
            throw CreateBytecodeGenEx(BytecodeGenErr::IncorrectArity, node.GetToken());
        }

        auto children = node.GetChildren();
        for (auto child = ++children.begin(); child != children.end(); ++child)
        {
            this->CompileExpression(*child);
        }
        this->AddOp(OpCode::Call, funcId);
        for (int count = 0; count < nParam; ++count) this->AddOp(OpCode::Pop);
        this->AddOp(OpCode::RestoreRet);
    }

    size_t BytecodeGen::AddOp(OpCode op)
    {
        _byteBuffer.push_back(static_cast<unsigned char>(op));
        return _byteBuffer.size() - 1;
    }

    size_t BytecodeGen::AddOp(OpCode op, int p0)
    {
        return this->AddOp(op, (unsigned char*)&p0);
    }

    size_t BytecodeGen::AddOp(OpCode op, unsigned int p0)
    {
        return this->AddOp(op, (unsigned char*)&p0);
    }

    size_t BytecodeGen::AddOp(OpCode op, float p0)
    {
        return this->AddOp(op, (unsigned char*)&p0);
    }

    size_t BytecodeGen::AddOp(OpCode op, unsigned char* p0)
    {
        size_t ret = _byteBuffer.size();

        _byteBuffer.push_back(static_cast<unsigned char>(op));
        _byteBuffer.push_back(p0[0]);
        _byteBuffer.push_back(p0[1]);
        _byteBuffer.push_back(p0[2]);
        _byteBuffer.push_back(p0[3]);
        Assert(*((unsigned int*)&(_byteBuffer[ret + 1])) == *(unsigned int*)p0, "Data should be correctly packed...");

        return ret;
    }

    void BytecodeGen::SetOpOperand(size_t opIdx, unsigned int p0)
    {
        this->SetOpOperand(opIdx, (unsigned char*)&p0);
    }

    void BytecodeGen::SetOpOperand(size_t opIdx, unsigned char* p0)
    {
        Assert(_byteBuffer.size() > opIdx, "Index out of bounds");

        _byteBuffer[opIdx + 1] = p0[0];
        _byteBuffer[opIdx + 2] = p0[1];
        _byteBuffer[opIdx + 3] = p0[2];
        _byteBuffer[opIdx + 4] = p0[3];
        Assert(*((unsigned int*)&(_byteBuffer[opIdx + 1])) == *(unsigned int*)p0, "Data should be correctly packed...");
    }

    void BytecodeGen::Verify(const AstNode& node, Symbol sym) const
    {
        if (node.GetSym() != sym)
        {
            throw CreateBytecodeGenEx(std::string("Expected token ") + SymbolToString(sym), BytecodeGenErr::UnexpectedToken, node.GetToken());
        }
    }

    OpCode BytecodeGen::MapBinaryOp(Symbol sym) const
    {
        switch (sym)
        {
        case Symbol::Eq: return OpCode::Eq;
        case Symbol::Or: return OpCode::Or;
        case Symbol::And: return OpCode::Add;
        case Symbol::Plus: return OpCode::Add;
        case Symbol::Minus: return OpCode::Sub;
        case Symbol::Mult: return OpCode::Mul;
        case Symbol::Div: return OpCode::Div;
        case Symbol::Modulo: return OpCode::Mod;
        case Symbol::LessThan: return OpCode::LT;
        case Symbol::LessThanEq: return OpCode::LTE;
        case Symbol::GreaterThan: return OpCode::GT;
        case Symbol::GreaterThanEq: return OpCode::GTE;
        default:
            AssertFail("Unmapped binary op: " << SymbolToString(sym));
            return OpCode::Unknown;
        }
    }

    OpCode BytecodeGen::MapUnaryAssignOp(Symbol sym) const
    {
        switch (sym)
        {
        case Symbol::Assign: return OpCode::AssignI;
        case Symbol::PlusEq: return OpCode::PlusEqI;
        case Symbol::MinusEq: return OpCode::MinusEqI;
        case Symbol::MultEq: return OpCode::MultEqI;
        case Symbol::DivEq: return OpCode::DivEqI;
        case Symbol::ModuloEq: return OpCode::ModuloEqI;
        default:
            AssertFail("Unmapped unary assign op: " << SymbolToString(sym));
            return OpCode::Unknown;
        }
    }

    void BytecodeGen::PushScope()
    {
        _scopeStack.push_back(std::map<std::string, int>());
        if (_scopeStack.size() == 1)
        {
            _paramOffset = -1;
            _localOffset = 0;
        }
    }

    void BytecodeGen::PopScope()
    {
        Assert(_scopeStack.size() > 0, "Can't pop an empty stack");
        _scopeStack.pop_back();
    }

    int BytecodeGen::AddParam(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::Ident, "Unexpected symbol");
        Assert(_scopeStack.size() == 1, "Params can only be added at root scope");

        int offset;
        const char* ident = node.GetToken()->GetString();
        if (this->LookupIdentOffset(ident, &offset))
        {
            throw CreateBytecodeGenEx(BytecodeGenErr::DuplicateParameter, node.GetToken());
        }

        offset = _paramOffset--;
        _scopeStack.back()[ident] = offset;
        return offset;
    }

    int BytecodeGen::AddLocal(const char* ident)
    {
        AssertNotNull(ident);

        int offset;
        if (!this->LookupIdentOffset(ident, &offset))
        {
            offset = _localOffset++;
            _scopeStack.back()[ident] = offset; 
            _fd->localLookup[offset] = ident;
        }

        return offset;
    }

    int BytecodeGen::LookupIdentOffset(const AstNode& node) const
    {
        Assert(node.GetSym() == Symbol::Ident, "Unexpected symbol");

        int offset;
        if (!this->LookupIdentOffset(node.GetToken()->GetString(), &offset))
        {
            throw CreateBytecodeGenEx(BytecodeGenErr::NoSuchIdent, node.GetToken());
        }

        return offset;
    }

    bool BytecodeGen::LookupIdentOffset(const char* ident, int* id) const
    {
        AssertNotNull(ident);
        AssertNotNull(id);

        size_t idx = _scopeStack.size();
        if (idx == 0) return false;

        do
        {
            --idx;
            auto entry = _scopeStack[idx].find(ident);
            if (entry != _scopeStack[idx].end())
            {
                *id = entry->second;
                return true;
            }
        } while (idx > 0);

        return false;
    }

    const char* BytecodeGenErrToString(BytecodeGenErr err)
    {
        switch (err)
        {
            ENUM_CASE_TO_STRING(BytecodeGenErr::NoError);
            ENUM_CASE_TO_STRING(BytecodeGenErr::UnexpectedToken);
            ENUM_CASE_TO_STRING(BytecodeGenErr::FunctionRedefinition);
            ENUM_CASE_TO_STRING(BytecodeGenErr::ParamCountExceeded);
            ENUM_CASE_TO_STRING(BytecodeGenErr::NoSuchFunction);
            ENUM_CASE_TO_STRING(BytecodeGenErr::IncorrectArity);
            ENUM_CASE_TO_STRING(BytecodeGenErr::NoSuchIdent);
            ENUM_CASE_TO_STRING(BytecodeGenErr::DuplicateParameter);

        default:
            AssertFail("Missing case for BytecodeGenErr");
        }

        return nullptr;
    }
}
