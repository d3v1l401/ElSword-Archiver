/*
      ELSWORD KOM ARCHIVER
	      MADE BY d3v1l401 (http://d3vsite.org/)

	PUBLIC VERSION BETA-1 (NO CRYPTO).

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <Windows.h>
#include <vector>
#include <rapidxml.hpp>
#include <rapidxml_utils.hpp>
#include <fstream>
#include <sstream>
#include <zlib.h>
#include <zconf.h>
#pragma comment (lib, "zdll.lib")
#pragma warning (disable: 4996)
using namespace rapidxml;
using namespace std;
#define CHUNK 16384

// KOG GC TEAM MASS FILE V.%d.%d.
char Header[27] = { 0 };
// Number of the files in the KOM.
unsigned long FilesNumber = 0;
// Seems to be an internal version trace of the packer version, never changes anyway.
unsigned long Unk2 = 0; // always 0x01000000 (1u).
// The entire KOM CRC, maybe usefull for anti-anti-kom-edit
char CRC[9] = { 0 };
// The size of the XML File in the KOM.
unsigned long XMLSize = NULL;
// Just a global var, used to store the KOM name.
char* KOMName = nullptr;

// This is the struct for the individual file information.
struct XML
{
	char* FName; // File name
	int Size; // Uncompressed file size.
	int CompressedSize; // Compressed file size.
	char *Checksum; // Always equal to FileTime, should be the modifyed CRC.
	char *FileTime; // Always equal to Checksum, should be the modifyed CRC; not a real date (results from 1999).
	int Algorithm; // Algorithm type used to decrypt or decompress the files.
} Files;

// Just the file structs list, used to unpack everything.
std::vector<XML> FilesList;

void die(char* Message)
{
	printf(Message);
	_getch();
	ExitProcess(0);
}

void SaveXML(char* Buffer, int Size)
{
	DeleteFileA("Komfiles.xml");
	FILE* XML = fopen("Komfiles.xml", "wb");
	if (!XML)
		die("Can't write XML file.\n");
	fwrite(Buffer, sizeof(char), Size, XML);
	fclose(XML);
}

int inflate(const void *src, int srcLen, void *dst, int dstLen) {
	z_stream strm = { 0 };
	strm.total_in = strm.avail_in = srcLen;
	strm.total_out = strm.avail_out = dstLen;
	strm.next_in = (Bytef *)src;
	strm.next_out = (Bytef *)dst;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	int err = -1;
	int ret = -1;

	err = inflateInit2(&strm, (15 + 32)); //15 window bits, and the +32 tells zlib to to detect if using gzip or zlib
	if (err == Z_OK) {
		err = inflate(&strm, Z_FINISH);
		if (err == Z_STREAM_END) {
			ret = strm.total_out;
		}
		else {
			inflateEnd(&strm);
			return err;
		}
	}
	else {
		inflateEnd(&strm);
		return err;
	}

	inflateEnd(&strm);
	return ret;
}

void SaveFile(char* Name, char* Buffer, int Size)
{
	DeleteFileA(Name);
	FILE* XML = fopen(Name, "wb");
	if (!XML)
		die("Can't write file.\n");
	fwrite(Buffer, sizeof(char), Size, XML);
	fclose(XML);
}

void Extract(const char* KOMName)
{
	FILE* File = fopen(KOMName, "rb");
	char* Buffer = nullptr;
	unsigned long FileSize = NULL;
	char* XMLBuff = nullptr;
	char* DataBuff = nullptr;
	char* UDataBuff = nullptr;

	if (!File)
		die("Unable to open the file.\n");

	// Getting file size.
	fseek(File, 0, SEEK_END);
	FileSize = ftell(File);
	rewind(File);

	printf("\nFile name: %s.\n", KOMName);
	printf("File size: %d.\n\n", FileSize);

	Buffer = (char*)malloc(FileSize * sizeof(char));
	memset(Buffer, 0x0C, FileSize);

	if (!Buffer)
	{
		printf("Dude, srsly you don't have %d left on your RAM?\n", FileSize);
		_getch();
		return;
	}

	if (fread(Buffer, sizeof(char), FileSize, File) != FileSize)
		die("Unable to read the file buffer.\n");
	fclose(File);

	// --- Let's get KOM's info.

	// KOM Header.

	strncpy_s(Header, Buffer, 26);
	printf("Archive version: %s.\n", Header);

	// Files number.

	FilesNumber = Buffer[52] | Buffer[53] << 8 | Buffer[54] << 16 | Buffer[55] << 24;
	printf("Files number: %d.\n", FilesNumber);

	// XML Size.

	XMLSize = Buffer[68] | Buffer[69] << 8 | Buffer[70] << 16 | Buffer[71] << 24;
	printf("XML size: %d.\n", XMLSize);

	// Let's get Unk 2

	Unk2 = Buffer[56] | Buffer[57] << 8 | Buffer[58] << 16 | Buffer[59] << 24;
	printf("Archiver version: %d.\n", Unk2);

	// CRC (TODO: Spoofing, to avoid anti kom editing)

	/* Not reading correctly, skip it, it's still useless.
	memcpy(CRC, (Buffer + 60), 8);
	*(char*)(CRC + 67) = '\0';
	printf("CRC: 0x%X.\n", CRC);
	*/

	// Let's get the XML.

	XMLBuff = (char*)malloc(XMLSize);
	memset(XMLBuff, 0x00, XMLSize);
	memcpy_s(XMLBuff, XMLSize, (Buffer + 72), XMLSize);
	*(char*)(XMLBuff + XMLSize) = '\0'; // Buffer giving problems determinating the end of the string...let's set an end to this! >:]

	// Now we have all...let's read the XML!

	SaveXML(XMLBuff, XMLSize);

	// Ok, so...libxml won't work, so i'll make this with rapidxml...

	xml_document<> doc;
	std::ifstream file("komfiles.xml");
	std::stringstream bufferx;
	bufferx << file.rdbuf();
	file.close();
	std::string content(bufferx.str());
	doc.parse<0>(&content[0]);

	xml_node<> *pRoot = doc.first_node(); // Files
	for (xml_node<> *pNode = pRoot->first_node("File"); pNode; pNode = pNode->next_sibling())
	{
		Files.Algorithm = NULL;
		Files.Checksum = NULL;
		Files.CompressedSize = NULL;
		Files.FileTime = NULL;
		Files.FName = NULL;
		Files.Size = NULL;
		// File name.
		xml_attribute<> *pAttr = pNode->first_attribute("Name");
		Files.FName = pAttr->value();
		// Its size.
		pAttr = pNode->first_attribute("Size");
		Files.Size = atoi(pAttr->value());
		// Its compressed size.
		pAttr = pNode->first_attribute("CompressedSize");
		Files.CompressedSize = atoi(pAttr->value());
		// Checksum.
		pAttr = pNode->first_attribute("Checksum");
		Files.Checksum = pAttr->value();
		// File timestamp (even if we don't care)
		pAttr = pNode->first_attribute("FileTime");
		Files.FileTime = pAttr->value();
		// MOST IMPORTANT: Algorithm type.
		pAttr = pNode->first_attribute("Algorithm");
		Files.Algorithm = atoi(pAttr->value());
		printf("\n[%s]\n"
			"\tSize: %d.\n"
			"\tCompressed size: %d.\n"
			//"\tChecksum: 0x%X.\n" // MUST BE CORRECTLY CONVERTED! (TODO)
			//"\tLast modify: 0x%X.\n"
			"\tAlgorithm type: %d.\n\n", Files.FName, Files.Size, Files.CompressedSize, Files.Algorithm); //Files.Checksum, Files.FileTime, Files.Algorithm);
		FilesList.push_back(Files);
	}

	// 72 = Entire header KOM info.
	// + XMLSize to skip to copy the XML we don't need.
	void* LastOffset = (Buffer + 72) + XMLSize;

	while (!FilesList.empty())
	{
		XML TData = FilesList.back();

		DataBuff = (char*)malloc(TData.CompressedSize);
		memset(DataBuff, 0x00, TData.CompressedSize);
		UDataBuff = (char*)malloc(TData.Size);
		memset(UDataBuff, 0x00, TData.Size);

		memcpy_s(DataBuff, TData.CompressedSize, (void*)LastOffset, TData.CompressedSize);
		// inflate(DataBuff, TData.CompressedSize, UDataBuff, TData.Size); // NOT WORKING
		/*
			Not working ZLIB Decompression because:
			 - Buffer must be decrypted first
			 - They may use a different configuration od ZLIB
			 - Use another decompression method, TBH i found the ZLIB one but i found it by reverse engineering client, so
			   i will use that one, when i finished to extract it.
			   Expect an update!
		*/
		SaveFile(TData.FName, DataBuff, TData.CompressedSize);

		// Skipping already extracted buffer, now time for the new buffer.
		LastOffset = (void*)((int)LastOffset + TData.CompressedSize);

		free(DataBuff);
		FilesList.pop_back();
	}

	_getch();
}

int main(int argc, char **argv)
{
	SetConsoleTitle("[ElSword KOM Archiver][d3v1l401]");
	printf("[ElSword KOM Archiver]\nMade by d3v1l401 (http://d3vsite.org/).\nCompiled at %s %s.\n\n", __DATE__, __TIME__);
	printf("This program is free software: you can redistribute it and/or modify\n"
		"it under the terms of the GNU General Public License as published by\n"
		"the Free Software Foundation, either version 3 of the License, or\n"
		"(at your option) any later version.\n"
		"\n"
		"This program is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the\n"
		"GNU General Public License for more details.\n"
		"\n"
		"You should have received a copy of the GNU General Public License\n"
		"along with this program.If not, see <http://www.gnu.org/licenses/>.\n\n\n\n");

	if (argc != 2)
	{
		printf("\tUsage: %s <data.kom>\n", argv[0]);
		_getch();
		return(0);
	}

	KOMName = argv[1];
	Extract(KOMName);



	return (1);
}