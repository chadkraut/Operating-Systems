#include <unistd.h>
#include <stdio.h>

main(argc)
{
system("/bin/ps -lyfww | egrep '[p]2|[v]ertical|[s]leep 2|[P]ID'");
fflush(stdout);
}
