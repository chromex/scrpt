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

        bool Accept(Symbol sym, bool push = false);
        bool Accept(Symbol sym, std::shared_ptr<Token>* token, bool push = false);
        bool Test(Symbol sym) const;
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
        bool ParseConstant();
        bool ParseParens();
        bool ParseList();
        bool ParseDict();
    };
}
