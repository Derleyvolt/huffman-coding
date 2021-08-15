#include "compression.cpp"

struct data_decompression {
    short*          leaf_index;
    unsigned char*  tree;
    unsigned char*  bitstring;
    unsigned char*  parsed_bitstring;
    unsigned int    file_len;
    int             bitstring_len;
    int             leafs_len;
    int             tree_len;
};

Node* retrieve_tree(int& idx, int& idx2, unsigned char* decomp, int tam, short* leaf_index) {
	if (idx >= tam) {
		return NULL;
    }

	Node* node = create_node();

	if (decomp[idx] != 0 || (decomp[idx] == 0 && leaf_index[idx2] == idx)) {
        idx2++;
		node->symbol = (unsigned char)decomp[idx];
		node->leaf   = true;
		return node;
	}

	idx++;
	node->left  = retrieve_tree(idx, idx2, decomp, tam, leaf_index);
	idx++;
	node->right = retrieve_tree(idx, idx2, decomp, tam, leaf_index);

	return node;
}

unsigned char* retrieve_string(Node* tree, unsigned char *bitstring, int tam, int file_len, int idx) {
    unsigned char* original_file = new unsigned char[file_len];

    Node* root = tree;

    int index = 0;

    bool border_case = tree->left > 0 ? 0 : 1;

    while(idx <= tam) {
        if(root->leaf) {
            original_file[index++] = root->symbol;
            root = tree;

            if(border_case)
                idx++;

            continue;
        }

        if(bitstring[idx] == 0) {
            root = root->left;
            idx++;
        } else {
            root = root->right;
            idx++;
        }
    }

    return original_file;
}

void parse_file(data_decompression *data, FILE* fp) {
    fread(&data->file_len, sizeof(unsigned int), 1, fp);

    // pega o tamanho do vetor de indices das folhas
    fread(&data->leafs_len, sizeof(unsigned short), 1, fp);

    data->leaf_index = new short[data->leafs_len];

    // ler o vetor de índices
    for(int i = 0; i < data->leafs_len; i++) {
        fread(&(data->leaf_index[i]), sizeof(short), 1, fp);
    }

    // pega o tamanho da árvore
    fread(&data->tree_len, sizeof(unsigned short), 1, fp);

    data->tree = new unsigned char[data->tree_len];

    for(int i = 0; i < data->tree_len; i++) {
        fread(&(data->tree[i]), sizeof(unsigned char), 1, fp);
    }

    fread(&(data->bitstring_len), sizeof(unsigned int), 1, fp);

    data->bitstring = new unsigned char[data->bitstring_len];

    int len = data->bitstring_len;

    len = len % 8 > 0 ? len + 8-(len%8) : len;

    for(int i = 0; i < len/8; i++) {
        fread(&(data->bitstring[i]), sizeof(unsigned char), 1, fp);
    }
}

void generate_original_file(char* file_name, unsigned char* data, int file_len) {
    FILE* fp = fopen(file_name, "wb");
	if(file_len > 0)
    	fwrite(data, file_len, 1, fp);
    fclose(fp);
}

double decompress_file(char* input, char* output) {
	Timer();
    data_decompression *data = new data_decompression();

	FILE* fp = fopen(input, "rb");

	fseek(fp, 0L, SEEK_END);
	int ps_len = ftell(fp);

	if(ps_len == 0) {
		generate_original_file(output, NULL, 0);
		fclose(fp);
		return Timer();
	}

	rewind(fp);

    parse_file(data, fp);

    int len = data->bitstring_len;
	unsigned char* original_file;
    int inc = 0, idx = 0, idx2 = 0;

    len = len % 8 > 0 ? len + 8 - (len%8) : len;

    data->parsed_bitstring = new unsigned char[data->bitstring_len];

    for(int i = 0; i < len/8; i++) {
        for(int j = 0; j < 8 && inc <= data->bitstring_len; j++) {
            data->parsed_bitstring[inc] = (data->bitstring[i]&1);
            data->bitstring[i]          = data->bitstring[i] >> 1;
            inc++;
        }
	}

    Node* tree = retrieve_tree(idx, idx2, data->tree, data->tree_len, data->leaf_index);

    original_file = retrieve_string(tree, data->parsed_bitstring, data->bitstring_len, data->file_len, 0);

    generate_original_file(output, original_file, data->file_len);


	delete[] original_file;
	delete[] data->parsed_bitstring;
	delete[] data->tree;
	delete[] data->leaf_index;
	delete[] data->bitstring;

    fclose(fp);

    return Timer();
}
