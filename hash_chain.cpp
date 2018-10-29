#include <stdio.h>
#include <stdint.h>
#include <openssl/evp.h>
#include <cstring>
#include <unistd.h>
using namespace std;

class hash_chain {
public:
	uint8_t * data;
	int data_len;
	uint8_t * pre_hash;
	hash_chain * next_node;
	hash_chain(){
		data = NULL;
		pre_hash = NULL;
		next_node = NULL;
	}
};
// hash chain create, only create a head node
hash_chain * hash_chain_create(void * base, int baselen) {
	// printf("hash_chain_create\n");
	hash_chain * head = new hash_chain();
	head->data = new uint8_t[baselen];
	head->data_len = baselen;
	memcpy(head->data, base, baselen);

	return head;
	
}
void hash_chain_add(hash_chain * head, void * new_content, int length) {
	// printf("hash_chain_add\n");
	// to the end node
	hash_chain *end = head;
	while(end->next_node != NULL) {
		end = end->next_node;
	}

	// now `end` is the last node
	// we add a new node to its tail
	end->next_node = new hash_chain();
	hash_chain * new_node = end->next_node;

	new_node->data_len = length;
	new_node->data = new uint8_t[length];

	memcpy(new_node->data, new_content, length);

	EVP_MD_CTX * ctx;
	// 使用sm3算法
	const EVP_MD * type = EVP_sm3();

	ctx = EVP_MD_CTX_create();
	EVP_DigestInit(ctx, type);

	if (end->pre_hash != NULL) {
		EVP_DigestUpdate(ctx, end->pre_hash, EVP_MD_size(type));
	}
	EVP_DigestUpdate(ctx, end->data, end->data_len);

	new_node->pre_hash = new uint8_t[EVP_MD_size(type)];
	EVP_DigestFinal(ctx, new_node->pre_hash, NULL);

	EVP_MD_CTX_free(ctx);
}

bool hash_chain_verify(hash_chain * head) {
	// printf("verifing\n");
	EVP_MD_CTX *ctx;
	const EVP_MD* type = EVP_sm3();

	hash_chain * last = head;
	hash_chain * next = head->next_node;

	int digest_len = EVP_MD_size(type);
	while (next != NULL) {
		uint8_t * test_data = new uint8_t[digest_len];
		ctx = EVP_MD_CTX_create();
		EVP_DigestInit(ctx, type);
		if (last->pre_hash != NULL) {
			EVP_DigestUpdate(ctx, last->pre_hash, digest_len);
		}
		EVP_DigestUpdate(ctx, last->data, last->data_len);
		EVP_DigestFinal(ctx, test_data, NULL);
		EVP_MD_CTX_free(ctx);
		if (memcmp(test_data, next->pre_hash, digest_len) != 0) {
			delete[] test_data;
			return false;
		}
		last = next;
		next = next->next_node;
	}
	return true;
}
int hash_chain2file(hash_chain * head, char * filename) {
	// printf("hash_chain2file\n");
	FILE* fp;
	if ((fp = fopen(filename, "wb")) == NULL) {
		printf("Error on open %s file\n", filename);
		return 1;
	}
	uint8_t con[32];
	memset(con, 0, sizeof(con));

	uint8_t have_next;
	hash_chain * tmp = head;
	while (tmp != NULL) {
		// write pre_hash 
		if (tmp->pre_hash == NULL) {
			fwrite(con, 32, 1, fp);	
		} else {
			fwrite(tmp->pre_hash, 32, 1, fp);
		}
		// write data length
		fwrite(&tmp->data_len, sizeof(tmp->data_len), 1, fp);
		// write data
		fwrite(tmp->data, tmp->data_len, 1, fp);

		if (tmp->next_node == NULL) {
			have_next = 0;
		} else {
			have_next = 1;
		}
		fwrite(&have_next, 1, 1, fp);
		tmp = tmp->next_node;
	}
	return 0;
}

