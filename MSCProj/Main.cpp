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
typedef struct { float v1, v2, v3, v4; } com_t;
// include the mamdani matrix generated with gen_cleaned.pl
#include <mamdani.ixx>

#if 0
// not needed...
static inline float pDistance(std::pair<float, float> a, std::pair<float, float> b, float maxDistance2)
{
	__declspec(align(16)) float va[4] = {
		0.f,
		0.f,
		a.second,
		a.first,
	};
	__declspec(align(16)) float vb[4] = {
		0.f,
		0.f,
		b.second,
		b.first,
	};
	__m128 reg1 = _mm_load_ps(va);
	__m128 reg2 = _mm_load_ps(vb);
	reg1 = _mm_sub_ps(reg1, reg2);
	reg2 = reg1;
	reg1 = _mm_mul_ps(reg1, reg2);
	_mm_store_ps(va, reg1);
	float temp = (va[2] + va[3]) / maxDistance2;
	if (temp > maxDistance2) return 1;
	else return temp / maxDistance2;
}
#endif

int main(int argc, char* argv[])
{
	// time i/o separately since it takes a while
	volatile clock_t cstartio = clock();
#pragma warning(disable:4996)
	FILE* f = fopen("date.txt", "r");
	//std::fstream g("output.txt", std::ios::out);
	FILE* g = fopen("output.txt", "w");
	// store the input in a vector for faster processing
	std::vector<float> inputs;
	inputs.reserve(10002);

	while (!feof(f))
	{
		float d;
		fscanf(f, "%f", &d);
		inputs.push_back(d);
	}

#ifdef STRESS_TEST
	inputs.insert(inputs.end(), inputs.begin(), inputs.end());
	inputs.insert(inputs.end(), inputs.begin(), inputs.end());
	inputs.insert(inputs.end(), inputs.begin(), inputs.end());
	inputs.insert(inputs.end(), inputs.begin(), inputs.end());
	inputs.insert(inputs.end(), inputs.begin(), inputs.end());
	inputs.insert(inputs.end(), inputs.begin(), inputs.end());
	inputs.insert(inputs.end(), inputs.begin(), inputs.end());
	inputs.insert(inputs.end(), inputs.begin(), inputs.end());
#endif

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
		if (found != g_mamdani.end()) {
			// setup values
			float errUnit = (err - found->first.left) / g_extents[0];
			float derrUnit = (derr - found->first.top) / g_extents[1];
			static __declspec(align(16)) float leftTerms[4] = { 1.f, 0.f, 1.f, 0.f };
			__declspec(align(16)) float va[4] = {
				-errUnit,
				errUnit,
				-errUnit,
				errUnit
			};
			static __declspec(align(16)) float rightTerms[4] = { 1.f, 1.f, 0.f, 0.f };
			__declspec(align(16)) float vb[4] = {
				-derrUnit,
				-derrUnit,
				derrUnit,
				derrUnit
			};
			// computations
			__m128 reg1, reg2, reg3;

			reg1 = _mm_load_ps(leftTerms);
			reg3 = _mm_load_ps(va);
			reg1 = _mm_add_ps(reg1, reg3);

			reg2 = _mm_load_ps(rightTerms);
			reg3 = _mm_load_ps(vb);
			reg2 = _mm_add_ps(reg2, reg3);

			reg1 = _mm_mul_ps(reg1, reg2);

			memcpy(va, (float*)(&found->second), 4 * sizeof(float));
			reg2 = _mm_load_ps(va);
			reg1 = _mm_mul_ps(reg1, reg2);

			_mm_store_ps(va, reg1);

			val = va[0] + va[1] + va[2] + va[3];
		}
		
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
	//std::copy(outputs.begin(), outputs.end(), std::ostream_iterator<float>(g, "\n"));	
	lastErr = 0;
	for (size_t i = 0; i < inputs.size(); ++i) {
		fprintf(g, "%10.6f, %10.6f => %10.6f\n", inputs[i], inputs[i] - lastErr, outputs[i]);
		lastErr = inputs[i];
	}
	cstopio = clock();
	spentio = (double)(cstopio - cstartio) / CLOCKS_PER_SEC;
	printf("Took %lfs to write %ld values to disk\n", spentio, outputs.size());

	return 0;
}