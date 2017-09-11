#include "../scrpt.h"

#define COMPONENTNAME "Parser"

namespace scrpt
{
    static CompilerException CreateExpectedSymEx(Symbol sym, Lexer* lexer);

    Parser::Parser()
        : _lexer(nullptr)
        , _currentNode(nullptr)
    {
        _currentNode = &_program;
    }

    void Parser::Consume(Lexer* lexer)
    {
        AssertNotNull(lexer);
        _lexer = lexer;
        _lexer->Advance();
        try
        {
            this->ParseProgram();
            Assert(_currentNode == &_program, "Tracking node must be root");
        }
        catch (...)
        {
            _lexer = nullptr;
            throw;
        }

        _lexer = nullptr;
    }

    AstNode* Parser::GetAst()
    {
        return &_program;
    }

    void Parser::DumpAst()
    {
        scrpt::DumpAst(&_program);
    }

    void Parser::PushNode()
    {
        _currentNode = _currentNode->AddChild(_lexer->Current());
    }

    void Parser::AddNode()
    {
        _currentNode->AddChild(_lexer->Current());
    }

    void Parser::PopNode()
    {
        _currentNode = _currentNode->GetParent();
    }

    bool Parser::Allow(Symbol sym)
    {
        if (this->Test(sym))
        {
            _lexer->Advance();
            return true;
        }

        return false;
    }

    bool Parser::Accept(Symbol sym)
    {
        if (this->Test(sym))
        {
            this->PushNode();
            _lexer->Advance();
            return true;
        }

        return false;
    }

    bool Parser::AcceptAndSwapOp(Symbol sym, std::vector<Symbol>& ltrMatch, bool postfix /* = false */)
    {
        std::shared_ptr<Token> token = _lexer->Current();
        if (this->Allow(sym))
        {
            // Check if the current node should be execute left-to-right against the newly accepted node. If
            // it is a match, pop the node so the swap below will set the current node as a child of the new
            // node, resulting in it being executed first
            Symbol pSym = _currentNode->GetSym();
            if (std::any_of(ltrMatch.begin(), ltrMatch.end(), [pSym](Symbol s) { return s == pSym; }))
            {
                this->PopNode();
            }

            _currentNode = _currentNode->SwapUnaryOp(token, postfix);
            return true;
        }

        return false;
    }

    bool Parser::Test(Symbol sym) const
    {
        AssertNotNull(_lexer->Current());
        return _lexer->Current()->GetSym() == sym;
    }

    bool Parser::Expect(Symbol sym)
    {
        if (this->Allow(sym))
        {
            return true;
        }

        throw CreateExpectedSymEx(sym, _lexer);
    }

    void Parser::ParseProgram()
    {
        while (this->Accept(Symbol::Func))
        {
            if (this->Test(Symbol::Ident))
            {
                this->AddNode();

                _lexer->Advance();
                this->Expect(Symbol::LParen);

                while (this->Test(Symbol::Ident))
                {
                    this->AddNode();

                    _lexer->Advance();
                    if (this->Allow(Symbol::Comma) && !this->Test(Symbol::Ident))
                    {
                        throw CreateExpectedSymEx(Symbol::Ident, _lexer);
                    }
                }

                this->Expect(Symbol::RParen);
                this->ParseBlock(true);
            }
            else
            {
                throw CreateExpectedSymEx(Symbol::Ident, _lexer);
            }

            this->PopNode();
        }

        this->Expect(Symbol::End);
    }

