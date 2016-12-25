#ifndef LEXER_H
#define LEXER_H

namespace scrpt
{
	enum class Symbol
	{
		Unknown,
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

	const char* SymbolToString(Symbol sym);

	class Lexer
	{
	public:
		Lexer(std::unique_ptr<char[]> sourceData);

		Symbol Next();
		size_t GetLine() const;;
		const char* GetIdent() const;
		const char* GetTerm() const;
		double GetNumber() const;

	private:
		void Advance();
		void ConsumeWhitespace();
		bool IsIdentCharacter(char c, bool firstChar) const;
		size_t GetIdentLength(const char* c) const;
		std::unique_ptr<char[]> GetIdent(const char* c) const;
		bool IsEndOfTerm(const char* c) const;
		size_t GetRawTermLength(const char* c) const;
		size_t GetTermLength(const char* c) const;
		std::unique_ptr<char[]> GetTerm(const char* c) const;
		double GetNumber(const char* c, size_t* rawLen) const;

		std::unique_ptr<char[]> _sourceData;
		const char* _location;
		std::unique_ptr<char[]> _ident;
		std::unique_ptr<char[]> _term;
		double _number;
		size_t _line;
		Symbol _token;
	};
}

#endif
