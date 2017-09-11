#pragma once

namespace scrpt
{
    enum class ParseErr
    {
        NoError,
        UnexpectedSymbol,
        BlockExpected,
        ExpressionExpected,
        StatementExpected,
    };
    const char* ParseErrToString(ParseErr err);

    class Parser
    {
    public:
        Parser();

        void Consume(Lexer* lexer);
        AstNode* GetAst();
        void DumpAst();

    private:
        Lexer* _lexer;
        AstNode _program;
        AstNode* _currentNode;

        void PushNode();
        void AddNode();
        void PopNode();

        // Checks for the symbol and advances the lexer if it matches, return true if match
        bool Allow(Symbol sym);
        // Same as allow but adds the token to the current parent on match
        bool Accept(Symbol sym);
        // Same as accept but replaces the previous child (t1) with the new token and adds t1 as a child of the new token
        bool AcceptAndSwapOp(Symbol sym, std::vector<Symbol>& ltrMatch, bool postfix = false);
        // Returns true if the symbols match but doesn't advance the lexer
        bool Test(Symbol sym) const;
        // Same as Allow but creates an error if it doesn't match
        bool Expect(Symbol sym);

        void ParseProgram();
        bool ParseBlock(bool expect);
        bool ParseStatement(bool expect);
        bool ParseExpression(bool expect);
        bool ParseExOr(bool expect);
        bool ParseExAnd(bool expect);
        bool ParseExEquals(bool expect);
        bool ParseExConcat(bool expect);
        bool ParseExCompare(bool expect);
        bool ParseExAdd(bool expect);
        bool ParseExMul(bool expect);
        bool ParseExPrefix(bool expect);
        bool ParseExPostfix(bool expect);
        bool ParseExTerm();
        bool ParseCall();
        bool ParseIndex();
        bool ParseDotExpand();
        bool ParseWhileLoop();
        bool ParseDoLoop();
        bool ParseForLoop();
        bool ParseIf();
        bool ParseBreak();
        bool ParseReturn();
        bool ParseContinue();
        bool ParseSwitch();
        bool ParseCase();
        bool ParseConstant(bool expect);
        bool ParseParens();
        bool ParseList();
        bool ParseDict();
    };
}
