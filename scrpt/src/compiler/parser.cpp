#include "../scrpt.h"

#define COMPONENTNAME "Parser"

namespace scrpt
{
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

	// TODO: Expect should throw with sufficient detail -- may need data provided to it for context
	// E.g.: "Expected Symbol::End but found Symbol::Ident\n<Code>\nPtr

	void Parser::ParseProgram()
	{
		while (_lexer->Accept(Symbol::Func))
		{
			if (_lexer->Test(Symbol::Ident))
			{
				TraceInfo("Func found with ident: " << _lexer->Current()->GetString());

				_lexer->Advance();
				_lexer->Expect(Symbol::LParen);

				while (_lexer->Test(Symbol::Ident))
				{
					TraceInfo("Found param: " << _lexer->Current()->GetString());

					_lexer->Advance();
					if (_lexer->Accept(Symbol::Comma))
					{
						// TODO: IdentExpected
					}
				}

				_lexer->Expect(Symbol::RParen);

				if (!this->ParseBlock())
				{
					// TODO: Throw no block
				}
			}
			else
			{
				// TODO: Throw no ident
			}
		}

		_lexer->Expect(Symbol::End);
	}

	bool Parser::ParseBlock()
	{
		if (_lexer->Accept(Symbol::LBracket))
		{
			TraceInfo("Parsing block");
			while (this->ParseStatement()) {}
			_lexer->Expect(Symbol::RBracket);
			return true;
		}

		return false;
	}

	bool Parser::ParseStatement()
	{
		if (this->ParseBlock()) return true;
		else if (this->ParseWhileLoop()) return true;
		else if (this->ParseDoLoop()) return true;
		else if (this->ParseForLoop()) return true;
		else if (this->ParseIf()) return true;
		
		return false;
	}

	bool Parser::ParseExpression()
	{
		// TODO
		return true;
	}

	bool Parser::ParseWhileLoop()
	{
		if (_lexer->Accept(Symbol::While))
		{
			TraceInfo("Parsing while");

			_lexer->Expect(Symbol::LParen);
			if (!this->ParseExpression())
			{
				// TODO: Throw no expression 
			}
			_lexer->Expect(Symbol::RParen);

			if (!this->ParseStatement())
			{
				// TODO: Throw statement expected
			}

			return true;
		}

		return false;
	}

	bool Parser::ParseDoLoop()
	{
		if (_lexer->Accept(Symbol::Do))
		{
			TraceInfo("Parsing do");

			_lexer->Expect(Symbol::LParen);
			if (!this->ParseExpression())
			{
				// TODO: Throw no expression
			}
			_lexer->Expect(Symbol::RParen);

			if (!this->ParseStatement())
			{
				// TODO: Throw statement expected
			}

			return true;
		}

		return false;
	}

	bool Parser::ParseForLoop()
	{
		// TODO
		return false;
	}

	bool Parser::ParseIf()
	{
		if (_lexer->Accept(Symbol::If))
		{
			TraceInfo("Parsing if");

			_lexer->Expect(Symbol::LParen);
			if (!this->ParseExpression())
			{
				// TODO: Throw no expression
			}
			_lexer->Expect(Symbol::RParen);

			if (!this->ParseStatement())
			{
				// TODO: Throw statement expected
			}

			while (_lexer->Accept(Symbol::Else))
			{
				TraceInfo("Parsing else");
				if (!this->ParseStatement())
				{
					// TODO: Throw statement expected
				}
			}

			return true;
		}

		return false;
	}
}
