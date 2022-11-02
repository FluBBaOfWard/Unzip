#include <nds.h>

#include <stdio.h>

#include "unzipnds.h"
#include "puff.h"

extern void drawText(const char *str, int row, int hilite);

/** Holds error message. */
char zipError[40];
/** Name of the currently "selected" file in the zip. */
char zipFilename[256];
CentralFileHdr cenHead;

/**
 * Tries to find the CentralFileHdr in the zip file, the FILE will have its position set to point to the CentralFileHdr if found.
 *
 * @param  *zipFile: The zip file to search.
 * @return Error code, 0 if no error.
 */
static int seekCentralZipHeader(FILE *zipFile) {
	EndOfCentralHdr hdr;

	hdr.magic = 0;
	fseek(zipFile, - ENDHEADLEN, SEEK_END);
	fread(&hdr, 1, ENDHEADLEN, zipFile);
	if (hdr.magic != ENDSIG) {
		fseek(zipFile, - ENDHEADLEN*2, SEEK_END);
		fread(&hdr, 1, ENDHEADLEN, zipFile);
		if (hdr.magic != ENDSIG) {
			return -1;
		}
	}
	fseek(zipFile, hdr.cdirOffset, SEEK_SET);
	return 0;
}

/**
 * Fills in the supplied CentralFileHdr with information from the zip file. The zip file must be at the position for the CentralFileHdr.
 *
 * @param  *hdr: The CentralFileHdr to fill.
 * @param  *zipFile: The zip FILE.
 * @return Error code, 0 if no error.
 */
static int fillCentralZipHeader(CentralFileHdr *hdr, FILE *zipFile) {
	hdr->magic = 0;
	fread(&hdr->magic, 1, sizeof(u32), zipFile);
	fread(&hdr->versionMadeBy, 1, sizeof(u16), zipFile);
	fread(&hdr->versionNeededToExtract, 1, sizeof(u16), zipFile);
	fread(&hdr->generalPurposeBitFlag, 1, sizeof(u16), zipFile);
	fread(&hdr->compressionMethod, 1, sizeof(u16), zipFile);
	fread(&hdr->lastModDosDatetime, 1, sizeof(u32), zipFile);
	fread(&hdr->crc32, 1, sizeof(u32), zipFile);
	fread(&hdr->cSize, 1, sizeof(u32), zipFile);
	fread(&hdr->ucSize, 1, sizeof(u32), zipFile);
	fread(&hdr->fileNameLength, 1, sizeof(u16), zipFile);
	fread(&hdr->extraFieldLength, 1, sizeof(u16), zipFile);
	fread(&hdr->fileCommentLength, 1, sizeof(u16), zipFile);
	fread(&hdr->diskNumberStart, 1, sizeof(u16), zipFile);
	fread(&hdr->internalFileAttributes, 1, sizeof(u16), zipFile);
	fread(&hdr->externalFileAttributes, 1, sizeof(u32), zipFile);
	fread(&hdr->relativeOffsetLocalHeader, 1, sizeof(u32), zipFile);
	if (hdr->magic == CENSIG) {
		return 0;
	} else {
		return -1;
	}
}

/**
 * Fills in the supplied LocalFileHdr with information from the zip file. The zip file must be at the position for the LocalFileHdr.
 *
 * @param  *hdr: The LocalFileHdr to fill.
 * @param  *zipFile: The zip FILE.
 * @return Error code, 0 if no error.
 */
static int fillLocalZipHeader(LocalFileHdr *hdr, FILE *zipFile) {
	hdr->magic = 0;
	fread(&hdr->magic, 1, sizeof(u32), zipFile);
	fread(&hdr->versionNeededToExtract, 1, sizeof(u16), zipFile);
	fread(&hdr->generalPurposeBitFlag, 1, sizeof(u16), zipFile);
	fread(&hdr->compressionMethod, 1, sizeof(u16), zipFile);
	fread(&hdr->lastModDosDatetime, 1, sizeof(u32), zipFile);
	fread(&hdr->crc32, 1, sizeof(u32), zipFile);
	fread(&hdr->cSize, 1, sizeof(u32), zipFile);
	fread(&hdr->ucSize, 1, sizeof(u32), zipFile);
	fread(&hdr->fileNameLength, 1, sizeof(u16), zipFile);
	fread(&hdr->extraFieldLength, 1, sizeof(u16), zipFile);
	if (hdr->magic == LOCSIG) {
		return 0;
	}
	else {
		return -1;
	}
}

/**
 * Returns CentralFileHdr of file in zip FILE if found, otherwise NULL.
 *
 * @param  *fileName: The name of the file to locate in the zip FILE.
 * @param  *zipFile: The zip FILE to search.
 * @return A CentralFileHdr for the specified fileName or NULL if not found.
 */