    bool Parser::ParseBlock(bool expect)
    {
        if (this->Accept(Symbol::LBracket))
        {
            while (this->ParseStatement(false)) {}
            this->Expect(Symbol::RBracket);

            this->PopNode();
            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::BlockExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseStatement(bool expect)
    {
        if (this->ParseWhileLoop()) return true;
        else if (this->ParseDoLoop()) return true;
        else if (this->ParseForLoop()) return true;
        else if (this->ParseIf()) return true;
        else if (this->ParseBreak()) return true;
        else if (this->ParseReturn()) return true;
        else if (this->ParseContinue()) return true;
        else if (this->ParseSwitch()) return true;
        else if (this->ParseBlock(false)) return true;
        else if (this->Allow(Symbol::SemiColon)) return true;
        else if (this->ParseExpression(false))
        {
            this->Expect(Symbol::SemiColon);
            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::StatementExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseExpression(bool expect)
    {
        static std::vector<Symbol> ltrMatch(0);

        if (this->ParseExOr(false))
        {
            if (this->AcceptAndSwapOp(Symbol::Assign, ltrMatch) ||
                this->AcceptAndSwapOp(Symbol::MultEq, ltrMatch) ||
                this->AcceptAndSwapOp(Symbol::DivEq, ltrMatch) ||
                this->AcceptAndSwapOp(Symbol::PlusEq, ltrMatch) ||
                this->AcceptAndSwapOp(Symbol::MinusEq, ltrMatch) ||
                this->AcceptAndSwapOp(Symbol::ModuloEq, ltrMatch) ||
                this->AcceptAndSwapOp(Symbol::ConcatEq, ltrMatch))
            {
                this->ParseExpression(true);
                this->PopNode();
            }

            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseExOr(bool expect)
    {
        static std::vector<Symbol> ltrMatch{ Symbol::Or };

        if (this->ParseExAnd(false))
        {
            if (this->AcceptAndSwapOp(Symbol::Or, ltrMatch))
            {
                AstNode* cn = _currentNode;
                this->ParseExOr(true);
                if (cn == _currentNode) this->PopNode();
            }

            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseExAnd(bool expect)
    {
        static std::vector<Symbol> ltrMatch{ Symbol::And };

        if (this->ParseExEquals(false))
        {
            if (this->AcceptAndSwapOp(Symbol::And, ltrMatch))
            {
                AstNode* cn = _currentNode;
                this->ParseExAnd(true);
                if (cn == _currentNode) this->PopNode();
            }

            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseExEquals(bool expect)
    {
        static std::vector<Symbol> ltrMatch{ Symbol::Eq, Symbol::NotEq };

        if (this->ParseExConcat(false))
        {
            if (this->AcceptAndSwapOp(Symbol::Eq, ltrMatch) ||
                this->AcceptAndSwapOp(Symbol::NotEq, ltrMatch))
            {
                AstNode* cn = _currentNode;
                this->ParseExEquals(true);
                if (cn == _currentNode) this->PopNode();
            }

            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseExConcat(bool expect)
    {
        static std::vector<Symbol> ltrMatch{ Symbol::Concat };

        if (this->ParseExCompare(false))
        {
            if (this->AcceptAndSwapOp(Symbol::Concat, ltrMatch))
            {
                AstNode* cn = _currentNode;
                this->ParseExConcat(true);
                if (cn == _currentNode) this->PopNode();
            }

            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseExCompare(bool expect)
    {
        static std::vector<Symbol> ltrMatch{ Symbol::LessThan, Symbol::GreaterThan, Symbol::LessThanEq, Symbol::GreaterThanEq };

        if (this->ParseExAdd(false))
        {
            if (this->AcceptAndSwapOp(Symbol::LessThan, ltrMatch) ||
                this->AcceptAndSwapOp(Symbol::GreaterThan, ltrMatch) ||
                this->AcceptAndSwapOp(Symbol::LessThanEq, ltrMatch) ||
                this->AcceptAndSwapOp(Symbol::GreaterThanEq, ltrMatch))
            {
                AstNode* cn = _currentNode;
                this->ParseExCompare(true);
                if (cn == _currentNode) this->PopNode();
            }

            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseExAdd(bool expect)
    {
        static std::vector<Symbol> ltrMatch{ Symbol::Plus, Symbol::Minus };

        if (this->ParseExMul(false))
        {
            if (this->AcceptAndSwapOp(Symbol::Plus, ltrMatch) ||
                this->AcceptAndSwapOp(Symbol::Minus, ltrMatch))
            {
                AstNode* cn = _currentNode;
                this->ParseExAdd(true);
                if (cn == _currentNode) this->PopNode();
            }

            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseExMul(bool expect)
    {
        static std::vector<Symbol> ltrMatch{ Symbol::Mult, Symbol::Div, Symbol::Modulo };

        if (this->ParseExPrefix(false))
        {
            if (this->AcceptAndSwapOp(Symbol::Mult, ltrMatch) ||
                this->AcceptAndSwapOp(Symbol::Div, ltrMatch) ||
                this->AcceptAndSwapOp(Symbol::Modulo, ltrMatch))
            {
                AstNode* cn = _currentNode;
                this->ParseExMul(true);
                if (cn == _currentNode) this->PopNode();
            }

            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseExPrefix(bool expect)
    {
        if (this->Accept(Symbol::Not) ||
            this->Accept(Symbol::PlusPlus) ||
            this->Accept(Symbol::MinusMinus) ||
            this->Accept(Symbol::Minus))
        {
            this->ParseExPrefix(true);
            this->PopNode();
            return true;
        }
        else if (this->ParseExPostfix(false))
        {
            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseExPostfix(bool expect)
    {
        static std::vector<Symbol> ltrMatch(0);

        if (this->ParseExTerm())
        {
			while (true)
			{
				if (this->AcceptAndSwapOp(Symbol::PlusPlus, ltrMatch, true) ||
					this->AcceptAndSwapOp(Symbol::MinusMinus, ltrMatch, true))
				{
                    this->PopNode();
				}
				else if (this->ParseCall() ||
                         this->ParseIndex() ||
                         this->ParseDotExpand())
				{}
				else
				{
					break;
				}
			}

            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseExTerm()
    {
        if (this->Accept(Symbol::Ident))
        {
            this->PopNode();
            return true;
        }

        return this->ParseConstant(false) || this->ParseParens();
    }

    bool Parser::ParseCall()
    {
        static std::vector<Symbol> ltrMatch(0);

        if (this->AcceptAndSwapOp(Symbol::LParen, ltrMatch, true))
        {
			bool expectExp = false;
			while (this->ParseExpression(expectExp))
			{
				expectExp = this->Allow(Symbol::Comma);
				if (!expectExp) break;
			}

			this->Expect(Symbol::RParen);
			this->PopNode();
			return true;
        }

        return false;
    }

    bool Parser::ParseIndex()
    {
        static std::vector<Symbol> ltrMatch(0);

        if (this->AcceptAndSwapOp(Symbol::LSquare, ltrMatch, true))
        {
            this->ParseExpression(true);
            this->Expect(Symbol::RSquare);
            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseDotExpand()
    {
        static std::vector<Symbol> ltrMatch(0);

        if (this->AcceptAndSwapOp(Symbol::Dot, ltrMatch, true) ||
            this->AcceptAndSwapOp(Symbol::Colon, ltrMatch, true))
        {
            if (!this->Accept(Symbol::Ident))
            {
                CreateExpectedSymEx(Symbol::Ident, _lexer);
            }

            this->PopNode();
            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseWhileLoop()
    {
        if (this->Accept(Symbol::While))
        {
            this->Expect(Symbol::LParen);
            this->ParseExpression(true);
            this->Expect(Symbol::RParen);
            this->ParseStatement(true);

            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseDoLoop()
    {
        if (this->Accept(Symbol::Do))
        {
            this->ParseStatement(true);

            this->Expect(Symbol::While);
            this->Expect(Symbol::LParen);
            this->ParseExpression(true);
            this->Expect(Symbol::RParen);
            this->Expect(Symbol::SemiColon);

            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseForLoop()
    {
        if (this->Accept(Symbol::For))
        {
            this->Expect(Symbol::LParen);
            if (!this->ParseExpression(false)) _currentNode->AddEmptyChild();
            this->Expect(Symbol::SemiColon);
            if (!this->ParseExpression(false)) _currentNode->AddEmptyChild();
            this->Expect(Symbol::SemiColon);
            if (!this->ParseExpression(false)) _currentNode->AddEmptyChild();
            this->Expect(Symbol::RParen);
            this->ParseStatement(true);

            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseIf()
    {
        if (this->Accept(Symbol::If))
        {
            this->Expect(Symbol::LParen);
            this->ParseExpression(true);
            this->Expect(Symbol::RParen);
            this->ParseStatement(true);

            while (this->Allow(Symbol::ElseIf))
            {
                this->Expect(Symbol::LParen);
                this->ParseExpression(true);
                this->Expect(Symbol::RParen);
                this->ParseStatement(true);
            }

            if (this->Allow(Symbol::Else))
            {
                this->ParseStatement(true);
            }

            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseBreak()
    {
        if (this->Accept(Symbol::Break))
        {
            this->Expect(Symbol::SemiColon);
            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseReturn()
    {
        if (this->Accept(Symbol::Return))
        {
            this->ParseExpression(false);
            this->Expect(Symbol::SemiColon);
            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseContinue()
    {
        if (this->Accept(Symbol::Continue))
        {
            this->Expect(Symbol::SemiColon);
            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseSwitch()
    {
        if (this->Accept(Symbol::Switch))
        {
            this->Expect(Symbol::LParen);
            this->ParseExpression(true);
            this->Expect(Symbol::RParen);
            this->Expect(Symbol::LBracket);

            while (this->ParseCase()) {}

            if (this->Accept(Symbol::Default))
            {
                this->Expect(Symbol::Colon);
                while (this->ParseStatement(false)) {}
                this->PopNode();
            }

            while (this->ParseCase()) {}

            this->Expect(Symbol::RBracket);

            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseCase()
    {
        if (this->Accept(Symbol::Case))
        {
            // TODO: This should be more restrictive than constant -- maps and lists shouldn't
            // be allowed
            this->ParseConstant(true);
            this->Expect(Symbol::Colon);
            while (this->ParseStatement(false)) {}
            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseConstant(bool expect)
    {
        if (this->Accept(Symbol::True) ||
            this->Accept(Symbol::False) ||
            this->Accept(Symbol::Int) ||
            this->Accept(Symbol::Float) ||
            this->Accept(Symbol::Terminal))
        {
            _currentNode->SetConstant();
            this->PopNode();
            return true;
        }

        if (this->ParseList() ||
            this->ParseDict())
        {
            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseParens()
    {
        if (this->Accept(Symbol::LParen))
        {
            this->ParseExpression(true);
            this->Expect(Symbol::RParen);
            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseList()
    {
        if (this->Accept(Symbol::LSquare))
        {
            _currentNode->SetConstant();
            bool allowMore = true;
            while (allowMore && this->ParseExpression(false))
            {
                allowMore = this->Allow(Symbol::Comma);
            }
            this->Expect(Symbol::RSquare);

            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseDict()
    {
        if (this->Accept(Symbol::LBracket))
        {
            _currentNode->SetConstant();
            bool allowMore = true;
            while (allowMore && this->ParseExTerm())
            {
                this->Expect(Symbol::Colon);
                this->ParseExpression(true);
                allowMore = this->Allow(Symbol::Comma);
            }
            this->Expect(Symbol::RBracket);

            this->PopNode();
            return true;
        }

        return false;
    }

    CompilerException CreateExpectedSymEx(Symbol sym, Lexer* lexer)
    {
        AssertNotNull(lexer);
        return CreateParseEx(std::string("Expected symbol ") + SymbolToString(sym), ParseErr::UnexpectedSymbol, lexer->Current());
    }

    const char* ParseErrToString(ParseErr err)
    {
        switch (err)
        {
            ENUM_CASE_TO_STRING(ParseErr::NoError);
            ENUM_CASE_TO_STRING(ParseErr::UnexpectedSymbol);
            ENUM_CASE_TO_STRING(ParseErr::BlockExpected);
            ENUM_CASE_TO_STRING(ParseErr::ExpressionExpected);
            ENUM_CASE_TO_STRING(ParseErr::StatementExpected);

        default:
            AssertFail("Missing case for ParseErr");
        }

        return nullptr;
    }
}
