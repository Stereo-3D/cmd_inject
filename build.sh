gcc -m64 -static -o cmd_inject_linux64 cmd_inject.c -Wshadow -Wall -Wextra -Ofast
gcc -m32 -static -o cmd_inject_linux32 cmd_inject.c -Wshadow -Wall -Wextra -Ofast
x86_64-w64-mingw32-gcc -static -o cmd_inject_win64.exe  cmd_inject.c -Wshadow -Wall -Wextra -Ofast
i686-w64-mingw32-gcc -static -o cmd_inject_win32.exe cmd_inject.c -Wshadow -Wall -Wextra -Ofast
