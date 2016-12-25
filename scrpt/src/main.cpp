#include "scrpt.h"

int main(int argc, char** argv)
{
	std::cout << "Lexer test" << std::endl;

	scrpt::Lexer lexer(scrpt::ReadFile(fs::path("testcode.scrpt")));

	scrpt::Symbol token;
	do
	{
		token = lexer.Next();
		std::cout << lexer.GetLine() << " " << scrpt::SymbolToString(token);
		if (token == scrpt::Symbol::Ident)
		{
			std::cout << ": " << lexer.GetIdent();
		}
		else if (token == scrpt::Symbol::Terminal)
		{
			std::cout << ": \"" << lexer.GetTerm() << "\"";
		}
		else if (token == scrpt::Symbol::Number)
		{
			std::cout << ": " << lexer.GetNumber();
		}

		std::cout << std::endl;
	} while (token != scrpt::Symbol::End && token != scrpt::Symbol::Unknown);

	system("pause");
}