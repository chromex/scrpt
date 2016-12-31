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
				TraceInfo("Func found with ident: " << _lexer->GetIdent());

				_lexer->Advance();
				_lexer->Expect(Symbol::LParen);

				while (_lexer->Test(Symbol::Ident))
				{
					TraceInfo("Found param: " << _lexer->GetIdent());

					_lexer->Advance();
					if (_lexer->Accept(Symbol::Comma))
					{
						// TODO: Error if the current token isn't an ident
					}
				}

				_lexer->Expect(Symbol::RParen);

				this->ParseBlock();
			}
			else
			{
				// TODO: Throw no ident
			}
		}

		_lexer->Expect(Symbol::End);
	}

	void Parser::ParseBlock()
	{
		_lexer->Expect(Symbol::LBracket);
		while (this->ParseStatement()) {}
		_lexer->Expect(Symbol::RBracket);
	}

	bool Parser::ParseStatement()
	{
		return false;
	}
}
