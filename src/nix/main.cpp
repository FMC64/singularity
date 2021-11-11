#include <ctime>
#include "../Solver.hpp"

int main(int argc, char **argv)
{
	argc--;
	argv++;

	size_t seed;
	if (argc == 1)
		seed = std::stoull(argv[0]);
	else
		seed = std::time(nullptr);

	Solver s(seed, [](scalar *args) {
		return args[0] * args[0];
	}, 1, 100, 10);
	auto best = s.run();
	auto b = best.format();
	std::printf("Seed: %zu\n", seed);
	std::printf("Best: %s\n", b.c_str());
	return 0;
}