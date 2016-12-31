#ifndef PARSER_H
#define PARSER_H

namespace scrpt
{
	class Parser
	{
	public:
		Parser();

		void Consume(Lexer* lexer);

	private:
		Lexer* _lexer;

		void ParseProgram();
		void ParseBlock();
		bool ParseStatement();
	};
}

#endif
