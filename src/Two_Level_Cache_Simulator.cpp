#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <stdlib.h>
#include <cmath>
#include <bitset>
#include <algorithm>

using namespace std;
//access state:
#define NA 0 // no action
#define RH 1 // read hit
#define RM 2 // read miss
#define WH 3 // Write hit
#define WM 4 // write miss


struct config{
	int L1blocksize;
	int L1setsize;
	int L1size;
	int L2blocksize;
	int L2setsize;
	int L2size;
};


/* you can define the cache class here, or design your own data structure for L1 and L2 cache */
class cache {

public:
	unsigned long blockSize;
	unsigned long setSize;
	unsigned long cacheSize;
	unsigned long noOfBlocks;
	unsigned long indexSize;
	unsigned long tagSize;
	unsigned long offsetSize;
	vector< vector<unsigned long> > cacheBlock;
	vector< vector<bool> > validBit;
	vector< vector<unsigned long> > LRU;

	// Constructor
	cache(unsigned long blockSize, unsigned long setSize, unsigned long cacheSize){
		this->blockSize = blockSize; // Cache block size
		this->cacheSize = cacheSize; // Cache size in kB
		this->setSize = (setSize == 0)?(((this->cacheSize) * 1024)/((this->blockSize))):(setSize); // Set associative value
		this->noOfBlocks = ((this->cacheSize) * 1024)/((this->blockSize) * (this->setSize)); // No of blocks available in each set
		this->indexSize = log2(this->noOfBlocks); // Index bit size
		this->offsetSize = log2(this->blockSize); // Offset bit size
		this->tagSize = (32 - this->indexSize - this->offsetSize); // Tag bit size (32 - index - offset)
		this->cacheBlock = vector< vector<unsigned long> >(this->noOfBlocks, vector<unsigned long>(this->setSize)); // 2-D vector(array) for addresses
		this->validBit = vector< vector<bool> >(this->noOfBlocks, vector<bool>(this->setSize)); // 2-D vector(array) for validBit
		this->LRU = vector< vector<unsigned long> >(this->noOfBlocks, vector<unsigned long>(this->setSize)); // 2-D vector(array) for MRU
	}


	void values(){
		cout << "Block Size: "<< blockSize << " bytes" << endl;
		cout << "Set Associative: "<< setSize << endl;
		cout << "Cache capacity: " << cacheSize << " kB" << endl;
		cout << "No of blocks: " << noOfBlocks << endl;
		cout << "Offset bits: " << offsetSize << endl;
		cout << "Index bits: " << indexSize << endl;
		cout << "Tag bits: " << tagSize << endl;
		cout << "Cache memory array size: " << cacheBlock.size() << " | " << cacheBlock[0].size() << endl;
		cout << "LRU: " << LRU.size() << " | " << LRU[0].size() << endl;
		cout << "-------------------------------" << endl;
	}

	bitset<32> tag(bitset<32> addr){
		return (addr.to_ulong()>>(indexSize + offsetSize));
	}

	bitset<32> index(bitset<32> addr){
		bitset<32> index = (addr.to_ulong()>>(offsetSize));
		bitset<32> mask = pow(2, indexSize) - 1;
		index = (index & mask);
		return index;
	}

	bitset<32> offset(bitset<32> addr){
		bitset<32> offset = (addr.to_ulong());
		bitset<32> mask = pow(2, offsetSize) - 1;
		offset = (offset & mask);
		return offset;
	}
};

