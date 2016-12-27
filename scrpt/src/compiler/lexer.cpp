#include "../scrpt.h"
#include <ctype.h>

#define COMPONENTNAME "Lexer"

namespace scrpt
{
	Lexer::Lexer(std::unique_ptr<char[]> sourceData)
		: _sourceData(nullptr)
		, _location(nullptr)
		, _lineStart(nullptr)
		, _ident(nullptr)
		, _term(nullptr)
		, _number(nan(nullptr))
		, _line(1)
		, _position(1)
		, _token(Symbol::Start)
	{
		_sourceData.swap(sourceData);
		AssertNotNull(_sourceData.get());

		_lineStart = _location = _sourceData.get();
	}

	Symbol Lexer::Next()
	{
		this->Advance();
		return _token;
	}

	size_t Lexer::GetLine() const { return _line; }
	size_t Lexer::GetPosition() const { return _position; }
	const char* Lexer::GetIdent() const { return _ident.get(); }
	const char* Lexer::GetTerm() const { return _term.get(); }
	double Lexer::GetNumber() const { return _number; }
	LexErr Lexer::GetError() const { return _err; }

	std::string Lexer::GetErrorString() const
	{
		std::stringstream ss;
		ss << "Lexical Analysis Failure: " << LexErrToString(_err) << std::endl << _line << ":" << _position << "> ";
		
		// Trim leading whitespace
		const char* start = _lineStart;
		while (*start != '\0' && isspace(*start)) ++start;
		if (start == '\0')
		{
			return "Lexer failure to compute failure line!";
		}

		// Trim newline
		const char* end = strchr(start, '\r');
		if (end == NULL) end = strchr(start, '\n');

		// And add source
		if (end == NULL)
			ss << start;
		else
			ss << std::string(start, end);

		// Start pointer line
		ss << std::endl << _line << ":" << _position << "> ";
		for (size_t pos = start - _lineStart; pos < _position-1; ++pos)
		{
			if (isspace(_lineStart[pos]))
				ss << _lineStart[pos];
			else
				ss << ' ';
		}
		ss << '^';

		return ss.str();
	}

	void Lexer::Advance()
	{
		if (_token == Symbol::Error)
		{
			AssertFail("Lexical analysis cannot continue once an error has been hit");
			return;
		}

		// Reset
		_token = Symbol::Error;
		_err = LexErr::UnknownSymbol;
		_ident.reset();
		_term.reset();
		_number = nan(nullptr);

		// Burn whitespace
		this->ConsumeWhitespace();

		_position = _location - _lineStart + 1;

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
			if (!isnan(_number))
			{
				_token = Symbol::Number;
				_location += rawLen;
			}
			else
			{
				_err = LexErr::InvalidNumber;
			}
		}
		else if (c == '"')
		{
			size_t rawLen;
			if (this->GetRawTermLength(_location, &rawLen))
			{
				_term = this->GetTerm(_location);
				if (_term.get() != nullptr)
				{
					_token = Symbol::Terminal;
					_location += rawLen;
				}
			}
			else
			{
				_err = LexErr::NonTerminatedString;
			}
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
		SINGLE_CHAR_SYM(',', Symbol::Comma)
		#define DOUBLE_COMPLEX_SYM(C, V, SymC, SymV) else if (c == C && cn == V) { _token = SymV; _location += 2; } else if (c == C) { _token = SymC; _location += 1; }
		DOUBLE_COMPLEX_SYM('=', '=', Symbol::Assign, Symbol::Eq)
		DOUBLE_COMPLEX_SYM('-', '=', Symbol::Minus, Symbol::MinusEq)
		DOUBLE_COMPLEX_SYM('+', '=', Symbol::Plus, Symbol::PlusEq)
		DOUBLE_COMPLEX_SYM('*', '=', Symbol::Mult, Symbol::MultEq)
		DOUBLE_COMPLEX_SYM('/', '=', Symbol::Div, Symbol::DivEq)
		DOUBLE_COMPLEX_SYM('!', '=', Symbol::Not, Symbol::NotEq)
		DOUBLE_COMPLEX_SYM('<', '=', Symbol::LessThan, Symbol::LessThanEq)
		DOUBLE_COMPLEX_SYM('>', '=', Symbol::GreaterThan, Symbol::GreaterThanEq)

		_err = _token != Symbol::Error ? LexErr::NoError : _err;
	}

	void Lexer::ConsumeWhitespace()
	{
		const char* pos = nullptr;
		do
		{
			pos = _location;

			while (*_location != '\0' && isspace(*_location))
			{
				if (*_location == '\n')
				{
					++_line;
					_lineStart = _location + 1;
				}

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
		strncpy_s(ident.get(), len+1, c, len);

		return ident;
	}

	bool Lexer::IsEndOfTerm(const char* c) const
	{
		return *c == '\0' || *c == '\n' || (*c == '"' && *(c - 1) != '\\');
	}

	bool Lexer::GetRawTermLength(const char* c, size_t* length) const
	{
		Assert(*c == '"', "Term must start with a quote");
		const char* start = c;

		do { ++c; } while (!this->IsEndOfTerm(c));
		if (*c == '\0' || *c == '\n') return false;

		*length = c - start + 1;
		return true;
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

	std::unique_ptr<char[]> Lexer::GetTerm(const char* c)
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
					_err = LexErr::UnknownStringEscape;
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
		double num = 0;
		
		// Parse numerator
		do
		{
			num *= 10.0;
			num += *c - '0';
		} while (*++c != '\0' && isdigit(*c));

		// Parse denominator
		if (*c == '.')
		{
			if (!isdigit(*++c))
			{
				// error!
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

#define SYMBOL_CASE_STRING(s) case s: return #s;

	const char* SymbolToString(Symbol sym)
	{
		switch (sym)
		{
			SYMBOL_CASE_STRING(Symbol::Error)
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
			SYMBOL_CASE_STRING(Symbol::Comma)
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
			SYMBOL_CASE_STRING(Symbol::LessThanEq)
			SYMBOL_CASE_STRING(Symbol::GreaterThan)
			SYMBOL_CASE_STRING(Symbol::GreaterThanEq)
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
	const char* LexErrToString(LexErr err)
	{
		switch (err)
		{
			SYMBOL_CASE_STRING(LexErr::NoError)
			SYMBOL_CASE_STRING(LexErr::UnknownSymbol)
			SYMBOL_CASE_STRING(LexErr::UnknownStringEscape)
			SYMBOL_CASE_STRING(LexErr::NonTerminatedString)
			SYMBOL_CASE_STRING(LexErr::InvalidNumber)

		default:
			AssertFail("Missing case for LexErr");
		}

		return nullptr;
	}
}