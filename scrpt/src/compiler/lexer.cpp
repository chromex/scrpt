#include "../scrpt.h"
#include <ctype.h>

#define COMPONENTNAME "Lexer"

namespace scrpt
{
	Lexer::Lexer(std::unique_ptr<char[]> sourceData)
		: _sourceData(nullptr)
		, _location(nullptr)
		, _ident(nullptr)
		, _line(1)
		, _token(Symbol::Start)
	{
		_sourceData.swap(sourceData);
		AssertNotNull(_sourceData.get());

		_location = _sourceData.get();
	}

	Symbol Lexer::Next()
	{
		this->Advance();

		return _token;
	}

	const char* Lexer::GetIdent() const
	{
		return _ident.get();
	}

	void Lexer::Advance()
	{
		// Reset
		_token = Symbol::Unknown;
		_ident.reset();

		// Burn whitespace
		this->ConsumeWhitespace();

		// Parse
		char c = *_location;
		if (c == NULL)
		{
			_token = Symbol::End;
		}
		else if (this->IsIdentCharacter(c, true))
		{
			std::unique_ptr<char[]> ident = this->GetIdent(_location);
			size_t len = strlen(ident.get());

			if (strcmp(ident.get(), "do") == 0)				_token = Symbol::Do;
			else if (strcmp(ident.get(), "for") == 0)		_token = Symbol::For;
			else if (strcmp(ident.get(), "case") == 0)		_token = Symbol::Case;
			else if (strcmp(ident.get(), "func") == 0)		_token = Symbol::Func;
			else if (strcmp(ident.get(), "true") == 0)		_token = Symbol::True;
			else if (strcmp(ident.get(), "false") == 0)		_token = Symbol::False;
			else if (strcmp(ident.get(), "while") == 0)		_token = Symbol::While;
			else if (strcmp(ident.get(), "return") == 0)	_token = Symbol::Return;
			else if (strcmp(ident.get(), "switch") == 0)	_token = Symbol::Switch;
			else
			{
				_token = Symbol::Ident;
				_ident.swap(ident);
			}

			_location += len;
		}
		#define SINGLE_CHAR_SYM(C, Sym) else if (c == C) { _token = Sym; _location += 1; }
		SINGLE_CHAR_SYM('(', Symbol::LParen)
		SINGLE_CHAR_SYM(')', Symbol::RParen)
		SINGLE_CHAR_SYM('{', Symbol::LBracket)
		SINGLE_CHAR_SYM('}', Symbol::RBracket)
		SINGLE_CHAR_SYM('[', Symbol::LSquare)
		SINGLE_CHAR_SYM(']', Symbol::RSquare)
		SINGLE_CHAR_SYM(';', Symbol::SemiColon)
		SINGLE_CHAR_SYM(':', Symbol::Colon)
	}

	void Lexer::ConsumeWhitespace()
	{
		// TODO: Comment support
		// TODO: Track line#
		while (*_location != '\0' && isspace(*_location))
		{
			++_location;
		}
	}

	bool Lexer::IsIdentCharacter(char c, bool firstChar) const
	{
		return
			(c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c == '_') ||
			(!firstChar && c >= '0' && c <= '9');
	}

	unsigned int Lexer::GetIdentLength(const char* c) const
	{
		int len = 0;
		while (*c != '\0' && this->IsIdentCharacter(*c++, len == 0)) ++len;
		return len;
	}

	std::unique_ptr<char[]> Lexer::GetIdent(const char* c) const
	{
		unsigned int len = this->GetIdentLength(c);
		Assert(len > 0, "GetIdent has zero length in ident lexer");

		std::unique_ptr<char[]> ident(new char[len+1]);
		strncpy(ident.get(), c, len);
		ident.get()[len] = '\0';

		return ident;
	}

	const char* SymbolToString(Symbol sym)
	{
		#define SYMBOL_CASE_STRING(s) case s: return #s;
		switch (sym)
		{
			SYMBOL_CASE_STRING(Symbol::Unknown)
			SYMBOL_CASE_STRING(Symbol::Start)
			SYMBOL_CASE_STRING(Symbol::LParen)
			SYMBOL_CASE_STRING(Symbol::RParen)
			SYMBOL_CASE_STRING(Symbol::LBracket)
			SYMBOL_CASE_STRING(Symbol::RBracket)
			SYMBOL_CASE_STRING(Symbol::LSquare)
			SYMBOL_CASE_STRING(Symbol::RSquare)
			SYMBOL_CASE_STRING(Symbol::Number)
			SYMBOL_CASE_STRING(Symbol::Terminal)
			SYMBOL_CASE_STRING(Symbol::Ident)
			SYMBOL_CASE_STRING(Symbol::True)
			SYMBOL_CASE_STRING(Symbol::False)
			SYMBOL_CASE_STRING(Symbol::Colon)
			SYMBOL_CASE_STRING(Symbol::Do)
			SYMBOL_CASE_STRING(Symbol::While)
			SYMBOL_CASE_STRING(Symbol::For)
			SYMBOL_CASE_STRING(Symbol::Switch)
			SYMBOL_CASE_STRING(Symbol::Case)
			SYMBOL_CASE_STRING(Symbol::Return)
			SYMBOL_CASE_STRING(Symbol::Eq)
			SYMBOL_CASE_STRING(Symbol::Assign)
			SYMBOL_CASE_STRING(Symbol::NotEq)
			SYMBOL_CASE_STRING(Symbol::LessThan)
			SYMBOL_CASE_STRING(Symbol::GreaterThan)
			SYMBOL_CASE_STRING(Symbol::Plus)
			SYMBOL_CASE_STRING(Symbol::Minus)
			SYMBOL_CASE_STRING(Symbol::Mult)
			SYMBOL_CASE_STRING(Symbol::Div)
			SYMBOL_CASE_STRING(Symbol::PlusEq)
			SYMBOL_CASE_STRING(Symbol::MinusEq)
			SYMBOL_CASE_STRING(Symbol::MultEq)
			SYMBOL_CASE_STRING(Symbol::DivEq)
			SYMBOL_CASE_STRING(Symbol::Not)
			SYMBOL_CASE_STRING(Symbol::SemiColon)
			SYMBOL_CASE_STRING(Symbol::Func)

		default:
			AssertFail("Missing case for symbol");
		}

		return nullptr;
	}
}