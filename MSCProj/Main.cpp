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
		fscanf(f, "%lf", &d);
		inputs.push_back(d);
	}
	volatile clock_t cstopio = clock();
	volatile double spentio = (double)(cstopio - cstartio) / CLOCKS_PER_SEC;

	printf("Took %fs to _read_ %ld values\n", spentio, inputs.size());

	// time the computation
	volatile clock_t cstart = clock();

	std::vector<float> outputs(inputs.size());

	// state variables
	float lastErr = 0.0; // previous error; initially 0
	auto func = [&](float err) -> float {
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
		
		// print out the result
		//fprintf(g, "(%9.5lf, %9.5lf) -> %9.5lf\n", err, derr, val);
		return val;

		// update state
		lastErr = err;
	};

	// process all inputs with our stateful function
	std::transform(inputs.begin(), inputs.end(), outputs.begin(), func);

	volatile clock_t cend = clock();

	// report spent time
	volatile double spent = (double)(cend - cstart) / CLOCKS_PER_SEC;

	printf("Took %fs to process %ld values\n", spent, inputs.size());

	cstartio = clock();
	std::copy(outputs.begin(), outputs.end(), std::ostream_iterator<double>(g, "\n"));
	cstopio = clock();
	spentio = (double)(cstopio - cstartio) / CLOCKS_PER_SEC;
	printf("Took %fs to write %ld values to disk\n", spentio, outputs.size());

	return 0;
}