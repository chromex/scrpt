#include "../scrpt.h"

#define COMPONENTNAME "BytecodeGen"

// Optimizations
// Ident assignment different than expr assignment (don't push ident handle)

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
            this->CompileFunction(node);
        }

        // Loop over root level nodes and call compile function
        // Accumulate intermediate bytecode
        // Create table of functions
        // Build final bytecode with function instruction offsets

        // Question: do we need an intermediate byte?
        // What about functions we can't map? 

        // Compile time arrity check
        // Runtime arrity check

        // String table
    }

    void BytecodeGen::DumpBytecode()
    {
        // Dump function table
        // Dump string table
        // Dump byte code
    }

    void BytecodeGen::CompileFunction(const AstNode& node)
    {
        this->Verify(node, Symbol::Func);
        auto children = node.GetChildren();
        Assert(children.size() >= 2, "Func node is missing minimum children of ident and block");
        auto ident = children.front();
        this->Verify(ident, Symbol::Ident);
        std::vector<std::string> params;
        TraceInfo("Func: " << ident.GetToken()->GetString());
        for (auto paramIter = ++children.begin(); paramIter != children.end(); ++paramIter)
        {
            // LBracket is the actual function impl node 
            if (paramIter->GetSym() == Symbol::LBracket) break;

            this->Verify(*paramIter, Symbol::Ident);
            params.push_back(paramIter->GetToken()->GetString());
            TraceInfo("Param: " << params.back());
        }
        Assert(params.size() == children.size() - 2, "Function child nodes don't compute");

        // TODO: Record params / snap scope
        
        auto block = children.back();
        this->Verify(block, Symbol::LBracket);

        for (auto statement : block.GetChildren())
        {
            this->CompileStatement(statement);
        }
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
                TraceInfo("OP: Return Val");
            }
            else
            {
                TraceInfo("OP: Return");
            }
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
                TraceInfo("OP: Pop");
            }
            else
            {
                AssertFail("Unhandled Symbol in statement compilation: " << SymbolToString(node.GetToken()->GetSym()));
            }
        }
    }

    // TODO: Need to special case all assignments when target operand is not an ident (including ++, --)
    bool BytecodeGen::CompileExpression(const AstNode& node)
    {
        bool success = true;
        switch (node.GetSym())
        {
        case Symbol::Int:
            TraceInfo("OP: Push Int " << node.GetToken()->GetInt());
            break;

        case Symbol::Float:
            TraceInfo("OP: Push Float " << node.GetToken()->GetFloat());
            break;

        case Symbol::Ident:
            TraceInfo("OP: Push ident val: " << node.GetToken()->GetString());
            break;

        case Symbol::Terminal:
            TraceInfo("OP: Push string: " << node.GetToken()->GetString());
            break;

        case Symbol::True:
            TraceInfo("OP: Push true");
            break;

        case Symbol::False:
            TraceInfo("OP: Push false");
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
            TraceInfo("OP: " << SymbolToString(node.GetSym()) << " Store to ident: " << node.GetFirstChild().GetToken()->GetString());
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
            TraceInfo("OP: " << SymbolToString(node.GetSym()));
            break;

        case Symbol::PlusPlus:
            Assert(node.GetFirstChild().GetSym() == Symbol::Ident, "Non ident assignment not yet supported");

            Assert(node.GetChildren().size() == 1, "Unexpected number of children");
            if (node.IsPostfix())
            {
                TraceInfo("OP: Postfix Increment: " << node.GetFirstChild().GetToken()->GetString());
            }
            else
            {
                TraceInfo("OP: Prefix Increment: " << node.GetFirstChild().GetToken()->GetString());
            }
            break;

        case Symbol::MinusMinus:
            Assert(node.GetFirstChild().GetSym() == Symbol::Ident, "Non ident assignment not yet supported");

            Assert(node.GetChildren().size() == 1, "Unexpected number of children");
            if (node.IsPostfix())
            {
                TraceInfo("OP: Postfix Decrement: " << node.GetFirstChild().GetToken()->GetString());
            }
            else
            {
                TraceInfo("OP: Prefix Decrement: " << node.GetFirstChild().GetToken()->GetString());
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
        TraceInfo("OP: Pop");
        // Record re-entry point
        TraceInfo("<< for re-entry point >>");
        this->CompileExpression(checkExpr);
        TraceInfo("OP: Branch to end (if false)");
        this->CompileStatement(blockStatement);
        this->CompileExpression(endExpr);
        TraceInfo("OP: Jump (to re-entry point");
    }

    void BytecodeGen::CompileWhile(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::While, "Unexpected node");
        Assert(node.GetChildren().size() == 2, "Unexpected child count on While node");

        // TODO: New scope

        auto checkExpr = node.GetFirstChild();
        auto blockStatement = node.GetLastChild();

        TraceInfo("<< while re-entry point >>");
        this->CompileExpression(checkExpr);
        TraceInfo("OP: BRF");
        this->CompileStatement(blockStatement);
        TraceInfo("OP: Jump (to re-entry point)");
    }

    void BytecodeGen::CompileDo(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::Do, "Unexpected node");
        Assert(node.GetChildren().size() == 2, "Unexpected child count on Do node");

        TraceInfo("<< do re-entry point >>");
        this->CompileStatement(node.GetFirstChild());
        this->CompileExpression(node.GetSecondChild());
        TraceInfo("OP: BRT (to do re-entry point)");
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
        TraceInfo("OP: Branch to end (if false)");
        this->CompileStatement(blockStatement);
    }

    void BytecodeGen::CompileCall(const AstNode& node)
    {
        Assert(node.GetSym() == Symbol::LParen, "Unexpected node");
        Assert(node.GetChildren().size() >= 1, "Unexpected child count on Call node");

        // TODO: Push params
        // TODO: Need to be able to revisit this bytecode...
        // TODO: Arrity check... push nArgs in stack frame? 
        TraceInfo("OP: Call " << node.GetFirstChild().GetToken()->GetString());
    }

    void BytecodeGen::Verify(const AstNode& node, Symbol sym) const
    {
        if (node.GetSym() != sym)
        {
            throw CreateBytecodeGenEx(std::string("Expected token ") + SymbolToString(sym), BytecodeGenErr::UnexpectedToken, node.GetToken());
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
