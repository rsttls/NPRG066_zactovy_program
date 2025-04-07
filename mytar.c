typedef struct posix_header
{                      /* byte offset */
   char name[100];     /*   0 */
   char mode[8];       /* 100 */
   char uid[8];        /* 108 */
   char gid[8];        /* 116 */
   char size[12];      /* 124 */
   char mtime[12];     /* 136 */
   char chksum[8];     /* 148 */
   char typeflag;      /* 156 */
   char linkname[100]; /* 157 */
   char magic[6];      /* 257 */
   char version[2];    /* 263 */
   char uname[32];     /* 265 */
   char gname[32];     /* 297 */
   char devmajor[8];   /* 329 */
   char devminor[8];   /* 337 */
   char prefix[155];   /* 345 */
                       /* 500 */
   char junk[12];      /*doplneni na 512*/
} posix_header;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCKSIZE 512
typedef unsigned long long ull;

// global variables
char *archivePath = NULL;
FILE *archive = NULL;
long archiveSize;
ull argc;
char **argv;
int hasFilesToFind = 0;
char **filesToFind;
int foundFirstZeroBlock = 0;
int ret = 0;


// reports files sizes
ull getTarFileSize(posix_header header)
{
   if ((header.size[0] & 0x80) == 0)
      return strtoull(header.size, NULL, 8);
   else
   {
      ull size = 0;
      for (int i = 1; i < 12; i++)
      {
         size = (size << 8) | (unsigned char)header.size[i];
      }
      return size;
   }
}


// checks if file is in listed and if non are listed than always true
int isInListed(char *str)
{
   if (str == NULL)
      return 2;
   if (hasFilesToFind == 0)
      return 1;
   for (size_t i = 0; i < argc; i++)
   {
      if (filesToFind[i] != NULL)
         if (strcmp(filesToFind[i], str) == 0)
         {
            filesToFind[i] = NULL;
            return 1;
         }
   }
   return 0;
}

void list()
{
   posix_header header;
   while (fread(&header, BLOCKSIZE, 1, archive) == 1)
   {

      if (header.name[0] == 0)
      {
         foundFirstZeroBlock = 1;
         break;
      }
      // checks for specification
      if (memcmp(header.magic, "ustar", 5) != 0)
      {
         fprintf(stderr, "mytar: This does not look like a tar archive\nmytar: Exiting with failure status due to previous errors");
         exit(2);
      }
      // check type
      if(header.typeflag != 0 && header.typeflag != '0')
      {
         fprintf(stderr, "mytar: Unsupported header type: %d\n", header.typeflag);
         exit(2);
      }
      // listing
      if (isInListed(header.name))
         printf("%s\n", header.name);
      ull fileSize = getTarFileSize(header);
      ull blocksToSkip = fileSize / BLOCKSIZE;
      if (fileSize % BLOCKSIZE)
         blocksToSkip++;
      fseek(archive, blocksToSkip * BLOCKSIZE, SEEK_CUR);
      if (ftell(archive) > archiveSize)
      {
         fprintf(stderr, "mytar: Unexpected EOF in archive\nmytar: Error is not recoverable: exiting now\n");
         exit(2);
      }
   }
}

void extract(int verbose)
{
   posix_header header;
   while (fread(&header, BLOCKSIZE, 1, archive) == 1)
   {
      if (header.name[0] == 0)
      {
         foundFirstZeroBlock = 1;
         break;
      }
      // checks for specification
      if (memcmp(header.magic, "ustar", 5) != 0)
      {
         fprintf(stderr, "mytar: This does not look like a tar archive\nmytar: Exiting with failure status due to previous errors");
         exit(2);
      }
      // check type
      if(header.typeflag != 0 && header.typeflag != '0')
      {
         fprintf(stderr, "IDK some errrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr\n");
         exit(2);
      }
      ull fileSize = getTarFileSize(header);

      if (isInListed(header.name))
      {
         // extract
         if (verbose == 1)
            printf("%s\n", header.name);
         ull blocksToRead = fileSize / BLOCKSIZE;
         FILE *f = fopen(header.name, "wb");
         if (f == NULL)
            return;

         char dt[BLOCKSIZE];
         for (ull i = 0; i < blocksToRead; i++)
         {
            if (fread(&dt, BLOCKSIZE, 1, archive) != 1)
            {
               fprintf(stderr, "mytar: Unexpected EOF in archive\nmytar: Error is not recoverable: exiting now\n");
               exit(2);
            }
            fwrite(&dt, BLOCKSIZE, 1, f);
         }
         if (fileSize % BLOCKSIZE)
         {
            if (fread(&dt, BLOCKSIZE, 1, archive) != 1)
            {
               fprintf(stderr, "mytar: Unexpected EOF in archive\nmytar: Error is not recoverable: exiting now\n");
               exit(2);
            }
            fwrite(&dt, fileSize % BLOCKSIZE, 1, f);
         }
         fclose(f);
      }
      else
      {
         // skip
         ull blocksToSkip = fileSize / BLOCKSIZE;
         if (fileSize % BLOCKSIZE)
            blocksToSkip++;
         fseek(archive, blocksToSkip * BLOCKSIZE, SEEK_CUR);
         if (ftell(archive) > archiveSize)
         {
            fprintf(stderr, "mytar: Unexpected EOF in archive\nmytar: Error is not recoverable: exiting now\n");
            exit(2);
         }
      }
   }
}

int main(int a, char **b)
{
   int f = 0, v = 0, x = 0, t = 0;
   argv = b;
   argc = a;
   filesToFind = malloc(sizeof(char *) * argc);
   // parsing input
   for (ull i = 1; i < argc; i++)
   {
      if (argv[i][0] == '-')
      {
         int j = 1;
         while (argv[i][j] != 0)
         {
            switch (argv[i][j])
            {
            case 'f':
               f = 1;
               break;
            case 'v':
               v = 1;
               break;
            case 'x':
               x = 1;
               break;
            case 't':
               t = 1;
               break;
            default:
               fprintf(stderr, "Unknown option -%c\n", argv[i][j]);
               break;
            }
            j++;
         }
      }
      else
      {
         if (archivePath == NULL)
         {
            archivePath = argv[i];
         }
         else
         {
            static int k = 0;
            hasFilesToFind = 1;
            filesToFind[k] = argv[i];
            k++;
         }
      }
   }
   if (f == 1)
   {

      if (archivePath == NULL)
      {
         fprintf(stderr, "No archive specified\n");
         return 2;
      }
      archive = fopen(archivePath, "rb");
      if (archive == NULL)
      {
         fprintf(stderr, "Archive does not exist\n");
         return 2;
      }
      fseek(archive,0,SEEK_END);
      archiveSize = ftell(archive);
      fseek(archive,0,SEEK_SET);
   }
   if (x)
      if (v)
         extract(1);
      else
         extract(0);
   else if (t)
      list();
   else
      exit(2);

   // single zero block
   posix_header h;
   if (fread(&h, BLOCKSIZE, 1, archive) == 0 && foundFirstZeroBlock)
      fprintf(stderr, "mytar: A lone zero block at %ld\n", ftell(archive) / 512);

   // missing files
   for (size_t i = 0; i < argc; i++)
   {
      if (filesToFind[i] != NULL)
      {
         fprintf(stderr, "mytar: %s: Not found in archive\n", filesToFind[i]);
         ret = 2;
      }
   }

   free(filesToFind);
   if(ret != 0)
      fprintf(stderr,"mytar: Exiting with failure status due to previous errors\n");
   return ret;
}
