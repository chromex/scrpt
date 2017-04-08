#include "../scrpt.h"

#define COMPONENTNAME "BytecodeGen"

// Register machine conversion...
// * Number of registers required is:
//   * Parameters + locals
//   * Keep a list and mark free / used as it goes
// * Parameters are still push on the stack + register space is reserved on top of the stack
// * Registers for non param / ident values are nulled out during runtime
// * Local registers are cleaned up on return
// * Bytecode decompiler needs to be updated
// * CompileExpression returns the storage register it used

namespace scrpt
{
    BytecodeGen::BytecodeGen()
        : _fd(nullptr)
    {
    }

    void BytecodeGen::AddExternFunc(const char* name, unsigned char nParam, const std::function<void(VM*)>& func)
    {
        AssertNotNull(name);
        Assert(_functionLookup.find(name) == _functionLookup.end(), "Extern already registered");

        _functionLookup[name] = (unsigned int)_functions.size();
        _functions.push_back(FunctionData{ name, nParam, 0xFFFFFFFF, true, func });
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

    Bytecode BytecodeGen::GetBytecode()
    {
        _byteBuffer.resize(_byteBuffer.size());
        Bytecode bytecode;
        bytecode.data = std::move(_byteBuffer);
        bytecode.functions = std::move(_functions);
        bytecode.strings = std::move(_strings);
        return bytecode;
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
            CreateBytecodeGenEx(BytecodeGenErr::ParameterCountExceeded, ident.GetToken());
        }

        _functionLookup[name] = (unsigned int)_functions.size();
        _functions.push_back(FunctionData{ ident.GetToken()->GetString(), (unsigned char)nParam, 0xFFFFFFFF, false });
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

        // Add an implicit return if there was no explicit one
        if (block.GetLastChild().GetSym() != Symbol::Return)
        {
            this->AddOp(OpCode::LoadNull);
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
                this->AddOp(OpCode::LoadNull);
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
                // TODO: Clear the returned register?
                AssertFail("nop");
                //this->AddOp(OpCode::Pop);
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
            this->AddOp(OpCode::LoadInt, node.GetToken()->GetInt());
            break;

        case Symbol::Float:
            this->AddOp(OpCode::LoadFloat, node.GetToken()->GetFloat());
            break;

        case Symbol::Ident:
            // TODO: Just return the ident register id?
            AssertFail("nop");
            //this->AddOp(OpCode::LoadIdent, this->LookupIdentOffset(node));
            break;

        case Symbol::Terminal:
            {
                unsigned int strId = 0;
                const char* str = node.GetToken()->GetString();
                auto entry = _stringLookup.find(str);
                if (entry == _stringLookup.end())
                {
                    strId = _stringLookup[str] = (unsigned int)_strings.size();
                    _strings.push_back(str);
                }
                else
                {
                    strId = entry->second;
                }

                this->AddOp(OpCode::LoadString, strId);
            }
            break;

        case Symbol::True:
            this->AddOp(OpCode::LoadTrue);
            break;

        case Symbol::False:
            this->AddOp(OpCode::LoadFalse);
            break;

        case Symbol::Assign:
        case Symbol::PlusEq:
        case Symbol::MinusEq:
        case Symbol::MultEq:
        case Symbol::DivEq:
        case Symbol::ModuloEq:
        case Symbol::ConcatEq:
            {
                Assert(node.GetChildren().size() == 2, "Unexpected number of children");
                const AstNode& firstChild = node.GetFirstChild();
                if (firstChild.GetSym() == Symbol::Ident)
                {
                    this->CompileExpression(node.GetSecondChild());
                    this->AddOp(this->MapUnaryAssignOp(node.GetSym()), this->AddLocal(firstChild.GetToken()->GetString()));
                }
                else if (firstChild.GetSym() == Symbol::LSquare)
                {
                    this->CompileExpression(node.GetSecondChild());
                    this->CompileExpression(firstChild.GetSecondChild());
                    this->AddOp(this->MapUnaryAssignIdxOp(node.GetSym()), this->AddLocal(firstChild.GetFirstChild().GetToken()->GetString()));
                }
                else
                {
                    Assert(node.GetFirstChild().GetSym() == Symbol::Ident, "Unknown assignment LHS");
                }
            }
            break;

        case Symbol::Eq:
        case Symbol::Or:
        case Symbol::And:
        case Symbol::Plus:
        case Symbol::Minus:
        case Symbol::Mult:
        case Symbol::Div:
        case Symbol::Modulo:
        case Symbol::Concat:
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
            this->AddOp(node.IsPostfix() ? OpCode::PostInc : OpCode::Inc, this->LookupIdentOffset(node.GetFirstChild()));
            break;

        case Symbol::MinusMinus:
            Assert(node.GetFirstChild().GetSym() == Symbol::Ident, "Non ident assignment not yet supported");

            Assert(node.GetChildren().size() == 1, "Unexpected number of children");
            this->AddOp(node.IsPostfix() ? OpCode::PostDec : OpCode::Dec, this->LookupIdentOffset(node.GetFirstChild()));
            break;

        case Symbol::LParen:
            if (node.IsPostfix())
            {
                this->CompileCall(node);
            }
            else
            {
                AssertFail("Unsupported");
                // TODO: tuples?
            }
            break;

        case Symbol::LSquare:
            if (node.IsConstant())
            {
                this->CompileList(node);
            }
            else
            {
                Assert(node.GetChildren().size() == 2, "Unexpected number of children");
                this->CompileExpression(node.GetFirstChild());
                this->CompileExpression(node.GetSecondChild());
                this->AddOp(OpCode::Index);
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

        // Begin
        if (!beginExpr.IsEmpty())
        {
            this->CompileExpression(beginExpr);
            // TODO: What do? Clear register?
            //this->AddOp(OpCode::Pop);
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
            // TODO: What do? Clear register?
            //this->AddOp(OpCode::Pop);
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

        std::vector<size_t> statementEndJmps;
        auto childIter = node.GetChildren().begin();
        size_t nCheckBlocks = node.GetChildren().size() / 2;

        this->PushScope();

        // If / ElIf blocks
        for (size_t count = 0; count < nCheckBlocks; ++count)
        {
            auto checkExpr = *childIter++;
            auto blockStatement = *childIter++;
            this->CompileExpression(checkExpr);
            size_t brIdx = this->AddOp(OpCode::BrF, unsigned int(0xFFFFFFFF));
            this->PushScope();
            this->CompileStatement(blockStatement);
            this->PopScope();
            if (node.GetChildren().size() > 2)
            {
                statementEndJmps.push_back(this->AddOp(OpCode::Jmp, unsigned int(0xFFFFFFFF)));
            }
            this->SetOpOperand(brIdx, (unsigned int)_byteBuffer.size());
        }

        // Else block
        if (childIter != node.GetChildren().end())
        {
            auto blockStatement = *childIter++;
            this->PushScope();
            this->CompileStatement(blockStatement);
            this->PopScope();
        }

        Assert(childIter == node.GetChildren().end(), "Not all If children processed");

        this->PopScope();

        // Set the exit jmps for the individual sections
        for (size_t offset : statementEndJmps)
        {
            this->SetOpOperand(offset, (unsigned int)_byteBuffer.size());
        }
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
            throw CreateBytecodeGenEx(BytecodeGenErr::UndeclaredFunctionReference, node.GetToken());
        }

        unsigned int funcId = funcIter->second;

        if ((unsigned char)nParam != _functions[funcId].nParam)
        {
            throw CreateBytecodeGenEx(BytecodeGenErr::IncorrectCallArity, node.GetToken());
        }

        auto children = node.GetChildren();
        for (auto child = ++children.begin(); child != children.end(); ++child)
        {
            // TODO: What do with results?!
            this->CompileExpression(*child);
        }
        this->AddOp(OpCode::Call, funcId);
        // TODO: What DO?!
        //for (size_t count = 0; count < nParam; ++count) this->AddOp(OpCode::Pop);
        this->AddOp(OpCode::RestoreRet);
    }

    void BytecodeGen::CompileList(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::LSquare, "Unexpected node");

        // TODO: Check number of children
        auto children = node.GetChildren();
        for (auto child = children.begin(); child != children.end(); ++child)
        {
            this->CompileExpression(*child);
        }
        this->AddOp(OpCode::MakeList, (unsigned int)children.size());
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
        case Symbol::And: return OpCode::And;
        case Symbol::Plus: return OpCode::Add;
        case Symbol::Minus: return OpCode::Sub;
        case Symbol::Mult: return OpCode::Mul;
        case Symbol::Div: return OpCode::Div;
        case Symbol::Modulo: return OpCode::Mod;
        case Symbol::Concat: return OpCode::Concat;
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
        case Symbol::Assign: return OpCode::Store;
        case Symbol::PlusEq: return OpCode::PlusEq;
        case Symbol::MinusEq: return OpCode::MinusEq;
        case Symbol::MultEq: return OpCode::MultEq;
        case Symbol::DivEq: return OpCode::DivEq;
        case Symbol::ModuloEq: return OpCode::ModuloEq;
        case Symbol::ConcatEq: return OpCode::ConcatEq;
        default:
            AssertFail("Unmapped unary assign op: " << SymbolToString(sym));
            return OpCode::Unknown;
        }
    }

