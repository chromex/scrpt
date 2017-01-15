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
		AstNode _program;
		AstNode* _currentNode;

		void PushNode();
		void AddNode();
		void PopNode();

		bool Accept(Symbol sym, bool push = false);
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
