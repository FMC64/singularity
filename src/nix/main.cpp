#include "../Solver.hpp"

int main(void)
{
	Solver s([](scalar *args) {
		return args[0];
	}, 1, 100, 5);
	s.run();
	return 0;
}