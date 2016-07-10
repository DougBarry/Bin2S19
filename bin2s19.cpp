// bin2s19.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

/* This program converts binary file into S19 format to defined address.
If input S19 file is given, files are added together.
Addresses in the S19 file should be in positive order. */

#include <stdio.h>
#include <string.h>

FILE *s19in;
FILE *s19out;
FILE *binary;

enum s19error { NoErr = 0, Eof = 1, NotS = 2, Len = 3, ChSum = 4, NHex = 5 };
struct s1struct {
	unsigned addr;  // Initial addrress of the packet
	unsigned char bytes, type;  // data bytes in the packet, type (1 or 9)
};

void manual();
int s19wr(char line[], unsigned char bufffer[], unsigned addr, unsigned char bytes, unsigned char type);
enum s19error s19info(char line[], struct s1struct *info);
unsigned char hex2bin2(char line[], enum s19error *err);
unsigned char hex2bin(char line[], enum s19error *err);
void bin2s1(FILE *binary, FILE *s19out, unsigned int addr, unsigned int addrtop);
unsigned char s19scan(FILE *s19in, char line[], struct s1struct *info);


unsigned int addr, addrtop;  //addrresses for binary file
unsigned char binwr;
struct s1struct info;

int main(int argc, char *argv[])
{
	int status;
	//	addr=0,addrtop=59000,argc=5;
	binwr = 0;
	char line[80], line19[80];
	unsigned char buffer[32];
	if (argc<5 | argc>6) {  // Check for correct number of arguments
		manual();
		return(1);
	}
	status = sscanf(argv[2], "%x", &addr);  // Get start address
	if (!status | status == EOF) {
		manual();
		return(1);
	}
	status = sscanf(argv[3], "%x", &addrtop);  // Get end address
	if (!status | status == EOF) {
		manual();
		return(1);
	}
	if (addrtop<addr) {
		manual();
		return(1);
	}
	if (argc == 6) {  // Open input S19 file
		s19in = fopen(argv[4], "rt");
		//		s19in=fopen("232rcv.s19","rt");
		if (!s19in) {
			printf("\nError: Can't open %s.\n", argv[4]);
			return(1);
		}
	}
	s19out = fopen(argv[4 + (argc == 6)], "w+t");  // Open output S19 file
												   //	s19out=fopen("s19.s19","w+t");
	if (!s19out) {
		printf("\nError: Can't open %s.\n", argv[4 + (argc == 6)]);
		return(1);
	}
	binary = fopen(argv[1], "rb");  // Open binary file
									//	binary=fopen("bin.bin","rb");
	if (!binary) {
		printf("\nError: Can't open %s.\n", argv[1]);
		return(1);
	}

	if (argc == 5) {  // If no input S19 file is given
		bin2s1(binary, s19out, addr, addrtop);  // convert binary file to S19
		s19wr(line, buffer, 0, 0, 9);  // and add S9
		fputs(line, s19out);
	}
	else {  // If input file is given
		do {
			if (s19scan(s19in, line19, &info)) return(1);  // Read one line from S19 file
			if (((long)info.addr + info.bytes <= addr) && info.type == 1) {  // If it is below binary file, write it
				fprintf(s19out, "%s\n", line19);
			}
			else if ((info.addr <= addrtop) && info.type == 1) {  // Check for overlaping
				printf("\nError: Binary and S19 files overlap.\n");
				return(1);
			}
			else if (!binwr) {  // If the line is above binary file, write binary file
				bin2s1(binary, s19out, addr, addrtop);
				binwr = 1;
				fprintf(s19out, "%s\n", line19); // Write previously read S1 line
			}
			else  // If the binary file was already written write the S1 line
				fprintf(s19out, "%s\n", line19);
		} while (info.type == 1);  // Do till the end of S19 file
	}
	_fcloseall();
	return(0);
}


