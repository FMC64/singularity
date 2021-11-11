#pragma once

#include <cstdio>
#include <limits>
#include <random>
#include "Expr.hpp"

class HqRng
{
	std::mt19937_64 m_g;

public:
	HqRng(size_t seed)
	{
		m_g.seed(seed);
	}

	size_t nextu(void)
	{
		return m_g();
	}

	scalar nextz(void)
	{
		return nextu() / static_cast<scalar>(static_cast<size_t>(-1));
	}

	scalar nextn(void)
	{
		return (nextz() -.5) * 2.0;
	}
};

using Model = scalar (scalar *args);

class Solver
{
	Model *m_model;
	size_t m_arg_count;
	size_t m_gen_size;
	size_t m_max_mut;
	size_t m_pool_size;
	Expr *m_exprs;
	HqRng m_rng;

public:
	Solver(size_t seed, Model *model, size_t arg_count, size_t gen_size, size_t max_mut) :
		m_model(model),
		m_arg_count(arg_count),
		m_gen_size(gen_size),
		m_max_mut(max_mut),
		m_pool_size(1 + m_gen_size),
		m_rng(seed)
	{
		m_exprs = new Expr[m_pool_size];
	}
	~Solver(void)
	{
		delete[] m_exprs;
	}

	inline scalar cost(Expr &e)
	{
		scalar width = 10.0, dw = 0.1;
		size_t it = 0;
		scalar res = 0.0;
		for (scalar i = -width; i < width; i += dw) {
			auto args = &i;
			auto c = std::fabs(e.eval(args) - m_model(args));
			res = (res * static_cast<scalar>(it) + c) / (static_cast<scalar>(it + 1));
			it++;
		}
		return res;
	}

	inline scalar cost_full(Expr &e)
	{
		return cost(e) + static_cast<scalar>(e.getNodeCount()) * 0.001;
	}

	inline void opt(Expr &e)
	{
		size_t c_count;
		scalar *cs[e.getNodeCount()];
		e.getConstants(c_count, cs);
		scalar o[c_count];
		for (size_t it = 0; it < 4; it++) {
			for (size_t i = 0; i < c_count; i++) {
				auto v = *cs[i];
				static constexpr scalar de = 0.0000001;
				auto s1 = cost(e);
				*cs[i] += de;
				auto s2 = cost(e);
				*cs[i] = v;
				o[i] -= s1 / ((s2 - s1) / de);
			}
			for (size_t i = 0; i < c_count; i++)
				*cs[i] = o[i];
		}
	}

	const Expr& run(void)
	{
		Expr *fav = &m_exprs[0];
		fav->push(Op::Constant);
		fav->push(0.0);
		fav->push(Op::End);
		for (size_t g = 0; g < 1000; g++) {
			Expr *best = nullptr;
			scalar best_cost = std::numeric_limits<scalar>::infinity();
			for (size_t i = 0; i < m_pool_size; i++) {
				auto c = &m_exprs[i];
				if (c != fav) {
					fav->shuffle(*c, m_rng, fav->getNodeCount() + m_max_mut, m_arg_count);
					opt(*c);
				}
				auto ccost = cost_full(*c);
				if (ccost < best_cost) {
					best = c;
					best_cost = ccost;
				}
			}
			fav = best;
			//std::printf("Gen %zu: %g\n", g, best_cost);
			if (best_cost - static_cast<scalar>(fav->getNodeCount()) * 0.001 < 0.00000001)
				break;
		}
		return *fav;
	}
};