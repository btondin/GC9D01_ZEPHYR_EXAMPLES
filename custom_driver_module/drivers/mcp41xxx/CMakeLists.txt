# Compila o arquivo .c do driver
zephyr_library()
zephyr_library_sources(mcp41xxx.c)

# Publica este diretório ('.') como um local de "include"
# Isso permite que o main.c encontre o "mcp41xxx.h"
zephyr_library_include_directories(.)