GPP_FLAGS = --std=gnu++11 -msse -I../MamdaniMatrix -O3 -ffast-math -msse3 -Wno-unused-result

MSCProj.bin: Main.cpp ../MamdaniMatrix/mamdani.ixx
	g++ $(GPP_FLAGS) Main.cpp -o MSCProj.bin

../MamdaniMatrix/mamdani.ixx: ../MamdaniMatrix/gen_cleaned.pl
	sh -c 'cd ../MamdaniMatrix/ && perl ./gen_cleaned.pl'

clean:
	rm -f MSCProj.bin
