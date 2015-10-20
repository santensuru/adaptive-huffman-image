/**
 * Adaptive Huffman Image
 *
 * Author: Djuned Fernando Djusdek
 *         5112.100.071
 *         Informatics - ITS
 *
 * Version 1.0
 *
 * This code only use for grayscale image from 8-bit *.bmp format file
 */

#include <iostream>
#include <algorithm>
#include <vector>
#include <cstring>
#include <fstream>
#include <queue>
#include <ctime>
#include <highgui.h>
#include <cv.h>

namespace vitter {
	
	#define SYMBOL 256
	#define NUMBER 512
	
	typedef struct node{
		unsigned char symbol; // symbol
		int weight,           // weight
		    number;           // number
		node *parent,         // parent
		     *left,           // left child
		     *right;          // right child
	} node;
	
	/**
	 * UPDATE
	 */
	
	typedef std::pair<int, node*> my_pair;
	
	void create_node(node **leaf, unsigned char symbol, bool is_nyt) {
		node *temp =  (node*) malloc(sizeof(node));
		if (is_nyt) {
			temp->symbol = 0x00;
			temp->weight = 0;
			
		} else {
			temp->symbol = symbol;
			temp->weight = 1;
			
		}
		temp->parent = NULL;
		temp->left = NULL;
		temp->right = NULL;
			
		*leaf = temp;
		
		return;
	}
	
	void merge_node(node **tree, node *left, node *right) {
		node *temp = (node*) malloc(sizeof(node));
		temp->weight = left->weight + right->weight;
		temp->left = left;
		temp->right = right;
		temp->left->parent = temp;
		temp->right->parent = temp;
		temp->symbol = 0x00;
		temp->parent = NULL;
		*tree = temp;
		
		return;
	}
	
	void delete_tree(node **tree) {
		if ((*tree)->left != NULL) {
			delete_tree(&(*tree)->left);
		}
		
		if ((*tree)->right != NULL) {
			delete_tree(&(*tree)->right);
		}
		
		if ((*tree) != NULL) {
			if ((*tree)->parent != NULL && (*tree)->left == NULL) {
				(*tree)->parent->left = NULL;
				free(*tree);
				
			} else if ((*tree)->parent != NULL && (*tree)->right == NULL) {
				(*tree)->parent->right = NULL;
				free(*tree);
				
			} else if ((*tree)->right == NULL && (*tree)->left == NULL){
				free(*tree);
				(*tree) = NULL;
				
			}
			
		}
		
		return;
	}
	
	void search_higher_block(node **tree, int weight, int *number, int parent_number, node **position, char *l_r) {
		if ((*tree)->weight == weight && (*tree)->number > *number && (*tree)->number != parent_number) {
			*position = (*tree)->parent;
			*number = (*tree)->number;
			if ((*tree)->parent->left->number == (*tree)->number) {
				strcpy(l_r, "l");
			} else {
				strcpy(l_r, "r");
			}
		}
		
		if ((*tree)->left != NULL) {
			search_higher_block(&(*tree)->left, weight, &*number, parent_number, &*position, l_r);
		}
		
		if ((*tree)->right != NULL) {
			search_higher_block(&(*tree)->right, weight, &*number, parent_number, &*position, l_r);
		}
			
		return;
	}
	
	void switch_node(node *tree, char *l_r, node *sibling, char *l_r_sibling) {	
		if (strcmp(l_r, "l") == 0 && strcmp(l_r_sibling, "l") == 0) {
			node *temp = tree->left;
			tree->left = sibling->left;
			sibling->left = temp;
			
			tree->left->parent = tree;
			sibling->left->parent = sibling;
			
		} else if (strcmp(l_r, "l") == 0) {
			node *temp = tree->left;
			tree->left = sibling->right;
			sibling->right = temp;
			
			tree->left->parent = tree;
			sibling->right->parent = sibling;
			
		} else if (strcmp(l_r_sibling, "l") == 0) {
			node *temp = tree->right;
			tree->right = sibling->left;
			sibling->left = temp;
			
			tree->right->parent = tree;
			sibling->left->parent = sibling;
			
		} else {
			node *temp = tree->right;
			tree->right = sibling->right;
			sibling->right = temp;
			
			tree->right->parent = tree;
			sibling->right->parent = sibling;
			
		}
	
		return;
	}
	
