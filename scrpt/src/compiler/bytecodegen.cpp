#include "../scrpt.h"

#define COMPONENTNAME "BytecodeGen"

const size_t MAX_PARAM = 125;

namespace scrpt
{
    BytecodeGen::BytecodeGen()
        : _fd(nullptr)
    {
    }

    void BytecodeGen::AddExternFunc(const char* name, unsigned char nParam, const std::function<void(VM*)>& func)
    {
        AssertNotNull(name);
        Assert(nParam <= MAX_PARAM, "Function takes invalid number of parameters");
        Assert(_functionLookup.find(name) == _functionLookup.end(), "Extern already registered");

        _functionLookup[name] = (unsigned int)_functions.size();
        _functions.push_back(FunctionData{ name, nParam, 0, 0xFFFFFFFF, true, func });
    }

    void BytecodeGen::Consume(const AstNode& ast)
    {
        for (const AstNode* node : ast.GetChildren())
        {
            if (node->GetSym() == Symbol::Func)
            {
                this->RecordFunction(*node);
            }
            else if (node->GetSym() == Symbol::Class)
            {
                this->RecordClass(*node);
            }
            else
            {
                AssertFail("Unexpected top level node");
            }
        }

        for (const AstNode* node : ast.GetChildren())
        {
            this->CompileFunction(*node);
        }
    }

    Bytecode BytecodeGen::GetBytecode()
    {
        _byteBuffer.resize(_byteBuffer.size());
        Bytecode bytecode;
        bytecode.data = std::move(_byteBuffer);
        bytecode.functions = std::move(_functions);
        bytecode.classes = std::move(_classes);
        bytecode.strings = std::move(_strings);
        return bytecode;
    }

    // TODO: Max functions
    void BytecodeGen::RecordFunction(const AstNode& node)
    {
        // Check the node
        this->Verify(node, Symbol::Func);
        auto& children = node.GetChildren();
        Assert(children.size() >= 2, "Func node is missing minimum children of ident and block");

        // Get the name
        const AstNode& ident = *children.front();
        this->Verify(ident, Symbol::Ident);
        std::string name = ident.GetToken()->GetString();

        if (_functionLookup.find(name) != _functionLookup.end())
        {
            CreateEx(Err::BytecodeGen_FunctionRedefinition, ident.GetToken());
        }

        size_t nParam = children.size() - 2;
        if (nParam > MAX_PARAM)
        {
            CreateEx(Err::BytecodeGen_ParameterCountExceeded, ident.GetToken());
        }

        _functionLookup[name] = (unsigned int)_functions.size();
        _functions.push_back(FunctionData{ name, (unsigned char)nParam, 0, 0xFFFFFFFF, false });
    }

    // TODO: Max classes
    void BytecodeGen::RecordClass(const AstNode& node)
    {
        // Check the node
        this->Verify(node, Symbol::Class);
        auto& children = node.GetChildren();
        Assert(children.size() > 0, "Class node is missing a name");
         
        // Get the name
        const AstNode& ident = *children.front();
        this->Verify(ident, Symbol::Ident);
        std::string name = ident.GetToken()->GetString();

        if (_classLookup.find(name) != _classLookup.end())
        {
            CreateEx(Err::BytecodeGen_ClassRedefinition, ident.GetToken());
        }

        unsigned int nMembers = 0;

        // Iterate through children, skipping the first which is the class name
        for (auto child = ++children.begin(); child != children.end(); ++child)
        {
            switch ((*child)->GetSym())
            {
                // Member
                case Symbol::Var:
                    ++nMembers;
                    break;

                // Method
                case Symbol::Func:
                    // TODO
                    break;

                // Ctor
                case Symbol::Ident:
                    // TODO
                    break;

                default:
                    AssertFail("Unexpected class level node");
            }
        }

        _classLookup[name] = (unsigned int)_classes.size();
        _classes.push_back(ClassData{ name, nMembers });
    }

