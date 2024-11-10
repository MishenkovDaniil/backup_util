CC = gcc
# Flags = -ldl -D _DEBUG -lsfml-graphics -lsfml-window -lsfml-system  
Debug_flags = -D _DEBUG -ggdb3 -O0 -Wextra -Waggressive-loop-optimizations -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts -Wconversion -Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op -Wopenmp-simd -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow -Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods -Wsuggest-final-types -Wswitch-default -Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code -Wunused -Wvariadic-macros -Wno-missing-field-initializers -Wno-narrowing -Wno-varargs -Wstack-protector -fcheck-new -fstack-protector -fstrict-overflow -flto-odr-type-merging -fno-omit-frame-pointer -Wstack-usage=8192 -pie -fPIE -Werror=vla -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr

backup_debug: backup/main.c backup/backup.c 
	$(CC) backup/main.c backup/backup.c $(Debug_flags) -o my_backup
backup: backup/main.c backup/backup.c
	$(CC) backup/main.c backup/backup.c -o my_backup
	
restore_debug: restore/main.c restore/restore.c 
	$(CC) restore/main.c restore/restore.c $(Debug_flags) -o my_restore
restore: restore/main.c restore/restore.c
	$(CC) restore/main.c restore/restore.c -o my_restore
