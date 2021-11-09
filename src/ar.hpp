#pragma once

#include <type_traits>

namespace ar {

template <typename Ta, typename Tb>
static inline constexpr std::remove_reference_t<Ta> max(Ta &&a, Tb &&b)
{
	if (a > b)
		return a;
	else
		return b;
}



}