	void queueing_node(node **tree, std::vector<my_pair> *queue, int deep) {
		(*queue).push_back(std::make_pair(deep, *tree));
		
		if ((*tree)->right != NULL) {
			queueing_node(&(*tree)->right, &*queue, deep+1);
		}
		
		if ((*tree)->left != NULL) {
			queueing_node(&(*tree)->left, &*queue, deep+1);
		}
		
		return;
	}
	
	void increment_weight(node **tree) {
		if ((*tree)->left != NULL && (*tree)->right != NULL) {
			(*tree)->weight = (*tree)->left->weight + (*tree)->right->weight;
			
		} else {
			(*tree)->weight++;
			
		}
		
		return;
	}
	
	void find_external_symbol(node **tree, unsigned char symbol, node **position) {
		if ((*tree)->left != NULL) {
			find_external_symbol(&(*tree)->left, symbol, &*position);
		}
		
		if ((*tree)->symbol == symbol && (*tree)->left == NULL && (*tree)->right == NULL && (*tree)->weight != 0) {
			*position = *tree;
		}
		
		if ((*tree)->right != NULL) {
			find_external_symbol(&(*tree)->right, symbol, &*position);
		}
		
		return;
	}
	
	// Not use, just for check the binary tree
	void print_tree(node **tree, int deep) {
		if((*tree)->left != NULL) {
			print_tree(&(*tree)->left, deep+1);
		}
		
		if((*tree)->right != NULL) {
			print_tree(&(*tree)->right, deep+1);
		}
		
		char symbol_re[3];
		sprintf(symbol_re, "%2x", (*tree)->symbol);
		
		if (symbol_re[0] == ' ') {
			symbol_re[0] = '0';
		}
		
		printf("%d 0x%s %c %d %3d", deep, symbol_re, (*tree)->symbol, (*tree)->number, (*tree)->weight);
		
		if ((*tree)->weight == 0) {
			std::cout << " << NYT";
			
		} else if ((*tree)->left == NULL) {
			printf(" << 0x%s", symbol_re);
			
		}
		
		std::cout << '\n';
		
		return;
	}
	
	bool my_sort(my_pair p, my_pair q) {
		return p.first < q.first;
	}
	
