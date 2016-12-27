#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "book.h"
#include "book_make.h"
#include "book_merge.h"
#include "fen.h"
#include "hash.h"
#include "list.h"
#include "main.h"
#include "move.h"
#include "move_gen.h"
#include "option.h"
#include "piece.h"
#include "square.h"

int main(int argc, char * argv[])
{
    util_init();
    square_init();
    piece_init();
    attack_init();    
    hash_init();
    my_random_init();

    if (argc >= 2 && !strcmp(argv[1], "MakeBook"))
	{
        book_make(argc, argv);
    }    
    else if (argc >= 2 && !strcmp(argv[1], "merge-book"))
	{
        book_merge(argc, argv);
    }
    else if (argc >= 2 && !strcmp(argv[1], "dump-book"))
	{
        book_dump(argc, argv);
    }    
    else if (argc >= 2 && !strcmp(argv[1], "info-book"))
	{
        book_info(argc, argv);
    }

    return 0;
}