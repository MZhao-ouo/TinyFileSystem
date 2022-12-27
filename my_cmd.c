#include "my_fs.h"

/* 格式化磁盘，即初始化磁盘上的文件系统 */
void my_format() {
    memset(myvhard, 0, SIZE);   // 将虚拟磁盘清零
    block0 *b0 = (block0 *)myvhard; // 指向引导块
    strcpy(b0->information, "Disk Size:1024  Block Num:1000");
    b0->root = 5;
    b0->startblock = myvhard + BLOCK_SIZE * 5;

    fat1 = (fat *)(myvhard + BLOCK_SIZE);    // 指向FAT1
    fat2 = (fat *)(myvhard + BLOCK_SIZE * 2);    // 指向FAT2
    for (int i = 0; i < 5; i++) {
        fat1[i].id = END;
        fat2[i].id = END;
    }
    for (int i = 5; i < 1000; i++) {
        fat1[i].id = FREE;
        fat2[i].id = FREE;
    }
    
    fcb *root = (fcb *)(myvhard + BLOCK_SIZE * 5);   // 指向根目录
    int start_fat = allocate_fat(2);
    memset(root, 0, BLOCK_SIZE * 2);

    fill_fcb(&root[0], ".", "", 0, 0, 0, start_fat, BLOCK_SIZE);
    fill_fcb(&root[1], "..", "", 0, 0, 0, start_fat, BLOCK_SIZE);

    startp = b0->startblock;  // 指向数据区的起始位置，前面有一个引导块，两个FAT1，两个FAT2
    memset(openfilelist, 0, sizeof(openfilelist));   // 初始化用户打开文件表
    fill_useropen(0, (fcb *)startp, "/", 0, 0, 1);   // 将表项0分配给根目录文件
    strcpy(currentdir, openfilelist[0].dir);    // 设置根目录为当前目录
    curdir = 0;
}

void my_cd(char *dirname) {
    printf("cding to %s\n", dirname);
    if (strcmp(dirname, ".") == 0)
        return;
    // cd复合路径
    for (char *c = dirname; *c != '\0'; c++) {
        if (*c == '/') {
            if (strcmp(dirname, "/") == 0) {
                curdir = 0;
                strcpy(currentdir, "/");
                return;
            }
            if (dirname[0] == '/')    // 绝对路径
                my_cd("/");
            char *token = strtok(dirname, "/");
            while (token != NULL) {
                my_cd(token);
                token = strtok(NULL, "/");
            }
            return;
        }
    }

    if (strlen(dirname) > 8) {
        printf("Directory name \"%s\" too long!\n", dirname);
        return;
    }

    fcb *curdirptr = (fcb *)(myvhard + BLOCK_SIZE * openfilelist[curdir].first);       // 指向当前目录的FCB
    
    // 返回上级目录
    if (strcmp(dirname, "..") == 0) {
        if (curdir == 0) {
            printf("Already in root directory!\n");
            return;
        } else {
            strcpy(currentdir, openfilelist[curdir].dir);
            fill_useropen(curdir, &curdirptr[1], currentdir, 0, 0, 1);
        }
        return;
    }

    for (int i = 2; i < MAXFCB; i++) {
        if (strcmp(curdirptr[i].filename, dirname) == 0 && curdirptr[i].free == 1) {
            if (curdirptr[i].attribute == 0) {
                curdir = 1;
                fill_useropen(curdir, &curdirptr[i], currentdir, 0, 0, 1);
                strcat(currentdir, dirname);
                strcat(currentdir, "/");
            } else {
                printf("It is not a directory!\n");
            }
            return;
        }
    }

    printf("Directory \"%s\" not found!\n", dirname);       // 没有找到目录
}

void my_mkdir(char *dirname) {
    if (strlen(dirname) > 8) {
        printf("Directory name is too long!\n");
        return;
    }
    int start_fat = allocate_fat(1);
    int flag = 0;
    fcb *newdir = (fcb *)(myvhard + BLOCK_SIZE * start_fat);
    fcb *curdirptr = (fcb *)(myvhard + BLOCK_SIZE * openfilelist[curdir].first);

    for (int i = 0; i < MAXFCB; i++) {
        if (curdirptr[i].free == 0) {
            fill_fcb(&curdirptr[i], dirname, "", 0, 0, 0, start_fat, BLOCK_SIZE);
            flag = 1;
            break;
        }
    }
    if (flag == 0) {
        printf("Directory is full!\n");
        return;
    }
    // 初始化新目录的FCB
    for(int i = 0; i < MAXFCB; i++)
        newdir[i].free = 0;
    fill_fcb(&newdir[0], ".", "", 0, 0, 0, start_fat, BLOCK_SIZE);
    fill_fcb(&newdir[1], "..", "", 0, 0, 0, openfilelist[curdir].first, BLOCK_SIZE);
}

void my_rmdir(char *dirname) {
    fcb *curdirptr = (fcb *)(myvhard + BLOCK_SIZE * openfilelist[curdir].first);

    for (int i = 0; i < MAXFCB; i++) {
        if (strcmp(curdirptr[i].filename, dirname) == 0 && curdirptr[i].free == 1) {
            if (curdirptr[i].attribute == 0) {
                // 判断目录是否为空
                fcb *targetDirPtr = (fcb *)(myvhard + BLOCK_SIZE * curdirptr[i].first);
                for (int j = 2; j < MAXFCB; j++) {
                    if (targetDirPtr[j].free == 1) {
                        printf("Directory is not empty!\n");
                        return;
                    }
                }
                curdirptr[i].free = 0;
                fat1[curdirptr[i].first].id = FREE;
                fat2[curdirptr[i].first].id = FREE;
            } else {
                printf("It is not a directory!\n");
            }
            return;
        }
    }

    printf("Directory not found!\n");
}

void my_ls() {
    fcb *curdirptr = (fcb *)(myvhard + BLOCK_SIZE * openfilelist[curdir].first);
    char attribute[80];

    printf("Name\tAttribute\tTime\tDate\tSize\n");
    for (int i = 0; i < MAXFCB; i++) {
        if (curdirptr[i].free == 1) {
            if (curdirptr[i].attribute == 0)
                strcpy(attribute, "Directory");
            else
                strcpy(attribute, "File");
            printf("%s\t%s\t%d\t%d\t%d\n", curdirptr[i].filename, attribute, curdirptr[i].time, curdirptr[i].date, curdirptr[i].length);
        }
    }
}

void my_create(char *filename) {
    fcb *curdirptr = (fcb *)(myvhard + BLOCK_SIZE * openfilelist[curdir].first);
}

void my_rm(char *filename) {

}

void my_open(char *filename) {

}

void my_close(int fd) {

}

void my_write(int fd) {

}

void my_read(int fd, int len) {

}

void my_exitsys() {
    FILE *fp = fopen("./my_disk", "wb");
    fwrite(myvhard, SIZE, 1, fp);
    fclose(fp);
    free(myvhard);
    
    exit(0);
}
