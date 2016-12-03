#include "../scrpt.h"

#define COMPONENTNAME "Lexer"

namespace scrpt
{
	Lexer::Lexer(std::unique_ptr<char[]> sourceData)
		: _sourceData(nullptr)
		, _line(1)
	{
		_sourceData.swap(sourceData);

		AssertNotNull(_sourceData.get());
	}

	Symbol Lexer::Next()
	{


		return Symbol::Unknown;
	}

	const char* SymbolToString(Symbol sym)
	{
		#define SYMBOL_CASE_STRING(s) case s: return #s;
		switch (sym)
		{
			SYMBOL_CASE_STRING(Symbol::Unknown)
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