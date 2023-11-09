libLinux:
	clang++ -shared -std=c++17 -o ./Code/lib/libjobsystem.so -fPIC ./Code/lib/*.cpp
	
compile:
	clang++ -shared -std=c++17 -o ./Code/lib/libjobsystem.so -fPIC ./Code/lib/*.cpp
	clang++ -g -std=c++17 -o output.out ./Code/exec/*.cpp -L./Code/lib -ljobsystem

run:
	./output.out

clean:
	find . -type f -name "*.out" -delete
	find . -type d -name "*.dSYM" -exec rm -r {} +
	find . -type d -name "*.vscode" -exec rm -r {} +
	find ./Data -type f -name "*output.txt" -delete