int main(int argc, char* argv[]){
    config cacheconfig;
    ifstream cache_params;
    string dummyLine;
    cache_params.open(argv[1]);
    while(!cache_params.eof())  // read config file
    {
		cache_params>>dummyLine;
		cache_params>>cacheconfig.L1blocksize;
		cache_params>>cacheconfig.L1setsize;
		cache_params>>cacheconfig.L1size;
		cache_params>>dummyLine;
		cache_params>>cacheconfig.L2blocksize;
		cache_params>>cacheconfig.L2setsize;
		cache_params>>cacheconfig.L2size;
    }


	// Implement by you:
	// initialize the hirearch cache system with those configs
	// probably you may define a Cache class for L1 and L2, or any data structure you like
    cache L1(cacheconfig.L1blocksize, cacheconfig.L1setsize, cacheconfig.L1size);
    cache L2(cacheconfig.L2blocksize, cacheconfig.L2setsize, cacheconfig.L2size);
    L1.values();
    L2.values();


    int L1AcceState =0; // L1 access state variable, can be one of NA, RH, RM, WH, WM;
    int L2AcceState =0; // L2 access state variable, can be one of NA, RH, RM, WH, WM;
    bool L2check = 0;
    bool RH1=false;
    bool RM1=false;
    bool RH2=false;
    bool RM2=false;
    bool WH1=false;
    bool WM1=false;
    bool WH2=false;
    bool WM2=false;


    ifstream traces;
    ofstream tracesout;
    string outname;
    outname = string(argv[2]) + ".out";

    traces.open(argv[2]);
    tracesout.open(outname.c_str());


    string line;
    string accesstype;  // the Read/Write access type from the memory trace;
    string xaddr;       // the address from the memory trace store in hex;
    unsigned int addr;  // the address from the memory trace store in unsigned int;
    bitset<32> accessaddr; // the address from the memory trace store in the bitset;

    unsigned long L1index;
    unsigned long L2index;
    unsigned long L1tag;
    unsigned long L2tag;

    int counter = 0;
    if (traces.is_open()&&tracesout.is_open()){
    	while (getline (traces,line)){   // read mem access file and access Cache
    		counter++;
            istringstream iss(line);
            if (!(iss >> accesstype >> xaddr)) {break;}
            stringstream saddr(xaddr);
            saddr >> std::hex >> addr;
            accessaddr = bitset<32> (addr);

            L1index = L1.index(accessaddr).to_ulong();
            L2index = L2.index(accessaddr).to_ulong();
            L1tag = L1.tag(accessaddr).to_ulong();
            L2tag = L2.tag(accessaddr).to_ulong();


            // Read L1 or L2 or both
            if (accesstype.compare("R")==0){

            	for(unsigned long i=0; i<L1.setSize;i++){
            		// Read Hit L1
            		if(L1.cacheBlock[L1index][i] == L1tag && L1.validBit[L1index][i]){
            			L1AcceState = RH;
						L2AcceState = NA;

						for(unsigned long j=0; j<L1.setSize;j++){
							if(L1.LRU[L1index][j] < L1.LRU[L1index][i] && L1.LRU[L1index][j] != 0 && L1.LRU[L1index][i] != 1){
								L1.LRU[L1index][j] += 1;
							}
						}
						L1.validBit[L1index][i] = 1;
						L1.LRU[L1index][i] = 1;
						L2check = 0;
						RH1 = true;
						RM1 = true;
						break;
            		}
            	}



            	if(!RH1){
            		L1AcceState = RM;
					L2check = 1;
					for(unsigned long i=0; i<L1.setSize;i++){
						if (L1.validBit[L1index][i] == 0){
							L1.cacheBlock[L1index][i] = L1tag;
							L1.validBit[L1index][i] = 1;
							L1.LRU[L1index][i] = 1;
							for(int j=0;j<i;j++){
								L1.LRU[L1index][j] += 1;
							}
							RM1 = true;
							break;
						}
					}
				}
				if(!RM1){
					for(unsigned long i=0; i<L1.setSize;i++){
						if(L1.LRU[L1index][i] == (L1.setSize)){
							L1.cacheBlock[L1index][i] = L1tag;
							L1.validBit[L1index][i] = 1;
							for(unsigned long j=0; j<L1.setSize;j++){
								if(j!=i){
									L1.LRU[L1index][j] += 1;
								}
							}
							L1.LRU[L1index][i] = 1;
							break;
						}
					}
				}


            	// If L2 has to be checked i.e. if L1 cache has read hit then L2 cache doesn't need to be accessed
            	if(L2check){
					for(unsigned long i=0; i<L2.setSize;i++){
						// Read Hit L2
						if(L2.cacheBlock[L2index][i] == L2tag && L2.validBit[L2index][i]){
							L2AcceState = RH;
							for(unsigned long j=0; j<L2.setSize;j++){
								if(L2.LRU[L2index][j] < L2.LRU[L2index][i] && L2.LRU[L2index][j] != 0 && L2.LRU[L2index][i] != 1){
									L2.LRU[L2index][j] += 1;
								}
							}
							L2.validBit[L2index][i] = 1;
							L2.LRU[L2index][i] = 1;
							RH2 = true;
							RM2 = true;
							break;
						}
					}


					if(!RH2){
						// Read Miss L2
						L2AcceState = RM;
						for(unsigned long i=0; i<L2.setSize;i++){
							if (L2.validBit[L2index][i] == 0){
								L2.cacheBlock[L2index][i] = L2tag;
								L2.validBit[L2index][i] = 1;
								L2.LRU[L2index][i] = 1;
								for(int j=0;j<i;j++){
									L2.LRU[L2index][j] += 1;
								}
								RM2 = true;
								break;
							}
						}
					}
					if(!RM2){
						for(unsigned long i=0; i<L2.setSize;i++){
							if(L2.LRU[L2index][i] == (L2.setSize)){
								L2.cacheBlock[L2index][i] = L2tag;
								L2.validBit[L2index][i] = 1;
								for(unsigned long j=0; j<L2.setSize;j++){
									if(j!=i){
										L2.LRU[L2index][j] += 1;
									}
								}
								L2.LRU[L2index][i] = 1;
								break;
							}
						}
					}
					// Reset the value of L2check
					L2check = 0;
				}

            }


            // Write L1 or L2 or both
            else {
            	for(unsigned long i=0; i<L1.setSize;i++){
            		// Write Hit L1
            		if(L1.cacheBlock[L1index][i] == L1tag){
            			L1AcceState = WH;
            			L1.validBit[L1index][i] = 1;
            			for(unsigned long j=0; j<L1.setSize;j++){
							if((L1.LRU[L1index][j] < L1.LRU[L1index][i]) && (L1.LRU[L1index][j] != 0) && (L1.LRU[L1index][i] != 1)){
								L1.LRU[L1index][j] += 1;
							}
						}
						L1.LRU[L1index][i] = 1;
						break;
            		}
            		// Write Miss L1
            		else {
            			L1AcceState = WM;
            		}
            	}

            	for(unsigned long i=0; i<L2.setSize;i++){
            		// Write Hit L2
					if(L2.cacheBlock[L2index][i] == L2tag){
						L2AcceState = WH;
						L2.validBit[L2index][i] = 1;
						for(unsigned long j=0; j<L2.setSize;j++){
							if((L2.LRU[L2index][j] < L2.LRU[L2index][i]) && (L2.LRU[L2index][j] != 0) && (L2.LRU[L2index][i] != 1)){
								L2.LRU[L2index][j] += 1;
							}
						}
						L2.LRU[L2index][i] = 1;
						break;
					}
					// Write Miss L2
					else {
						L2AcceState = WM;
					}
				}
            }



             RH1=false;
             RM1=false;
             RH2=false;
             RM2=false;
             WH1=false;
             WM1=false;
             WH2=false;
             WM2=false;
            tracesout<< L1AcceState << " " << L2AcceState << endl;  // Output hit/miss results for L1 and L2 to the output file;
            //cout << "-----------------------------------------------" << endl;
        }
        traces.close();
        tracesout.close();

        cout << "Done" << endl;
    }
    else cout<< "Unable to open trace or traceout file ";

    return 0;
}
