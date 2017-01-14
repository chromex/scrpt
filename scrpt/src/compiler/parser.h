#pragma once

namespace scrpt
{
	class Parser
	{
	public:
		Parser();

		void Consume(Lexer* lexer);

	private:
		Lexer* _lexer;

		bool Accept(Symbol sym);
		bool Test(Symbol sym) const;
		bool Expect(Symbol sym);

		void ParseProgram();
		bool ParseBlock(bool expect);
		bool ParseStatement(bool expect);
		bool ParseExpression(bool expect);
		bool ParseWhileLoop();
		bool ParseDoLoop();
		bool ParseForLoop();
		bool ParseIf();
	};
}
