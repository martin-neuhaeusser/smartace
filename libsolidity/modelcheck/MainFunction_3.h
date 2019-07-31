/**
 * @date 2019
 * First-pass visitor for converting Solidity methods into functions in C.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/TypeTranslator.h>
#include <list>
#include <ostream>
#include <set>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * Prints a forward declaration for each explicit (member function) and implicit
 * (default constructor, map accessor, etc.) Solidity function, according to the
 * C model.
 */
class MainFunction_3 : public ASTConstVisitor
{
public:
    // Constructs a printer for all function forward decl's required by the ast.
    MainFunction_3(
        ASTNode const& _ast,
		TypeConverter const& _converter,
		bool _forward_declare
    );

    // Prints each function-like declaration once, in some order. Special
	// functions, such as constructors and accessors are also generated.
    void print(std::ostream& _stream);

protected:


  bool visit(FunctionDefinition const& _node) override;
  void endVisit(ContractDefinition const& _node) override;

private:
	ASTNode const& m_ast;
	TypeConverter const& m_converter;
	std::ostream* m_ostream = nullptr;

  int i=0;
	const bool m_forward_declare;
  std::set<ContractDefinition const*> m_built;

	// Helper functions to partition complex from primitive types, and to set
	// said values with either default or non-deterministic data.
	static bool is_basic_type(Type const& _type);
	static std::string to_init_value(std::string _name, Type const& _type);
	static std::string to_nd_value(std::string _name, Type const& _type);

	// Formats all declarations as a C-function argument list. The given order
	// of arguments is maintained. If a scope is provided, then the arguments
	// are assumed to be of a stateful Solidity method, bound to structures of
	// the given type. If values are defaulted to zero, then the constructor
	// will have a default value of zero for each parameter.
	void print_args(
		std::vector<ASTPointer<VariableDeclaration>> const& _args, ASTNode const* _scope
	);
};

}
}
}
