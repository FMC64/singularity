#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <sstream>
#include "ar.hpp"

template <typename T>
concept Rngable = requires(T v) {
	requires std::is_same_v<decltype(v.nextu()), size_t>;
	requires std::is_same_v<decltype(v.nextz()), double>;
	requires std::is_same_v<decltype(v.nextn()), double>;
};

using scalar = double;
using arg = uint8_t;

enum class Op : uint8_t {
	Constant = 0,	// Starting values
	Arg = 1,

	Opp = 2,		// Self-modifiers
	Inv = 3,
	Sin = 4,
	Exp = 5,
	Log = 6,

	Add = 7,		// Associative
	Mul = 8,

	End = 9
};

static inline constexpr auto op_self = Op::Opp;
static inline constexpr auto op_ass = Op::Add;

class Expr {
	size_t m_size = 0;
	size_t m_allocated = 0;
	uint8_t *m_buf = nullptr;

	size_t m_node_count = 0;

public:
	Expr(void)
	{
	}
	Expr(size_t size, uint8_t *data)
	{
		alloc(size);
		std::memcpy(m_buf, data, size);
	}
	~Expr(void)
	{
		std::free(m_buf);
	}

	void alloc(size_t n)
	{
		size_t w = m_size + n;
		if (w > m_allocated) {
			m_allocated = ar::max(m_allocated * static_cast<size_t>(2), w);
			m_buf = reinterpret_cast<uint8_t*>(std::realloc(m_buf, m_allocated));
		}
	}

	template <typename T>
	void push(T &&v)
	{
		static constexpr size_t s = sizeof(std::remove_reference_t<T>);
		alloc(s);
		*reinterpret_cast<std::remove_reference_t<T>*>(m_buf + m_size) = v;
		m_size += s;
		if constexpr (std::is_same_v<std::remove_reference_t<T>, Op>)
			if (v != Op::End)
				m_node_count++;
	}

private:
	inline uint8_t* e_constant(uint8_t *pos, scalar *, scalar &res) const
	{
		res = *reinterpret_cast<scalar*>(pos);
		return pos + sizeof(scalar);
	}

	inline uint8_t* e_arg(uint8_t *pos, scalar *args, scalar &res) const
	{
		res = args[*reinterpret_cast<arg*>(pos)];
		return pos + sizeof(arg);
	}

	#define EXPR_SELF_MOD(name, op) inline uint8_t* name(uint8_t *pos, scalar *, scalar &res) const	\
	{												\
		res = op;										\
		return pos;										\
	}												\

	EXPR_SELF_MOD(e_opp, -res)
	EXPR_SELF_MOD(e_inv, 1.0 / res)
	EXPR_SELF_MOD(e_sin, std::sin(res))
	EXPR_SELF_MOD(e_exp, std::exp(res))
	EXPR_SELF_MOD(e_log, std::log(res))

	#undef EXPR_SELF_MOD

	#define EXPR_ASS(name, op) inline uint8_t* name(uint8_t *pos, scalar *args, scalar &res) const	\
	{												\
		double other;										\
		auto r = e(pos, args, other);								\
		res = op;										\
		return r;										\
	}

	EXPR_ASS(e_add, res + other)
	EXPR_ASS(e_mul, res * other)

	#undef EXPR_ASS

	using Handler = uint8_t* (Expr::*)(uint8_t* pos, scalar *args, scalar &res) const;
	static inline Handler e_table[] = {
		&Expr::e_constant,	// 0
		&Expr::e_arg,		// 1

		&Expr::e_opp,		// 2
		&Expr::e_inv,		// 3
		&Expr::e_sin,		// 4
		&Expr::e_exp,		// 5
		&Expr::e_log,		// 6

		&Expr::e_add,		// 7
		&Expr::e_mul		// 8
	};

	inline uint8_t* e(uint8_t *pos, scalar *args, scalar &res) const
	{
		double acc;
		while (true) {
			auto i = *pos++;
			if (static_cast<Op>(i) == Op::End)
				break;
			pos = (this->*e_table[i])(pos, args, acc);
		}
		res = acc;
		return pos;
	}

	inline uint8_t* skip(uint8_t *pos) const
	{
		while (true) {
			auto o = *pos++;
			auto op = static_cast<Op>(o);
			if (op == Op::End)
				break;
			if (op == Op::Constant)
				pos += sizeof(scalar);
			else if (op == Op::Arg)
				pos += sizeof(arg);
			else if (o < static_cast<uint8_t>(op_ass));	// self-mod
			else	// ass
				pos = skip(pos);
		}
		return pos;
	}


