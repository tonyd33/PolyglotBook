
// main.c

// includes

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "attack.h"
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
#include "uci.h"
#include "util.h"
#include "ini.h"
#include "util.h"


// constants


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


// make_ini()

static void make_ini(ini_t *ini){
    option_t *opt;
    ini_insert_ex(ini,"polyglot",
		  "EngineCommand",
		  option_get(Option,"EngineCommand"));
    ini_insert_ex(ini,"polyglot",
		  "EngineDir",
		  option_get(Option,"EngineDir"));
    option_start_iter(Option);
    while((opt=option_next(Option))){
        if(my_string_case_equal(opt->name,"SettingsFile")) continue;
        if(my_string_case_equal(opt->name,"EngineCommand")) continue;
        if(my_string_case_equal(opt->name,"EngineDir")) continue;
        if(!my_string_equal(opt->value,opt->default_)&& !IS_BUTTON(opt->type))
        {
            ini_insert_ex(ini,"polyglot",opt->name,opt->value);
        }
    }
    option_start_iter(Uci->option);
    while((opt=option_next(Uci->option))){
        if(!strncmp(opt->name,"UCI_",4) &&
            !my_string_case_equal(opt->name,"UCI_LimitStrength") &&
            !my_string_case_equal(opt->name,"UCI_Elo"))continue;
        if(!my_string_equal(opt->value,opt->default_)&&
           !IS_BUTTON(opt->type)){
            ini_insert_ex(ini,"engine",opt->name,opt->value);
        }
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

   // init

    Init = FALSE;

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

        // What is the config file? This is very hacky right now.

        // Do we want a config file at all?

    arg_index=0;
    NoIni=FALSE;
    while((arg=argv[arg_index++])){
        if(my_string_equal(arg,"-noini")){
            NoIni=TRUE;
            break;
        }
    }
    arg_shift_left(argv,arg_index-1);
    parse_args(ini_command,argv+1);
    if(NoIni){
        option_set(Option,"SettingsFile","<empty>");
    }

        // Ok see if first argument looks like config file
    
    if(argv[1] && !my_string_equal(argv[1],"epd-test") && !(argv[1][0]=='-')){
                // first argument must be  config file
        if(!NoIni){
            option_set(Option,"SettingsFile",argv[1]);
        }else{
                // ignore
        }
        arg_shift_left(argv,1);
    }else{
            // Config file is the default.
            // This has already been set above or in "option_init_pg()"
    }



        // if we use a config file: load it!
    
    if(!my_string_equal(option_get_string(Option,"SettingsFile"),"<empty>")){
        if(ini_parse(ini,option_get_string(Option,"SettingsFile"))){
            my_fatal("main(): Can't open config file \"%s\": %s\n",
                   option_get_string(Option,"SettingsFile"),
                   strerror(errno));
        }
    }

        // Concession to WB 4.4.0
        // Treat "polyglot_1st.ini" and "polyglot_2nd.ini" specially

    if(option_get_bool(Option,"WbWorkArounds3")){
        const char *SettingsFile=option_get(Option,"SettingsFile");
        if(strstr(SettingsFile,"polyglot_1st.ini")||
           strstr(SettingsFile,"polyglot_2nd.ini")){
            option_set(Option,"SettingsFile","<empty>");
        }
    }

        // Look at command line for logging option. It is important
        // to start logging as soon as possible.

     if((entry=ini_find(ini_command,"PolyGlot","Log"))){
         option_set(Option,entry->name,entry->value);
    }
    if((entry=ini_find(ini_command,"PolyGlot","LogFile"))){
        option_set(Option,entry->name,entry->value);
    }
    
       // start logging if required
    
    if (option_get_bool(Option,"Log")) {
        my_log_open(option_get_string(Option,"LogFile"));
    }

        // log welcome stuff
    
    if(!my_string_equal(option_get_string(Option,"SettingsFile"),"<empty>")){
        my_log("POLYGLOT INI file \"%s\"\n",option_get_string(Option,"SettingsFile"));
    }


        // scavenge command line for options necessary to start the engine


    if((entry=ini_find(ini_command,"PolyGlot","EngineCommand"))){
        option_set(Option,entry->name,entry->value);
    }
    if((entry=ini_find(ini_command,"PolyGlot","EngineDir"))){
        option_set(Option,entry->name,entry->value);
    }
    if((entry=ini_find(ini_command,"PolyGlot","EngineName"))){
        option_set(Option,entry->name,entry->value);
    }

    // Make sure that EngineCommand has been set
    if(my_string_case_equal(option_get(Option,"EngineCommand"),"<empty>")){
      my_fatal("main(): EngineCommand not set\n");
    }

        // start engine
    
    engine_open(Engine);

    if(!engine_active(Engine)){
        my_fatal("main(): Could not start \"%s\"\n",option_get(Option,"EngineCommand"));
    }

        // switch to UCI mode if necessary
    
    if (option_get_bool(Option,"UCI")) {
        my_log("POLYGLOT *** Switching to UCI mode ***\n");
    }

        // initialize uci parsing and send uci command. 
        // Parse options and wait for uciok
    
    // XXX
    uci_open(Uci,Engine);

    option_set_default(Option,"EngineName",Uci->name);

        // get engine name from engine if not supplied in config file or on
        // the command line

    if (my_string_equal(option_get_string(Option,"EngineName"),"<empty>")) {
        option_set(Option,"EngineName",Uci->name);
    }


        // In the case we have been invoked with NoIni or StandardIni
        // we still have to load a config file.

    if(my_string_equal(option_get_string(Option,"SettingsFile"),"<empty>")){

            //  construct the name of the ConfigFile from the EngineName
        
        char tmp[StringSize];
        char option_file[StringSize];
        int i;
        snprintf(tmp,sizeof(tmp),"%s.ini",
                 option_get_string(Option,"EngineName"));
        tmp[sizeof(tmp)-1]='\0';
        for(i=0;i<strlen(tmp);i++){
            if(tmp[i]==' '){
                tmp[i]='_';
            }
        }
        my_path_join(option_file,
                     option_get_string(Option,"SettingsDir"),
                     tmp);
    // Load the config file
        option_set(Option,"SettingsFile",option_file);

        my_log("POLYGLOT INI file \"%s\"\n",option_get_string(Option,"SettingsFile"));
        if(ini_parse(ini,option_file)){
            my_log("POLYGLOT Unable to open %s\n",
                   option_get_string(Option,"SettingsFile")); 
        }
    }


    // Parse the command line and merge remaining options.

    ini_start_iter(ini_command);
    while((entry=ini_next(ini_command))){
        ini_insert(ini,entry);
    }

        // Remind the reader about the options that are now in effect.

    my_log("POLYGLOG OPTIONS \n");
    ini_disp(ini);

        // Cater to our biggest customer:-)
    
    if(option_get_bool(Option,"OnlyWbOptions")){
        wb_select();
    }

        // done initializing
    
    Init = TRUE;
    
        // collect engine options from config file(s) and send to engine
    
    ini_start_iter(ini);
    while((entry=ini_next(ini))){
        if(my_string_case_equal(entry->section,"engine")){
                // also updates value in Uci->option
            uci_send_option(Uci,entry->name,"%s",entry->value);
        }
    }

    
    
        // Anything that hasn't been parsed yet is a syntax error
        // It seems that XBoard sometimes passes empty strings as arguments
        // to PolyGlot. We ignore these. 

    argc=1;
    while((arg=argv[argc++])){
        if(!my_string_equal(arg,"")){
            my_fatal("main(): Incorrect use of option: \"%s\"\n",argv[argc-1]);
        }
    }

    return EXIT_SUCCESS; 
}
