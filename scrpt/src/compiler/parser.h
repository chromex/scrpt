#pragma once

namespace scrpt
{
    class Parser
    {
    public:
        Parser();

        void Consume(Lexer* lexer);
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
        bool ParseEx1(bool expect);
        bool ParseEx2(bool expect);
        bool ParseEx3(bool expect);
        bool ParseEx4(bool expect);
        bool ParseEx5(bool expect);
        bool ParseEx6(bool expect);
        bool ParseWhileLoop();
        bool ParseDoLoop();
        bool ParseForLoop();
        bool ParseIf();
        bool ParseBreak();
        bool ParseReturn();
        bool ParseContinue();
        bool ParseSwitch();
        bool ParseCase();
        bool ParseTerm();
        bool ParseConstant();
    };
}
