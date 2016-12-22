#ifndef LEXER_H
#define LEXER_H

namespace scrpt
{
	enum class Symbol
	{
		Unknown,
		Start,
		LParen, //
		RParen,//
		LBracket,//
		RBracket,//
		LSquare,//
		RSquare,//
		Number,
		Terminal,
		Ident, //
		True, //
		False, //
		Colon,//
		Do, //
		While, //
		For, //
		Switch, //
		Case, //
		Return, //
		Eq,
		Assign,
		NotEq,
		LessThan,
		GreaterThan,
		Plus,
		Minus,
		Mult,
		Div,
		PlusEq,
		MinusEq,
		MultEq,
		DivEq,
		Not,
		SemiColon,//
		Func, //
		End,
	};

	const char* SymbolToString(Symbol sym);

	class Lexer
	{
	public:
		Lexer(std::unique_ptr<char[]> sourceData);

		Symbol Next();
		int Line;
		const char* GetIdent() const;

	private:
		void Advance();
		void ConsumeWhitespace();
		bool IsIdentCharacter(char c, bool firstChar) const;
		unsigned int GetIdentLength(const char* c) const;
		std::unique_ptr<char[]> GetIdent(const char* c) const;

		std::unique_ptr<char[]> _sourceData;
		const char* _location;
		std::unique_ptr<char[]> _ident;
		int _line;
		Symbol _token;
	};
}

#endif
