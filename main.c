
// main.c

// includes

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
#include "util.h"
#include "ini.h"
#include "util.h"


static const char * const Version = "1.4.70b";
static const char * const HelpMessage = "\
SYNTAX\n\
* polyglot [configfile] [-noini] [-ec engine] [-ed enginedirectory] [-en enginename] [-log true/false] [-lf logfile] [-pg <name>=<value>]* [-uci <name>=<value>]*\n\
* polyglot make-book [-pgn inputfile] [-bin outputfile] [-max-ply ply] [-min-game games] [-min-score score] [-only-white] [-only-black] [-uniform]\n\
* polyglot merge-book -in1 inputfile1 -in2 inputfile2 [-out outputfile]\n\
* polyglot info-book [-bin inputfile] [-exact]\n\
* polyglot dump-book [-bin inputfile] -color color [-out outputfile]\n\
* polyglot [configfile] epd-test [engineoptions] [-epd inputfile] [-min-depth depth] [-max-depth depth] [-min-time time] [-max-time time] [-depth-delta delta]\n\
* polyglot perft [-fen fen] [-max-depth depth]\
";

static const int SearchDepth = 63;
static const double SearchTime = 3600.0;

// variables

static bool Init;

// prototypes

static void stop_search  ();

// functions

// arg_shift_left()

static void arg_shift_left(char **argv, int index){
    int i;
    for(i=index; argv[i]!=NULL; i++){
        argv[i]=argv[i+1];
    }
}

// parse_args()

static void parse_args(ini_t *ini, char **argv){
    int arg_index;
    char *arg;
    arg_index=0;
    while((arg=argv[arg_index])){
        if(my_string_equal(arg,"-ec") && argv[arg_index+1]){
            ini_insert_ex(ini,"PolyGlot","EngineCommand",argv[arg_index+1]);
            arg_shift_left(argv,arg_index);
            arg_shift_left(argv,arg_index);
            continue;
        }if(my_string_equal(arg,"-ed") && argv[arg_index+1]){
            ini_insert_ex(ini,"PolyGlot","EngineDir",argv[arg_index+1]);
            arg_shift_left(argv,arg_index);
            arg_shift_left(argv,arg_index);
            continue;
        }
        if(my_string_equal(arg,"-en") && argv[arg_index+1]){
            ini_insert_ex(ini,"PolyGlot","EngineName",argv[arg_index+1]);
            arg_shift_left(argv,arg_index);
            arg_shift_left(argv,arg_index);
            continue;
        }
        if(my_string_equal(arg,"-log") &&
           argv[arg_index+1] &&
           IS_BOOL(argv[arg_index+1])){
            ini_insert_ex(ini,
                          "PolyGlot",
                          "Log",
                          TO_BOOL(argv[arg_index+1])?"true":"false");
            arg_shift_left(argv,arg_index);
            arg_shift_left(argv,arg_index);
            continue;
        }
        if(my_string_equal(arg,"-lf") && argv[arg_index+1]){
            ini_insert_ex(ini,"PolyGlot","LogFile",argv[arg_index+1]);
            arg_shift_left(argv,arg_index);
            arg_shift_left(argv,arg_index);
            continue;
        }
        if(my_string_equal(arg,"-wb") &&
           argv[arg_index+1]&&
           IS_BOOL(argv[arg_index+1])){
               ini_insert_ex(ini,"PolyGlot",
                             "OnlyWbOptions",
                             TO_BOOL(argv[arg_index+1])?"true":"false");
               arg_shift_left(argv,arg_index);
               arg_shift_left(argv,arg_index);
               continue;
        }
        if((my_string_equal(arg,"-pg")||my_string_equal(arg,"-uci")) &&
           argv[arg_index+1]){
            int ret;
            char section[StringSize];
            char name[StringSize];
            char value[StringSize];
            ret=ini_line_parse(argv[arg_index+1],section,name,value);
            if(ret==NAME_VALUE){
                if(my_string_equal(arg,"-pg")){
                    ini_insert_ex(ini,"PolyGlot",name,value);
                }else{
                    ini_insert_ex(ini,"Engine",name,value);
                }
            }
            arg_shift_left(argv,arg_index);
            arg_shift_left(argv,arg_index);
            continue;
        }
        arg_index++;
    }
}

int wb_select(){
    option_t *opt;
    option_start_iter(Option);
    while((opt=option_next(Option))){
        opt->mode&=~XBOARD;
        if(opt->mode & XBSEL){
            opt->mode|=XBOARD; 
        }
    }
}

int main(int argc, char * argv[])
{
    ini_t ini[1], ini_command[1];
    ini_entry_t *entry;
    char *arg;
    int arg_index;
    bool NoIni;
    option_t *opt;

    if(argc>=2 && ((my_string_case_equal(argv[1],"help")) || (my_string_case_equal(argv[1],"-help")) || (my_string_case_equal(argv[1],"--help")) ||  (my_string_case_equal(argv[1],"-h")) ||  my_string_case_equal(argv[1],"/?"))){
        printf("%s\n",HelpMessage);
        return EXIT_SUCCESS;
    }

    util_init();
    option_init_pg();
    
    square_init();
    piece_init();
    attack_init();
    
    hash_init();

    my_random_init();

    ini_init(ini);
    ini_init(ini_command);

        // book utilities: do not touch these
    
    if (argc >= 2 && my_string_equal(argv[1],"make-book")) {
        book_make(argc,argv);
        return EXIT_SUCCESS;
    }
    
    if (argc >= 2 && my_string_equal(argv[1],"merge-book")) {
        book_merge(argc,argv);
        return EXIT_SUCCESS;
    }

    if (argc >= 2 && my_string_equal(argv[1],"dump-book")) {
        book_dump(argc,argv);
        return EXIT_SUCCESS;
    }
    
    if (argc >= 2 && my_string_equal(argv[1],"info-book")) {
        book_info(argc,argv);
        return EXIT_SUCCESS;
    }

    return EXIT_SUCCESS; 
}
