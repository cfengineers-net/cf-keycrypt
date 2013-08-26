/*
   cf-keycrypt.c

   Copyright (C) cfengineers.net

   Written and maintained by Jon Henrik Bjornstad <jonhenrik@cfengineers.net>

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/rsa.h>

#define STDIN     0
#define STDOUT    1

void usage() {
	printf(
"\n"
"Usage: cf-keycrypt [-e public-key|-d private-key] -o outfile -i infile [-h]\n"
"\n"
"Use CFEngine cryptographic keys to encrypt and decrypt files, eg. files containing\n"
"passwords or certificates.\n"
"\n" 
"  -e       Encrypt with key\n"
"  -d       Decrypt with key\n"
"  -i       File to encrypt/decrypt\n"
"  -o       File to write encrypted/decrypted contents to\n"
"  -h       Print help\n"
"\n"
"Examples:\n"
"  Encrypt:\n"
"    cf-keycrypt -e /var/cfengine/ppkeys/localhost.pub -i myplain.dat -o mycrypt.dat\n"
"\n"
"  Decrypt:\n"
"    cf-keycrypt -d /var/cfengine/ppkeys/localhost.priv -i mycrypt.dat -o myplain.dat\n"
"\n"
"Written and maintained by Jon Henrik Bjornstad <jonhenrik@cfengineers.net>\n"
"\n"
"Copyright (C) cfengineers.net\n"
"\n"
);
}

void *readseckey(char *secfile) {
	FILE *fp=NULL;
	RSA* PRIVKEY=NULL;
	static char *passphrase = "Cfengine passphrase";
	unsigned long err;

	if ((fp = fopen(secfile,"r")) == NULL) {
		printf("Couldn't find a private key - use cf-key to get one");
		return (void *)NULL;
	}
 
	if ((PRIVKEY = PEM_read_RSAPrivateKey(fp,(RSA **)NULL,NULL,passphrase)) == NULL) {
		err = ERR_get_error();
		printf("PEM_readError reading Private Key = %s\n",ERR_reason_error_string(err));
		PRIVKEY = NULL;
		fclose(fp);
		return (void *)NULL;
	} else {
		return (void *)PRIVKEY;
	}
}

void *readpubkey(char *pubfile) {
	FILE *fp;
	RSA *key=NULL;
	static char *passphrase = "Cfengine passphrase";

	if((fp = fopen(pubfile, "r")) == NULL) {
		fprintf(stderr,"Error: Cannot locate Public Key file.\n");
		return NULL;
	}
	if((key = PEM_read_RSAPublicKey(fp,(RSA **)NULL,NULL,passphrase)) == NULL) {
		fprintf(stderr,"Error: failed reading Public Key in '%s' file.\n", pubfile);
		return NULL;
	}
	fclose(fp);
	return (void *)key;
}

long int rsa_encrypt(char *pubfile, char *filein, char *fileout) {
	int ks=0;
	unsigned long size=0, len=0, ciphsz=0;
	RSA* key=NULL;
	FILE *infile=NULL;
	FILE *outfile = NULL;
	char *tmpciph=NULL, *tmpplain=NULL;

	key = (RSA *)readpubkey(pubfile);
	if (!key) {
		return -1;
	}

	if(!(infile = fopen(filein, "r"))) {
		fprintf(stderr, "Error: Cannot locate input file.\n");
		return -1;
	}

	if((strcmp("-",fileout) == 0)) {
		outfile = stdout;
		//printf("Write to stdout\n");
	} else if(!(outfile = fopen(fileout, "w"))){
		fprintf(stderr, "Error: Cannot create output file.\n");
		return -1;
	}

	ks = RSA_size(key);
	tmpplain = (unsigned char *)malloc(ks * sizeof(unsigned char));
	tmpciph = (unsigned char *)malloc(ks * sizeof(unsigned char));
	srand(time(NULL));
	
	while(!feof(infile)) {
		memset(tmpplain, '\0', ks);
		memset(tmpciph, '\0', ks);
		len = fread(tmpplain, 1, ks-11, infile);
		if((size = RSA_public_encrypt(strlen(tmpplain), tmpplain, tmpciph, key, RSA_PKCS1_PADDING)) == -1) {
			fprintf(stderr, "%s\n", ERR_error_string(ERR_get_error(), NULL));
			return -1;
		}
		fwrite(tmpciph,sizeof(unsigned char),ks,outfile);
	}
	fclose(infile);
	fclose(outfile);
	free(tmpciph);
	free(tmpplain);
	RSA_free(key);
	return ciphsz;
}

long int rsa_decrypt(char *secfile, char *cryptfile, char *plainfile) {
	unsigned long int plsz=0, size=0, ks=0, len=0;
	RSA* key=NULL;
	char *tmpplain=NULL, *tmpciph=NULL;

	FILE *fp=NULL;
	FILE *outfile = NULL;

	key = (RSA *)readseckey(secfile);
	if (!key) {
		return -1;
	}

	if(!(fp = fopen(cryptfile, "r"))) {
		fprintf(stderr, "Error: Cannot locate input file.\n");
		return -1;
	}

	if((strcmp("-",plainfile) == 0)) {
		outfile = stdout;
		//printf("Write to stdout\n");
	}else if(!(outfile = fopen(plainfile, "w"))){
		fprintf(stderr, "Error: Cannot create output file.\n");
		return -1;
	}

	ks = RSA_size(key);
	tmpciph = (unsigned char *)malloc(ks * sizeof(unsigned char));
	tmpplain = (unsigned char *)malloc(ks * sizeof(unsigned char));
	//printf("Keysize: %d\n", ks);
	while(!feof(fp)) {
		memset(tmpciph, '\0', ks);
		memset(tmpplain, '\0', ks);
		len = fread(tmpciph, 1, ks, fp);
		if(len > 0){
			if((size = RSA_private_decrypt(ks, tmpciph, tmpplain, key, RSA_PKCS1_PADDING)) == -1) {
				fprintf(stderr, "%s\n", ERR_error_string(ERR_get_error(), NULL));
				return -1;
			}
		}
		fwrite(tmpplain,1,strlen(tmpplain),outfile);
	}
	fclose(fp);
	fclose(outfile);
	free(tmpplain);
	free(tmpciph);
	RSA_free(key);
	return plsz;
}

int main(int argc, char *argv[]) {
	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	opterr = 0;
	char *key = NULL;
	char *infile = NULL;
	char *outfile = NULL;
	int encrypt = 0;
	int decrypt = 0;
	int c = 0;
	int size = 0;

  while ((c = getopt (argc, argv, "he:d:i:o:")) != -1)
		switch (c)
			{
				case 'e':
					encrypt = 1;
					key = optarg;
					break;
				case 'd':
					decrypt = 1;
					key = optarg;
					break;
				case 'i':
					infile = optarg;
					break;
				case 'o':
					outfile = optarg;
					break;
				case 'h':
					usage();
					exit(1);
				default:
					printf("ERROR: Unknown option '-%c'\n", optopt);
					usage();
					exit(1);
			}

	if(encrypt > 0 && key && infile && outfile){
		size = rsa_encrypt(key, infile, outfile);
		if(size < 0)
			exit(1);
	}else if(decrypt > 0 && key && infile && outfile){
		size = rsa_decrypt(key, infile, outfile);
		if(size < 0)
			exit(1);
	}else{
		usage();
		exit(1);
	}
	exit(0);
}
