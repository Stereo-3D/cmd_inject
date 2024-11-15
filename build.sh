clang -m64 -static -o cmd_inject_linux64 cmd_inject.c -Wpedantic -Wshadow -Wall -Wextra -O3 -ffast-math
clang -m32 -static -o cmd_inject_linux32 cmd_inject.c -Wpedantic -Wshadow -Wall -Wextra -O3 -ffast-math
clang -target x86_64-w64-mingw32 -static -o cmd_inject_win64.exe cmd_inject.c -Wpedantic -Wshadow -Wall -Wextra -O3 -ffast-math
clang -target i686-w64-mingw32 -static -o cmd_inject_win32.exe cmd_inject.c -Wpedantic -Wshadow -Wall -Wextra -O3 -ffast-math
