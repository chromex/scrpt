#ifndef AST_H
#define AST_H

namespace scrpt
{
	class AstNode
	{
		typedef std::list<AstNode> ChildList;

	public:

		Symbol GetSym() const;
		void AddChild(AstNode node);

	private:
		AstNode(Symbol sym);

		const Symbol _sym;
	};
}

#endif