    void BytecodeGen::CompileFunction(const AstNode& node)
    {
        this->Verify(node, Symbol::Func);
        const AstNode& ident = node.GetFirstChild();

        // Update function table with bytecode offset
        _fd = &_functions[_functionLookup[ident.GetToken()->GetString()]];
        _fd->entry = (unsigned int)_byteBuffer.size();

        // Start function scope
        this->PushScope();
        _paramOffset = -2;

        // Get the params
        auto& children = node.GetChildren();
        size_t nParam = children.size() - 2;
        size_t nParamAdded = 0;
        for (auto paramIter = ++children.rbegin(); nParamAdded < nParam; ++paramIter)
        {
            this->Verify(**paramIter, Symbol::Ident);
            int offset = this->AddParam(**paramIter);
            _fd->localLookup[offset] = (*paramIter)->GetToken()->GetString();
            ++nParamAdded;
        }
        
        const AstNode& block = node.GetLastChild();
        this->Verify(block, Symbol::LBracket);

        for (AstNode* statement : block.GetChildren())
        {
            this->CompileStatement(*statement);
        }

        // Add an implicit return if there was no explicit one
        if (block.GetChildren().size() == 0 || block.GetLastChild().GetSym() != Symbol::Return)
        {
            char reg = this->ClaimRegister(block);
            this->AddOp(OpCode::LoadNull, reg);
            this->AddOp(OpCode::Ret, reg);
            this->ReleaseRegister(reg);
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
            this->CompileReturn(node);
            break;

        case Symbol::LBracket:
            // TODO: This needs to either error or support statement level map decl
            this->PushScope();
            for (AstNode* child : node.GetChildren())
            {
                this->CompileStatement(*child);
            }
            this->PopScope();
            break;

        case Symbol::Var:
            this->CompileDecl(node);
            break;

        default:
            auto result = this->CompileExpression(node);
            if (std::get<0>(result))
            {
                this->ReleaseRegister(std::get<1>(result));
            }
            else
            {
                AssertFail("Unhandled Symbol in compilation: " << SymbolToString(node.GetToken()->GetSym()));
            }
        }
    }

