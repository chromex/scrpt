#ifndef LEXER_H
#define LEXER_H

namespace scrpt
{
	enum class Symbol
	{
		Error,
		Start,
		LParen, 
		RParen,
		LBracket,
		RBracket,
		LSquare,
		RSquare,
		Number,
		Terminal,
		Ident, 
		True, 
		False, 
		Colon,
		Comma,
		Do, 
		While, 
		For, 
		Switch, 
		Case, 
		Break, 
		Default, 
		Return, 
		Eq,
		Assign,
		Not, 
		NotEq,
		LessThan,
		LessThanEq,
		GreaterThan,
		GreaterThanEq,
		Plus,
		Minus,
		Mult,
		Div,
		PlusEq,
		MinusEq,
		MultEq,
		DivEq,
		SemiColon,
		Func, 
		End,
	};

	enum class LexErr
	{
		NoError,
		UnknownSymbol,
		UnknownStringEscape,
		NonTerminatedString,
		InvalidNumber,
	};

	const char* SymbolToString(Symbol sym);
	const char* LexErrToString(LexErr err);

	class Lexer
	{
	public:
		Lexer(std::unique_ptr<char[]> sourceData);

		Symbol Next();
		size_t GetLine() const;
		size_t GetPosition() const;
		const char* GetIdent() const;
		const char* GetTerm() const;
		double GetNumber() const;
		LexErr GetError() const;
		std::string GetErrorString() const;

	private:
		void Advance();
		void ConsumeWhitespace();
		bool IsIdentCharacter(char c, bool firstChar) const;
		size_t GetIdentLength(const char* c) const;
		std::unique_ptr<char[]> GetIdent(const char* c) const;
		bool IsEndOfTerm(const char* c) const;
		bool GetRawTermLength(const char* c, size_t* length) const;
		size_t GetTermLength(const char* c) const;
		std::unique_ptr<char[]> GetTerm(const char* c);
		double GetNumber(const char* c, size_t* rawLen) const;

		std::unique_ptr<char[]> _sourceData;
		const char* _location;
		const char* _lineStart;
		std::unique_ptr<char[]> _ident;
		std::unique_ptr<char[]> _term;
		double _number;
		size_t _line;
		size_t _position;
		Symbol _token;
		LexErr _err;
	};
}

#endif
