#include "../Solver.hpp"

int main(void)
{
	Solver s([](scalar *args) {
		return args[0];
	}, 1, 100, 5);
	auto b = s.run().format();
	std::printf("Best: %s\n", b.c_str());
	return 0;
}