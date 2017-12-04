// ==========================================================
// RepZ project
// 
// Component: xnp
// Sub-component: libnp
// Purpose: Encrypt datastream between the client and the np server
//
// Initial author: Nova and iain17
// Started: 2014-12-24
// ==========================================================

#include "StdInc.h"
#include "NPEncryption.h"
#include <string>
#include <sstream>
#include "base64.h"

#define LTM_DESC
//#include <tomcrypt.h>

//Crypto++
#include <osrng.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cryptlib.h>
#include "hex.h"
#include "filters.h"
#include "aes.h"
#include "ccm.h"
#include "assert.h"

using CryptoPP::StringSink;
using CryptoPP::StringSource;
using CryptoPP::StreamTransformationFilter;
using CryptoPP::HexEncoder;
using CryptoPP::HexDecoder;
using CryptoPP::Exception;
using CryptoPP::AutoSeededRandomPool;

using std::string;
using std::exit;
using std::cout;
using std::cerr;
using std::endl;

using CryptoPP::AES;
using CryptoPP::CBC_Mode;

using namespace std;



//Thank you Nova
//Input: Plain text const char*
//Return: encrypted hex'xed char*
LIBNP_API char* LIBNP_CALL NP_Encrypt(const char* plain2){

	AutoSeededRandomPool prng;

	byte key[AES::DEFAULT_KEYLENGTH] = { '5', 'A', '3', 'D', '9', '4', '8', 'D', '4', '9', '7', 'B', 'A', 'B', '9', 'C' }; //Random array of 16 characters
	// Fixes key to 35413344393438443439374241423943
	//prng.GenerateBlock(key, sizeof(key)); //From original program to randomize key
	byte iv[AES::BLOCKSIZE];
	prng.GenerateBlock(iv, sizeof(iv));

	string plain = "CBC Mode Test Once Again"; //Set string
	string cipher, encoded, recovered;

	//ofstream myFile("myOutput.txt"); //Debug


	/*********************************\
	\*********************************/

	// Pretty print key
	encoded.clear();
	StringSource(key, sizeof(key), true,
		new HexEncoder(
		new StringSink(encoded)
		) // HexEncoder
		); // StringSource
	Log_Print("key: %s\n", encoded);
	//myFile << "Key: " << encoded << endl; //Debug

	// Pretty print iv
	encoded.clear();
	StringSource(iv, sizeof(iv), true,
		new HexEncoder(
		new StringSink(encoded)
		) // HexEncoder
		); // StringSource
	Log_Print("iv: %s\n", encoded);
	//myFile << "IV: " << encoded << endl; //Debug

	/*********************************\
	\*********************************/

	try
	{
		Log_Print("plain text: %s\n", plain);
		CBC_Mode< AES >::Encryption e;
		e.SetKeyWithIV(key, sizeof(key), iv);

		//	e.SetKeyWithIV(test_key, sizeof(test_key), iv);
		// The StreamTransformationFilter removes
		//  padding as required.
		StringSource s(plain, true,
			new StreamTransformationFilter(e,
			new StringSink(cipher)
			) // StreamTransformationFilter
			); // StringSource

#if 0
		StreamTransformationFilter filter(e);
		filter.Put((const byte*)plain.data(), plain.size());
		filter.MessageEnd();

		const size_t ret = filter.MaxRetrievable();
		cipher.resize(ret);
		filter.Get((byte*)cipher.data(), cipher.size());
#endif
	}
	catch (const CryptoPP::Exception& e)
	{
		Log_Print("ERROR: %s\n", e.what());
		return NULL;
	}

	/*********************************\
	\*********************************/

	// Pretty print
	encoded.clear();
	StringSource(cipher, true,
		new HexEncoder(
		new StringSink(encoded)
		) // HexEncoder
		); // StringSource
	Log_Print("cipher text: %s\n", encoded);
	//myFile << "Cipher: " << encoded << endl; //Debug
	//myFile.close(); // Debug
	/*********************************\
	\*********************************/

	try
	{
		CBC_Mode< AES >::Decryption d;
		d.SetKeyWithIV(key, sizeof(key), iv);

		// The StreamTransformationFilter removes
		//  padding as required.
		StringSource s(cipher, true,
			new StreamTransformationFilter(d,
			new StringSink(recovered)
			) // StreamTransformationFilter
			); // StringSource

#if 0
		StreamTransformationFilter filter(d);
		filter.Put((const byte*)cipher.data(), cipher.size());
		filter.MessageEnd();

		const size_t ret = filter.MaxRetrievable();
		recovered.resize(ret);
		filter.Get((byte*)recovered.data(), recovered.size());
#endif

		Log_Print("recovered text: %s\n", recovered);
	}
	catch (const CryptoPP::Exception& e)
	{
		Log_Print("ERROR: %s\n", e.what());
		return NULL;
	}

	/*********************************\
	\*********************************/

}



