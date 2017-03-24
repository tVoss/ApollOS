#include "tests.h"

#include "rofs.h"

int test_rofs() {
    int error = 0;

    dentry_t dentry;
    read_dentry_by_index(0, &dentry);

    // Set error if file name is wrong
    error |= dentry.file_name[0] != '.';

    read_dentry_by_name("frame0.txt", &dentry);

    error |= dentry.file_name[0] != 'f';

    uint8_t data[188];
    read_data(dentry.inode_num, 0, data, 188);
    data[187] = 0;

    printf("Data from frame0.txt:\n%s\n", data);

    return error;
}
