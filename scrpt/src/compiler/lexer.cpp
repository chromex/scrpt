#include "../scrpt.h"
#include <ctype.h>

#define COMPONENTNAME "Lexer"

// Update grammer: break, default
// Test code needs to cover all symbols and symbol cases
// Lexer errors

namespace scrpt
{
	Lexer::Lexer(std::unique_ptr<char[]> sourceData)
		: _sourceData(nullptr)
		, _location(nullptr)
		, _ident(nullptr)
		, _term(nullptr)
		, _number(nan(nullptr))
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

	size_t Lexer::GetLine() const { return _line; }
	const char* Lexer::GetIdent() const { return _ident.get(); }
	const char* Lexer::GetTerm() const { return _term.get(); }
	double Lexer::GetNumber() const { return _number; }

	void Lexer::Advance()
	{
		// Reset
		_token = Symbol::Unknown;
		_ident.reset();
		_term.reset();
		_number = nan(nullptr);

		// Burn whitespace
		this->ConsumeWhitespace();

		// Parse
		char c = *_location;
		char cn = *(_location + 1);
		if (c == '\0')
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
			else if (strcmp(ident.get(), "break") == 0)		_token = Symbol::Break;
			else if (strcmp(ident.get(), "false") == 0)		_token = Symbol::False;
			else if (strcmp(ident.get(), "while") == 0)		_token = Symbol::While;
			else if (strcmp(ident.get(), "return") == 0)	_token = Symbol::Return;
			else if (strcmp(ident.get(), "switch") == 0)	_token = Symbol::Switch;
			else if (strcmp(ident.get(), "default") == 0)	_token = Symbol::Default;
			else
			{
				_token = Symbol::Ident;
				_ident.swap(ident);
			}

			_location += len;
		}
		else if (isdigit(c))
		{
			size_t rawLen = 0;
			_number = this->GetNumber(_location, &rawLen);
			if (isnan(_number))
			{
				AssertFail("Illegal number");
			}
			_token = Symbol::Number;
			_location += rawLen;
		}
		else if (c == '"')
		{
			size_t rawLen = this->GetRawTermLength(_location);
			// TODO: check rawLen
			_term = this->GetTerm(_location);
			_token = Symbol::Terminal;
			_location += rawLen;
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
		#define DOUBLE_COMPLEX_SYM(C, V, SymC, SymV) else if (c == C && cn == V) { _token = SymV; _location += 2; } else if (c == C) { _token = SymC; _location += 1; }
		DOUBLE_COMPLEX_SYM('=', '=', Symbol::Assign, Symbol::Eq)
		DOUBLE_COMPLEX_SYM('-', '=', Symbol::Minus, Symbol::MinusEq)
		DOUBLE_COMPLEX_SYM('+', '=', Symbol::Plus, Symbol::PlusEq)
		DOUBLE_COMPLEX_SYM('*', '=', Symbol::Mult, Symbol::MultEq)
		DOUBLE_COMPLEX_SYM('/', '=', Symbol::Div, Symbol::DivEq)
		DOUBLE_COMPLEX_SYM('!', '=', Symbol::Not, Symbol::NotEq)
		DOUBLE_COMPLEX_SYM('<', '=', Symbol::LessThan, Symbol::LessThanEq)
		DOUBLE_COMPLEX_SYM('>', '=', Symbol::GreaterThan, Symbol::GreaterThanEq)
	}

	void Lexer::ConsumeWhitespace()
	{
		// TODO: Track line#
		const char* pos = nullptr;
		do
		{
			pos = _location;

			while (*_location != '\0' && isspace(*_location))
			{
				if (*_location == '\n') ++_line;
				++_location;
			}

			if (*_location == '/' && *(_location + 1) == '/')
			{
				_location += 2;
				while (*_location != '\0' && *_location != '\n') ++_location;
			}
		} while (pos != _location);
	}

	bool Lexer::IsIdentCharacter(char c, bool firstChar) const
	{
		return
			(c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c == '_') ||
			(!firstChar && c >= '0' && c <= '9');
	}

	size_t Lexer::GetIdentLength(const char* c) const
	{
		unsigned int len = 0;
		while (*c != '\0' && this->IsIdentCharacter(*c++, len == 0)) ++len;
		return len;
	}

	std::unique_ptr<char[]> Lexer::GetIdent(const char* c) const
	{
		size_t len = this->GetIdentLength(c);
		Assert(len > 0, "GetIdent has zero length in ident lexer");

		std::unique_ptr<char[]> ident(new char[len+1]);
		strncpy(ident.get(), c, len);
		ident.get()[len] = '\0';

		return ident;
	}

	bool Lexer::IsEndOfTerm(const char* c) const
	{
		return *c == '\0' || (*c == '"' && *(c - 1) != '\\');
	}

	size_t Lexer::GetRawTermLength(const char* c) const
	{
		Assert(*c == '"', "Term must start with a quote");
		const char* start = c;

		do { ++c; } while (!this->IsEndOfTerm(c));
		if (*c == '\0') return -1;

		return c - start + 1;
	}

	size_t Lexer::GetTermLength(const char* c) const
	{
		Assert(*c == '"', "Term must start with a quote");
		const char* start = c++;
		size_t len = 0;

		while (!this->IsEndOfTerm(c))
		{
			++len;
			if (*c++ == '\\')
			{
				++c;
			}
		}

		return len;
	}

	std::unique_ptr<char[]> Lexer::GetTerm(const char* c) const
	{
		size_t len = this->GetTermLength(c);
		std::unique_ptr<char[]> term(new char[len+1]);
		term.get()[len] = '\0';
		size_t pos = 0;
		while (!this->IsEndOfTerm(++c))
		{
			if (*c == '\\')
			{
				++c;
				switch (*c)
				{
				case 't': term.get()[pos] = '\t'; break;
				case 'n': term.get()[pos] = '\n'; break;
				case '\\': term.get()[pos] = '\\'; break;
				case '"': term.get()[pos] = '\"'; break;
				default: 
					// TODO: Unknown escape in term
					return nullptr;
				}
			}
			else
			{
				term.get()[pos] = *c;
			}

			++pos;
		}
		
		return term;
	}

	double Lexer::GetNumber(const char* c, size_t* rawLen) const
	{
		AssertNotNull(c);
		AssertNotNull(rawLen);

		const char* start = c;
		bool parseDenom = false;
		double num = 0;
		
		// Parse numerator
		if (*c != '0')
		{
			do
			{
				num *= 10.0;
				num += *c - '0';
			} while (*++c != '\0' && isdigit(*c));

			parseDenom = *c == '.';
		}
		else // *c == '0'
		{
			if (*++c != '.') { *rawLen = 1; return 0; }
			parseDenom = true;
		}

		// Parse denominator
		if (parseDenom)
		{
			if (!isdigit(*++c))
			{
				return nan(nullptr);
			}

			unsigned int div = 1;
			do
			{
				div *= 10;
				num += (*c - '0') / (double)div;
			} while (*++c != '\0' && isdigit(*c));
		}

		*rawLen = c - start;
		return num;
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
			SYMBOL_CASE_STRING(Symbol::Break)
			SYMBOL_CASE_STRING(Symbol::Default)
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
			SYMBOL_CASE_STRING(Symbol::End)

		default:
			AssertFail("Missing case for symbol");
		}

		return nullptr;
	}
}