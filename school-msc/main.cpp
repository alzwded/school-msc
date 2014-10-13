#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <vector>
#include <cmath>
#include <ctime>

#define BUFFSIZE 1024 // size of the memory to examine at any one time

typedef struct {
    float k1;
    float k2;
    float v;
    float unusued;
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
    HANDLE hMapFile;
    HANDLE hFile;
    BOOL bFlag;
    LPVOID lpMapAddress;
    data_t* data;
    clock_t start = clock();

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

    // read inputs file
    FILE* finputs;
    (void) fopen_s(&finputs, "db.mtd", "r");
    std::vector<inputPair_t> inputs;
    while(!feof(finputs))
    {
        inputPair_t in;
        fscanf_s(finputs, "%f%f", &in.k1, &in.k2);
        inputs.push_back(in);
    }

    FILE* fout;
    (void) fopen_s(&fout, "output.txt", "w");

    // here's the data, do stuff with it
    data = (data_t*)lpMapAddress;

    for(auto i = inputs.begin(); i != inputs.end(); ++i)
    {
        size_t k1idx = (size_t)( floorf((i->k1 - metadata.k1_base) / metadata.k1_step) );
        size_t k2idx = (size_t)( floorf((i->k2 - metadata.k2_base) / metadata.k2_step) );
        size_t idx = k1idx * sizeof(data_t) + k2idx;
        data_t datum = data[idx];

        // write results
        fprintf_s(fout, "f(%5.2f, %5.2f) = %f\n", i->k1, i->k2, datum.v);
    }

    clock_t end = clock();
    printf("Computed %ld values in %ld ticks, or %fs, timed using clock()",
        inputs.size(),
        end - start,
        (float)(end - start) / CLOCKS_PER_SEC);
    
    fclose(fout);
    fclose(fmetadata);
    fclose(finputs);
    bFlag = UnmapViewOfFile(lpMapAddress);
    bFlag = CloseHandle(hMapFile);
    bFlag = CloseHandle(hFile);

    return 0;
}
