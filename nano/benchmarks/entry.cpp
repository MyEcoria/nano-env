#include <nano/lib/logging.hpp>

#include <benchmark/benchmark.h>

// Customized main, based on BENCHMARK_MAIN macro
int main (int argc, char ** argv)
{
	nano::logger::initialize_dummy ();

	benchmark::MaybeReenterWithoutASLR (argc, argv);
	char arg0_default[] = "benchmark";
	char * args_default = reinterpret_cast<char *> (arg0_default);
	if (!argv)
	{
		argc = 1;
		argv = &args_default;
	}
	::benchmark::Initialize (&argc, argv);
	if (::benchmark::ReportUnrecognizedArguments (argc, argv))
		return 1;
	::benchmark::RunSpecifiedBenchmarks ();
	::benchmark::Shutdown ();
	return 0;
}

static void BM_StringCreation (benchmark::State & state)
{
	for (auto _ : state)
		std::string empty_string;
}
// Register the function as a benchmark
BENCHMARK (BM_StringCreation);

// Define another benchmark
static void BM_StringCopy (benchmark::State & state)
{
	std::string x = "hello";
	for (auto _ : state)
		std::string copy (x);
}
BENCHMARK (BM_StringCopy);