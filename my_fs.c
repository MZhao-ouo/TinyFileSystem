#include "my_fs.h"

/* 初始化所建立的文件系统 */
void startsys() {
    myvhard = (unsigned char *)malloc(SIZE);    // 申请虚拟磁盘空间
    if (myvhard == NULL) {
        printf("Error: cannot allocate memory space!\n");
        exit(0);
    }
    FILE *fp = fopen("./my_disk", "rb");   // 读入磁盘上的文件系统内容到虚拟磁盘
    if (!fp) {
        printf("Can't find \"./my_disk\". Initing ...\n");
        my_format();        // 如果还没有创建文件系统，调用my_format创建
    } else {
        printf("Loading from \"./my_disk\" ... \n");
        fread(myvhard, SIZE, 1, fp);
        fclose(fp);

        block0 *b0 = (block0 *)myvhard;
        b0->startblock = myvhard + BLOCK_SIZE * 5;
        startp = b0->startblock;  // 指向数据区的起始位置，前面有一个引导块，两个FAT1，两个FAT2
    }
    memset(openfilelist, 0, sizeof(openfilelist));   // 初始化用户打开文件表
    fill_useropen(0, (fcb *)startp, "/", 0, 0, 1);   // 将表项0分配给根目录文件
    strcpy(currentdir, openfilelist[0].dir);    // 设置根目录为当前目录
    curdir = 0;

    printf("File system ready!\n");
}

int main(int argc, char *argv[]) {
    char full_input[80];
    char *token[3];

    startsys();     // 调用startsys()函数将磁盘文件映射到虚拟磁盘中
    printf("Welcome to use my file system!\n");
    // printf("You can use the following commands:\n");
    while(1) {
        printf("%s > $ ", currentdir);
        fgets(full_input, 80, stdin);
        full_input[strlen(full_input) - 1] = '\0';

        int i = 0;
        token[i] = strtok(full_input, " ");
        while (token[i] != NULL) {
            i++;
            token[i] = strtok(NULL, " ");
        };

        if (strcmp(token[0], "exit") == 0 && i == 1) {
            exit(0);
        } else if (strcmp(token[0], "format") == 0 && i == 1) {
            my_format();
        } else if (strcmp(token[0], "cd") == 0 && i == 2) {
            my_cd(token[1]);
        } else if (strcmp(token[0], "mkdir") == 0 && i == 2) {
            my_mkdir(token[1]);
        } else if (strcmp(token[0], "rmdir") == 0 && i == 2) {
            my_rmdir(token[1]);
        } else if (strcmp(token[0], "ls") == 0 && i == 1) {
            my_ls();
        } else if (strcmp(token[0], "create") == 0 && i == 2) {
            my_create(token[1]);
        } else if (strcmp(token[0], "rm") == 0 && i == 2) {
            my_rm(token[1]);
        } else if (strcmp(token[0], "open") == 0 && i == 2) {
            my_open(token[1]);
        } else if (strcmp(token[0], "close") == 0 && i == 2) {
            my_close(token[1]);
        } else if (strcmp(token[0], "write") == 0 && i == 2) {
            my_write(token[1]);
        } else if (strcmp(token[0], "read") == 0 && i == 3) {
            my_read(token[1], token[2]);
        } else if (strcmp(token[0], "exitsys") == 0 && i == 1) {
            my_exitsys();
        } else {
            printf("Error: command not found!\n");
        }
    }

    return 0;
}
