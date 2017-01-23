#include "../scrpt.h"

#define COMPONENTNAME "BytecodeGen"

// TODO: For the loops, don't use the Statement parse block since we need to supress scope push

// TODO: for decl line has a new scope, for block ALSO has a new scope... same for if's and everything else
// TODO: Only need to push a scope if it has new locals
// TODO: Need to sum locals 

namespace scrpt
{
    BytecodeGen::BytecodeGen()
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

        // Build final bytecode with function instruction offsets

        // Compile time arrity check
        // Runtime arrity check

        // String table
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

        // Get the params
        std::vector<std::string> params;
        for (auto paramIter = ++children.begin(); paramIter != children.end(); ++paramIter)
        {
            // LBracket is the actual function impl node 
            if (paramIter->GetSym() == Symbol::LBracket) break;

            this->Verify(*paramIter, Symbol::Ident);
            params.push_back(paramIter->GetToken()->GetString());
        }
        Assert(params.size() == children.size() - 2, "Function child nodes don't compute");

        if (params.size() > 255)
        {
            // TODO: Check param count (255 max)
            AssertFail("Need a real error here");
        }

        // TODO: Check for redefinition of function name

        _functionLookup[ident.GetToken()->GetString()] = (unsigned int)_functions.size();
        _functions.push_back(FunctionData{ ident.GetToken()->GetString(), (unsigned char)params.size(), 0xFFFFFFFF });
    }

    void BytecodeGen::CompileFunction(const AstNode& node)
    {
        auto ident = node.GetFirstChild();

        // Update function table with bytecode offset
        _functions[_functionLookup[ident.GetToken()->GetString()]].entry = (unsigned int)_byteBuffer.size();

        // TODO: Record params / snap scope
        
        auto block = node.GetLastChild();
        this->Verify(block, Symbol::LBracket);

        for (auto statement : block.GetChildren())
        {
            this->CompileStatement(statement);
        }

        // TODO: If no return added, add one
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
            if (node.GetChildren().size() == 1)
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
            // TODO: Push scope?
            for (auto child : node.GetChildren())
            {
                this->CompileStatement(child);
            }
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
            // TODO: Need ident offset
            this->AddOp(OpCode::PushIdent, int(0xFFFFFFFF));
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
            // TODO: Need ident offset
            this->AddOp(this->MapUnaryAssignOp(node.GetSym()), int(0xFFFFFFFF));
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
            if (node.IsPostfix())
            {
                // TODO: Ident offset
                this->AddOp(OpCode::PostIncI, int(0xFFFFFFFF));
            }
            else
            {
                // TODO: Ident offset
                this->AddOp(OpCode::IncI, int(0xFFFFFFFF));
            }
            break;

        case Symbol::MinusMinus:
            Assert(node.GetFirstChild().GetSym() == Symbol::Ident, "Non ident assignment not yet supported");

            Assert(node.GetChildren().size() == 1, "Unexpected number of children");
            if (node.IsPostfix())
            {
                // TODO: Ident offset
                this->AddOp(OpCode::PostDecI, int(0xFFFFFFFF));
            }
            else
            {
                // TODO: Ident offset
                this->AddOp(OpCode::DecI, int(0xFFFFFFFF));
            }
            break;

        case Symbol::LParen:
            if (node.IsPostfix())
            {
                this->CompileCall(node);
            }
            else
            {
                // TODO ?
            }
            break;

        default:
            AssertFail("Unhandled Symbol in expression compilation: " << SymbolToString(node.GetToken()->GetSym()));
            success = false;
        }

        return success;
    }

    // TODO: Support empty node
    void BytecodeGen::CompileFor(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::For, "Unexpected node");
        Assert(node.GetChildren().size() == 4, "Unexpected child count on For node");

        // TODO: New scope

        auto beginExpr = node.GetFirstChild();
        auto checkExpr = node.GetSecondChild();
        auto endExpr = node.GetThirdChild();
        auto blockStatement = node.GetLastChild();

        this->CompileExpression(beginExpr);
        this->AddOp(OpCode::Pop);
        unsigned int reentry = (unsigned int)_byteBuffer.size();
        this->CompileExpression(checkExpr);
        size_t brIdx = this->AddOp(OpCode::BrF, unsigned int(0xFFFFFFFF));
        this->CompileStatement(blockStatement);
        this->CompileExpression(endExpr);
        this->AddOp(OpCode::Jmp, reentry);
        this->SetOpOperand(brIdx, (unsigned int)_byteBuffer.size());
    }

    void BytecodeGen::CompileWhile(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::While, "Unexpected node");
        Assert(node.GetChildren().size() == 2, "Unexpected child count on While node");

        // TODO: New scope

        auto checkExpr = node.GetFirstChild();
        auto blockStatement = node.GetLastChild();

        unsigned int reentry = (unsigned int)_byteBuffer.size();
        this->CompileExpression(checkExpr);
        size_t brIdx = this->AddOp(OpCode::BrF, unsigned int(0xFFFFFFFF));
        this->CompileStatement(blockStatement);
        this->AddOp(OpCode::Jmp, reentry);
        this->SetOpOperand(brIdx, (unsigned int)_byteBuffer.size());
    }

    void BytecodeGen::CompileDo(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::Do, "Unexpected node");
        Assert(node.GetChildren().size() == 2, "Unexpected child count on Do node");

        unsigned int reentry = (unsigned int)_byteBuffer.size();
        this->CompileStatement(node.GetFirstChild());
        this->CompileExpression(node.GetSecondChild());
        this->AddOp(OpCode::BrT, reentry);
    }

    void BytecodeGen::CompileIf(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::If, "Unexpected node");
        Assert(node.GetChildren().size() >= 2, "Unexpected child count on If node");

        // TODO: New scope
        // TODO: ElIf and Else support

        auto checkExpr = node.GetFirstChild();
        auto blockStatement = node.GetSecondChild();

        this->CompileExpression(checkExpr);
        size_t brIdx = this->AddOp(OpCode::BrF, unsigned int(0xFFFFFFFF));
        this->CompileStatement(blockStatement);
        this->SetOpOperand(brIdx, (unsigned int)_byteBuffer.size());
    }

    void BytecodeGen::CompileCall(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::LParen, "Unexpected node");
        Assert(node.GetChildren().size() >= 1, "Unexpected child count on Call node");

        auto ident = node.GetFirstChild();
        this->Verify(ident, Symbol::Ident);
        size_t nParam = node.GetChildren().size() - 1;

        // TODO: Check too many params (> 255)
        unsigned int funcId = _functionLookup[node.GetFirstChild().GetToken()->GetString()];

        if ((unsigned char)nParam != _functions[funcId].nParam)
        {
            // TODO
            AssertFail("Need a real error here");
        }

        // TODO: Push params
        this->AddOp(OpCode::Call, funcId);
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
            // TODO: Compiler bug ex vs compilation bug ex?
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
            // TODO: Compiler bug ex vs compilation bug ex?
            return OpCode::Unknown;
        }
    }

    const char* BytecodeGenErrToString(BytecodeGenErr err)
    {
        switch (err)
        {
            ENUM_CASE_TO_STRING(BytecodeGenErr::NoError);
            ENUM_CASE_TO_STRING(BytecodeGenErr::UnexpectedToken);

        default:
            AssertFail("Missing case for BytecodeGenErr");
        }

        return nullptr;
    }
}
