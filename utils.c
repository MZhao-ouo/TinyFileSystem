#include "my_fs.h"

int fill_useropen(int index, fcb *fcbptr, char *dir, int count, char fcbstate, char topenfile) {
    strcpy(openfilelist[index].filename, fcbptr->filename);
    openfilelist[index].attribute = fcbptr->attribute;
    openfilelist[index].create_stamp = fcbptr->create_stamp;
    openfilelist[index].modify_stamp = fcbptr->modify_stamp;
    openfilelist[index].first = fcbptr->first;
    openfilelist[index].length = fcbptr->length;

    strcpy(openfilelist[index].dir, dir);
    openfilelist[index].count = count;
    openfilelist[index].fcbstate = fcbstate;
    openfilelist[index].topenfile = topenfile;

    return TRUE;
}

int fill_fcb(fcb *fcbptr, char *filename, unsigned char attribute, unsigned short create_stamp, unsigned short modify_stamp, unsigned short first, unsigned long length) {
    strcpy(fcbptr->filename, filename);
    fcbptr->attribute = attribute;
    fcbptr->create_stamp = create_stamp;
    fcbptr->modify_stamp = modify_stamp;
    fcbptr->first = first;
    fcbptr->length = length;
    fcbptr->free = 1;

    return TRUE;
}

int allocate_fat(int num) {
    int first = -1;
    int last = -1;

    fat *fat1;
    fat *fat2;
    fat1 = (fat *)(myvhard + BLOCK_SIZE);    // 指向FAT1
    fat2 = (fat *)(myvhard + BLOCK_SIZE * 2);    // 指向FAT2

    for (int i = 5; i < 1000; i++) {
        if (fat1[i].id == FREE && fat2[i].id == FREE) {
            if (first == -1) {
                first = i;
            } else {
                fat1[last].id = i;
                fat2[last].id = i;
            }
            num--;
            if (num == 0) {
                fat1[i].id = END;
                fat2[i].id = END;
                break;
            } else {
                last = i;
            }
        }
    }
    
    return first;
}

int free_fat(int index) {
    fat *fat1;
    fat *fat2;
    fat1 = (fat *)(myvhard + BLOCK_SIZE);    // 指向FAT1
    fat2 = (fat *)(myvhard + BLOCK_SIZE * 2);    // 指向FAT2
    int next_fat;

    do {
        next_fat = fat1[index].id;
        fat1[index].id = FREE;
        fat2[index].id = FREE;
    } while (next_fat != END);

    return 1;
}

int find_fd() {
    for (int i = 2; i < MAXOPENFILE; i++) {
        if (openfilelist[i].topenfile == 0) {
            return i;
        }
    }
    return -1;
}

int get_dir_block(char *full_dir) {
    fcb *dir_fcb_p = (fcb *)startp;
    char *token;

    token = strtok(full_dir, "/");
    while(token != NULL) {
        for (int i = 0; i < MAXFCB; i++) {
            if (strcmp(dir_fcb_p[i].filename, token) == 0 && dir_fcb_p[i].attribute == 0) {
                dir_fcb_p = (fcb *)(myvhard + dir_fcb_p[i].first * BLOCK_SIZE);
            }
        }
        token = strtok(NULL, "/");
    }

    return dir_fcb_p[0].first;
}