	void update(node **tree, unsigned char symbol, std::vector<unsigned char> *dictionary, node **nyt) {
		// search in dictionary
		std::vector<unsigned char>::iterator it;
		it = std::search_n ((*dictionary).begin(), (*dictionary).end(), 1, symbol);
		
		node *temp;
		
		std::vector<my_pair> queue;
		
		if (it != (*dictionary).end()) {
			find_external_symbol(&*tree, symbol, &temp);
			
			node *inner_temp = NULL;
			char l_r[2];
			if (temp->parent->left->number == temp->number) {
				strcpy(l_r, "l");
				
			} else {
				strcpy(l_r, "r");
				
			}
			
			char l_r_sibling[2];
			int number = temp->number;
			search_higher_block(&*tree, temp->weight, &number, temp->parent->number, &inner_temp, l_r_sibling);
			if (inner_temp != NULL) {
				switch_node(temp->parent, l_r, inner_temp, l_r_sibling);
			}
				
		// NYT
		} else {
			node *new_nyt;
			create_node(&new_nyt, 0x00, true);
			node *new_node;
			create_node(&new_node, symbol, false);
	
			node *old_nyt = NULL;
			merge_node(&old_nyt, new_nyt, new_node);
			
			if (*tree == NULL) {
				*tree = old_nyt;
				*nyt = (*tree)->left;
				
			} else {
				old_nyt->parent = (*nyt)->parent;
				(*nyt)->parent->left = old_nyt;
				*nyt = old_nyt->left;
				
			}
			
			// goto old nyt
			temp = old_nyt;
			
			// give number
			queueing_node(&*tree, &queue, 0);
			std::sort(queue.begin(), queue.end(), my_sort);
			
			int num = NUMBER;
			for (int i=0; i<queue.size(); i++) {
				queue.at(i).second->number = --num;
			}
			
			queue.clear();
			
			(*dictionary).push_back(symbol);
			
		}
		
		// increment weight
		increment_weight(&temp);
		
		while(temp->parent != NULL) {
			// go to parent node
			temp = temp->parent;
			
			// if not root
			if (temp->parent != NULL)
			{
				node *inner_temp = NULL;
				
				char l_r[2];
				if (temp->parent->left->number == temp->number) {
					strcpy(l_r, "l");
					
				} else {
					strcpy(l_r, "r");
					
				}
				
				char l_r_sibling[2];
				int number = temp->number;
				search_higher_block(&*tree, temp->weight, &number, temp->parent->number, &inner_temp, l_r_sibling);
				if (inner_temp != NULL) {
					switch_node(temp->parent, l_r, inner_temp, l_r_sibling);
				}
				
			}
			increment_weight(&temp);
			
			queueing_node(&*tree, &queue, 0);
			std::sort(queue.begin(), queue.end(), my_sort);
			
			int num = NUMBER;
			for (int i=0; i<queue.size(); i++) {
				queue.at(i).second->number = --num;
			}
			
			queue.clear();
		}
		
		*tree = temp;
		
		return;
	}
	
	/**
	 * ENCODE
	 */
	
	void get_the_code(node **tree, unsigned char symbol, char *do_code, std::queue<char> *code_write) {
		char temp[strlen(do_code)+1];
		if ((*tree)->symbol == symbol && (*tree)->left == NULL && (*tree)->right == NULL && (*tree)->weight != 0) {
			for (int i=0; i<strlen(do_code); i++) {
				(*code_write).push(do_code[i]);
			}
			
			return;
		}
		
		strcpy(temp, do_code);
		if ((*tree)->left != NULL) {
			get_the_code(&(*tree)->left, symbol, strcat(temp, "0"), &*code_write);
		}
		
		strcpy(temp, do_code);
		if ((*tree)->right != NULL) {
			get_the_code(&(*tree)->right, symbol, strcat(temp, "1"), &*code_write);
		}
			
		return;
	}
	
	void get_nyt_code(node **tree, char *do_code, std::queue<char> *code_write) {
		char temp[strlen(do_code)+1];
		if ((*tree)->weight == 0 && (*tree)->left == NULL && (*tree)->right == NULL) {
			for (int i=0; i<strlen(do_code); i++) {
				(*code_write).push(do_code[i]);
			}
			
			return;
		}
		
		strcpy(temp, do_code);
		if ((*tree)->left != NULL) {
			get_nyt_code(&(*tree)->left, strcat(temp, "0"), &*code_write);
		}
		
		strcpy(temp, do_code);
		if ((*tree)->right != NULL) {
			get_nyt_code(&(*tree)->right, strcat(temp, "1"), &*code_write);
		}
			
		return;
	}
	
	void get_standard_code(unsigned char symbol, std::queue<char> *code_write) {
		for (int i=0; i<8; i++) {
			if ((symbol & 0x80) == 0x80) {
				(*code_write).push('1');
				
			} else {
				(*code_write).push('0');
				
			}
			symbol <<= 1;
		}
		
		return;
	}
				
	void write_to_file(std::ofstream *file, std::queue<char> *code_write) {
		unsigned char temp;
		temp = temp & 0x00;
		for (int i=0; i<8; i++) {
			if ((*code_write).front() == '1') {
				temp ^= 0x01;
			}
			
			if (i != 7) {
				temp <<= 1;
			}
			
			(*code_write).pop();
		}
		*file << temp;
		
		return;
	}
	
