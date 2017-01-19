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
        if (this->ParseEx0(false))
        {
            std::shared_ptr<Token> token;
            if (this->Accept(Symbol::Assign, &token) ||
                this->Accept(Symbol::MultEq, &token) ||
                this->Accept(Symbol::DivEq, &token) ||
                this->Accept(Symbol::PlusEq, &token) ||
                this->Accept(Symbol::MinusEq, &token) ||
                this->Accept(Symbol::ModuloEq, &token))
            {
                this->ParseExpression(true);
                _currentNode->CondenseBinaryOp(token);
            }

            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseEx0(bool expect)
    {
        if (this->ParseEx1(false))
        {
            std::shared_ptr<Token> token;
            if (this->Accept(Symbol::Or, &token))
            {
                this->ParseEx0(true);
                _currentNode->CondenseBinaryOp(token);
            }

            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseEx1(bool expect)
    {
        if (this->ParseEx2(false))
        {
            std::shared_ptr<Token> token;
            if (this->Accept(Symbol::And, &token))
            {
                this->ParseEx1(true);
                _currentNode->CondenseBinaryOp(token);
            }

            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseEx2(bool expect)
    {
        if (this->ParseEx3(false))
        {
            std::shared_ptr<Token> token;
            if (this->Accept(Symbol::Eq, &token) ||
                this->Accept(Symbol::NotEq, &token))
            {
                this->ParseEx2(true);
                _currentNode->CondenseBinaryOp(token);
            }

            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseEx3(bool expect)
    {
        if (this->ParseEx4(false))
        {
            std::shared_ptr<Token> token;
            if (this->Accept(Symbol::LessThan, &token) ||
                this->Accept(Symbol::GreaterThan, &token) ||
                this->Accept(Symbol::LessThanEq, &token) ||
                this->Accept(Symbol::GreaterThanEq, &token))
            {
                this->ParseEx3(true);
                _currentNode->CondenseBinaryOp(token);
            }

            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseEx4(bool expect)
    {
        if (this->ParseEx5(false))
        {
            std::shared_ptr<Token> token;
            if (this->Accept(Symbol::Plus, &token) ||
                this->Accept(Symbol::Minus, &token))
            {
                this->ParseEx4(true);
                _currentNode->CondenseBinaryOp(token);
            }

            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseEx5(bool expect)
    {
        if (this->ParseEx6(false))
        {
            std::shared_ptr<Token> token;
            if (this->Accept(Symbol::Mult, &token) ||
                this->Accept(Symbol::Div, &token) ||
                this->Accept(Symbol::Modulo, &token))
            {
                this->ParseEx5(true);
                _currentNode->CondenseBinaryOp(token);
            }

            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseEx6(bool expect)
    {
        if (this->Accept(Symbol::Not, true) ||
            this->Accept(Symbol::PlusPlus, true) ||
            this->Accept(Symbol::MinusMinus, true) ||
            this->Accept(Symbol::Minus, true))
        {
            this->ParseEx6(true);
            this->PopNode();
            return true;
        }
        else if (this->ParseEx7(false))
        {
            return true;
        }

        if (expect) throw CreateParseEx(ParseErr::ExpressionExpected, _lexer->Current());
        return false;
    }

    bool Parser::ParseEx7(bool expect)
    {
        if (this->ParseEx8())
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

    bool Parser::ParseEx8()
    {
        if (this->Accept(Symbol::Ident, true))
        {
            this->PopNode();
            return true;
        }

        return this->ParseConstant() || this->ParseParens();
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
            this->ParseExpression(false);
            if (this->Accept(Symbol::Colon, true))
            {
                this->PopNode();
                this->ParseExpression(false);
            }

            this->Expect(Symbol::RSquare);
            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseDotExpand()
    {
        std::shared_ptr<Token> token;
        if (this->Accept(Symbol::Dot, &token))
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
            this->ParseExpression(false);
            this->Expect(Symbol::SemiColon);
            this->ParseExpression(false);
            this->Expect(Symbol::SemiColon);
            this->ParseExpression(false);
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

            while (this->Accept(Symbol::ElseIf, true))
            {
                this->Expect(Symbol::LParen);
                this->ParseExpression(true);
                this->Expect(Symbol::RParen);
                this->ParseStatement(true);

                this->PopNode();
            }

            if (this->Accept(Symbol::Else, true))
            {
                this->ParseStatement(true);
                this->PopNode();
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
            this->ParseExpression(true);
            this->Expect(Symbol::Colon);
            while (this->ParseStatement(false)) {}
            this->PopNode();
            return true;
        }

        return false;
    }

    bool Parser::ParseConstant()
    {
        if (this->Accept(Symbol::True, true) ||
            this->Accept(Symbol::False, true) ||
            this->Accept(Symbol::Number, true) ||
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
            while (allowMore && this->ParseExpression(false))
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
}
