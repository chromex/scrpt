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
		PlusPlus,
		Minus,
		MinusMinus,
		Mult,
		Div,
		Modulo,
		PlusEq,
		MinusEq,
		MultEq,
		DivEq,
		ModuloEq,
		And,
		Or,
		SemiColon,
		Func, 
		End,
	};
	const char* SymbolToString(Symbol sym);

	enum class LexErr
	{
		NoError,
		UnknownSymbol,
		UnknownStringEscape,
		NonTerminatedString,
		InvalidNumber,
	};
	const char* LexErrToString(LexErr err);

	class Token
	{
	public:
		Token(
			Symbol sym,
			std::shared_ptr<const char> sourceData,
			const char* symLocation,
			const char* symLineStart,
			size_t lineNumber,
			size_t linePosition,
			LexErr err,
			std::unique_ptr<const char> string,
			double number);

		Symbol GetSym() const;
		const char* GetString() const;
		double GetNumber() const;
		LexErr GetLexError() const;
		std::string GetFormattedTokenCode() const;

	private:
		Symbol _sym;
		std::weak_ptr<const char> _sourceData;
		const char* _symLocation;
		const char* _symLineStart;
		size_t _lineNumber;
		size_t _linePosition;
		LexErr _err;
		std::unique_ptr<const char> _string;
		double _number;
	};

	class Lexer
	{
	public:
		Lexer(std::unique_ptr<char[]> sourceData);

		Symbol Current() const;
		void Advance();
		bool Accept(Symbol sym);
		bool Test(Symbol sym) const;
		bool Expect(Symbol sym);

		size_t GetLine() const;
		size_t GetPosition() const;
		const char* GetIdent() const;
		const char* GetTerm() const;
		double GetNumber() const;
		LexErr GetError() const;
		std::string GetErrorString() const;

	private:
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
