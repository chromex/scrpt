#include "../scrpt.h"

#define COMPONENTNAME "Parser"

namespace scrpt
{
	static CompilerException CreateExpectedSymEx(Symbol sym, Lexer* lexer);

	Parser::Parser()
		: _lexer(nullptr)
	{
	}

	void Parser::Consume(Lexer* lexer)
	{
		AssertNotNull(lexer);
		_lexer = lexer;
		_lexer->Advance();
		this->ParseProgram();
		_lexer = nullptr;
	}

	bool Parser::Accept(Symbol sym)
	{
		AssertNotNull(_lexer->Current());
		if (_lexer->Current()->GetSym() == sym)
		{
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
		while (this->Accept(Symbol::Func))
		{
			if (this->Test(Symbol::Ident))
			{
				TraceInfo("Func found with ident: " << _lexer->Current()->GetString());

				_lexer->Advance();
				this->Expect(Symbol::LParen);

				while (this->Test(Symbol::Ident))
				{
					TraceInfo("Found param: " << _lexer->Current()->GetString());

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
		}

		this->Expect(Symbol::End);
	}

	bool Parser::ParseBlock(bool expect)
	{
		if (this->Accept(Symbol::LBracket))
		{
			TraceInfo("Parsing block");
			while (this->ParseStatement(false)) {}
			this->Expect(Symbol::RBracket);
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
		if (this->Accept(Symbol::While))
		{
			TraceInfo("Parsing while");

			this->Expect(Symbol::LParen);
			this->ParseExpression(true);
			this->Expect(Symbol::RParen); 
			this->ParseStatement(true);

			return true;
		}

		return false;
	}

	bool Parser::ParseDoLoop()
	{
		if (this->Accept(Symbol::Do))
		{
			TraceInfo("Parsing do");

			this->Expect(Symbol::LParen);
			this->ParseExpression(true);
			this->Expect(Symbol::RParen);
			this->ParseStatement(true);

			return true;
		}

		return false;
	}

	bool Parser::ParseForLoop()
	{
		if (this->Accept(Symbol::For))
		{
			TraceInfo("Parsing for");

			this->Expect(Symbol::LParen);
			this->ParseExpression(false);
			this->Expect(Symbol::SemiColon);
			this->ParseExpression(false);
			this->Expect(Symbol::SemiColon);
			this->ParseExpression(false);
			this->Expect(Symbol::RParen);
			this->ParseStatement(true);

			return true;
		}

		return false;
	}

	bool Parser::ParseIf()
	{
		if (this->Accept(Symbol::If))
		{
			TraceInfo("Parsing if");

			this->Expect(Symbol::LParen);
			this->ParseExpression(true);
			this->Expect(Symbol::RParen);
			this->ParseStatement(true);

			while (this->Accept(Symbol::Else))
			{
				TraceInfo("Parsing else");
				this->ParseStatement(true);
			}

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
