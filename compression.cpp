#include <iostream>
#include <string.h>
#include <chrono>

using namespace std;

#define MAX_LEN      256
#define MAX_TREE_LEN 2000

struct Node {
	Node*          left;
	Node*          right;
	bool           leaf;
	unsigned char  symbol;
	int            frequency;
	unsigned char  compression[257]; // pior cenário
};

short decompress_tree[MAX_TREE_LEN];

Node* create_node() {
	Node* node      	  = new Node();
	node->frequency 	  = 0;
	node->leaf        	  = false;
	node->left      	  = nullptr;
	node->right     	  = nullptr;

	memset(node->compression, 2, sizeof(node->compression));
	return node;
}

void linearize_tree(Node* tree, int& tree_len) {
	if (tree == NULL)
		return;
	if(tree->leaf) {
		decompress_tree[tree_len] = tree->symbol;
		tree_len++;
	}
	else {
		decompress_tree[tree_len] = -1;
		tree_len++;
	}

	linearize_tree(tree->left, tree_len);
	linearize_tree(tree->right, tree_len);
}

void serialize_tree(short *tree_encoding, int tree_len, int file_len, FILE* fp) {
    // confirmará as supostas posições da árvore que são folhas, ficará no header
    // e terá o tamanho do número de símbolos usados.
    short* character_ocurrence = new short[tree_len];

    short len2 = 0;

    for(int i = 0; i < tree_len; i++) {
        if(tree_encoding[i] != -1) { 
            character_ocurrence[len2] = i;
            len2++;
        }
        if(tree_encoding[i] == -1)
            tree_encoding[i] = 0;
    }

    fwrite(&file_len, 4, 1, fp);

    // escreve os indices dos simbolos nas folhas da árvore
    fwrite(&len2, 1, 2, fp);

    for(int i = 0; i < len2; i++) {
        fwrite(&character_ocurrence[i], sizeof(short), 1, fp);
    }

    fwrite(&tree_len, sizeof(unsigned short), 1, fp);

    for(int i = 0; i < tree_len; i++) {
        fwrite((unsigned char*)&tree_encoding[i], sizeof(unsigned char), 1, fp);
    }
    
    delete[] character_ocurrence;
}

void serialize_bitstring(unsigned char* bitstring, int bitstring_len, FILE* fp) {
    // escreve o tamanho da bitstring
    fwrite(&bitstring_len, sizeof(unsigned int), 1, fp);

    //comprime a bitstring nos bits de um byte.
    int idx = 0;
    unsigned char *ptr = new unsigned char[bitstring_len/8+1], bt = 0;
    memset(ptr, 0, bitstring_len/8+1);

    for(int i = 1; i <= bitstring_len; i++) {
        bool bit = bitstring[i-1];
        bt = (bt | (bit<<((i-1)%8)));

        if(i % 8 == 0) {
            ptr[idx++] = bt;
            bt = 0;
        } else if(i == bitstring_len) {
            ptr[idx] = bt;
        }
    }

    fwrite(ptr, 1, bitstring_len/8+1, fp);
    fclose(fp);

    delete[] ptr;
}

double Timer() {
    static chrono::steady_clock::time_point  start;
    static bool                              elapsed;

    double aux = 0.f;

    if(!elapsed) {
        start = chrono::steady_clock::now();
    }
    else {
        aux = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count()/1000.f;
    }

    elapsed ^= 1;
    return aux;
}

unsigned char compress_tb[MAX_LEN][257];

unsigned char* read_file(char* file_name, int& len) {
	FILE *fl = fopen(file_name, "rb");
	fseek(fl, 0L, SEEK_END);
	len = ftell(fl);
	fseek(fl, 0L, SEEK_SET);
	unsigned char *file = new unsigned char[len+10];
	fread(file, len, 1, fl);
	return file;
}

int* get_frequency(unsigned char* file, int len) {
	int* frequency = new int[MAX_LEN];
	memset(frequency, 0, MAX_LEN * 4);
	for (int i = 0; i < len; i++) {
		frequency[file[i]]++;
	}
	return frequency;
}

