#define BLOCK_SIZE 1024     // ÿ���̿�Ĵ�С
#define SIZE 1024000        // ������̵Ĵ�С
#define END 65535       // FAT�е��ļ�������־
#define FREE 0      // FAT���̿���б�־
#define ROOTBLOCKNUM 2      // ��Ŀ¼ռ�õ��̿���
#define MAXOPENFILE 10      // ���ͬʱ���ļ���
#define TRUE 1
#define FALSE 0
#define MAXFCB 32       // ÿ��Ŀ¼������ļ���

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct FCB{
    char filename[8];
    char exname[3];
    unsigned char attribute;        // �����ֶ� 0Ŀ¼ 1����
    char free;      // Ŀ¼���Ƿ�Ϊ�� 0�� 1�ѷ���
    unsigned short time;
    unsigned short date;
    unsigned short first;       // �ļ���ʼ�̿��
    unsigned long length;       // �ļ����ȣ��ֽ�����
}fcb;

typedef struct FAT{
    unsigned short id;      // FAT�����ֵ
}fat;

typedef struct USEROPEN{
    char filename[8];
    char exname[3];
    unsigned char attribute;        // �����ֶ� 0Ŀ¼ 1����
    unsigned short time;
    unsigned short date;
    unsigned short first;       // �ļ���ʼ�̿��
    unsigned long length;       // �ļ����ȣ��ֽ�����
    char dir[80];       // ���ļ����ڵ�Ŀ¼���Ա���ټ���ļ��Ƿ��Ѿ���
    int count;      // ��дָ�����ļ��е�λ��
    char fcbstate;      // �Ƿ��޸����ļ���FCB���� 0δ�޸� 1�޸�
    char topenfile;     // ��ʾ���û����ļ������Ƿ�Ϊ�� 0�� 1�ѱ�ռ��
}useropen;

typedef struct BLOCK0{
    char information[200];
    unsigned short root;        // ��Ŀ¼����ʼ�̿��
    unsigned char *startblock;      // �����������������ʼλ��
}block0;

unsigned char *myvhard;     // ָ��������̵���ʼλ��
useropen openfilelist[MAXOPENFILE];   // �û����ļ�������
int curdir;     // ��¼��ǰĿ¼���ļ�������
char currentdir[80];    // ��¼��ǰĿ¼��·��
unsigned char *startp;  // ָ������������ʼλ��
fat *fat1;
fat *fat2;

// utils.c
int fill_useropen(int index, fcb *fcbptr, char *dir, int count, char fcbstate, char topenfile);
int fill_fcb(fcb *fcbptr, char *filename, char *exname, unsigned char attribute, unsigned short time, unsigned short date, unsigned short first, unsigned long length);
int allocate_fat(int num);

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
void my_write(int fd);
void my_read(int fd, int len);
void my_exitsys();