/*
This function writes S record to line.
buffer points to the location of the bytes data bytes
type is 1 or 9
*/
int s19wr(char line[], unsigned char buffer[], unsigned addr, unsigned char bytes, unsigned char type) {
	if (type == 1) {
		unsigned char sum, i, j;
		sum = 0xFF - (unsigned char)addr - (unsigned char)(addr / 256) - (bytes + 3);
		sprintf(line, "S1%02X%04X", bytes + 3, addr);
		for (i = 0, j = 6; i<bytes; i++) {
			sum -= buffer[i];
			sprintf(line + (j += 2), "%02X", buffer[i]);
		}
		sprintf(line + (j += 2), "%02X\n", sum);
	}
	else {
		sprintf(line, "S903%04X%02X\n", addr, (unsigned char)(0xFF - 3 - (unsigned char)addr - (unsigned char)(addr / 256)));
	}
	return(0);
}

/*
This function fills the structure info with address, number of bytes and
type of S record in line.
*/
enum s19error s19info(char line[], struct s1struct *info) {
	enum s19error err = NoErr;
	if (strlen(line)<10)  // Check for minimum length.
		return(Len);
	if (!strncmp(line, "S1", 2)) (*info).type = 1;
	else if (!strncmp(line, "S9", 2)) (*info).type = 9;
	else return(NotS);
	(*info).bytes = hex2bin2(line + 2, &err) - 3;  // Get length (data)
	if (err != NoErr) return(NoErr);
	if (strlen(line) != 10 + 2 * (*info).bytes)  // Check for correct length.
		return(Len);
	else
		(*info).addr = 256 * hex2bin2(line + 4, &err) + hex2bin2(line + 6, &err);  // Get address.
	return(NoErr);
}


/* hex2bin2 converts two ASCII characters to number using hex2bin */
unsigned char hex2bin2(char line[], enum s19error *err) {
	return(16 * hex2bin(line, err) + hex2bin(line + 1, err));
}

/* hex2bin converts ASCII character '0'-'9' and 'A'-'F' to number.
If character cannot be converted, enum s19error *err is set. */
unsigned char hex2bin(char line[], enum s19error *err) {
	unsigned char tmp;
	tmp = line[0] - '0';
	if (tmp>'F' - '0' || (9<tmp&&tmp<'A' - '0')) {
		*err = NHex;
		return(0);
	}
	if (tmp>9) tmp -= 'A' - '9' - 1;
	return(tmp);
}

void manual() {
	printf("\nThis program converts binary file into S19 file.");
	printf("\nAs an option binary file can be added to existing S19 file.");
	printf("\nWritten by Bostjan Glazar, 2002.");
	printf("\n\nUsage: BIN2S19 <binary file> <initial address> <end address>");
	printf("\n       [<input S19>] <output S19>");
	printf("\n\nNumber are in hexadecimal format.\n");
}


/*
This function converts binary file to S1 records.
addr is starting address, while addrtop is limiting address to which data is
written. S9 is not added at the end.
*/
void bin2s1(FILE *binary, FILE *s19out, unsigned int addr, unsigned int addrtop) {
	unsigned char end = 0;
	unsigned char bytes, buffer[32];
	char line[81];
	do {
		bytes = fread(buffer, 1, 0x20, binary);
		if (!bytes) break;
		if ((long)addr + bytes >= (long)addrtop + 1) {
			s19wr(line, buffer, addr, addrtop - addr + 1, 1);
			end = 1;
		}
		else
			s19wr(line, buffer, addr, bytes, 1);
		fputs(line, s19out);
		addr += 0x20;
	} while (!end);
}


/*
This function gets a line from file and gets a S info from it.
If an error occurres it is displayed and 1 is returned.
*/
unsigned char s19scan(FILE *s19in, char line[], struct s1struct *info) {
	int status;
	enum s19error err;
	status = fscanf(s19in, "%74s", line);
	if (!status | status == EOF) {
		printf("\nError: Unexpected end of S19 file.");
		return(1);
	}
	if (err = s19info(line, info)) {
		switch (err) {
		case Eof:	printf("Error: End of file reached before detecting S9."); break;
		case NotS: printf("Error: S record does not begin with ""S1"" or ""S2""."); break;
		case Len: printf("Error: Incorrect length in S record."); break;
		case ChSum: printf("Error: Checksum error."); break;
		case NHex: printf("Error: A non-hex character detected in S record.");
		}
		_fcloseall();
		return(1);
	}
	return(0);
}