	void encode(node **tree, unsigned char symbol, std::vector<unsigned char> *dictionary, std::queue<char> *code_write, std::ofstream *file, node **nyt) {
		// search in dictionary
		std::vector<unsigned char>::iterator it;
		it = std::search_n ((*dictionary).begin(), (*dictionary).end(), 1, symbol);
		
		// symbol exist
		if (it != (*dictionary).end()) {
			char do_code[1];
			do_code[0] = '\0';
			
			get_the_code(&*tree, symbol, do_code, &*code_write);
			
		} else {
			char do_code[1];
			do_code[0] = '\0';
			
			if (*tree != NULL)
				get_nyt_code(&*tree, do_code, &*code_write);
			
			get_standard_code(symbol, &*code_write);
			
		}
		
		// call update procedure
		update(&*tree, symbol, &*dictionary, &*nyt);
		
		// write to file
		while ((*code_write).size() >= 8) {
			write_to_file(&*file, &*code_write);
		}
		
		return;
	}
	
	/**
	 * DECODE
	 */
	
	bool read_from_file(std::ifstream *file, std::queue<char> *code_read) {
		char temp;
		
		temp &= 0x00;
		
		if ((*file).get(temp)) {
			for (int i=0; i<8; i++) {
				if ((temp & 0x80) == 0x80) {
					(*code_read).push('1');
					
				} else {
					(*code_read).push('0');
					
				}
				temp <<= 1;
			}
			
			return true;
			
		} else {
			return false;
			
		}
	}
	
	void get_char_from_code(std::queue<char> *code_read, unsigned char *character) {
		unsigned char temp;
		temp &= 0x00;
		for (int i=0; i<8; i++) {
			if ((*code_read).front() == '1') {
				temp ^= 0x01;
			}
			
			if (i != 7) {
				temp <<= 1;
			}
			
			(*code_read).pop();
		}
		character[0] = temp;
		
		return;
	}
	
	void decode(node **tree, std::vector<unsigned char> *dictionary, std::queue<char> *code_read, std::ifstream *file, IplImage **out_file, node **nyt, bool *oke, short offset, unsigned long *position) {
		// 4 byte
		while ((*code_read).size() < 32 && *oke) {
			*oke = read_from_file(&*file, &*code_read);
		}
		
		node *temp = *tree;
		
		if (temp == NULL) {
			unsigned char symbol[1];
			get_char_from_code(&*code_read, symbol);
			
			(*out_file)->imageData[(*position)++] = (char) symbol[0];
			(*out_file)->imageData[(*position)++] = (char) symbol[0];
			(*out_file)->imageData[(*position)++] = (char) symbol[0];
			
			// call update procedure
			update(&*tree, symbol[0], &*dictionary, &*nyt);
			
		} else {
			while (temp->left != NULL && temp->right != NULL && (*code_read).size() > offset) {
				if ((*code_read).front() == '0') {
					temp = temp->left;
					(*code_read).pop();
					
				} else {
					temp = temp->right;
					(*code_read).pop();
					
				}
				
				// 4 byte
				while ((*code_read).size() < 32 && *oke) {
					*oke = read_from_file(&*file, &*code_read);
				}
				
			}
			
			if ((*code_read).size() == offset && temp->left != NULL && temp->right != NULL) {
				return;
			}
			
			if (temp->weight == 0) {
				unsigned char symbol[1];
				get_char_from_code(&*code_read, symbol);
				
				(*out_file)->imageData[(*position)++] = (char) symbol[0];
				(*out_file)->imageData[(*position)++] = (char) symbol[0];
				(*out_file)->imageData[(*position)++] = (char) symbol[0];
				
				// call update procedure
				update(&*tree, symbol[0], &*dictionary, &*nyt);
				
			} else {
				
				(*out_file)->imageData[(*position)++] = (char) temp->symbol;
				(*out_file)->imageData[(*position)++] = (char) temp->symbol;
				(*out_file)->imageData[(*position)++] = (char) temp->symbol;
				
				// call update procedure
				update(&*tree, temp->symbol, &*dictionary, &*nyt);
				
			}
		}
		
		return;
	}
	
