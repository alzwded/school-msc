#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef min // FU windows.h
#include <utility>
#include <stdio.h>
#include <vector>
#include <cmath>
#include <ctime>

// our value type in the DB
typedef struct {
    float v;
} data_t;

// our input type
typedef struct {
    float k1;
    float k2;
} inputPair_t;

// our metadata container
typedef struct {
    float k1_base, k1_step;
    size_t k1_n;
    float k2_base, k2_step;
    size_t k2_n;
} metadata_t;

int main(int argc, char* argv[])
{
    if(argc != 2) return 255;

    // start timing
    clock_t start = clock();
    // we'll substract I/O time later
    double timeInInput = 0.0;

    // get a handle to our db file
    HANDLE hFile = CreateFileA(
        "db.dat",
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    // map it
    HANDLE hMapFile = CreateFileMapping(
        hFile,
        NULL,         
        PAGE_READONLY,
        0,            
        0,            
        NULL);   
    // view it
    LPVOID lpMapAddress = MapViewOfFile(
        hMapFile,
        FILE_MAP_READ,
        0,
        0,
        0);

    // read metadata
    FILE* fmetadata;
    (void) fopen_s(&fmetadata, "db.mtd", "r");
    metadata_t metadata;
    fscanf_s(fmetadata, "%f%ld%f%f%ld%f",
        &metadata.k1_base, &metadata.k1_n, &metadata.k1_step,
        &metadata.k2_base, &metadata.k2_n, &metadata.k2_step);

    // init output file
    FILE* fout;
    (void) fopen_s(&fout, "output.txt", "w");

    // reserve our inputs and outputs to save time later
    std::vector<float> outputs;
    outputs.reserve(10000);
    std::vector<inputPair_t> inputs;
    inputs.reserve(10000);

    // read everything in one big chunk because we'd block
    // on I/O if we intermix I/O and computations
    FILE* finputs;
    (void) fopen_s(&finputs, argv[1], "r");
    inputPair_t in;
    // time the reading because it's awefully slow
    clock_t startInputClock = clock();
    while(!feof(finputs)) {
        auto hr = fscanf_s(finputs, "%f%f", &in.k1, &in.k2);
        if(hr != 2) break;

        inputs.push_back(in);
    }
    clock_t endInputClock = clock();
    // count I/O time
    timeInInput += endInputClock - startInputClock;

    // here's the data, do stuff with it
    data_t* data = (data_t*)lpMapAddress;
    
    long const nbOfInputs = inputs.size();
    inputPair_t* p = inputs.data();
    inputPair_t* pStart = p;
    inputPair_t* pEnd = pStart + nbOfInputs;
    long const sizeOfData = metadata.k1_n * metadata.k2_n;
    for(; p < pEnd; ++p) {
        inputPair_t& in = *p;

        // compute approximate indexes
        size_t k1idx = (size_t)( floorf( (in.k1 - metadata.k1_base) / metadata.k1_step + 0.5f) );
        size_t k2idx = (size_t)( floorf( (in.k2 - metadata.k2_base) / metadata.k2_step + 0.5f) );
        size_t idx = k1idx * metadata.k2_n + k2idx;
        // clamp
        idx = std::min((size_t)sizeOfData - 1, idx);
        // grab the result
        data_t datum = data[idx];

        // write results
        outputs.push_back(datum.v);
    }

    // stop timing because I/O
    clock_t end = clock();

    // write output file
    for(size_t i = 0; i < outputs.size(); ++i)
    {
        fprintf_s(fout, "f(%5.2f, %5.2f) = %f\n", 
            inputs[i].k1, inputs[i].k2, outputs[i]);
    }

    // report time
    double theTime = (double)(end - start) / CLOCKS_PER_SEC;
    timeInInput /= CLOCKS_PER_SEC;

    printf("Computed %ld values in %ld ticks, or %lfs, timed using clock()\n",
        nbOfInputs,
        end - start,
        theTime);

    printf("Mind you, %lfs was spent in I/O operations on the %s file\n"
        "and thus, only %fs were spent actually computing stuff\n",
        timeInInput, 
        argv[1],
        theTime - timeInInput);
    
    // cleanup
    fclose(fout);
    fclose(fmetadata);
    fclose(finputs);
    (void) UnmapViewOfFile(lpMapAddress);
    (void) CloseHandle(hMapFile);
    (void) CloseHandle(hFile);

    return 0;
}
