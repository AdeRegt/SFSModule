#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define ASSUMED_SECTOR_SIZE 512
#define SFS_MAX_FILE_NAME 12
#define SFS_FILE_TABLE_ENTRIES 32

//
// Filesystem created by Alexandros de Regt
// 

typedef struct {
	// this code is the bootjump which should make the sector able to boot
	unsigned char 		bootjmp[3];
	// this signature proves it is a SFS sector
	unsigned char		signature; // always 0xCD
	// this versionnumber should be 1
	unsigned char		version; // always 1
	// sectorsize of medium
	unsigned short		sectorsize;
	// offset to first sectortable
	unsigned short		offset_first_sectortable;
	// sectors per sectortable 
	unsigned char		sectortablesize;
	// bootcode
	unsigned char		bootcode[502];
}__attribute__((packed)) SFSBootsector;

typedef struct{
	// filename
	unsigned char filename[SFS_MAX_FILE_NAME];
	// fileid
	unsigned char fileid;
	// filesize in last sector
	unsigned short filesize_lastsector;
	// flags
	unsigned char flags;
}__attribute__((packed)) SFSFileEntry;

typedef struct{
	SFSFileEntry entries[SFS_FILE_TABLE_ENTRIES];
}__attribute__((packed)) SFSDirectory;

FILE *bestand;
int maxsector;
int sectoroffset = 0;
SFSBootsector *bootsector;

int readSector(int LBA,unsigned char* buffer){
	printf("[INFO] Reading sector %i \n",LBA);
	LBA += sectoroffset;
	if(buffer==NULL){
		printf("[ERRO] Trying to read sector to nothing!\n");
		exit(EXIT_FAILURE);
	}
	if(LBA==maxsector||LBA>maxsector){
		printf("[ERRO] Trying to read sector %i while max sector is %i \n",LBA,maxsector);
		exit(EXIT_FAILURE);
	}
	int offset = LBA*ASSUMED_SECTOR_SIZE;
	fseek(bestand,0,SEEK_SET);
	fseek(bestand,offset,SEEK_SET);
	fread(buffer,1,ASSUMED_SECTOR_SIZE,bestand);
}

void openImage(char* path){
	if(bestand!=NULL){
		printf("[ERRO] Trying to reopening file!\n");
		exit(EXIT_FAILURE);
	}
	if(path==NULL){
		printf("[ERRO] Trying to open null path!\n");
		exit(EXIT_FAILURE);
	}

	bestand = fopen(path,"r");
	if(bestand==NULL){
		printf("[ERRO] Unable to open file!\n");
		exit(EXIT_FAILURE);
	}

	fseek(bestand,0,SEEK_END);
	maxsector = ftell(bestand)/ASSUMED_SECTOR_SIZE;
	rewind(bestand);
	printf("[INFO] There are %i sectors on this disk\n",maxsector);

	unsigned char *devicediscoverybuffer = malloc(ASSUMED_SECTOR_SIZE);
	readSector(0,devicediscoverybuffer);

	unsigned char allocA = devicediscoverybuffer[0x1BE + 0x04] & 0xFF;
	unsigned char allocB = devicediscoverybuffer[0x1CE + 0x04] & 0xFF;
	unsigned char allocC = devicediscoverybuffer[0x1DE + 0x04] & 0xFF;
	unsigned char allocD = devicediscoverybuffer[0x1EE + 0x04] & 0xFF;
	if(allocA==0xCD||allocB==0xCD||allocC==0xCD||allocD==0xCD){
		printf("[INFO] MBR pressent in this image\n");
		if(allocA==0xCD){
			printf("[INFO] Sanderslando File System pressent in MBR-entry1\n");
			unsigned long val = ((unsigned long*)(devicediscoverybuffer + 0x1BE + 0x8))[0];
			sectoroffset = val;
		}
		if(allocB==0xCD){
			printf("[INFO] Sanderslando File System pressent in MBR-entry2\n");
			unsigned long val = ((unsigned long*)(devicediscoverybuffer + 0x1CE + 0x8))[0];
			sectoroffset = val;
		}
		if(allocC==0xCD){
			printf("[INFO] Sanderslando File System pressent in MBR-entry3\n");
			unsigned long val = ((unsigned long*)(devicediscoverybuffer + 0x1DE + 0x8))[0];
			sectoroffset = val;
		}
		if(allocD==0xCD){
			printf("[INFO] Sanderslando File System pressent in MBR-entry4\n");
			unsigned long val = ((unsigned long*)(devicediscoverybuffer + 0x1EE + 0x8))[0];
			sectoroffset = val;
		}
		printf("[INFO] Entering Sanderslando File System partition at offset %i \n",sectoroffset);
		readSector(0,devicediscoverybuffer);
	}
	bootsector = (SFSBootsector*)devicediscoverybuffer;
	if(bootsector->signature==0xCD&&bootsector->version==1&&bootsector->sectorsize==ASSUMED_SECTOR_SIZE){
		return;
	}
	printf("[ERRO] No signatures found!\n");
	exit(EXIT_FAILURE);
}

unsigned char *map;
SFSDirectory *dir;

void traverse(char* path){
	int dirtableloc = bootsector->offset_first_sectortable + bootsector->sectortablesize;
	int maptableloc = bootsector->offset_first_sectortable;
	map = (unsigned char*)malloc(ASSUMED_SECTOR_SIZE);
	dir = (SFSDirectory *)malloc(ASSUMED_SECTOR_SIZE);
	readSector(dirtableloc,(unsigned char*)dir);
	readSector(maptableloc,(unsigned char*)map);
}

int main(int argc,char** argv){
	printf("[INFO] Sanderslando File System Module Reader \n");
	printf("[INFO] There are %i arguments\n",argc);
	if(argc>1){
		printf("[INFO] There is a minimum of 2 arguments\n");
		char* command = argv[1];
		if(strcmp("help",command)==0){
			printf("[HELP] USAGE:\n");
			printf("[HELP] %s [COMMAND] [IMAGE] [ARGUMENT]\n",argv[0]);
			printf("[HELP] \n");
			printf("[HELP] Commands:\n");
			printf("[HELP] - help:  This command\n");
			printf("[HELP] - dir :  Show files in directory of argument1\n");
			printf("[HELP] - add :  Add file argument2 to location argument1 \n");
			printf("[HELP] - load:  Download file argument1 to location argument2\n");
		}else if(strcmp("dir",command)==0&&argc==4){
			openImage(argv[2]);
			traverse(argv[3]);
			for(int i = 0 ; i < SFS_FILE_TABLE_ENTRIES ; i++){
				SFSFileEntry ent = dir->entries[i];
				unsigned char* filename = ent.filename;
				if(filename[0]==0x00){
					continue;
				}
				printf("[FILE] ");
				for(int t = 0 ; t < SFS_MAX_FILE_NAME ; t++){
					printf("%c",filename[t]);
				}
				printf(" \n");
			}
		}else if(strcmp("add",command)==0&&argc==4){
		}else if(strcmp("load",command)==0&&argc==4){
		}else{
			printf("[ERRO] Invalid command \"%s\" use \"%s help\" for more information\n",command,argv[0]);
			exit(EXIT_FAILURE);
		}
		printf("[INFO] That is all\n");
		exit(EXIT_SUCCESS);
	}else{
		printf("[ERRO] Not enough arguments. Use \"%s help\" for more information\n",argv[0]);
		exit(EXIT_FAILURE);
	}
}