	/**
	 * COMPRESS
	 */
	
	void compress(IplImage **in, std::ofstream *out) {
		node *root = NULL;
		node *nyt = NULL;
		
		std::vector<unsigned char> dictionary;
		std::queue<char> code_write;
		unsigned char symbol[1];
		
		unsigned short offset = 0;
		
		// initiate file with width, height & offset
		(*out).write((char*)&(*in)->width, 4);
		(*out).write((char*)&(*in)->height, 4);
		*out << (unsigned char) 0x00;
		
		for (int j=0; j<(*in)->height; j++) {
    		for (int i=0; i<(*in)->width*3; i+=3) {
    			encode(&root, (unsigned char) (*in)->imageData[(*in)->width*3 * j + i], &dictionary, &code_write, &*out, &nyt);
	    	}
	    }
		
		// write to file for offset
		if (code_write.size() > 0) {
			offset = 8 - code_write.size();
			
			while (code_write.size() < 8) {
				code_write.push('0');
			}
			
			write_to_file(&*out, &code_write);
			
			(*out).seekp(8);
			
			char write_offset[1];
			write_offset[0] = (char) offset;
			
			(*out).write(write_offset, 1);
		}
		
		dictionary.clear();
		delete_tree(&root);
		
		return;
	}
	
	/**
	 * DECOMPRESS
	 */
	
	void decompress(std::ifstream *in, IplImage **out) {
		node *root = NULL;
		node *nyt = NULL;
		
		std::vector<unsigned char> dictionary;
		std::queue<char> code_read;
		
		bool oke = true;
		
		unsigned long position = 0;
		
		unsigned short offset = 0;
		char temp[10];
		memset(temp, 0, 10);
		
		(*in).read(temp, 9);
		
		int width = *(int*)&temp[0];
		int height = *(int*)&temp[4];
		offset = (unsigned short) temp[8];
		
		*out = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);
		
		(*out)->width = width;
		(*out)->height = height;
		
		do {
			decode(&root, &dictionary, &code_read, &*in, &*out, &nyt, &oke, offset, &position);
		} while ((code_read.size() > offset || oke));
		
		dictionary.clear();
		delete_tree(&root);
		
		return;
	}

}

int main(int argc, char* argv[]) {	
	if (argc != 3) {
		printf("Usage: %s -c | -d filename\n",argv[0]);
		
		return 1;
		
    } else {
		time_t start, end;
		time(&start);
		
    	if (strcmp(argv[1], "-c") == 0) {
    		IplImage *in = cvLoadImage(argv[2]);
    		
    		std::ofstream out;
			char filename_out[strlen(argv[2]) + 3];
			sscanf(argv[2], "%[^.]", filename_out);
    		strcat(filename_out, ".ah");
			out.open(filename_out, std::ios::out | std::ios::binary);
    		
    		vitter::compress(&in, &out);
    		
    		time(&end);
			
			std::cout << "\nExecution time: +/- " << difftime(end, start) << "s\n";
    		
		} else if (strcmp(argv[1], "-d") == 0) {
			std::ifstream in;
			in.open(argv[2], std::ios::in | std::ios::binary);
			
			IplImage *out;
			
    		vitter::decompress(&in, &out);
    		
    		time(&end);
			
			std::cout << "\nExecution time: +/- " << difftime(end, start) << "s\n";
			
    		cvShowImage("Restore", out);
    		
    		cvWaitKey(0);
    		
		} else {
			printf("Usage: %s -c | -d filename\n",argv[0]);
			
			return 1;
			
		}
		
	}
	
	return 0;
}

