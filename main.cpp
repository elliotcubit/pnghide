#include <fstream>
#include <iostream>
#include <string>
#include <stdlib.h>

#include <zlib.h> // We will use zlib for our CRC calculation

#define bswap32 __builtin_bswap32

using namespace std;

string chunkHeader = "seCr";
unsigned char fileHeader[] = {0x89, 'P', 'N', 'G', 13, 10, 26, 10};

// TODO detect endianness of host machine

void encode(string in, string hide, string out){
	ifstream ifs;
	ifs.open(in, ifstream::binary);

	ifstream hfs;
	hfs.open(hide, ifstream::binary | ifstream::ate );

	ofstream ofs;
	ofs.open(out, ifstream::binary);

	// verify that we have a png file
	unsigned char b;
	for (int i=0; i<8; i++){
		b = ifs.get();
		ofs << b;
		if (b != fileHeader[i]){
			cout << "A .PNG file was not passed into the program." << endl;
			ifs.close();
			ofs.close();
			hfs.close();
			exit(1);
		}
	}

	// read out the required IHDR chunk.
	unsigned int size;
	ifs.read(reinterpret_cast<char *>(&size), sizeof(4));

	// rewrite the size to the new file
	ofs.write(reinterpret_cast<char *>(&size), sizeof(size));	

	size = bswap32(size) + 8; // +8 -- 4 bytes for IHDR and 4 bytes for CRC

	// copy all the bytes of it to ofs
	for (int i=0; i<size; i++){
		b = ifs.get();
		ofs << b;
	}

	// calculate size of our payload.
	size = (unsigned int) hfs.tellg() -1; // -1 because this includes EOF

	unsigned char * dat = (unsigned char *) malloc(size+4);
	for (int i=0; i<4; i++){
		*(dat+i) = chunkHeader.c_str()[i];
	}
	

	// write that size 
	size = bswap32(size); // haha
	ofs.write(reinterpret_cast<char *>(&size), sizeof(size));
	size = bswap32(size);

	// then write hfs to data pointer
	hfs.seekg(0);
	for (unsigned int i=4; i<size+4; i++){
		*(dat+i) = hfs.get();
	}


	// write data to ofs
	ofs.write(reinterpret_cast<char *>(dat), size+4);

	const unsigned char * data = dat; 

	uint32_t crc = crc32(0L, Z_NULL, 0);
	crc = bswap32(crc32(crc, data, size+4));

	ofs.write(reinterpret_cast<char *>(&crc), sizeof(crc));

	// now write the rest of ifs to ofs.
	unsigned int place = ifs.tellg();
	ifs.seekg(0, ifs.end);
	size = ifs.tellg();
	ifs.seekg(place);

	for (int i=place; i<size; i++){
		b = ifs.get();
		ofs << b;
	}

	ifs.close();
	hfs.close();
	ofs.close();
}

void decode(string in){
	ifstream ifs;
	ifs.open(in, ifstream::binary);

	// read out random stuff
	// there are always 33 bytes before our header
	unsigned char b;
	for (int i=0; i<33; i++){
		b = ifs.get();
	}

	// size of secret chunk is here if it exists
	uint32_t size;
	ifs.read(reinterpret_cast<char *>(&size), sizeof(4));

	size = bswap32(size);

	// verify this is a png from our program
	for (int i=0; i<4; i++){
		b = ifs.get();
		if (b != chunkHeader[i]){
			cout << "Nothing is hidden here." << endl;
			ifs.close();
			exit(1);
		}
	}

	// if it is write our stuff
	ofstream ofs;
	ofs.open("decoded.out", ofstream::binary);

	for (int i=0; i<size; i++){
		b = ifs.get();
		ofs << b;
	}

	ofs.close();


	
}

int main(int argc, char * argv[]){

	if (argc < 2){
		cout << "Usage: ./pnghide <-e> <-d> [filename] (outfilename) (filetohide)" << endl;
		exit(1);
	}

	if (string(argv[1]) == "-e"){
		encode(argv[2], argv[4], argv[3]);
	}
	else{
		decode(argv[2]);
	}


}