static const CentralFileHdr *locateFileInZip(const char *fileName, FILE *zipFile) {
	if (seekCentralZipHeader(zipFile) == 0) {
		while (fillCentralZipHeader(&cenHead, zipFile) == 0) {
			fread(zipFilename, 1, cenHead.fileNameLength, zipFile);
			zipFilename[cenHead.fileNameLength] = 0;
			if (strstr(zipFilename, fileName)) {
				return &cenHead;
			}
			fseek(zipFile, cenHead.extraFieldLength + cenHead.fileCommentLength, SEEK_CUR);
		}
	}

	return NULL;
}

/**
 * Returns CentralFileHdr of file with the specified CRC32 in zip FILE if found, otherwise NULL.
 *
 * @param  crc32: The CRC32 of the file to locate in the zip FILE.
 * @param  *zipFile: The zip FILE to search.
 * @return A CentralFileHdr for the specified fileName or NULL if not found.
 */
static const CentralFileHdr *locateFileWithCRC32InZip(const u32 crc32, FILE *zipFile) {
	if (seekCentralZipHeader(zipFile) == 0) {
		while (fillCentralZipHeader(&cenHead, zipFile) == 0) {
			fread(zipFilename, 1, cenHead.fileNameLength, zipFile);
			zipFilename[cenHead.fileNameLength] = 0;
			if (cenHead.crc32 == crc32) {
				return &cenHead;
			}
			fseek(zipFile, cenHead.extraFieldLength + cenHead.fileCommentLength, SEEK_CUR);
		}
	}

	return NULL;
}

/**
 * Returns CentralFileHdr of first file of specified filetype in zip FILE, otherwise NULL.
 *
 * @param  *fileTypes: All file types that are valid (".txt.bin.rom", etc).
 * @param  *zipFile: The zip file to search.
 * @return A CentralFileHdr of a file of the specified fileType or NULL if not found.
 */
static const CentralFileHdr *locateFileTypeInZip(const char *fileTypes, FILE *zipFile) {
	char fileExtension[8];
	const char *ext;
	int i;
	if (seekCentralZipHeader(zipFile) == 0) {
		while (fillCentralZipHeader(&cenHead, zipFile) == 0) {
			fread(zipFilename, 1, cenHead.fileNameLength, zipFile);
			zipFilename[cenHead.fileNameLength] = 0;
			ext = strrchr(zipFilename, '.');
			if (ext != NULL) {
				strlcpy(fileExtension, ext, 8);
				// Make file extension lower case
				for (i = 1; i < 6; i++) {
					if (fileExtension[i]) {
						fileExtension[i] |= 0x20;
					}
				}
				if (strstr(fileTypes, fileExtension)) {
					return &cenHead;
				}
			}
			fseek(zipFile, cenHead.extraFieldLength + cenHead.fileCommentLength, SEEK_CUR);
		}
	}

	return NULL;
}

/**
 * Loads and decompresses a file in a zip FILE to the destination.
 *
 * @param  *dest: Destination where to put the resulting data.
 * @param  *cfHdr: A central file header for the file.
 * @param  *zipFile: The zipFile.
 * @param  maxSize: The maximum size of the destination.
 * @return Error code, 0 if no error.
 */
static int loadAndDecompressZip(void *dest, const CentralFileHdr *cfHdr, FILE *zipFile, const int maxSize) {
	int err = 0;
	unsigned char *src;
	LocalFileHdr zipHead;

	fseek(zipFile, cfHdr->relativeOffsetLocalHeader, SEEK_SET);
	if (fillLocalZipHeader(&zipHead, zipFile) == 0) {
		int cSize = cfHdr->cSize;					// Localhead doesn't allways contain all info.
		int ucSize = cfHdr->ucSize;					// Centralhead should allways be available.
		if (ucSize > maxSize) {
			strlcpy(zipError, "File too large!", sizeof(zipError));
			err = 1;
			fclose(zipFile);
			return err;
		}
		if (zipHead.compressionMethod == DEFLATE) {		// DEFLATE compression = 8.
			if ((src = malloc(cSize))) {
				fseek(zipFile, zipHead.extraFieldLength + zipHead.fileNameLength, SEEK_CUR);
				fread(src, 1, cSize, zipFile);
				drawText("   Please wait, decompressing.", 11, 0);
				puff(dest, (unsigned long *)&ucSize, src, (unsigned long *)&cSize);
				free(src);
				drawText("                              ", 11, 0);
			}
			else if ((cSize+0x10000) < maxSize){
				src = dest+(maxSize-cSize);
				fseek(zipFile, zipHead.extraFieldLength + zipHead.fileNameLength, SEEK_CUR);
				fread(src, 1, cSize, zipFile);
				drawText("   Please wait, decompressing.", 11, 0);
				puff(dest, (unsigned long *)&ucSize, src, (unsigned long *)&cSize);
				drawText("                              ", 11, 0);
			}
			else {
				strlcpy(zipError, "Out of memory!", sizeof(zipError));
				err = 1;
			}
		}
		else if (zipHead.compressionMethod == STORE) {		// STORE, no compression = 0.
			fseek(zipFile, zipHead.extraFieldLength + zipHead.fileNameLength, SEEK_CUR);
			fread(dest, 1, zipHead.ucSize, zipFile);
		}
		else {									// Only supports DEFLATE & STORE.
			strlcpy(zipError, "Unsupported compression in zip.", sizeof(zipError));
			err = 1;
		}
	}
	else {
		strlcpy(zipError, "Couldn't find file in zip.", sizeof(zipError));
		err = 1;
	}
	return err;
}

