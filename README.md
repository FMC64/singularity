# singularity
A stochastic inferrer of arbitrary real-to-real functions

## Initial public release commentary (31st of August, 2023)

This is perhaps my most experimental project at this time. This project aims at guessing what is the equation of a provided opaque real-to-real function.

It works pretty well for simple functions but it the convergence time seems exponential to straight up infinite for more involved ones. Maybe a bit of tweaking on the population ajustment at each generation can yield better results on specific instances.

I'm happy with the design of the evaluator, performance-wise. I was able to maximize the locality with a sequential micro-code, which I still find to ideal as of today. To make something more efficient, a dynamic recompiler would be needed, which is suddenly not portable anymore.

Overall it was fun to work on this small cute meta project!
