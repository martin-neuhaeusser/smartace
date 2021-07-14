/**
 * Utility to perform taint analysis with client variables.
 * 
 * @date 2021
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>

#include <list>
#include <map>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * Utility to extract assignment destination (wrt. taint analysis).
 */
class TaintDestination : public ASTConstVisitor
{
public:
    // Extracts destination from _expr.
    TaintDestination(Expression const& _expr);

    // Extracts the destination, or throws an exception.
    VariableDeclaration const& extract();

protected:
	bool visit(MemberAccess const& _node) override;
	bool visit(Identifier const& _node) override;

private:
    // Updates m_dest. If m_dest is set, throws.
    void update_dest(Declaration const* _ref);

    // First destination located.
    VariableDeclaration const* m_dest = nullptr;
};

// -------------------------------------------------------------------------- //

/**
 * Performs intraprocedural tain propogation for a single method. Tainted
 * sources are flagged before running the analysis.
 *
 * All analysis is flow-insensitive and field-insensitive.
 *
 * Analysis is very coase. For example, if `x = e`, and e contains tainted
 * variable y, then x is now tainted, regardless of interpretation of e.
 */
class TaintAnalysis : public ASTConstVisitor
{
public:
    // Analysis with tainted sources numbered 0 to (_sources - 1).
    TaintAnalysis(size_t _sources);

    // Marks a tainted source before running the analysis.
    void taint(VariableDeclaration const& _decl, size_t _i);

    // Runs taint analysis until a fixed point is reached.
    void run(FunctionDefinition const& _decl);

    // Returns the taint result for _decl.
    std::vector<bool> const& taint_for(VariableDeclaration const& _decl) const;

    // Returns the number of taitned sources.
    size_t source_count() const;

protected:
	bool visit(VariableDeclarationStatement const& _node) override;
	bool visit(Assignment const& _node) override;
	bool visit(FunctionCall const&) override;
	bool visit(MemberAccess const& _node) override;
	bool visit(Identifier const& _node) override;

private:
    // Moves taint from _decl to m_taintee.
    void propogate(Declaration const* _ref);

    // Applies all taint to m_taint due to inprecision.
    void propogate_unknown();

    // Default response for unknown variables.
    std::vector<bool> m_default_taint;

    // Set to false whenever the taint database is updated.
    bool m_changed = false;

    // The number of taited sources.
    size_t m_sources;

    // Maps each variable declaration to the inputs it has been tainted by.
    // Ex. If m_taint[v][i] is true, then variable v is tainted by input i.
    std::map<VariableDeclaration const*, std::vector<bool>> m_taint;

    // Current variable being tainted.
    VariableDeclaration const *m_taintee;
};

// -------------------------------------------------------------------------- //

}
}
}
