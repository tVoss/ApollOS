#include "tests.h"

#include "rofs.h"

int test_rofs() {
    dentry_t dentry;
    read_dentry_by_name(".", &dentry);

    return dentry.file_name[0] == '.';
}
