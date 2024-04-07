#include <stdio.h>
#include <unistd.h>

int main() {
    // Arguments for the ipcrm command
    char *arg0 = "ipcrm";
    char *arg1 = "-a";

    // Execute the ipcrm command
    execlp(arg0, arg0, arg1, NULL);

    // If execlp returns, an error occurred
    perror("execlp");
    return 1;
}