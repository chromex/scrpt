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
		this->ParseProgram();
		DumpAst(&_program);
		Assert(_currentNode == &_program, "Tracking node must be root");
		_lexer = nullptr;
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
		AssertNotNull(_lexer->Current());
		if (_lexer->Current()->GetSym() == sym)
		{
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
		if (this->ParseBlock(false)) return true;
		else if (this->ParseWhileLoop()) return true;
		else if (this->ParseDoLoop()) return true;
		else if (this->ParseForLoop()) return true;
		else if (this->ParseIf()) return true;
		else if (this->ParseBreak()) return true;
		else if (this->ParseReturn()) return true;
		else if (this->ParseSwitch()) return true;
		
		if (expect) throw CreateParseEx(ParseErr::StatementExpected, _lexer->Current());
		return false;
	}

	bool Parser::ParseExpression(bool expect)
	{
		// TODO: Parse
		// TODO: Throw on expect
		return true;
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
			// TODO: Is this necessary for parsing?
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

	CompilerException CreateExpectedSymEx(Symbol sym, Lexer* lexer)
	{
		AssertNotNull(lexer);
		return CreateParseEx(std::string("Expected symbol ") + SymbolToString(sym), ParseErr::UnexpectedSymbol, lexer->Current());
	}
}
