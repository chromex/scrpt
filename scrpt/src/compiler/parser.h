#ifndef PARSER_H
#define PARSER_H

namespace scrpt
{
	enum class ParserErr
	{
		NoError,
		ExpressionExpected,
		StatementExpected,
		IdentExpected,
		BlockExpected,
	};

	class Parser
	{
	public:
		Parser();

		void Consume(Lexer* lexer);

	private:
		Lexer* _lexer;

		void ParseProgram();
		bool ParseBlock();
		bool ParseStatement();
		bool ParseExpression();
		bool ParseWhileLoop();
		bool ParseDoLoop();
		bool ParseForLoop();
		bool ParseIf();
	};
}

#endif
