MSCProj.bin: Main.cpp ../MamdaniMatrix/mamdani.ixx
	g++ --std=gnu++11 -O3 -msse -I../MamdaniMatrix/ Main.cpp -o MSCProj.bin

../MamdaniMatrix/mamdani.ixx: ../MamdaniMatrix/gen_cleaned.pl
	pushd .
	cd ../MamdaniMatrix/ && perl ./gen_cleaned.pl
	popd

clean:
	rm -f MSCProj.bin
