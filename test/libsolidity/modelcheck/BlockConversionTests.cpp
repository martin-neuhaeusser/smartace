/**
 * @date 2019
 * Comprehensive tests for libsolidity/modelcheck/BlockConversionVisitor.{h,cpp}.
 */

#include <libsolidity/modelcheck/model/Block.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>
#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/ContractDependance.h>

#include <sstream>

using namespace std;
using langutil::SourceLocation;

namespace dev
{
namespace solidity
{
namespace modelcheck
{
namespace test
{

BOOST_FIXTURE_TEST_SUITE(
    BlockConversion,
    ::dev::solidity::test::AnalysisFramework
)

// Tests that input parameters are registered as declarations.
BOOST_AUTO_TEST_CASE(argument_registration)
{
    char const* text = R"(
		contract A {
			function f(int a, int b) public {
                a;
                b;
            }
		}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual, expect;
    actual << *FunctionBlockConverter(
        func, statedata, newcalls, converter
    ).convert();
    expect << "{";
    expect << "(func_user_a).v;";
    expect << "(func_user_b).v;";
    expect << "}";
    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

// Tests that else statements and bodies are optional and that branch bodies are
// properly scoped
BOOST_AUTO_TEST_CASE(if_statement)
{
    char const* text = R"(
		contract A {
            int a;
			function if_stmt() public {
                if (a == 1) { }
                if (a == 1) { int a; }
                a;
            }
			function if_else_stmt() public {
                if (a == 1) { }
                else { }
                if (a == 1) { int a; }
                else { int a; }
                a;
            }
		}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& fncs = ctrt.definedFunctions();

    auto if_stmt = (fncs[0]->name() == "if_stmt") ? fncs[0] : fncs[1];
    auto else_stmt = (fncs[0]->name() == "if_stmt") ? fncs[1] : fncs[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual_if, expected_if;
    actual_if << *FunctionBlockConverter(
        *if_stmt, statedata, newcalls, converter
    ).convert();
    expected_if << "{";
    expected_if << "if(((self->user_a).v)==(1))";
    expected_if << "{";
    expected_if << "}";
    expected_if << "if(((self->user_a).v)==(1))";
    expected_if << "{";
    expected_if << "sol_int256_t func_user_a;";
    expected_if << "}";
    expected_if << "(self->user_a).v;";
    expected_if << "}";
    BOOST_CHECK_EQUAL(actual_if.str(), expected_if.str());

    ostringstream actual_else, expected_else;
    actual_else << *FunctionBlockConverter(
        *else_stmt, statedata, newcalls, converter
    ).convert();
    expected_else << "{";
    expected_else << "if(((self->user_a).v)==(1)){}";
    expected_else << "else {}";
    expected_else << "if(((self->user_a).v)==(1)){sol_int256_t func_user_a;}";
    expected_else << "else {sol_int256_t func_user_a;}";
    expected_else << "(self->user_a).v;";
    expected_else << "}";
    BOOST_CHECK_EQUAL(actual_else.str(), expected_else.str());
}

// Tests that while and for loops work in general, that expressions of a for
// loop are optional, and that loops are correctly scoped.
BOOST_AUTO_TEST_CASE(loop_statement)
{
    char const* text = R"(
		contract A {
            uint a;
            uint i;
			function while_stmt() public {
                while (a != a) { }
                while (a != a) { int i; }
                i;
            }
            function for_stmt() public {
                for (; a < 10; ++a) { int i; }
                for (int i = 0; ; ++i) { i; }
                for (int i = 0; i < 10; ) { ++i; }
                for (int i = 0; i < 10; ++i) { }
                i;
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& fncs = ctrt.definedFunctions();

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    auto while_stmt = (fncs[0]->name() == "while_stmt") ? fncs[0] : fncs[1];
    ostringstream actual_while, expected_while;
    actual_while << *FunctionBlockConverter(
        *while_stmt, statedata, newcalls, converter
    ).convert();
    expected_while << "{";
    expected_while << "while(((self->user_a).v)!=((self->user_a).v)){}";
    expected_while << "while(((self->user_a).v)!=((self->user_a).v))";
    expected_while << "{sol_int256_t func_user_i;}";
    expected_while << "(self->user_i).v;";
    expected_while << "}";
    BOOST_CHECK_EQUAL(actual_while.str(), expected_while.str());

    auto for_stmt = (fncs[0]->name() == "while_stmt") ? fncs[1] : fncs[0];
    ostringstream actual_for, expected_for;
    actual_for << *FunctionBlockConverter(
        *for_stmt, statedata, newcalls, converter
    ).convert();
    expected_for << "{";
    expected_for << "for(;((self->user_a).v)<(10);++((self->user_a).v))"
                 << "{sol_int256_t func_user_i;}";
    expected_for << "for(sol_int256_t func_user_i=Init_sol_int256_t(0);;++("
                 << "(func_user_i).v)){(func_user_i).v;}";
    expected_for << "for(sol_int256_t func_user_i=Init_sol_int256_t(0);"
                 << "((func_user_i).v)<(10);)"
                 << "{++((func_user_i).v);}";
    expected_for << "for(sol_int256_t func_user_i=Init_sol_int256_t(0);"
                 << "((func_user_i).v)<(10);++((func_user_i).v)){}";
    expected_for << "(self->user_i).v;";
    expected_for << "}";
    BOOST_CHECK_EQUAL(actual_for.str(), expected_for.str());
}

// Ensures continue statements remain unchanged.
BOOST_AUTO_TEST_CASE(continue_statement)
{
    char const* text = R"(
		contract A {
			function void_func() public {
                while (false) { continue; }
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual, expect;
    actual << *FunctionBlockConverter(
        func, statedata, newcalls, converter
    ).convert();
    expect << "{";
    expect << "while(0){continue;}";
    expect << "}";
    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

// Ensures break statements remain unchanged.
BOOST_AUTO_TEST_CASE(break_statement)
{
    char const* text = R"(
		contract A {
			function void_func() public {
                while (false) { break; }
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual, expect;
    actual << *FunctionBlockConverter(
        func, statedata, newcalls, converter
    ).convert();
    expect << "{";
    expect << "while(0){break;}";
    expect << "}";
    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

// Ensures return statements remain unchanged.
BOOST_AUTO_TEST_CASE(return_statement)
{
    char const* text = R"(
		contract A {
			function void_func() public { return; }
            function int_func() public returns (int) { return 10 + 5; }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& fncs = ctrt.definedFunctions();

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    auto void_func = (fncs[0]->name() == "void_func") ? fncs[0] : fncs[1];
    ostringstream actual_void, expect_void;
    actual_void << *FunctionBlockConverter(
        *void_func, statedata, newcalls, converter
    ).convert();
    expect_void << "{";
    expect_void << "return;";
    expect_void << "}";
    BOOST_CHECK_EQUAL(actual_void.str(), expect_void.str());

    auto int_func = (fncs[0]->name() == "void_func") ? fncs[1] : fncs[0];
    ostringstream actual_int, expect_int;
    actual_int << *FunctionBlockConverter(
        *int_func, statedata, newcalls, converter
    ).convert();
    expect_int << "{";
    expect_int << "return Init_sol_int256_t((10)+(5));";
    expect_int << "}";
    BOOST_CHECK_EQUAL(actual_int.str(), expect_int.str());
}

// Ensures that variable declarations will generate C declarations, and that
// these declarations will be added to the internal state of the block
// converter. Also ensures that these variables will be popped once said scope
// is exited.
BOOST_AUTO_TEST_CASE(variable_declaration_statement)
{
    char const* text = R"(
		contract A {
            int a;
            int c;
			function f() public {
                int b;
                {
                    int c;
                    a; b; c;
                }
                { a; b; c; }
                a; b; c;
            }
		}
	)";

    const auto &unit = *parseAndAnalyse(text);
    const auto &ctrt = *retrieveContractByName(unit, "A");
    const auto &func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(
        func, statedata, newcalls, converter
    ).convert();
    expected << "{";
    expected << "sol_int256_t func_user_b;";
    expected << "{";
    expected << "sol_int256_t func_user_c;";
    expected << "(self->user_a).v;";
    expected << "(func_user_b).v;";
    expected << "(func_user_c).v;";
    expected << "}";
    expected << "{";
    expected << "(self->user_a).v;";
    expected << "(func_user_b).v;";
    expected << "(self->user_c).v;";
    expected << "}";
    expected << "(self->user_a).v;";
    expected << "(func_user_b).v;";
    expected << "(self->user_c).v;";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Ensures that if a function has named return values, that those variables will
// be implicitly declared.
BOOST_AUTO_TEST_CASE(named_function_retvars)
{
    char const* text = R"(
		contract A {
			function f() public returns (int a) { a = 5; }
		}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual_named, expected_named;
    actual_named << *FunctionBlockConverter(
        func, statedata, newcalls, converter
    ).convert();
    expected_named << "{";
    expected_named << "sol_int256_t func_user_a;";
    expected_named << "((func_user_a).v)=(5);";
    expected_named << "return func_user_a;";
    expected_named << "}";
    BOOST_CHECK_EQUAL(actual_named.str(), expected_named.str());
}

// Tests type-aware resolution of MemberAccess expressions. This is tested on
// the block level as type annotation is not trial, and automatically handled by
// the analysis framework.
BOOST_AUTO_TEST_CASE(member_access_expressions)
{
    char const* text = R"(
        contract A {
            struct B { int i; }
            struct C { B b; }
            B b;
            C c;
            int public d;
            function f() public payable {
                this.d;
                b.i;
                c.b.i;
                block.number;
                block.timestamp;
                msg.sender;
                msg.value;
                address(this).balance;
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(
        func, statedata, newcalls, converter
    ).convert();
    expected << "{";
    expected << "if(((paid).v)==(1))(((self)->model_balance).v)+=((value).v);";
    expected << "(self)->user_d;";
    expected << "((self->user_b).user_i).v;";
    expected << "(((self->user_c).user_b).user_i).v;";
    expected << "(blocknum).v;";
    expected << "(timestamp).v;";
    expected << "(sender).v;";
    expected << "(value).v;";
    expected << "((self)->model_balance).v;";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Tests interal method calls. Internal calls are those which reference contract
// members, without the use of member access (`<ctx>.<method>(...)` or
// `this.<method>(...)`).
BOOST_AUTO_TEST_CASE(internal_method_calls)
{
    char const* text = R"(
		contract A {
			function f() public { }
            function g(int a) public { }
            function h(int a, int b) public { }
            function p() public pure { }
            function q(int a) public pure { }
            function r(int a, int b) public pure { }
            function test() public {
                f();
                g(1);
                h(1, 2);
                p();
                q(1);
                r(1, 2);
            }
		}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);
    
    for (auto func_ptr : ctrt.definedFunctions())
    {
        if (func_ptr->name() == "test")
        {
            ostringstream actual, expected;
            FunctionBlockConverter fbc(
                *func_ptr, statedata, newcalls, converter
            );
            fbc.set_for(FunctionSpecialization(*func_ptr));
            actual << *fbc.convert();
            expected << "{";
            expected << "Method_A_Funcf(self,sender,value,blocknum"
                     << ",timestamp,Init_sol_bool_t(0),origin);";
            expected << "Method_A_Funcg(self,sender,value,blocknum"
                     << ",timestamp,Init_sol_bool_t(0),origin"
                     << ",Init_sol_int256_t(1));";
            expected << "Method_A_Funch(self,sender,value,blocknum"
                     << ",timestamp,Init_sol_bool_t(0),origin"
                     << ",Init_sol_int256_t(1),Init_sol_int256_t(2));";
            expected << "Method_A_Funcp(self,sender,value,blocknum"
                     << ",timestamp,Init_sol_bool_t(0),origin);";
            expected << "Method_A_Funcq(self,sender,value,blocknum"
                     << ",timestamp,Init_sol_bool_t(0),origin"
                     << ",Init_sol_int256_t(1));";
            expected << "Method_A_Funcr(self,sender,value,blocknum"
                     << ",timestamp,Init_sol_bool_t(0),origin"
                     << ",Init_sol_int256_t(1),Init_sol_int256_t(2));";
            expected << "}";
            BOOST_CHECK_EQUAL(actual.str(), expected.str());
            break;
        }
    }
}

// Tests external method calls. External method calls reference members of a
// given contract by address, through the use of member accessors
// (`<ctx>.<method>(...)` or `this.<method>(...)`).
BOOST_AUTO_TEST_CASE(external_method_calls)
{
    char const* text = R"(
		contract A {
			function f() public { }
            function g() public pure { }
		}
        contract B {
            A a;
            B b;
            function f() public { }
            function test() public {
                a.f();
                a.g();
                b.f();
                this.f();
                (this.f)();
            }
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "B");

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);
    
    for (auto func_ptr : ctrt.definedFunctions())
    {
        if (func_ptr->name() == "test")
        {
            ostringstream actual, expected;
            FunctionBlockConverter fbc(
                *func_ptr, statedata, newcalls, converter
            );
            fbc.set_for(FunctionSpecialization(*func_ptr));
            actual << *fbc.convert();
            expected << "{";
            expected << "Method_A_Funcf(&(self->user_a),(self)->model_address"
                     << ",Init_sol_uint256_t(0),blocknum,timestamp"
                     << ",Init_sol_bool_t(1),origin);";
            expected << "Method_A_Funcg(&(self->user_a),(self)->model_address"
                     << ",Init_sol_uint256_t(0),blocknum,timestamp"
                     << ",Init_sol_bool_t(1),origin);";
            expected << "Method_B_Funcf(&(self->user_b),(self)->model_address"
                     << ",Init_sol_uint256_t(0),blocknum,timestamp"
                     << ",Init_sol_bool_t(1),origin);";
            expected << "Method_B_Funcf(self,(self)->model_address"
                     << ",Init_sol_uint256_t(0),blocknum,timestamp"
                     << ",Init_sol_bool_t(1),origin);";
            expected << "Method_B_Funcf(self,(self)->model_address"
                     << ",Init_sol_uint256_t(0),blocknum,timestamp"
                     << ",Init_sol_bool_t(1),origin);";
            expected << "}";;
            BOOST_CHECK_EQUAL(actual.str(), expected.str());
            break;
        }
    }
}

// Tests conversion of transfer/send into _pay. This is tested on the block
// level due to the complexity of annotated FunctionCall expressions.
BOOST_AUTO_TEST_CASE(payment_function_calls)
{
    char const* text = R"(
		contract A {
			function f(address payable dst) public {
                dst.transfer(5);
                dst.send(10);
                (dst.send)(15);
            }
		}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(
        func, statedata, newcalls, converter
    ).convert();
    expected << "{";
    expected << "_pay(&((self)->model_balance),Init_sol_address_t("
             << "(func_user_dst).v),Init_sol_uint256_t(5));";
    expected << "_pay_use_rv(&((self)->model_balance),Init_sol_address_t("
             << "(func_user_dst).v),Init_sol_uint256_t(10));";
    expected << "_pay_use_rv(&((self)->model_balance),Init_sol_address_t("
             << "(func_user_dst).v),Init_sol_uint256_t(15));";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Tests conversion of assert/require into C-horn. This is tested on the block
// level due to the complexity of annotated FunctionCall expressions.
BOOST_AUTO_TEST_CASE(verification_function_calls)
{
    char const* text = R"(
		contract A {
			function f(address payable dst) public {
                require(true);
                require(true, "test");
                assert(true);
            }
		}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(
        func, statedata, newcalls, converter
    ).convert();
    expected << "{";
    expected << "sol_require(1,0);";
    expected << "sol_require(1,0);";
    expected << "sol_assert(1,0);";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Ensures that `<struct>(<v1>, ..., <vn>)` will be mapped to
// `Init_<contract>_<struct>(<v1>, ..., <vn>)`.
BOOST_AUTO_TEST_CASE(struct_ctor_calls)
{
    char const* text = R"(
		contract A {
            struct B { mapping(uint => uint) a; }
            struct C { uint a; }
            struct D { uint a; uint b; }
			function f() public {
                B(); C(1); D(1, 2);
            }
		}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(
        func, statedata, newcalls, converter
    ).convert();
    expected << "{";
    expected << "Init_A_StructB();";
    expected << "Init_A_StructC(Init_sol_uint256_t(1));";
    expected << "Init_A_StructD(Init_sol_uint256_t(1),Init_sol_uint256_t(2));";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Ensures that `new <contract>(<v1>, ..., <vn>)` will be mapped to
// `Init_<contract>(<v1>, ..., <vn>)`.
BOOST_AUTO_TEST_CASE(contract_ctor_calls)
{
    char const* text = R"(
        contract A { }
        contract B {
            int a;
            constructor(int _a) public {
                a = _a;
            }
        }
		contract C {
            A a;
            B b;
			function f() public {
                a = new A();
                b = new B(10);
            }
		}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "C");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(
        func, statedata, newcalls, converter
    ).convert();
    expected << "{";
    expected << "Init_A(&(self->user_a),(self)->model_address"
             << ",Init_sol_uint256_t(0),blocknum,timestamp,Init_sol_bool_t(1)"
             << ",origin);";
    expected << "Init_B(&(self->user_b),(self)->model_address"
             << ",Init_sol_uint256_t(0),blocknum,timestamp,Init_sol_bool_t(1)"
             << ",origin,Init_sol_int256_t(10));";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Ensure that when not caught by another expression (ie, assignmet), that an
// index access to a map will be replaced by a Read_ call. Furthermore, this
// ensures that nested index accesses will be replaced by Ref_ calls.
BOOST_AUTO_TEST_CASE(read_only_index_access)
{
    char const* text = R"(
        contract A {
            struct B { mapping(int => mapping(int => int)) arr2; }
            struct C { B b; }
            mapping(int => mapping(int => int)) arr1;
            B b;
            C c;
            function f() public {
                b.arr2[3 + 4][3 + 4];
                c.b.arr2[5 + 6][5 + 6];
                arr1[10][10];
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(
        func, statedata, newcalls, converter
    ).convert();
    expected << "{";
    expected << "(Read_Map_1(&((self->user_b).user_arr2),"
             << "Init_sol_int256_t((3)+(4)),Init_sol_int256_t((3)+(4)))).v;";
    expected << "(Read_Map_1(&(((self->user_c).user_b).user_arr2),"
             << "Init_sol_int256_t((5)+(6)),Init_sol_int256_t((5)+(6)))).v;";
    expected << "(Read_Map_2(&(self->user_arr1),Init_sol_int256_t(10),"
             << "Init_sol_int256_t(10))).v;";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Ensures that when the l-value of an assignment is an IndexAccess, that the
// forementioned assignment will be rewritten as a Write_ call.
BOOST_AUTO_TEST_CASE(map_assignment)
{
    char const* text = R"(
        contract A {
            struct B { int m; }
            struct C { mapping(int => int) m; }
            mapping(int => int) a;
            mapping(int => B) b;
            C c;
            mapping(int => mapping(int => int)) d;
            function f() public {
                a[1] = 2;
                a[1] += 2;
                b[1].m += 2;
                c.m[1] = 2;
                d[1][2] = 3;
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(
        func, statedata, newcalls, converter
    ).convert();
    expected << "{";
    expected << "Write_Map_2(&(self->user_a),Init_sol_int256_t(1),"
             << "Init_sol_int256_t(2));";
    expected << "Write_Map_2(&(self->user_a),Init_sol_int256_t(1),"
             << "Init_sol_int256_t(((Read_Map_2(&(self->user_a),"
             << "Init_sol_int256_t(1))).v)+(2)));";
    expected << "(((Read_Map_3(&(self->user_b),Init_sol_int256_t(1))).user_m).v"
             << ")=((((Read_Map_3(&(self->user_b),Init_sol_int256_t(1))).user_m"
             << ").v)+(2));";
    expected << "Write_Map_1(&((self->user_c).user_m),Init_sol_int256_t(1),"
             << "Init_sol_int256_t(2));";
    expected << "Write_Map_4(&(self->user_d),Init_sol_int256_t(1),"
             << "Init_sol_int256_t(2),Init_sol_int256_t(3));";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Tests all supported typecasts in their most explicit forms.
BOOST_AUTO_TEST_CASE(type_casting)
{
    char const* text = R"(
        contract A {
            address a;
            int s;
            uint u;
            bool b;
            function f() public view {
                address(5.0);
                address(a); int(a); uint(a);
                address(s); int(s); uint(s);
                address(u); int(u); uint(u);
                bool(b);
                address(this);
            }
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(
        func, statedata, newcalls, converter
    ).convert();
    expected << "{";
    expected << "((int)(global_index_const_5));";
    expected << "(self->user_a).v;";
    expected << "(self->user_a).v;";
    expected << "((unsigned int)((self->user_a).v));";
    expected << "(self->user_s).v;";
    expected << "(self->user_s).v;";
    expected << "((unsigned int)((self->user_s).v));";
    expected << "((int)((self->user_u).v));";
    expected << "((int)((self->user_u).v));";
    expected << "(self->user_u).v;";
    expected << "(self->user_b).v;";
    expected << "((self)->model_address).v;";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Tests that storage variables may be declared within a function, and that they
// are deferencable.
BOOST_AUTO_TEST_CASE(storage_variable_resolution)
{
    char const* text = R"(
        contract A {
            struct B { int i; }
            B b;
            function f() public view {
                B storage b_ref = b;
                b_ref.i;
            }
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(
        func, statedata, newcalls, converter
    ).convert();
    expected << "{";
    expected << "struct A_StructB*func_user_b__ref=&(self->user_b);";
    expected << "((func_user_b__ref)->user_i).v;";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Tests that storage variables may be assigned to from storage. A reference to
// said storage must be acquired.
BOOST_AUTO_TEST_CASE(storage_variable_assignment)
{
    char const* text = R"(
        contract A {
            struct B { int i; }
            B b;
            function f() public view {
                B storage b_ref = b;
                b_ref = b;
            }
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(
        func, statedata, newcalls, converter
    ).convert();
    expected << "{";
    expected << "struct A_StructB*func_user_b__ref=&(self->user_b);";
    expected << "(func_user_b__ref)=(&(self->user_b));";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Regression test to ensure "else if" is not contracted into "elseif".
BOOST_AUTO_TEST_CASE(else_if_formatting_regression)
{
    char const* text = R"(
        contract A {
            function f() public view {
                if (true) {} else if (false) {}
            }
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(
        func, statedata, newcalls, converter
    ).convert();
    expected << "{";
    expected << "if(1){}";
    expected << "else if(0){}";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Ensures that wrapped return types are unwrapped.
BOOST_AUTO_TEST_CASE(function_call_unwraps_data)
{
    char const* text = R"(
        contract A {
            function f() public pure returns (uint) { return 5; }
            function g() public pure { f(); }
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& fncs = ctrt.definedFunctions();

    auto func = (fncs[0]->name() == "g") ? fncs[0] : fncs[1];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream actual, expect;
    FunctionBlockConverter fbc(*func, statedata, newcalls, converter);
    fbc.set_for(FunctionSpecialization(*func));
    actual << *fbc.convert();
    expect << "{"
           << "(Method_A_Funcf(self,sender,value,blocknum,timestamp"
           << ",Init_sol_bool_t(0),origin)).v;"
           << "}";
    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

BOOST_AUTO_TEST_CASE(modifier_nesting)
{
    char const* text = R"(
        contract A {
            modifier modA() {
                _;
                _;
                return;
            }
            modifier modB() {
                _;
                return;
            }
            function f() public modA() modB() pure { }
            function g() public modA() modB() { }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& fncs = ctrt.definedFunctions();

    auto const& func_f = (fncs[0]->name() == "f") ? *fncs[0] : *fncs[1];
    auto const& func_g = (fncs[0]->name() == "f") ? *fncs[1] : *fncs[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ModifierBlockConverter::Factory f_factory(func_f, converter.get_name(func_f));
    ModifierBlockConverter::Factory g_factory(func_g, converter.get_name(func_g));

    ostringstream f0_actual, f0_expect;
    f0_actual << *f_factory.generate(
        0, statedata, newcalls, converter
    ).convert();
    f0_expect << "{"
              << "Method_A_Funcf_mod1(self,sender,value,blocknum,timestamp"
              << ",Init_sol_bool_t(0),origin);"
              << "Method_A_Funcf_mod1(self,sender,value,blocknum,timestamp"
              << ",Init_sol_bool_t(0),origin);"
              << "return;"
              << "}";
    BOOST_CHECK_EQUAL(f0_actual.str(), f0_expect.str());

    ostringstream g0_actual, g0_expect;
    g0_actual << *g_factory.generate(
        0, statedata, newcalls, converter
    ).convert();
    g0_expect << "{";
    g0_expect << "Method_A_Funcg_mod1(self,sender,value,blocknum,timestamp"
              << ",Init_sol_bool_t(0),origin);";
    g0_expect << "Method_A_Funcg_mod1(self,sender,value,blocknum,timestamp"
              << ",Init_sol_bool_t(0),origin);";
    g0_expect << "return;";
    g0_expect << "}";
    BOOST_CHECK_EQUAL(g0_actual.str(), g0_expect.str());

    ostringstream f1_actual, f1_expect;
    f1_actual << *f_factory.generate(
        1, statedata, newcalls, converter
    ).convert();
    f1_expect << "{";
    f1_expect << "Method_A_Funcf_base(self,sender,value,blocknum,timestamp"
              << ",Init_sol_bool_t(0),origin);";
    f1_expect << "return;";
    f1_expect << "}";
    BOOST_CHECK_EQUAL(f1_actual.str(), f1_expect.str());

    ostringstream g1_actual, g1_expect;
    g1_actual << *g_factory.generate(
        1, statedata, newcalls, converter
    ).convert();
    g1_expect << "{";
    g1_expect << "Method_A_Funcg_base(self,sender,value,blocknum,timestamp"
              << ",Init_sol_bool_t(0),origin);";
    g1_expect << "return;";
    g1_expect << "}";;
    BOOST_CHECK_EQUAL(g1_actual.str(), g1_expect.str());
}

BOOST_AUTO_TEST_CASE(modifier_retval)
{
    char const* text = R"(
        contract A {
            modifier modA() {
                _;
                return;
                _;
            }
            function f() modA() public returns (int) { return 5; }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream expected, actual;
    ModifierBlockConverter::Factory factory(func, converter.get_name(func));
    actual << *factory.generate(0, statedata, newcalls, converter).convert();
    expected << "{";
    expected << "sol_int256_t func_model_rv;";
    expected << "(func_model_rv)=(Method_A_Funcf_base(self,sender,value"
             << ",blocknum,timestamp,Init_sol_bool_t(0),origin));";
    expected << "return func_model_rv;";
    expected << "(func_model_rv)=(Method_A_Funcf_base(self,sender,value"
             << ",blocknum,timestamp,Init_sol_bool_t(0),origin));";
    expected << "return func_model_rv;";
    expected << "}";

    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

BOOST_AUTO_TEST_CASE(modifier_args)
{
    char const* text = R"(
        contract A {
            modifier modA(int a, int b) {
                require(a > b);
                _;
            }
            function f(int a, int b) modA(b + 5, a) public { }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    NewCallGraph newcalls;
    newcalls.record(unit);

    FullSourceContractDependance analyzer(unit);
    ContractDependance deps(analyzer);

    CallState statedata(deps);

    ostringstream expected, actual;
    ModifierBlockConverter::Factory factory(func, converter.get_name(func));
    actual << *factory.generate(0, statedata, newcalls, converter).convert();
    expected << "{";
    expected << "sol_int256_t func_user_a=Init_sol_int256_t("
             << "((func_model_b).v)+(5));";
    expected << "sol_int256_t func_user_b=Init_sol_int256_t("
             << "(func_model_a).v);";
    expected << "sol_require(((func_user_a).v)>((func_user_b).v),0);";
    expected << "Method_A_Funcf_base(self,sender,value,blocknum,timestamp"
             << ",Init_sol_bool_t(0),origin,func_model_a,func_model_b);";
    expected << "}";

    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
