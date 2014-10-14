#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <vector>
#include <cmath>
#include <ctime>

#define BUFFSIZE 1024 // size of the memory to examine at any one time

typedef struct {
    float v;
} data_t;

typedef struct {
    float k1;
    float k2;
} inputPair_t;

typedef struct {
    float k1_base, k1_n, k1_step;
    float k2_base, k2_n, k2_step;
} metadata_t;

int main(int argc, char* argv[])
{
	if(argc != 2) return 255;

    HANDLE hMapFile;
    HANDLE hFile;
    BOOL bFlag;
    LPVOID lpMapAddress;
    data_t* data;
    clock_t start = clock();
	double timeInInput = 0.0;

    hFile = CreateFileA(
        "db.dat",
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    hMapFile = CreateFileMapping(
        hFile,
        NULL,         
        PAGE_READONLY,
        0,            
        0,            
        NULL);        
    lpMapAddress = MapViewOfFile(
        hMapFile,
        FILE_MAP_READ,
        0,
        0,
        0);

    // read metadata
    FILE* fmetadata;
    (void) fopen_s(&fmetadata, "db.mtd", "r");
    metadata_t metadata;
    fscanf_s(fmetadata, "%f%f%f%f%f%f",
        &metadata.k1_base, &metadata.k1_n, &metadata.k1_step,
        &metadata.k2_base, &metadata.k2_n, &metadata.k2_step);

    FILE* fout;
    (void) fopen_s(&fout, "output.txt", "w");

	std::vector<float> outputs;
	outputs.reserve(10000);
	std::vector<inputPair_t> inputs;
	inputs.reserve(10000);
    FILE* finputs;
    (void) fopen_s(&finputs, argv[1], "r");
    inputPair_t in;
	clock_t startInputClock = clock();
	while(!feof(finputs)) {
		auto hr = fscanf_s(finputs, "%f%f", &in.k1, &in.k2);
		if(hr != 2) break;

		inputs.push_back(in);
	}
	clock_t endInputClock = clock();
	timeInInput += endInputClock - startInputClock;

    // here's the data, do stuff with it
    data = (data_t*)lpMapAddress;
	
	long const nbOfInputs = inputs.size();
	inputPair_t* p = inputs.data();
	inputPair_t* pStart = p;
	for(; p - pStart < nbOfInputs; ++p) {
		inputPair_t& in = *p;

        size_t k1idx = (size_t)( floorf((in.k1 - metadata.k1_base) / metadata.k1_step) );
        size_t k2idx = (size_t)( floorf((in.k2 - metadata.k2_base) / metadata.k2_step) );
        size_t idx = k1idx * sizeof(data_t) + k2idx;
        data_t datum = data[idx];

        // write results
		outputs.push_back(datum.v);
    }

	// stop timing because I/O
    clock_t end = clock();

	for(size_t i = 0; i < outputs.size(); ++i)
	{
        fprintf_s(fout, "f(%5.2f, %5.2f) = %f\n", 
			inputs[i].k1, inputs[i].k2, outputs[i]);
	}

	double theTime = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Computed %ld values in %ld ticks, or %lfs, timed using clock()\n",
        nbOfInputs,
        end - start,
        theTime);

	timeInInput /= CLOCKS_PER_SEC;

	printf("Mind you, %lfs was spent in I/O operations on the %s file\n"
		"and thus, only %fs were spent actually computing stuff\n",
		timeInInput, argv[1], theTime - timeInInput);
    
    fclose(fout);
    fclose(fmetadata);
    fclose(finputs);
    bFlag = UnmapViewOfFile(lpMapAddress);
    bFlag = CloseHandle(hMapFile);
    bFlag = CloseHandle(hFile);

    return 0;
}