    std::tuple<bool, char> BytecodeGen::CompileExpression(const AstNode& node)
    {
        bool success = true;
        char outReg = 0, reg0, reg1;

        switch (node.GetSym())
        {
        case Symbol::Int:
            outReg = this->ClaimRegister(node);
            this->AddOp(OpCode::LoadInt, outReg, node.GetToken()->GetInt());
            break;

        case Symbol::Float:
            outReg = this->ClaimRegister(node);
            this->AddOp(OpCode::LoadFloat, outReg, node.GetToken()->GetFloat());
            break;

        case Symbol::Ident:
            if (!this->LookupIdentOffset(node.GetToken()->GetString(), &outReg))
            {
                auto funcIter = _functionLookup.find(node.GetToken()->GetString());
                if (funcIter != _functionLookup.end())
                {
                    outReg = this->ClaimRegister(node);
                    this->AddOp(OpCode::LoadFunc, outReg, funcIter->second);
                }
                else
                {
                    throw CreateEx(Err::BytecodeGen_UndeclaredIdentifierReference, node.GetToken());
                }
            }
            break;

        case Symbol::Terminal:
            {
                unsigned int strId = this->GetStringId(node.GetToken()->GetString());
                outReg = this->ClaimRegister(node);
                this->AddOp(OpCode::LoadString, outReg, strId);
            }
            break;

        case Symbol::True:
            outReg = this->ClaimRegister(node);
            this->AddOp(OpCode::LoadTrue, outReg);
            break;

        case Symbol::False:
            outReg = this->ClaimRegister(node);
            this->AddOp(OpCode::LoadFalse, outReg);
            break;

        case Symbol::Assign:
            {
                Assert(node.GetChildren().size() == 2, "Unexpected number of children");
                const AstNode& firstChild = node.GetFirstChild();
                Symbol firstChildSym = firstChild.GetSym();
                if (firstChildSym == Symbol::Ident)
                {
                    char rhsReg = GetRegResult(this->CompileExpression(node.GetSecondChild()));
                    outReg = this->LookupIdentOffset(firstChild);
                    this->AddOp(OpCode::Store, outReg, rhsReg);
                    this->ReleaseRegister(rhsReg);
                }
                else if (firstChildSym == Symbol::LSquare || firstChildSym == Symbol::Dot)
                {
                    outReg = GetRegResult(this->CompileExpression(node.GetSecondChild()));
                    char objReg = GetRegResult(this->CompileExpression(firstChild.GetFirstChild()));
                    char indexReg;
                    if (firstChildSym == Symbol::LSquare)
                    {
                        indexReg = GetRegResult(this->CompileExpression(firstChild.GetSecondChild()));
                    }
                    else
                    {
                        unsigned int indexStrId = this->GetStringId(firstChild.GetSecondChild().GetToken()->GetString());
                        indexReg = this->ClaimRegister(firstChild);
                        this->AddOp(OpCode::LoadString, indexReg, indexStrId);
                    }
                    this->AddOp(OpCode::StoreIdx, objReg, indexReg, outReg);
                    this->ReleaseRegister(objReg);
                    this->ReleaseRegister(indexReg);
                }
                else
                {
                    Assert(node.GetFirstChild().GetSym() == Symbol::Ident, "Unknown assignment LHS");
                }
            }
            break;

        case Symbol::PlusEq:
        case Symbol::MinusEq:
        case Symbol::MultEq:
        case Symbol::DivEq:
        case Symbol::ModuloEq:
        case Symbol::ConcatEq:
            {
                Assert(node.GetChildren().size() == 2, "Unexpected number of children");
                const AstNode& firstChild = node.GetFirstChild();
                Symbol firstChildSym = firstChild.GetSym();
                if (firstChildSym == Symbol::Ident)
                {
                    // TODO: Ideally the math ops can write to one of their own input registers rather than
                    // perform the op and then store it
                    char rhsReg = GetRegResult(this->CompileExpression(node.GetSecondChild()));
                    outReg = this->LookupIdentOffset(node.GetFirstChild());
                    char tmpReg = this->ClaimRegister(node);
                    this->AddOp(this->MapUnaryAssignOp(node.GetSym()), outReg, rhsReg, tmpReg);
                    this->AddOp(OpCode::Store, outReg, tmpReg);
                    this->ReleaseRegister(rhsReg);
                    this->ReleaseRegister(tmpReg);
                }
                else if (firstChildSym == Symbol::LSquare || firstChildSym == Symbol::Dot)
                {
                    char rhsReg = GetRegResult(this->CompileExpression(node.GetSecondChild()));
                    char objReg = GetRegResult(this->CompileExpression(firstChild.GetFirstChild()));
                    char indexReg;
                    if (firstChildSym == Symbol::LSquare)
                    {
                        // Bracket indexing
                        indexReg = GetRegResult(this->CompileExpression(firstChild.GetSecondChild()));
                    }
                    else
                    {
                        // Dotted indexing
                        unsigned int indexStrId = this->GetStringId(firstChild.GetSecondChild().GetToken()->GetString());
                        indexReg = this->ClaimRegister(firstChild);
                        this->AddOp(OpCode::LoadString, indexReg, indexStrId);
                    }
                    char lhsReg = this->ClaimRegister(node);
                    this->AddOp(OpCode::Index, objReg, indexReg, lhsReg);
                    outReg = this->ClaimRegister(node);
                    this->AddOp(this->MapUnaryAssignOp(node.GetSym()), lhsReg, rhsReg, outReg);
                    this->AddOp(OpCode::StoreIdx, objReg, indexReg, outReg);
                    this->ReleaseRegister(rhsReg);
                    this->ReleaseRegister(objReg);
                    this->ReleaseRegister(indexReg);
                    this->ReleaseRegister(lhsReg);
                }
                else
                {
                    Assert(node.GetFirstChild().GetSym() == Symbol::Ident, "Unknown assignment LHS");
                }
            }
            break;

        case Symbol::Minus:
            if (node.GetChildren().size() == 1)
            {
                reg0 = GetRegResult(this->CompileExpression(node.GetFirstChild()));
                outReg = this->ClaimRegister(node);
                this->AddOp(OpCode::Neg, reg0, outReg);
                this->ReleaseRegister(reg0);
                break;
            }
            // !!! Intentionally leaks into the binary op handling below

        case Symbol::Eq:
        case Symbol::Or:
        case Symbol::And:
        case Symbol::Plus:
        case Symbol::Mult:
        case Symbol::Div:
        case Symbol::Modulo:
        case Symbol::Concat:
        case Symbol::LessThan:
        case Symbol::LessThanEq:
        case Symbol::GreaterThan:
        case Symbol::GreaterThanEq:
            Assert(node.GetChildren().size() == 2, "Unexpected number of children");
            reg0 = GetRegResult(this->CompileExpression(node.GetFirstChild()));
            reg1 = GetRegResult(this->CompileExpression(node.GetSecondChild()));
            outReg = this->ClaimRegister(node);
            this->AddOp(this->MapBinaryOp(node.GetSym()), reg0, reg1, outReg);
            this->ReleaseRegister(reg0);
            this->ReleaseRegister(reg1);
            break;

        case Symbol::PlusPlus:
            Assert(node.GetFirstChild().GetSym() == Symbol::Ident, "Non ident assignment not yet supported");

            Assert(node.GetChildren().size() == 1, "Unexpected number of children");
            outReg = this->LookupIdentOffset(node.GetFirstChild());
            this->AddOp(node.IsPostfix() ? OpCode::PostInc : OpCode::Inc, outReg);
            break;

        case Symbol::MinusMinus:
            Assert(node.GetFirstChild().GetSym() == Symbol::Ident, "Non ident assignment not yet supported");

            Assert(node.GetChildren().size() == 1, "Unexpected number of children");
            outReg = this->LookupIdentOffset(node.GetFirstChild());
            this->AddOp(node.IsPostfix() ? OpCode::PostDec : OpCode::Dec, outReg);
            break;

        case Symbol::LParen:
            if (node.IsPostfix())
            {
                outReg = this->CompileCall(node);
            }
            else
            {
                Assert(node.GetChildren().size() == 1, "Unexpected number of children");
                outReg = GetRegResult(this->CompileExpression(node.GetFirstChild()));
            }
            break;

        case Symbol::LSquare:
            if (node.IsConstant())
            {
                outReg = this->CompileList(node);
            }
            else
            {
                Assert(node.GetChildren().size() == 2, "Unexpected number of children");
                reg0 = GetRegResult(this->CompileExpression(node.GetFirstChild()));
                reg1 = GetRegResult(this->CompileExpression(node.GetSecondChild()));
                outReg = this->ClaimRegister(node);
                this->AddOp(OpCode::Index, reg0, reg1, outReg);
                this->ReleaseRegister(reg0);
                this->ReleaseRegister(reg1);
            }
            break;

        case Symbol::LBracket:
            Assert(node.IsConstant(), "No such thing as a non-constant LBracket expression");
            outReg = this->CompileMap(node);
            break;

        case Symbol::Dot:
            {
                Assert(node.GetChildren().size() == 2, "Unexpected number of children");
                Assert(node.GetSecondChild().GetSym() == Symbol::Ident, "Unexpected second child");
                unsigned int strId = this->GetStringId(node.GetSecondChild().GetToken()->GetString());
                reg0 = GetRegResult(this->CompileExpression(node.GetFirstChild()));
                reg1 = this->ClaimRegister(node.GetSecondChild());
                this->AddOp(OpCode::LoadString, reg1, strId);
                outReg = this->ClaimRegister(node);
                this->AddOp(OpCode::Index, reg0, reg1, outReg);
                this->ReleaseRegister(reg0);
                this->ReleaseRegister(reg1);
            }
            break;

        default:
            AssertFail("Unhandled Symbol in expression compilation: " << SymbolToString(node.GetToken()->GetSym()));
            success = false;
        }

        return std::make_tuple(success, outReg);
    }

