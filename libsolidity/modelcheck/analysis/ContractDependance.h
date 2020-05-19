/**
 * @date 2020
 * A set of tools to analyze the dependance between contracts, their methods and
 * their structs.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/analysis/AllocationSites.h>

#include <list>
#include <map>
#include <set>
#include <vector>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * A utility class which extracts all calls made by invoking a given function.
 */
class CallReachAnalyzer: public ASTConstVisitor
{
public:
    // Determines all calls originating from the body of _func.
    CallReachAnalyzer(FunctionDefinition const& _func);

    std::set<FunctionDefinition const*> m_calls;

    std::set<VariableDeclaration const*> m_reads;

protected:
    bool visit(IndexAccess const& _node) override;

    void endVisit(FunctionCall const& _node) override;
};

// -------------------------------------------------------------------------- //

/**
 * The contract dependance is a second pass over the contract construction
 * graph. It is compared against a model (a list of contracts to model) and then
 * uses these contracts to determine the structures and methods we require to
 * resolve all calls.
 */
class ContractDependance
{
public:
    using ContractSet = std::set<ContractDefinition const*>;
    using ContractList = std::vector<ContractDefinition const*>;
    using FunctionSet = std::set<FunctionDefinition const*>;
    using FuncInterface = std::list<FunctionDefinition const*>;
    using SuperCalls = std::vector<FunctionDefinition const*>;
    using VarSet = std::set<VariableDeclaration const*>;

    // A utility used by ContractDependance to expand the entire model. The
    // DependanceAnalyzer handles targeted analysis without concern for how each
    // component will be stitched together by the ContractDependance structure.
    class DependancyAnalyzer
    {
    public:
        // The _model parameter is needed for non-test setups, to list top level
        // contracts in the scheduler.
        DependancyAnalyzer(ContractDependance::ContractList _model);

        virtual ~DependancyAnalyzer() = default;

        // Returns all methods exposed (and used) by _ctrt.
        virtual FuncInterface get_interfaces_for(
            ContractDefinition const* _ctrt
        ) const = 0;

        // Returns the super call chain for _func.
        virtual SuperCalls get_superchain_for(
            FunctionDefinition const* _func
        ) const = 0;

        // The list of all contracts in the analysis.
        ContractDependance::ContractSet m_contracts;

        // The list of all contracts specified by the model.
        ContractDependance::ContractList m_model;
    };

    // Default constructor used to orchestrate dependancy analysis.
    ContractDependance(DependancyAnalyzer const& _analyzer);

    // Returns all top level contracts in the graph, given the graph is meant
    // to generate a scheduler.
    ContractList const& get_model() const;

    // Returns all methods in the graph. This includes methods which are called
    // indirectly (i.e., as a call to super).
    FunctionSet const& get_executed_code() const;

    // Returns true if the contract is ever used.
    bool is_deployed(ContractDefinition const* _actor) const;

    // Returns the public method of a contract.
    FuncInterface const& get_interface(ContractDefinition const* _actor) const;

    // Returns all super calls for a given method.
    SuperCalls const& get_superchain(FunctionDefinition const* _func) const;

    // Returns all methods invoked by this call.
    FunctionSet const& get_function_roi(FunctionDefinition const* _func) const;

    // Returns all mapping declarations touched by a given function.
    VarSet const& get_map_roi(FunctionDefinition const* _func) const;

private:
    ContractSet m_contracts;

    ContractList m_model;

    FunctionSet m_functions;

    std::map<ContractDefinition const*, FuncInterface> m_interfaces;

    std::map<FunctionDefinition const*, SuperCalls> m_superchain;

    std::map<FunctionDefinition const*, FunctionSet> m_callreach;

    std::map<FunctionDefinition const*, VarSet> m_mapreach;
};

// -------------------------------------------------------------------------- //


/**
 * An implementation of DependancyAnalyzer which expands all calls. This is
 * meant for codegen testing.
 */
class FullSourceContractDependance
    : public ContractDependance::DependancyAnalyzer
{
public:
    // All contracts reachable for _srcs are included.
    FullSourceContractDependance(SourceUnit const& _srcs);

    ~FullSourceContractDependance() override = default;

    ContractDependance::FuncInterface get_interfaces_for(
        ContractDefinition const* _ctrt
    ) const override;

    ContractDependance::SuperCalls get_superchain_for(
        FunctionDefinition const* _func
    ) const override;
};

// -------------------------------------------------------------------------- //

/**
 * An implementation of DependancyAnalyzer which expands only the calls needed
 * by a given model, with a given allocation graph.
 */
class ModelDrivenContractDependance
    : public ContractDependance::DependancyAnalyzer
{
public:
    // All contracts reachable from _model, taking into account downcasting in
    // _graph, are included.
    ModelDrivenContractDependance(
        std::vector<ContractDefinition const*> _model,
        NewCallGraph const& _graph
    );

    ~ModelDrivenContractDependance() override = default;

    ContractDependance::FuncInterface get_interfaces_for(
        ContractDefinition const* _ctrt
    ) const override;

    ContractDependance::SuperCalls get_superchain_for(
        FunctionDefinition const* _func
    ) const override;

private:
    // Utility class used to extract the actual chain of super calls.
    class SuperChainExtractor : public ASTConstVisitor
    {
    public:
        SuperChainExtractor(FunctionDefinition const& _call);

        ContractDependance::SuperCalls m_superchain;

    protected:
	    bool visit(FunctionCall const& _node) override;
    };
};

// -------------------------------------------------------------------------- //

}
}
}
