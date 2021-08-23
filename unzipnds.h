#ifndef UNZIPNDS_HEADER
#define UNZIPNDS_HEADER

#ifdef __cplusplus
extern "C" {
#endif

/* PKZIP header definitions */
#define ZIPMAG 0x4b50			/* Two-byte zip lead-in ("PK")*/
#define CENREM 0x0201			/* Remaining two bytes in zip signature */
#define CENSIG 0x02014b50		/* Full central signature */
#define LOCREM 0x0403			/* Remaining two bytes in zip signature */
#define LOCSIG 0x04034b50		/* Full local signature */
#define ENDSIG 0x06054b50		/* Full end signature */
#define ENDHEADLEN 22			/* End header length */

#define  CRPFLG 1				/*  bit for encrypted entry */
#define  EXTFLG 8				/*  bit for extended local header */

typedef struct pkzipCentralFile { /* CENTRAL */
    u32 magic;					/* "P","K",0x01,0x02 */
    u16 versionMadeBy;
    u16 versionNeededToExtract;
    u16 generalPurposeBitFlag;
    u16 compressionMethod;
    u32 lastModDosDatetime;
    u32 crc32;
    u32 cSize;                  /* Compressed size */
    u32 ucSize;                 /* Uncompressed size */
    u16 fileNameLength;
    u16 extraFieldLength;
    u16 fileCommentLength;
    u16 diskNumberStart;
    u16 internalFileAttributes;
    u32 externalFileAttributes;
    u32 relativeOffsetLocalHeader;
} CentralFileHdr;

typedef struct pkzipLocalFile { /* LOCAL */
    u32 magic;					/* "P","K",0x03,0x04 */
    u16 versionNeededToExtract;
    u16 generalPurposeBitFlag;
    u16 compressionMethod;
    u32 lastModDosDatetime;
    u32 crc32;
    u32 cSize;
    u32 ucSize;
    u16 fileNameLength;
    u16 extraFieldLength;
} LocalFileHdr;

typedef struct pkzipEndofCentralDir { /* CENTRAL END */
    u32 magic;					/* "P","K",0x05,0x06 */
    u16 diskNumber;
    u16 startDisk;
    u16 entryCount;
    u16 totalEntryCount;
    u32 cdirSize;
    u32 cdirOffset;
    u16 commentLength;
} EndOfCentralHdr;

/**
 * Loads specified file in zip to destination (uncompressed).
 * @param  *dst: Destination where to put uncompressed data.
 * @param  *zipName: Name of zip file.
 * @param  *fileName: Name of file in zip.
 * @param  maxSize: The maximum size of destination.
 * @return Error code, 0 if no error.
 */
int loadFileInZip(void *dst, const char *zipName, const char *fileName, const int maxSize);

/**
 * Loads a file with the specified CRC32 in zip to destination (uncompressed).
 * This is mostly usefull for arcade games.
 * @param  *dst: Destination where to put uncompressed data.
 * @param  *zipName: Name of zip file.
 * @param  *CRC32: CRC32 of file in zip.
 * @param  maxSize: The maximum size of destination.
 * @return Error code, 0 if no error.
 */
int loadFileWithCRC32InZip(void *dst, const char *zipName, const u32 crc32, const int maxSize);

/**
 * Loads the first file in zip of specified file type to the destination (uncompressed).
 * @param  *dst: Destination where to put uncompressed data.
 * @param  *zipName: Name of zip file.
 * @param  *fileTypes: All file types that are valid (".txt.bin.rom", etc).
 * @param  maxSize: The maximum size of destination.
 * @return Error code, 0 if no error.
 */
int loadFileTypeInZip(void *dst, const char *zipName, const char *fileTypes, const int maxSize);

/**
 * Checks if a file name exists in the zip file.
 * @param  *zipName: Name of zip file.
 * @param  *fileName: File name to find in zip.
 * @return Error code, 0 if file name found.
 */
int findFileInZip(const char *zipName, const char *fileName);

/**
 * Checks if a file with the specified CRC32 exists in the zip file.
 * This is mostly usefull for arcade games.
 * @param  *zipName: Name of zip file.
 * @param  *crc32: File CRC32 to find in zip.
 * @return Error code, 0 if file name found.
 */
int findFileWithCRC32InZip(const char *zipName, const u32 crc32);

/**
 * Goes through all the files in the zip and puts their name in the tbl.
 * @param  *zipName: Name of zip file.
 * @param  *tbl: Where to put all names.
 * @return Error code, 0 if no error.
 */
int listFilesInZip(const char *zipName, char *tbl);

extern char zipError[];
extern char zipFilename[];
extern CentralFileHdr cenHead;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UNZIPNDS_HEADER
