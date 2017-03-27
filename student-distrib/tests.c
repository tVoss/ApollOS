#include "tests.h"

#include "keyboard.h"
#include "lib.h"
#include "rofs.h"

/* 0: List all files
 *
 */
void cp2_tests(int test, int param) {
    clear_terminal();
    switch (test) {
        case 0:
            list_all_files();
            break;
    }
}
