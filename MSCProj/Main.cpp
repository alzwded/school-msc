#include <cstdio>
#include <vector>
#include <ctime>
#include <algorithm>

#include <map>
#include <utility>
typedef struct { double left, top, right, bottom; } rect_t;
#include <mamdani.ixx>

int main(int argc, char* argv[])
{
	clock_t cstartio = clock();
#pragma warning(disable:4996)
	FILE* f = fopen("date.txt", "r");
	FILE* g = fopen("output.txt", "w");
	std::vector<double> inputs;
	inputs.reserve(10000);

	while (!feof(f))
	{
		double d;
		fscanf(f, "%lf", &d);
		inputs.push_back(d);
	}
	clock_t cstopio = clock();
	double spentio = (double)(cstopio - cstartio) / CLOCKS_PER_SEC;

	printf("Took %fs to _read_ %ld values\n", spentio, inputs.size());

	clock_t cstart = clock();

	double derr = 0.0;
	double lastErr = 0.0;
	auto func = [&](double err) -> void {
		derr = err - lastErr;

		auto found = std::find_if(g_mamdani.begin(), g_mamdani.end(), [&derr, &err](decltype(g_mamdani.front())& o) -> bool {
			return o.first.left <= err
				&& o.first.right > err
				&& o.first.top <= derr
				&& o.first.bottom > derr;
		});

		double val = 0.0;
		if (found != g_mamdani.end()) val = found->second;
		
		fprintf(g, "(%9.5lf, %9.5lf) -> %9.5lf\n", err, derr, val);

		lastErr = err;
	};

	std::for_each(inputs.begin(), inputs.end(), func);

	clock_t cend = clock();

	double spent = (double)(cend - cstart) / CLOCKS_PER_SEC;

	printf("Took %fs to process %ld values\n", spent, inputs.size());

	return 0;
}