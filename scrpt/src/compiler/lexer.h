#ifndef LEXER_H
#define LEXER_H

namespace scrpt
{
	enum class Symbol
	{
		Unknown,
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
		Return,
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
		SemiColon,
		Func,
	};

	const char* SymbolToString(Symbol sym);

	class Lexer
	{
	public:
		Lexer(std::unique_ptr<char[]> sourceData);

		Symbol Next();
		int Line;

	private:
		std::unique_ptr<char[]> _sourceData;
		int _line;
	};
}

#endif
