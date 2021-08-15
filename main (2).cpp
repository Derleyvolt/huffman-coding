#include <iostream>
#include <string.h>
#include "decompression.cpp"

int main(int argv, char **argc) {
	double elapsed_time = 0;

	if(argv == 4) {
		char operation  = argc[1][0];
		char* in_file   = argc[2]; 
		char* out_file  = argc[3];

		if(operation == 'c') {
			elapsed_time = compress_file(in_file, out_file);
			cout << "COMPRESSAO" << endl << endl;
			cout << "Tempo decorrido: " << fixed << elapsed_time << " segundos" << endl;
		} else if(operation == 'd') {
			elapsed_time = decompress_file(in_file, out_file);
			cout << "DESCOMPRESSAO" << endl << endl;
			cout << "Tempo decorrido: " << fixed << elapsed_time << " segundos" << endl;
		}
	} else {
		cout << "Modo de uso" << endl << endl;
		cout << "COMPRIMIR:    Digite c entrada.extensao saida.extensao" << endl;
		cout << "DESCOMPRIMIR: Digite d entrada.extensao saida.extensao" << endl;
	}

	return 0;
}