hash_chain* file2hash_chain(char * filename) {
	// printf("file2hash_chain\n");
	FILE * fp = fopen(filename, "rb");
	if (fp == NULL) {
		printf("Error on open %s file\n", filename);
		return NULL;
	}

	uint8_t con[32];
	memset(con, 0, sizeof(con));

	uint8_t * pre_hash = new uint8_t[32];
	hash_chain * head = new hash_chain();
	hash_chain * tmp = head;
	uint8_t next;
	while(true) {

		// read pre_hash	
		fread(pre_hash, 32, 1, fp);
		if (memcmp(pre_hash, con, 32) != 0) {
			tmp->pre_hash = new uint8_t[32];
			memcpy(tmp->pre_hash, pre_hash, 32);
		}

		// read data length
		fread(&tmp->data_len, sizeof(tmp->data_len), 1, fp);

		// read data
		tmp->data = new uint8_t[tmp->data_len];
		fread(tmp->data, tmp->data_len, 1, fp);

		// read next state
		fread(&next, 1, 1, fp);
		if (next == 0) {
			break;
		}
		tmp->next_node = new hash_chain();
		tmp = tmp->next_node;
	}
	return head;

}
void print_chain(hash_chain * chain) {
	int cnt = 0;
	while(chain != NULL) {
		printf("Node %d\n", cnt++);
		// pre_hash
		if (chain->pre_hash != NULL) {
			printf("pre_hash: ");
			for (int i = 0;i < 32;++i)
				printf("%02x", chain->pre_hash[i]);
			printf("\n");
		}
		else
			printf("pre_hash: NULL\n");
		// data length
		printf("data_len: %d\n", chain->data_len);
		// data
		printf("data: ");
		for (int i = 0;i < chain->data_len;++i) {
			printf("%02x", chain->data[i]);
		}
		printf("\n");
		chain = chain->next_node;
	}
}
void help(){
	printf("Usage:\n");
	printf("hash_chain create [string-to-hash] [hash-chain-filename]\n");
	printf("hash_chain add [string-to-add-to-hash-chain]"
		" [existing-hash-chain-file] [new-hash-chain-file]\n");
	printf("hash_chain verify [existing-hash-chain-file]\n");
	printf("hash_chain print [existing-hash-chain-file]\n");
}
int main(int argc, char * argv[]) {
	
	int ch;
	char con_str[4][10] = {"create", "add", "verify", "print"};
	if (argc <= 2) {
		help();
	} else if (strcmp(argv[1], con_str[0]) == 0) {
		// create 
		if (argc != 4) {
			help();
		} else {
			int len = strlen(argv[2]);
			char * str = new char[len];
			memcpy(str, argv[2], len);
			hash_chain * chain = hash_chain_create((void*)str, len);
			print_chain(chain);
			// store in file
			hash_chain2file(chain, argv[3]);
		}
		
	} else if (strcmp(argv[1], con_str[1]) == 0) {
		// add
		if (argc != 5) {
			help();
		} else {
			int len = strlen(argv[2]);
			char * str = new char[len];
			memcpy(str, argv[2], len);
			hash_chain * chain = file2hash_chain(argv[3]);
			// print_chain(chain);
			if (chain != NULL) {
				hash_chain_add(chain, (void*)str, len);
				hash_chain2file(chain, argv[4]);
			}
		}
	} else if (strcmp(argv[1], con_str[2]) == 0) {
		// verify
		if (argc != 3) {
			help();
		} else {
			hash_chain * chain = file2hash_chain(argv[2]);
			if (chain != NULL) {
				// print_chain(chain);
				if (hash_chain_verify(chain)) {
					printf("Success\n");
				} else {
					printf("Failure\n");
				}
			}
		}
	} else if (strcmp(argv[1], con_str[3]) == 0) {
		if (argc != 3) {
			help();
		} else {
			hash_chain * chain = file2hash_chain(argv[2]);
			if (chain != NULL) {
				print_chain(chain);
			}
		}
	} else {
		printf("Wrong arguments\n");
		help();
	}
	return 0;
}