#include "test.hpp"

#include "Expr.hpp"

test_case(expr_0)
{
	Expr e;
	e.push(Op::Constant);
	e.push(static_cast<scalar>(2.3));
	e.push(Op::End);
	test_assert(e.eval(nullptr) == 2.3);
	test_assert(e.is_coherent());
}

test_case(expr_1)
{
	Expr e;
	double a = 5.2, b = 9.3;
	e.push(Op::Constant);
	e.push(static_cast<scalar>(a));
	e.push(Op::Mul);
	e.push(Op::Constant);
	e.push(static_cast<scalar>(b));
	e.push(Op::End);
	e.push(Op::End);
	test_assert(e.eval(nullptr) == a * b);
	test_assert(e.is_coherent());
}

test_case(expr_2)
{
	Expr e;
	double a = 5.2, b = 9.3;
	e.push(Op::Constant);
	e.push(static_cast<scalar>(a));
	e.push(Op::Add);
	e.push(Op::Constant);
	e.push(static_cast<scalar>(b));
	e.push(Op::End);
	e.push(Op::Log);
	e.push(Op::End);
	test_assert(e.eval(nullptr) == std::log(a + b));
	test_assert(e.is_coherent());
}

test_case(expr_4)
{
	Expr e;
	e.push(Op::Arg);
	e.push(static_cast<arg>(0));
	e.push(Op::Add);
	e.push(Op::Arg);
	e.push(static_cast<arg>(1));
	e.push(Op::End);
	e.push(Op::Exp);
	e.push(Op::End);
	double args[] = {
		-8.6,
		3.0
	};
	test_assert(e.eval(args) == std::exp(args[0] + args[1]));
	test_assert(e.is_coherent());
}