    void BytecodeGen::CompileFor(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::For, "Unexpected node");
        Assert(node.GetChildren().size() == 4, "Unexpected child count on For node");

        const AstNode& beginExpr = node.GetFirstChild();
        const AstNode& checkExpr = node.GetSecondChild();
        const AstNode& endExpr = node.GetThirdChild();
        const AstNode& blockStatement = node.GetLastChild();

        this->PushScope();

        // Begin
        if (!beginExpr.IsEmpty())
        {
            if (beginExpr.GetSym() == Symbol::Var)
            {
                this->CompileDecl(beginExpr);
            }
            else
            {
                char reg = GetRegResult(this->CompileExpression(beginExpr));
                this->ReleaseRegister(reg);
            }
        }

        // Check
        unsigned int reentry = (unsigned int)_byteBuffer.size();
        size_t brIdx = 0;
        if (!checkExpr.IsEmpty())
        {
            char reg = GetRegResult(this->CompileExpression(checkExpr));
            brIdx = this->AddOp(OpCode::BrF, reg, unsigned int(0xFFFFFFFF));
            this->ReleaseRegister(reg);
        }

        // Block
        this->PushScope();
        this->CompileStatement(blockStatement);
        this->PopScope();

        // End 
        if (!endExpr.IsEmpty())
        {
            char reg = GetRegResult(this->CompileExpression(endExpr));
            this->ReleaseRegister(reg);
        }
        this->AddOp(OpCode::Jmp, reentry);
        if (!checkExpr.IsEmpty())
        {
            this->SetOpOperand(brIdx, 1, (unsigned int)_byteBuffer.size());
        }