int findFileInZip(const char *zipName, const char *fileName) {
	int err = 1;
	FILE *zipFile;

	if ((zipFile = fopen(zipName, "r"))) {
		if (locateFileInZip(fileName, zipFile) != NULL) {
			err = 0;		// File found in zip
		}
		fclose(zipFile);
	}
	return err;
}

int findFileWithCRC32InZip(const char *zipName, const u32 crc32) {
	int err = 1;
	FILE *zipFile;

	if ((zipFile = fopen(zipName, "r"))) {
		if (locateFileWithCRC32InZip(crc32, zipFile) != NULL) {
			err = 0;		// File found in zip
		}
		fclose(zipFile);
	}
	return err;
}

int loadFileInZip(void *dest, const char *zipName, const char *fileName, const int maxSize) {
	int err = 0;
	FILE *zipFile;

	if ((zipFile = fopen(zipName, "r"))) {
		const CentralFileHdr *cfHdr = locateFileInZip(fileName, zipFile);
		if ( cfHdr != NULL ) {
			err = loadAndDecompressZip(dest, cfHdr, zipFile, maxSize);
		}
		else {
			strlcpy(zipError, "Couldn't find file in zip.", sizeof(zipError));
			err = 1;
		}
		fclose(zipFile);
	}
	else {
		strlcpy(zipError, "Couldn't open zip file.", sizeof(zipError));
		err = 1;
	}
	return err;
}

int loadFileWithCRC32InZip(void *dest, const char *zipName, const u32 crc32, const int maxSize) {
	int err = 0;
	FILE *zipFile;

	if ((zipFile = fopen(zipName, "r"))) {
		const CentralFileHdr *cfHdr = locateFileWithCRC32InZip(crc32, zipFile);
		if (cfHdr != NULL) {
			err = loadAndDecompressZip(dest, cfHdr, zipFile, maxSize);
		}
		else {
			strlcpy(zipError, "Couldn't find file in zip.", sizeof(zipError));
			err = 1;
		}
		fclose(zipFile);
	}
	else {
		strlcpy(zipError, "Couldn't open zip file.", sizeof(zipError));
		err = 1;
	}
	return err;
}

int loadFileTypeInZip(void *dest, const char *zipName, const char *fileTypes, const int maxSize) {
	int err = 0;
	FILE *zipFile;

	if ((zipFile = fopen(zipName, "r"))) {
		const CentralFileHdr *cfHdr = locateFileTypeInZip(fileTypes, zipFile);
		if (cfHdr != NULL) {
			err = loadAndDecompressZip(dest, cfHdr, zipFile, maxSize);
		}
		else {
			strlcpy(zipError, "Couldn't find file in zip.", sizeof(zipError));
			err = 1;
		}
		fclose(zipFile);
	}
	else {
		strlcpy(zipError, "Couldn't open zip file.", sizeof(zipError));
		err = 1;
	}
	return err;
}

int listFilesInZip(const char *zipName, char *table) {
	int len;
	FILE *zipFile;
	char txtBuf[256];
	LocalFileHdr zipHead;

	if ((zipFile = fopen(zipName, "r"))) {
		while (fillLocalZipHeader(&zipHead, zipFile) == 0) {
			fread(txtBuf, 1, zipHead.fileNameLength, zipFile);
			txtBuf[zipHead.fileNameLength] = 0;
			strcat(table, txtBuf);
			len = zipHead.extraFieldLength + zipHead.cSize;
			fseek(zipFile, len, SEEK_CUR);
		}
		fclose(zipFile);
	}
	else {
		return 1;
	}
	return 0;
}
