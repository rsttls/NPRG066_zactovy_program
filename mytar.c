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

#define BLOCKSIZE 512
typedef unsigned long long ull;

// global variables
char *archivePath = NULL;
FILE *archive = NULL;
char **arg;

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


void list()
{
   posix_header header;
   while (fread(&header, BLOCKSIZE, 1, archive) == 1)
   {
      if (header.name[0] == 0)
         break;
      printf("%s\n", header.name);
      ull fileSize = getTarFileSize(header);
      ull blocksToSkip = fileSize / BLOCKSIZE;
      if (fileSize % BLOCKSIZE)
         blocksToSkip++;
      if (fseek(archive, blocksToSkip * BLOCKSIZE, SEEK_CUR) != 0)
      {
         printf("fseek fail: maybe unexpected EOF\n");
         return;
      }
   }
}

void extract(int verbose)
{
   posix_header header;
   while (fread(&header, BLOCKSIZE, 1, archive) == 1)
   {
      if (header.name[0] == 0)
         break;
      if (verbose == 1)
         printf("%s\n", header.name);
      long long fileSize = getTarFileSize(header);
      long long blocksToRead = fileSize / BLOCKSIZE;

      FILE *f = fopen(header.name, "wb");
      if (f == NULL)
         return;

      char dt[BLOCKSIZE];
      for (long long i = 0; i < blocksToRead; i++)
      {
         fread(&dt, BLOCKSIZE, 1, archive);
         fwrite(&dt, BLOCKSIZE, 1, f);
      }
      if (fileSize % BLOCKSIZE)
      {
         fread(&dt, BLOCKSIZE, 1, archive);
         fwrite(&dt, fileSize % BLOCKSIZE, 1, f);
      }
      fclose(f);
   }
}

int main(int argc, char **argv)
{
   int f = 0, v = 0, x = 0, t = 0;
   arg = argv;
   // parsing input
   for (int i = 1; i < argc; i++)
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
               printf("Unknown option -%c", argv[i][j]);
               break;
            }
            j++;
         }
      }
      else
      {
         if (archivePath == 0)
         {
            archivePath = argv[i];
         }
         else
         {
            // add to leftovers maybe a vector
         }
      }
   }
   if (f == 1)
   {

      if (archivePath == NULL)
      {
         printf("No archive specified\n");
         return 0;
      }
      archive = fopen(archivePath, "rb");
      if (archive == NULL)
      {
         printf("Archive does not exist\n");
         return 0;
      }
   }
   if (x)
      if (v)
         extract(1);
      else
         extract(0);
   else if (t)
      list();
   else
      printf("No operation\n");
   return 0;
}