        this->PopScope();
    }

    void BytecodeGen::CompileWhile(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::While, "Unexpected node");
        Assert(node.GetChildren().size() == 2, "Unexpected child count on While node");

        const AstNode& checkExpr = node.GetFirstChild();
        const AstNode& blockStatement = node.GetLastChild();

        this->PushScope();

        unsigned int reentry = (unsigned int)_byteBuffer.size();
        char checkReg = GetRegResult(this->CompileExpression(checkExpr));
        size_t brIdx = this->AddOp(OpCode::BrF, checkReg, unsigned int(0xFFFFFFFF));
        this->ReleaseRegister(checkReg);
        this->PushScope();
        this->CompileStatement(blockStatement);
        this->PopScope();
        this->AddOp(OpCode::Jmp, reentry);
        this->SetOpOperand(brIdx, 1, (unsigned int)_byteBuffer.size());

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
        char checkReg = GetRegResult(this->CompileExpression(node.GetSecondChild()));
        this->AddOp(OpCode::BrT, checkReg, reentry);
        this->ReleaseRegister(checkReg);

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
            const AstNode& checkExpr = **childIter++;
            const AstNode& blockStatement = **childIter++;
            char checkReg = GetRegResult(this->CompileExpression(checkExpr));
            size_t brIdx = this->AddOp(OpCode::BrF, checkReg, unsigned int(0xFFFFFFFF));
            this->ReleaseRegister(checkReg);
            this->PushScope();
            this->CompileStatement(blockStatement);
            this->PopScope();
            if (node.GetChildren().size() > 2)
            {
                statementEndJmps.push_back(this->AddOp(OpCode::Jmp, unsigned int(0xFFFFFFFF)));
            }
            this->SetOpOperand(brIdx, 1, (unsigned int)_byteBuffer.size());
        }

        // Else block
        if (childIter != node.GetChildren().end())
        {
            const AstNode& blockStatement = **childIter++;
            this->PushScope();
            this->CompileStatement(blockStatement);
            this->PopScope();
        }

        Assert(childIter == node.GetChildren().end(), "Not all If children processed");

        this->PopScope();

        // Set the exit jmps for the individual sections
        for (size_t offset : statementEndJmps)
        {
            this->SetOpOperand(offset, 0, (unsigned int)_byteBuffer.size());
        }
    }

    void BytecodeGen::CompileReturn(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::Return, "Unexpected node");
        Assert(node.GetChildren().size() < 2, "Unexpected number of children");
        char reg;
        if (node.GetChildren().size() > 0)
        {
            reg = GetRegResult(this->CompileExpression(node.GetFirstChild()));
        }
        else
        {
            reg = this->ClaimRegister(node);
            this->AddOp(OpCode::LoadNull, reg);
        }
        this->AddOp(OpCode::Ret, reg);
        this->ReleaseRegister(reg);
    }

    void BytecodeGen::CompileDecl(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::Var, "Unexpected node");
        Assert(node.GetChildren().size() > 0 && node.GetChildren().size() < 3, "Unexpected number of children");

        // TODO: Claiming the register before resolving the expression does result in some register
        // use bloat that isn't necessary.
        char identReg = this->AddLocal(node.GetFirstChild());
        if (node.GetChildren().size() == 2)
        {
            char rhsReg = GetRegResult(this->CompileExpression(node.GetSecondChild()));
            this->AddOp(OpCode::Store, identReg, rhsReg);
            this->ReleaseRegister(rhsReg);
        }
    }

    char BytecodeGen::CompileCall(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::LParen, "Unexpected node");
        Assert(node.GetChildren().size() >= 1, "Unexpected child count on Call node");

        const AstNode& firstChild = node.GetFirstChild();
        bool classCall = firstChild.GetSym() == Symbol::Colon;

        size_t nParam = classCall ? node.GetChildren().size() : node.GetChildren().size() - 1;
        if (nParam > MAX_PARAM)
        {
            throw CreateEx(Err::BytecodeGen_ParameterCountExceeded, node.GetToken());
        }

        // Get the function handle
        char funcReg;
        if (classCall)
        {
            // If the LHS is a ":" then we special case for a class call (implicit this)
            Assert(firstChild.GetChildren().size() == 2, "Unexpected child count on ClassCall node");
            Assert(firstChild.GetSecondChild().GetSym() == Symbol::Ident, "Unexpected second child");

            unsigned int strId = this->GetStringId(firstChild.GetSecondChild().GetToken()->GetString());

            char sourceReg = GetRegResult(this->CompileExpression(firstChild.GetFirstChild()));
            char indexReg = this->ClaimRegister(firstChild.GetSecondChild());
            this->AddOp(OpCode::LoadString, indexReg, strId);
            funcReg = this->ClaimRegister(node);
            this->AddOp(OpCode::Index, sourceReg, indexReg, funcReg);
            this->AddOp(OpCode::Push, sourceReg);
            this->ReleaseRegister(sourceReg);
            this->ReleaseRegister(indexReg);
        }
        else
        {
            // Otherwise just compile the LHS as an expression
            funcReg = GetRegResult(this->CompileExpression(firstChild));
        }

        // Compile parameters for the call
        auto& children = node.GetChildren();
        for (auto child = ++children.begin(); child != children.end(); ++child)
        {
            char reg = GetRegResult(this->CompileExpression(**child));
            this->AddOp(OpCode::Push, reg);
            this->ReleaseRegister(reg);
        }

        this->AddOp(OpCode::Call, funcReg, (char)nParam);
        this->ReleaseRegister(funcReg);

        if (nParam > 0) this->AddOp(OpCode::PopN, (char)nParam);

        char reg = this->ClaimRegister(node);
        this->AddOp(OpCode::RestoreRet, reg);
        return reg;
    }

    char BytecodeGen::CompileList(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::LSquare, "Unexpected node");

        // TODO: Check number of children
        auto& children = node.GetChildren();
        for (auto child = children.begin(); child != children.end(); ++child)
        {
            char reg = GetRegResult(this->CompileExpression(**child));
            this->AddOp(OpCode::Push, reg);
            this->ReleaseRegister(reg);
        }

        char reg = this->ClaimRegister(node);
        this->AddOp(OpCode::MakeList, reg, (unsigned int)children.size());
        return reg;
    }

    char BytecodeGen::CompileMap(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::LBracket, "Unexpected node");
        Assert(node.IsConstant(), "Unexpected node");
        Assert(node.GetChildren().size() % 2 == 0, "Map should have an even child count");

        // TODO: Check child count
        auto& children = node.GetChildren();
        for (auto child = children.begin(); child != children.end(); ++child)
        {
            char reg = GetRegResult(this->CompileExpression(**child));
            this->AddOp(OpCode::Push, reg);
            this->ReleaseRegister(reg);
        }

        char reg = this->ClaimRegister(node);
        this->AddOp(OpCode::MakeMap, reg, (unsigned int)children.size());
        return reg;
    }

    size_t BytecodeGen::AddOp(OpCode op)
    {
        _byteBuffer.push_back(static_cast<unsigned char>(op));
        return _byteBuffer.size() - 1;
    }

    size_t BytecodeGen::AddOp(OpCode op, char reg0)
    {
        size_t ret = _byteBuffer.size();
        _byteBuffer.push_back(static_cast<unsigned char>(op));
        _byteBuffer.push_back(static_cast<unsigned char>(reg0));
        return ret;
    }

    size_t BytecodeGen::AddOp(OpCode op, char reg0, char reg1)
    {
        size_t ret = _byteBuffer.size();
        _byteBuffer.push_back(static_cast<unsigned char>(op));
        _byteBuffer.push_back(static_cast<unsigned char>(reg0));
        _byteBuffer.push_back(static_cast<unsigned char>(reg1));
        return ret;
    }

    size_t BytecodeGen::AddOp(OpCode op, char reg0, char reg1, char reg2)
    {
        size_t ret = _byteBuffer.size();
        _byteBuffer.push_back(static_cast<unsigned char>(op));
        _byteBuffer.push_back(static_cast<unsigned char>(reg0));
        _byteBuffer.push_back(static_cast<unsigned char>(reg1));
        _byteBuffer.push_back(static_cast<unsigned char>(reg2));
        return ret;
    }

    size_t BytecodeGen::AddOp(OpCode op, char reg0, unsigned int data)
    {
        size_t ret = this->AddOp(op, reg0);
        this->AddData((unsigned char*)&data);
        return ret;
    }

    size_t BytecodeGen::AddOp(OpCode op, char reg0, float data)
    {
        size_t ret = this->AddOp(op, reg0);
        this->AddData((unsigned char*)&data);
        return ret;
    }

    size_t BytecodeGen::AddOp(OpCode op, char reg0, int data)
    {
        size_t ret = this->AddOp(op, reg0);
        this->AddData((unsigned char*)&data);
        return ret;
    }

    size_t BytecodeGen::AddOp(OpCode op, unsigned int data)
    {
        size_t ret = this->AddOp(op);
        this->AddData((unsigned char*)&data);
        return ret;
    }

    void BytecodeGen::AddData(unsigned char* data)
    {
        size_t ret = _byteBuffer.size();
        _byteBuffer.push_back(data[0]);
        _byteBuffer.push_back(data[1]);
        _byteBuffer.push_back(data[2]);
        _byteBuffer.push_back(data[3]);
        Assert(*((unsigned int*)&(_byteBuffer[ret])) == *(unsigned int*)data, "Data should be correctly packed");
    }

    void BytecodeGen::SetOpOperand(size_t opIdx, int offset, unsigned int p0)
    {
        this->SetOpOperand(opIdx, offset, (unsigned char*)&p0);
    }

    void BytecodeGen::SetOpOperand(size_t opIdx, int offset, unsigned char* p0)
    {
        Assert(_byteBuffer.size() > opIdx, "Index out of bounds");

        _byteBuffer[opIdx + offset + 1] = p0[0];
        _byteBuffer[opIdx + offset + 2] = p0[1];
        _byteBuffer[opIdx + offset + 3] = p0[2];
        _byteBuffer[opIdx + offset + 4] = p0[3];
        Assert(*((unsigned int*)&(_byteBuffer[opIdx + offset + 1])) == *(unsigned int*)p0, "Data should be correctly packed...");
    }

    void BytecodeGen::Verify(const AstNode& node, Symbol sym) const
    {
        if (node.GetSym() != sym)
        {
            throw CreateEx(std::string("Expected token ") + SymbolToString(sym), Err::BytecodeGen_UnexpectedToken, node.GetToken());
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
        case Symbol::PlusEq: return OpCode::Add;
        case Symbol::MinusEq: return OpCode::Sub;
        case Symbol::MultEq: return OpCode::Mul;
        case Symbol::DivEq: return OpCode::Div;
        case Symbol::ModuloEq: return OpCode::Mod;
        case Symbol::ConcatEq: return OpCode::Concat;
        default:
            AssertFail("Unmapped unary assign op: " << SymbolToString(sym));
            return OpCode::Unknown;
        }
    }

    void BytecodeGen::PushScope()
    {
        _scopeStack.push_back(std::map<std::string, char>());
    }

    void BytecodeGen::PopScope()
    {
        Assert(_scopeStack.size() > 0, "Can't pop an empty stack");
        auto& scope = _scopeStack.back();
        for (auto& entry : scope)
        {
            this->ReleaseRegister(entry.second, true);
        }
        _scopeStack.pop_back();
    }

    char BytecodeGen::ClaimRegister(const AstNode& node, bool lock)
    {
        for (size_t idx = 0; idx < _registers.size(); ++idx)
        {
            if (!_registers.test(idx))
            {
                _registers.set(idx);
                if (lock) _lockedRegisters.set(idx);
                _fd->nLocalRegisters = std::max(_fd->nLocalRegisters, idx + 1);
                return (char)idx;
            }
        }

        throw CreateEx(Err::BytecodeGen_InsufficientRegisters, node.GetToken());
    }

    void BytecodeGen::ReleaseRegister(char reg, bool unlock)
    {
        if (reg < 0)
        {
            // Ignore negative registers since they are parameter registers
            return;
        }

        Assert(_registers.test(reg), "Releasing unclaimed register");
        Assert(!unlock || _lockedRegisters.test(reg), "Unlocking a non-locked register");
        if (unlock || !_lockedRegisters.test(reg))
        {
            _registers.reset(reg);
            _lockedRegisters.reset(reg);
        }
    }

    char BytecodeGen::GetRegResult(const std::tuple<bool, char>& result)
    {
        // TODO: This should support errors?
        return std::get<1>(result);
    }

    char BytecodeGen::AddParam(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::Ident, "Unexpected symbol");
        Assert(_scopeStack.size() == 1, "Params can only be added at root scope");

        char offset;
        const char* ident = node.GetToken()->GetString();
        if (this->LookupIdentOffset(ident, &offset))
        {
            throw CreateEx(Err::BytecodeGen_DuplicateParameterName, node.GetToken());
        }

        offset = _paramOffset--;
        _scopeStack.back()[ident] = offset;
        return offset;
    }

    char BytecodeGen::AddLocal(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::Ident, "Unexpected symbol");

        // Check for multiple declaration
        if (!this->IsLocalIdentAvailable(node))
        {
            throw CreateEx(Err::BytecodeGen_MulipleDeclaration, node.GetToken());
        }

        // Reserve a register
        char reg = this->ClaimRegister(node, true);

        // Add data to the scope stack and function debug local lookup
        const char* ident = node.GetToken()->GetString();
        _scopeStack.back()[ident] = reg;
        _fd->localLookup[reg] = ident;

        return reg;
    }

    bool BytecodeGen::IsLocalIdentAvailable(const AstNode& node) const
    {
        Assert(node.GetSym() == Symbol::Ident, "Unexpected symbol");
        Assert(_scopeStack.size() > 0, "Adding local with an empty scope stack");

        auto& scope = _scopeStack.back();
        return scope.find(node.GetToken()->GetString()) == scope.end();
    }

    char BytecodeGen::LookupIdentOffset(const AstNode& node) const
    {
        Assert(node.GetSym() == Symbol::Ident, "Unexpected symbol");

        char offset;
        if (!this->LookupIdentOffset(node.GetToken()->GetString(), &offset))
        {
            throw CreateEx(Err::BytecodeGen_UndeclaredIdentifierReference, node.GetToken());
        }

        return offset;
    }

    unsigned int BytecodeGen::GetStringId(const char* str)
    {
        unsigned int strId = 0;
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
        
        return strId;
    }

    bool BytecodeGen::LookupIdentOffset(const char* ident, char* id) const
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
}
