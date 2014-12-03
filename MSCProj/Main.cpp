#include <cstdio>
#include <vector>
#include <ctime>
#include <algorithm>
#include <iterator>
#include <fstream>

// include & define dependent libraries and types
#include <map>
#include <utility>
typedef struct { float left, top, right, bottom; } rect_t;
// include the mamdani matrix generated with gen_cleaned.pl
#include <mamdani.ixx>

int main(int argc, char* argv[])
{
	// time i/o separately since it takes a while
	volatile clock_t cstartio = clock();
#pragma warning(disable:4996)
	FILE* f = fopen("date.txt", "r");
	std::fstream g("output.txt", std::ios::out);
	// store the input in a vector for faster processing
	std::vector<float> inputs;
	inputs.reserve(10002);

	while (!feof(f))
	{
		float d;
		fscanf(f, "%f", &d);
		inputs.push_back(d);
	}

	std::vector<float> outputs(inputs.size());

	volatile clock_t cstopio = clock();
	volatile double spentio = (double)(cstopio - cstartio) / CLOCKS_PER_SEC;

	printf("Took %lfs to _read_ %ld values\n", spentio, inputs.size());

	// time the computation
	volatile clock_t cstart = clock();

	// state variables
	float lastErr = 0.0; // previous error; initially 0
	auto func = [&](float err) -> float {
		// compute the derivate of the error using the previous value
		float derr = err - lastErr;

		// locate the partition the current point falls in
		auto found = std::find_if(g_mamdani.begin(), g_mamdani.end(), [&derr, &err](decltype(g_mamdani.front())& o) -> bool {
			return o.first.left <= err
				&& o.first.right > err
				&& o.first.top <= derr
				&& o.first.bottom > derr;
		});

		float val = 0.0;
		if (found != g_mamdani.end()) val = found->second;
		
		// store the result
		return val;

		// update state
		lastErr = err;
	};

	// process all inputs with our stateful function
	std::transform(inputs.begin(), inputs.end(), outputs.begin(), func);

	volatile clock_t cend = clock();

	// report spent time
	volatile double spent = (double)(cend - cstart) / CLOCKS_PER_SEC;

	printf("Took %lfs to process %ld values\n", spent, inputs.size());

	// time the output separately
	cstartio = clock();
	std::copy(outputs.begin(), outputs.end(), std::ostream_iterator<float>(g, "\n"));
	cstopio = clock();
	spentio = (double)(cstopio - cstartio) / CLOCKS_PER_SEC;
	printf("Took %lfs to write %ld values to disk\n", spentio, outputs.size());

	return 0;
}