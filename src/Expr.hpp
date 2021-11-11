#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "ar.hpp"

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
	struct nonempty_init{};
	static inline constexpr nonempty_init nonempty_init_v{};

	Expr(void)
	{
	}
	Expr(nonempty_init)
	{
		push(Op::Constant);
		push(0.0);
		push(Op::End);
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
			m_allocated = ar::max(m_allocated, static_cast<size_t>(1)) * static_cast<size_t>(2);
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
		return pos + sizeof(*pos);
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
		&Expr::e_mul,		// 8
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
};