    OpCode BytecodeGen::MapUnaryAssignIdxOp(Symbol sym) const
    {
        switch (sym)
        {
        case Symbol::Assign: return OpCode::StoreIdx;
        case Symbol::PlusEq: return OpCode::PlusEqIdx;
        case Symbol::MinusEq: return OpCode::MinusEqIdx;
        case Symbol::MultEq: return OpCode::MultEqIdx;
        case Symbol::DivEq: return OpCode::DivEqIdx;
        case Symbol::ModuloEq: return OpCode::ModuloEqIdx;
        case Symbol::ConcatEq: return OpCode::ConcatEqIdx;
        default:
            AssertFail("Unmapped unary assign idex op: " << SymbolToString(sym));
            return OpCode::Unknown;
        }
    }

    void BytecodeGen::PushScope()
    {
        _scopeStack.push_back(std::map<std::string, int>());
        if (_scopeStack.size() == 1)
        {
            _paramOffset = -2;
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
            throw CreateBytecodeGenEx(BytecodeGenErr::DuplicateParameterName, node.GetToken());
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
            throw CreateBytecodeGenEx(BytecodeGenErr::UndeclaredIdentifierReference, node.GetToken());
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
            ENUM_CASE_TO_STRING(BytecodeGenErr::ParameterCountExceeded);
            ENUM_CASE_TO_STRING(BytecodeGenErr::UndeclaredFunctionReference);
            ENUM_CASE_TO_STRING(BytecodeGenErr::IncorrectCallArity);
            ENUM_CASE_TO_STRING(BytecodeGenErr::UndeclaredIdentifierReference);
            ENUM_CASE_TO_STRING(BytecodeGenErr::DuplicateParameterName);

        default:
            AssertFail("Missing case for BytecodeGenErr");
        }

        return nullptr;
    }
}