void sort(Node** tree, int len) {
	for (int i = 0; i < len; i++) {
		for (int j = i + 1; j < len; j++) {
			if (tree[i]->frequency < tree[j]->frequency) {
				swap(tree[i], tree[j]);
			}
		}
	}
}

void linear_sorting(Node** tree, int len) {
	for(int i = len-1; i >= 1; i--) {
		if(tree[i]->frequency > tree[i-1]->frequency) {
			swap(tree[i], tree[i-1]);
		} else {
			break;
		}
	}
}

Node* create_tree(Node** tree, int len) {
	while (len > 1) {
		Node* temp = create_node();

		temp->frequency = tree[len - 1]->frequency + tree[len - 2]->frequency;

		temp->left  = tree[len - 1];
		temp->right = tree[len - 2];

		tree[len - 2] = temp;

		len--;

		linear_sorting(tree, len);
	}

	return tree[0];
}

void release_mem_tree(Node* node) {
	if(node == nullptr)
		return;
	release_mem_tree(node->left);
	release_mem_tree(node->right);

	delete node;
}

void generate_compress_form(Node* node, unsigned char compress[257], int len = 0) {
	if (node == NULL) {
		return;
	}

	if (node->leaf) {
		for(int i = 0; i < len; i++) {
			node->compression[i] = compress[i];
		}	

		return;
	}

	compress[len] = 0;
	generate_compress_form(node->left, compress, len+1);
	compress[len] = 1;
	generate_compress_form(node->right, compress, len+1);
}

void get_bitstring(Node* tree) {
	if(tree == NULL)
		return;
	if(tree->leaf) {
		for(int i = 0; tree->compression[i] != 2; i++) {
			compress_tb[tree->symbol][i] = tree->compression[i];
		}

		return;
	}

	get_bitstring(tree->left);
	get_bitstring(tree->right);
}

unsigned char* generate_bitstring(unsigned char* file, int len, int& bitstring_len, int* freq) {
	int bits_len = 0;

	for(int i = 0; i < 256; i++) {
		if(freq[i]) {
			unsigned char* cache_speedup = compress_tb[i];
			int  len_aux = 0;
			for(int j = 0; cache_speedup[j] != 2; j++) {
				len_aux++;
			}

			bits_len += freq[i] * len_aux;
		}
	}

	unsigned char* bitstring = new unsigned char[bits_len+2];

	bitstring_len = bits_len;

	int idx = 0;

	for(int i = 0; i < len; i++) {
		unsigned char* cache_speedup = compress_tb[file[i]];
		for(int j = 0; cache_speedup[j] != 2; j++) {
			bitstring[idx] = cache_speedup[j];
			idx++;
		}
	}

	bitstring[idx] = 2;
	return bitstring;
}

double compress_file(char* input, char* output) {
	Timer();

	unsigned char* bitstring;
	unsigned char  cp[257];

	int bitstring_len  = 0; 
	int tree_len       = 0; 
	int idx            = 0;

	memset(compress_tb, 2, sizeof(compress_tb));

	int file_len;

	FILE *fp = fopen(output, "wb");

	unsigned char* file = read_file(input, file_len);

	if(file_len == 0) {
		fclose(fp);
		return Timer();
	}

	int* freq = get_frequency(file, file_len);

	Node* tree[MAX_LEN];

	for (int i = 0; i < MAX_LEN; i++) {
		if (freq[i] > 0) {
			tree[idx]            = create_node();
			tree[idx]->symbol    = i;
			tree[idx]->frequency = freq[i];
			tree[idx]->leaf      = true;
			idx++;
		}
	}

	sort(tree, idx);

	Node* tr = create_tree(tree, idx);

	if(file_len > 1) {
		generate_compress_form(tr, cp);
	} else if(file_len == 1) {
		tr->compression[0] = 1;
	}

	get_bitstring(tr);

	bitstring = generate_bitstring(file, file_len, bitstring_len, freq);

	linearize_tree(tr, tree_len);

	release_mem_tree(tr);

	serialize_tree(decompress_tree, tree_len, file_len, fp);

	serialize_bitstring(bitstring, bitstring_len, fp);

	delete[] freq;
	delete[] file;
	delete[] bitstring;

	return Timer();
}