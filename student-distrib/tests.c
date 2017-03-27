#include "tests.h"

#include "keyboard.h"
#include "lib.h"
#include "rofs.h"

extern void cp2_test_1() {
    clear();
    list_all_files();
}

extern void cp2_test_2(char *name) {
    clear();
    dentry_t dentry;

    int error = read_dentry_by_name(name, &dentry);
    if (error) {
        printf("Could not read file %s!\n", name);
        return;
    }

    uint8_t buf[4096] = {0};
    read_data(dentry.inode_num, 0, buf, 4096);
    printf("%s\n", buf);
    printf("file_name: %s\n", name);
}

extern void cp2_test_3(int index) {
    clear();
    dentry_t dentry;

    int error = read_dentry_by_index(index, &dentry);
    if (error) {
        printf("Could not read file at index %d!\n", index);
        return;
    }

    uint8_t buf[4096] = {0};
    read_data(dentry.inode_num, 0, buf, 4096);
    printf("%s\n", buf);

    char name[33] = {0};
    strncpy(name, dentry.file_name, 32);
    printf("file_name: %s\n", name);
}

extern void cp2_test_4(rtc_freq_t freq) {
    clear();
    rtc_set_frequency(freq);
}
