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

    bool Parser::Accept(Symbol sym, bool push)
    {
        return this->Accept(sym, nullptr, push);
    }

    bool Parser::Accept(Symbol sym, std::shared_ptr<Token>* token, bool push)
    {
        AssertNotNull(_lexer->Current());
        if (_lexer->Current()->GetSym() == sym)
        {
            if (token != nullptr) *token = _lexer->Current();
            if (push) this->PushNode();
            _lexer->Advance();
            return true;
        }

        return false;
    }

    bool Parser::AcceptAndSwap(Symbol sym, bool postfix /* = false */)
    {
        std::shared_ptr<Token> token;
        if (this->Accept(sym, &token))
        {
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
        AssertNotNull(_lexer->Current());
        if (this->Accept(sym))
        {
            return true;
        }

        throw CreateExpectedSymEx(sym, _lexer);
    }

    void Parser::ParseProgram()
    {
        while (this->Accept(Symbol::Func, true))
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
                    if (this->Accept(Symbol::Comma) && !this->Test(Symbol::Ident))
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
        if (this->Accept(Symbol::LBracket, true))
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
        else if (this->Accept(Symbol::SemiColon)) return true;
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
            if (this->AcceptAndSwap(Symbol::Assign) ||
                this->AcceptAndSwap(Symbol::MultEq) ||
                this->AcceptAndSwap(Symbol::DivEq) ||
                this->AcceptAndSwap(Symbol::PlusEq) ||
                this->AcceptAndSwap(Symbol::MinusEq) ||
                this->AcceptAndSwap(Symbol::ModuloEq) ||
                this->AcceptAndSwap(Symbol::ConcatEq))
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
            if (this->AcceptAndSwap(Symbol::Or))
            {
                this->ParseExOr(true);
                this->PopNode();
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
            if (this->AcceptAndSwap(Symbol::And))
            {
                this->ParseExAnd(true);
                this->PopNode();
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
            if (this->AcceptAndSwap(Symbol::Eq) ||
                this->AcceptAndSwap(Symbol::NotEq))
            {
                this->ParseExEquals(true);
                this->PopNode();
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
            if (this->AcceptAndSwap(Symbol::Concat))
            {
                this->ParseExConcat(true);
                this->PopNode();
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
            if (this->AcceptAndSwap(Symbol::LessThan) ||
                this->AcceptAndSwap(Symbol::GreaterThan) ||
                this->AcceptAndSwap(Symbol::LessThanEq) ||
                this->AcceptAndSwap(Symbol::GreaterThanEq))
            {
                this->ParseExCompare(true);
                this->PopNode();
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
            if (this->AcceptAndSwap(Symbol::Plus) ||
                this->AcceptAndSwap(Symbol::Minus))
            {
                this->ParseExAdd(true);
                this->PopNode();
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
            if (this->AcceptAndSwap(Symbol::Mult) ||
                this->AcceptAndSwap(Symbol::Div) ||
                this->AcceptAndSwap(Symbol::Modulo))
            {
                this->ParseExMul(true);
                this->PopNode();
            }

            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseExPrefix(bool expect)
    {
        if (this->Accept(Symbol::Not, true) ||
            this->Accept(Symbol::PlusPlus, true) ||
            this->Accept(Symbol::MinusMinus, true) ||
            this->Accept(Symbol::Minus, true))
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
        if (this->ParseExTerm())
        {
            std::shared_ptr<Token> token;
			while (true)
			{
				if (this->Accept(Symbol::PlusPlus, &token) ||
					this->Accept(Symbol::MinusMinus, &token))
				{
					_currentNode->SwapUnaryOp(token, true);
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
        if (this->Accept(Symbol::Ident, true))
        {
            this->PopNode();
            return true;
        }

        return this->ParseConstant(false) || this->ParseParens();
    }

    bool Parser::ParseCall()
    {
        std::shared_ptr<Token> token;
        if (this->Accept(Symbol::LParen, &token))
        {
			_currentNode = _currentNode->SwapUnaryOp(token, true);
			bool expectExp = false;
			while (this->ParseExpression(expectExp))
			{
				expectExp = this->Accept(Symbol::Comma);
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
        std::shared_ptr<Token> token;
        if (this->Accept(Symbol::LSquare, &token))
        {
            _currentNode = _currentNode->SwapUnaryOp(token, true);
            this->ParseExpression(true);
            this->Expect(Symbol::RSquare);
            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseDotExpand()
    {
        std::shared_ptr<Token> token;
        if (this->Accept(Symbol::Dot, &token) ||
            this->Accept(Symbol::Colon, &token))
        {
            _currentNode = _currentNode->SwapUnaryOp(token, true);
            if (!this->Accept(Symbol::Ident, true))
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
        if (this->Accept(Symbol::While, true))
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
        if (this->Accept(Symbol::Do, true))
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
        if (this->Accept(Symbol::For, true))
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
        if (this->Accept(Symbol::If, true))
        {
            this->Expect(Symbol::LParen);
            this->ParseExpression(true);
            this->Expect(Symbol::RParen);
            this->ParseStatement(true);

            while (this->Accept(Symbol::ElseIf))
            {
                this->Expect(Symbol::LParen);
                this->ParseExpression(true);
                this->Expect(Symbol::RParen);
                this->ParseStatement(true);
            }

            if (this->Accept(Symbol::Else))
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
        if (this->Accept(Symbol::Break, true))
        {
            this->Expect(Symbol::SemiColon);
            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseReturn()
    {
        if (this->Accept(Symbol::Return, true))
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
        if (this->Accept(Symbol::Continue, true))
        {
            this->Expect(Symbol::SemiColon);
            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseSwitch()
    {
        if (this->Accept(Symbol::Switch, true))
        {
            this->Expect(Symbol::LParen);
            this->ParseExpression(true);
            this->Expect(Symbol::RParen);
            this->Expect(Symbol::LBracket);

            while (this->ParseCase()) {}

            if (this->Accept(Symbol::Default, true))
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
        if (this->Accept(Symbol::Case, true))
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
        if (this->Accept(Symbol::True, true) ||
            this->Accept(Symbol::False, true) ||
            this->Accept(Symbol::Int, true) ||
            this->Accept(Symbol::Float, true) ||
            this->Accept(Symbol::Terminal, true))
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
            return true;
        }

        return false;
    }

    bool Parser::ParseList()
    {
        if (this->Accept(Symbol::LSquare, true))
        {
            _currentNode->SetConstant();
            bool allowMore = true;
            while (allowMore && this->ParseExpression(false))
            {
                allowMore = this->Accept(Symbol::Comma);
            }
            this->Expect(Symbol::RSquare);

            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseDict()
    {
        if (this->Accept(Symbol::LBracket, true))
        {
            _currentNode->SetConstant();
            bool allowMore = true;
            while (allowMore && this->ParseExTerm())
            {
                this->Expect(Symbol::Colon);
                this->ParseExpression(true);
                allowMore = this->Accept(Symbol::Comma);
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
