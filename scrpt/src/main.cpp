#include "scrpt.h"

int main(int argc, char** argv)
{
	std::cout << "Lexer test" << std::endl;

	scrpt::Lexer lexer(scrpt::ReadFile(fs::path("testcode.scrpt")));

	scrpt::Symbol token;
	do
	{
		 token = lexer.Next();
		 if (token == scrpt::Symbol::Ident)
		 {
			 std::cout << scrpt::SymbolToString(token) << ": " << lexer.GetIdent() << std::endl;
		 }
		 else
		 {
			 std::cout << scrpt::SymbolToString(token) << std::endl;
		 }
	} while (token != scrpt::Symbol::End && token != scrpt::Symbol::Unknown);

	system("pause");
}