/*
tomcrypt trial

LIBNP_API void LIBNP_CALL NP_Encrypt(char* buffer, int bufferLen, char* outPacket, int outPacketLen){

	register_cipher(&aes_desc);
	register_hash(&sha1_desc);
	register_prng(&fortuna_desc);

	ltc_mp = ltm_desc;

	char name[12];
	name[0] = 'a'; name[1] = 'e'; name[2] = 's'; name[3] = '\0';

	int aes = find_cipher(name);

	prng_state prng;

	name[0] = 's'; name[1] = 'h'; name[2] = 'a'; name[3] = '1'; name[4] = '\0';
	int hash_idx = find_hash(name);

	name[0] = 'f'; name[2] = 'r'; name[1] = 'o'; name[3] = 't'; name[4] = 'u'; name[5] = 'n'; name[6] = 'a'; name[7] = '\0';
	int index = find_prng(name); // I've no crypto experience at all, but this PRNG seemed nice as it uses sha256; why is there no 'recommended PRNG'?

	rng_make_prng(512, index, &prng, NULL);

	// generate a key pair for this file
	unsigned char key[32];
	unsigned char iv[16];
	memset(key, 0, sizeof(key));
	memset(iv, 0, sizeof(iv));

	//fortuna_read(key, 32, &prng);
	//fortuna_read(iv, 16, &prng);
	rng_get_bytes(key, 32, NULL);
	rng_get_bytes(iv, 16, NULL);

	// move them to a single pack; as stack allocation may be uncertain
	unsigned char keyiv[48];
	memcpy(&keyiv[0], key, sizeof(key));
	memcpy(&keyiv[32], iv, sizeof(iv));

	// encrypt the file
	symmetric_CBC ctr;

	unsigned char niv[16];
	memset(niv, 0, sizeof(niv));

	cbc_start(aes, niv, key, 32, 0, &ctr);
	cbc_setiv(iv, sizeof(iv), &ctr);
	cbc_encrypt((uint8_t*)buffer, (uint8_t*)&outPacket[256], bufferLen + (16 - (bufferLen % 16)), &ctr);
	cbc_done(&ctr);

	// make a header thing
	const char publicKeyDER[] = { 0x30, 0x82, 0x01, 0x22, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0F, 0x00, 0x30, 0x82, 0x01, 0x0A, 0x02, 0x82, 0x01, 0x01, 0x00, 0xC4, 0xAC, 0x85, 0xEB, 0x19, 0x06, 0xC3, 0x83, 0x30, 0x93, 0x69, 0xEC, 0xF0, 0xF6, 0x5C, 0x50, 0xFA, 0x33, 0x6B, 0x96, 0x2E, 0x8E, 0x3B, 0x52, 0x7E, 0x53, 0x2A, 0x01, 0x1D, 0x2E, 0xF2, 0xAE, 0xDD, 0x12, 0xEF, 0xC1, 0xC8, 0x72, 0x7D, 0x4A, 0x57, 0xF7, 0x8D, 0x8A, 0x26, 0x2F, 0xD1, 0x83, 0x5D, 0x16, 0xBF, 0xE7, 0x45, 0x61, 0x4C, 0xDE, 0x6A, 0xE7, 0x53, 0x48, 0x2F, 0x2A, 0xAB, 0x68, 0x3C, 0x5F, 0xDF, 0xA6, 0x4C, 0x51, 0x50, 0x77, 0xA2, 0x4A, 0xA0, 0x12, 0xA4, 0x4E, 0x26, 0x26, 0x24, 0x8F, 0x6F, 0x1C, 0xEB, 0x98, 0x90, 0xEB, 0x58, 0x81, 0x5F, 0x01, 0xBD, 0xF6, 0x52, 0x17, 0xCD, 0x80, 0x51, 0xC9, 0x6D, 0x13, 0xAA, 0x02, 0x80, 0x0D, 0xE1, 0x67, 0x7D, 0xC3, 0x5A, 0x86, 0x0A, 0x93, 0x2F, 0x72, 0x43, 0x0F, 0xE9, 0x9A, 0x4B, 0x44, 0x2F, 0x41, 0xE6, 0x7B, 0xD5, 0x53, 0x95, 0x99, 0xA3, 0x06, 0x56, 0x60, 0xF1, 0x95, 0xDB, 0x23, 0x29, 0x68, 0x52, 0x30, 0x7A, 0xD9, 0xBB, 0x8D, 0xEC, 0x85, 0xDF, 0xA3, 0x64, 0x15, 0xAA, 0xD2, 0xAB, 0x02, 0xF9, 0x6C, 0x8A, 0x5E, 0x96, 0x59, 0x7A, 0xD0, 0x03, 0xD8, 0xEA, 0x18, 0x65, 0xFD, 0xEA, 0x8C, 0xF0, 0xD9, 0x9B, 0x11, 0x16, 0xA8, 0xCE, 0x76, 0x19, 0x4B, 0x49, 0xF6, 0xA5, 0x5A, 0xEF, 0x24, 0x81, 0xB9, 0x17, 0xE5, 0x08, 0xE2, 0x6F, 0x9E, 0xDB, 0x73, 0x39, 0x2A, 0x95, 0xBF, 0xA1, 0x55, 0x3B, 0xB6, 0x24, 0x86, 0xFC, 0x35, 0xF3, 0x7E, 0x3C, 0x96, 0x5A, 0xC0, 0x0A, 0x10, 0x21, 0xF4, 0xCA, 0x7B, 0x5E, 0x83, 0x09, 0x4E, 0x85, 0x4F, 0x1D, 0xE3, 0xC5, 0x96, 0xAB, 0x49, 0x8A, 0x70, 0x73, 0x3C, 0x88, 0x36, 0x4F, 0xD4, 0x4D, 0x60, 0x2B, 0x0F, 0x8D, 0x0D, 0x44, 0x7C, 0xF9, 0x1D, 0xAC, 0x11, 0xFB, 0xDD, 0x02, 0x03, 0x01, 0x00, 0x01 };
	rsa_key rkey;
	rsa_import((uint8_t*)publicKeyDER, sizeof(publicKeyDER), &rkey);

	unsigned long outSize = 256;

	rsa_encrypt_key(keyiv, sizeof(keyiv), (uint8_t*)outPacket, &outSize, NULL, 0, &prng, index, hash_idx, &rkey);
}
*/