	inline uint8_t* count_nodes(uint8_t *pos, size_t &res) const
	{
		while (true) {
			auto o = *pos++;
			auto op = static_cast<Op>(o);
			if (op == Op::End)
				break;
			res++;
			if (op == Op::Constant)
				pos += sizeof(scalar);
			else if (op == Op::Arg)
				pos += sizeof(arg);
			else if (o < static_cast<uint8_t>(op_ass));	// self-mod
			else	// ass
				pos = count_nodes(pos, res);
		}
		return pos;
	}

	inline uint8_t* f_constant(uint8_t *pos, std::string &res) const
	{
		std::stringstream ss;
		ss << *reinterpret_cast<scalar*>(pos);
		res = ss.str();
		return pos + sizeof(scalar);
	}

	inline uint8_t* f_arg(uint8_t *pos, std::string &res) const
	{
		std::stringstream ss;
		ss << "args[" << static_cast<size_t>(*reinterpret_cast<arg*>(pos)) << "]";
		res = ss.str();
		return pos + sizeof(arg);
	}

	inline uint8_t* f_opp(uint8_t *pos, std::string &res) const
	{
		std::stringstream ss;
		ss << "-" << res;
		res = ss.str();
		return pos;
	}

	#define EXPR_FFUN(name, op) inline uint8_t* name(uint8_t *pos, std::string &res) const	\
	{											\
		std::stringstream ss;								\
		ss << op << res << ")";								\
		res = ss.str();									\
		return pos;									\
	}											\

	EXPR_FFUN(f_inv, "(1.0 / ")
	EXPR_FFUN(f_sin, "sin(")
	EXPR_FFUN(f_exp, "exp(")
	EXPR_FFUN(f_log, "log(")

	#define EXPR_FASS(name, op) inline uint8_t* name(uint8_t *pos, std::string &res) const	\
	{											\
		std::string other;								\
		pos = f(pos, other);								\
		std::stringstream ss;								\
		ss << "(" << res << op << other << ")";						\
		res = ss.str();									\
		return pos;									\
	}											\

	EXPR_FASS(f_add, " + ")
	EXPR_FASS(f_mul, " * ")

	using FHandler = uint8_t* (Expr::*)(uint8_t* pos, std::string &res) const;
	static inline FHandler f_table[] = {
		&Expr::f_constant,	// 0
		&Expr::f_arg,		// 1

		&Expr::f_opp,		// 2
		&Expr::f_inv,		// 3
		&Expr::f_sin,		// 4
		&Expr::f_exp,		// 5
		&Expr::f_log,		// 6

		&Expr::f_add,		// 7
		&Expr::f_mul		// 8
	};

	inline uint8_t* f(uint8_t *pos, std::string &res) const
	{
		while (true) {
			auto i = *pos++;
			if (static_cast<Op>(i) == Op::End)
				break;
			pos = (this->*f_table[i])(pos, res);
		}
		return pos;
	}

	template <Rngable Rng>
	inline uint8_t* s(Expr &dst, Rng &rng, uint8_t *pos, size_t i, const uint8_t *muts, const size_t arg_count) const
	{
		while (true) {
			auto o = *pos++;
			auto op = static_cast<Op>(o);
			dst.push(op);
			if (op == Op::End)
				break;
			auto m = muts[i++];
			if (op == Op::Constant) {
				dst.push(*reinterpret_cast<scalar*>(pos));
				pos += sizeof(scalar);
			} else if (op == Op::Arg) {
				dst.push(*reinterpret_cast<arg*>(pos));
				pos += sizeof(arg);
			} else if (o < static_cast<uint8_t>(op_ass)) {		// self-mod
			} else {	// ass
				pos = s(dst, rng, pos, i, muts, arg_count);
			}
		}
		return pos;
	}

public:
	inline scalar eval(scalar *args) const
	{
		scalar res;
		e(m_buf, args, res);
		return res;
	}

	inline bool is_coherent(void) const
	{
		size_t nc = 0;
		count_nodes(m_buf, nc);
		return m_node_count == nc;
	}

	inline std::string format(void) const
	{
		std::string res;
		f(m_buf, res);
		return res;
	}

	template <Rngable Rng>
	inline void shuffle(Expr &dst, Rng &rng, size_t max_mut, size_t arg_count) const
	{
		size_t mutc = rng.nextu() % max_mut;
		uint8_t muts[m_node_count];	// 0: identity, 1: add, 2: remove
		std::memset(muts, 0, sizeof(muts));
		for (size_t i = 0; i < mutc; i++)
			muts[rng.nextu() % m_node_count] = (rng.nextu() & 1) + 1;

		dst.m_size = 0;
		dst.m_node_count = 0;
		s(dst, rng, m_buf, 0, muts, arg_count);
	}
};