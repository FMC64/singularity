#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <sstream>
#include "ar.hpp"

using scalar = double;
using arg = uint8_t;

template <typename T>
concept Rngable = requires(T v) {
	requires std::is_same_v<decltype(v.nextu()), size_t>;
	requires std::is_same_v<decltype(v.nextz()), scalar>;
	requires std::is_same_v<decltype(v.nextn()), scalar>;
};

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
		//*reinterpret_cast<std::remove_reference_t<T>*>(m_buf + m_size) = v;
		std::memcpy(m_buf + m_size, &v, s);
		m_size += s;
		if constexpr (std::is_same_v<std::remove_reference_t<T>, Op>)
			if (v != Op::End)
				m_node_count++;
	}

private:
	inline uint8_t* e_constant(uint8_t *pos, scalar *, scalar &res) const
	{
		//res = *reinterpret_cast<scalar*>(pos);
		std::memcpy(&res, pos, sizeof(scalar));
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
		scalar other;										\
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
		scalar acc;
		while (true) {
			auto i = *pos++;
			if (static_cast<Op>(i) == Op::End)
				break;
			size_t ndx = static_cast<size_t>(i);
			auto h = e_table[ndx];
			pos = (this->*h)(pos, args, acc);
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
			size_t ndx = static_cast<size_t>(i);
			auto fh = f_table[ndx];
			pos = (this->*fh)(pos, res);
		}
		return pos;
	}

	template <Rngable Rng>
	void push_constant_value(Rng &rng)
	{
		push(static_cast<scalar>(rng.nextn()));
	}

	template <Rngable Rng>
	void push_constant_arg(Rng &rng, const size_t arg_count)
	{
		push(static_cast<arg>(rng.nextu() % arg_count));
	}

	template <bool PushOp = false, Rngable Rng>
	void push_constant_op(Op op, Rng &rng, const size_t arg_count)
	{
		if constexpr (PushOp)
			push(op);
		if (op == Op::Constant)
			push_constant_value(rng);
		else
			push_constant_arg(rng, arg_count);
	}

	template <Rngable Rng>
	void push_constant(Rng &rng, size_t arg_count)
	{
		push_constant_op<true>(static_cast<Op>(static_cast<uint8_t>(rng.nextu() & 1)), rng, arg_count);
	}

	template <Rngable Rng>
	inline uint8_t* s(Expr &dst, Rng &rng, uint8_t *pos, size_t &i, const uint8_t *muts, const size_t arg_count, bool inhibited, size_t last_constant) const
	{
		while (true) {
			auto o = *pos++;
			auto op = static_cast<Op>(o);
			if (op == Op::End) {
				dst.push(op);
				break;
			}

			auto m = muts[i++];
			inhibited = inhibited || m == 2;
			if (!inhibited) {
				if (o < static_cast<uint8_t>(op_self))
					last_constant = dst.m_size;
				dst.push(op);
			}
			if (op == Op::Constant) {
				if (!inhibited)
					dst.push(*reinterpret_cast<scalar*>(pos));
				else if (m == 2)
					dst.push_constant(rng, arg_count);
				pos += sizeof(scalar);
			} else if (op == Op::Arg) {
				if (!inhibited)
					dst.push(*reinterpret_cast<arg*>(pos));
				else if (m == 2)
					dst.push_constant(rng, arg_count);
				pos += sizeof(arg);
			} else if (o < static_cast<uint8_t>(op_ass)) {		// self-mod
			} else {	// ass
				pos = s(dst, rng, pos, i, muts, arg_count, inhibited, -1);
			}

			if (m == 1 && !inhibited) {
				uint8_t to_a = rng.nextu() % static_cast<uint8_t>(Op::End);
				if (to_a < static_cast<uint8_t>(op_self)) {	// constant
					if (last_constant != static_cast<size_t>(-1))
						dst.m_size = last_constant;
					dst.push(static_cast<Op>(to_a));
					dst.push_constant_op(static_cast<Op>(to_a), rng, arg_count);
				} else if (to_a < static_cast<uint8_t>(op_ass)) {	// self
					dst.push(static_cast<Op>(to_a));
				} else {
					dst.push(static_cast<Op>(to_a));
					dst.push_constant(rng, arg_count);
					dst.push(Op::End);
				}
			}
		}
		return pos;
	}

	inline uint8_t* c_constants(uint8_t *pos, size_t &count, scalar **dst)
	{
		while (true) {
			auto o = *pos++;
			auto op = static_cast<Op>(o);
			if (op == Op::End)
				break;
			if (op == Op::Constant) {
				dst[count++] = reinterpret_cast<scalar*>(pos);
				pos += sizeof(scalar);
			} else if (op == Op::Arg)
				pos += sizeof(arg);
			else if (o < static_cast<uint8_t>(op_ass));	// self-mod
			else	// ass
				pos = c_constants(pos, count, dst);
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
	inline Expr* shuffle(Expr &dst, Expr &opool, Rng &rng, size_t max_mut, size_t arg_count) const
	{
		size_t mutc = (rng.nextu() % max_mut) + 1;
		uint8_t muts[m_node_count];	// 0: identity, 1: add, 2: remove
		std::memset(muts, 0, sizeof(muts));
		Expr *last = const_cast<Expr*>(this);
		for (size_t i = 0; i < mutc; i++) {
			size_t w = rng.nextu() % m_node_count;
			muts[w] = (rng.nextu() & 1) + 1;

			if (i == 0) {
				dst.m_size = 0;
				dst.m_node_count = 0;
				size_t i = 0;
				last->s(dst, rng, m_buf, i, muts, arg_count, false, -1);
				last = &dst;
			} else {
				size_t i = 0;
				auto n = last == &dst ? &opool : &dst;
				last->s(*n, rng, m_buf, i, muts, arg_count, false, -1);
				last = n;
			}

			if (last->m_node_count == 0) {
				last->m_size = 0;
				last->m_node_count = 0;
				last->push(Op::Constant);
				last->push(0.0);
				last->push(Op::End);
			}

			muts[w] = 0;
		}
		return last;

	}

	size_t getNodeCount(void) const
	{
		return m_node_count;
	}

	void getConstants(size_t &count, scalar **dst)
	{
		count = 0;
		c_constants(m_buf, count, dst);
	}
};