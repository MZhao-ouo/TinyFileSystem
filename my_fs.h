#define BLOCK_SIZE 1024     // 每个盘块的大小
#define SIZE 1024000        // 虚拟磁盘的大小
#define END 65535       // FAT中的文件结束标志
#define FREE 0      // FAT中盘块空闲标志
#define ROOTBLOCKNUM 2      // 根目录占用的盘块数
#define MAXOPENFILE 10      // 最多同时打开文件数
#define TRUE 1
#define FALSE 0
#define MAXFCB 32       // 每个目录下最多文件数
#define MAX_TEXT_SIZE 10000

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct FCB{
    char filename[8];
    unsigned char attribute;        // 属性字段 0目录 1数据
    char free;      // 目录项是否为空 0空 1已分配
    unsigned short create_stamp;
    unsigned short modify_stamp;
    unsigned short first;       // 文件起始盘块号
    unsigned long length;       // 文件长度（字节数）
}fcb;

typedef struct FAT{
    unsigned short id;      // FAT表项的值
}fat;

typedef struct USEROPEN{
    char filename[8];
    unsigned char attribute;        // 属性字段 0目录 1数据
    unsigned short create_stamp;
    unsigned short modify_stamp;
    unsigned short first;       // 文件起始盘块号
    unsigned long length;       // 文件长度（字节数）
    char dir[80];       // 打开文件所在的目录，以便快速检查文件是否已经打开
    int count;      // 读写指针在文件中的位置
    char fcbstate;      // 是否修改了文件的FCB内容 0未修改 1修改
    char topenfile;     // 表示该用户打开文件表项是否为空 0空 1已被占用
}useropen;

typedef struct BLOCK0{
    char information[200];
    unsigned short root;        // 根目录的起始盘块号
    unsigned char *startblock;      // 虚拟磁盘上数据区开始位置
}block0;

unsigned char *myvhard;     // 指向虚拟磁盘的起始位置
useropen openfilelist[MAXOPENFILE];   // 用户打开文件表数组
int curdir;     // 记录当前目录的文件描述符
char currentdir[80];    // 记录当前目录的路径
unsigned char *startp;  // 指向数据区的起始位置

// utils.c
int fill_useropen(int index, fcb *fcbptr, char *dir, int count, char fcbstate, char topenfile);
int fill_fcb(fcb *fcbptr, char *filename, unsigned char attribute, unsigned short create_stamp, unsigned short modify_stamp, unsigned short first, unsigned long length);
int allocate_fat(int num);
int free_fat(int index);
int find_fd();
int get_dir_block(char *full_dir);

// my_func.c
void my_format();
void my_cd(char *dirname);
void my_mkdir(char *dirname);
void my_rmdir(char *dirname);
void my_ls();
void my_create(char *filename);
void my_rm(char *filename);
void my_open(char *filename);
void my_close(int fd);
int my_write(int fd);
int do_write(int fd, char* text, int len, char wstyle);
int my_read(int fd, int len);
int do_read(int fd, int len, char* text);
void my_exitsys();
