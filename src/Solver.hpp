#pragma once

#include <ctime>
#include <limits>
#include <random>
#include "Expr.hpp"

class HqRng
{
	std::mt19937_64 m_g;

public:
	HqRng(void)
	{
		m_g.seed(std::time(nullptr));
	}

	size_t nextu(void)
	{
		return m_g();
	}

	double nextz(void)
	{
		return nextu() / static_cast<double>(static_cast<size_t>(-1));
	}

	double nextn(void)
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
	Solver(Model *model, size_t arg_count, size_t gen_size, size_t max_mut) :
		m_model(model),
		m_arg_count(arg_count),
		m_gen_size(gen_size),
		m_max_mut(max_mut),
		m_pool_size(1 + m_gen_size)
	{
		m_exprs = new Expr[m_pool_size];
	}
	~Solver(void)
	{
		delete[] m_exprs;
	}

	scalar cost(Expr &e)
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

	const Expr& run(void)
	{
		Expr *fav = &m_exprs[0];
		fav->push(Op::Constant);
		fav->push(0.0);
		fav->push(Op::End);
		for (size_t g = 0; g < 20; g++) {
			Expr *best = nullptr;
			double best_cost = std::numeric_limits<scalar>::infinity();
			for (size_t i = 0; i < m_pool_size; i++) {
				auto c = &m_exprs[i];
				if (c != fav)
					fav->shuffle(*c, m_rng, m_max_mut, m_arg_count);
				auto ccost = cost(*c);
				if (ccost < best_cost) {
					best = c;
					best_cost = ccost;
				}
			}
			fav = best;
		}
		return *fav;
	}
};