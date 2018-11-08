/*
 * Ecran: A program that supports the multiplexing
 * of multiple virtual terminal sessions onto a single physical terminal.
 */

#include <unistd.h>
#include <stdlib.h>
#include <ncurses.h>
#include <sys/signal.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include "ecran.h"
#include "session.h"

static void initialize();
static void curses_init(void);
static void curses_fini(void);
static void finalize(void);
void fg(SESSION *session);

void set_status(char *status);
int err = 0;

int main(int argc, char *argv[]) {
        int c;
        char * filename;
        split_screenmode = 0;
        helpmode = 0;

        signal(SIGCHLD,sigchld_handler);
        signal(SIGALRM,sigalrm_handler);
        alarm(1);
        initialize();

        while((c = getopt(argc,argv,"o:")) != -1){
            switch(c){
                case 'o':
                filename = optarg;
                int error = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXG | S_IRWXU | S_IRWXO);

                err = 1;
                int e;

                if((e = dup2(error, 2)) == -1)
                    exit_error();

                if(close(error) == -1)
                    exit_error();

                break;


            }
        }

        char sg[100];
        if(optind < argc){
            while(optind< argc){
                strcat(sg, argv[optind]);
                strcat(sg, " ");
                optind++;

            }
            FILE *fp;
            fp = popen(sg, "r");



            int c;
            while((c = fgetc(fp)) != EOF){
                vscreen_putc(fg_session->vscreen, c);
            }
                vscreen_show(fg_session->vscreen);
                set_status("");


                while(1){
                    int in = wgetch(main_screen);
                    if(in == 'q'){
                        finalize();
                    }
        }
        mainloop();
    }
}

/*
 * Initialize the program and launch a single session to run the
 * default shell.
 */
static void initialize() {
    curses_init();
    char *path = getenv("SHELL");
    if(path == NULL)
	path = "/bin/bash";

    char *argv[2] = { " (ecran session)", NULL };
    session_init(path, argv);
}

/*
 * Cleanly terminate the program.  All existing sessions should be killed,
 * all pty file descriptors should be closed, all memory should be deallocated,
 * and the original screen contents should be restored before terminating
 * normally.  Note that the current implementation only handles restoring
 * the original screen contents and terminating normally; the rest is left
 * to be done.
 */
static void finalize(void) {
    for(int i = 0; i < MAX_SESSIONS; i++){
        if(sessions[i] != NULL )
        session_kill(sessions[i]);
    }
    curses_fini();
    help_fini();

    exit(EXIT_SUCCESS);
}

/*
 * Helper method to initialize the screen for use with curses processing.
 * You can find documentation of the "ncurses" package at:
 * https://invisible-island.net/ncurses/man/ncurses.3x.html
 */
static void curses_init(void) {
    initscr();
    int r;
    if((r = raw())==ERR)         // Don't generate signals, and make typein
        exit(EXIT_FAILURE);
                                 // immediately available.
    if((r = noecho())==ERR)      // Don't echo -- let the pty handle it.
        exit(EXIT_FAILURE);

    main_screen = stdscr;
    main_screen = newwin(LINES -1,COLS,0,0);
    status_screen = stdscr;
    status_screen = newwin(1,COLS,LINES-1,0);
    split_screen1 = newwin(LINES -1,COLS/2,0,0);
    split_screen2 = newwin(LINES -1,COLS/2,0,COLS/2);
    help = newwin(LINES -1,COLS,0,0);

    help_init();

    //refresh();
    if((r = nodelay(main_screen, TRUE)) == ERR)  // Set non-blocking I/O on input.
        exit(EXIT_FAILURE);


    if((r = wclear(main_screen)) == ERR)         // Clear the screen.
        exit(EXIT_FAILURE);


    if((r = refresh()) == ERR)                   // Make changes visible.
        exit(EXIT_FAILURE);



    wrefresh(status_screen);
}

/*
 * Helper method to finalize curses processing and restore the original
 * screen contents.
 */
void curses_fini(void) {
    endwin();
}

/*
 * Function to read and process a command from the terminal.
 * This function is called from mainloop(), which arranges for non-blocking
 * I/O to be disabled before calling and restored upon return.
 * So within this function terminal input can be collected one character
 * at a time by calling getch(), which will block until input is available.
 * Note that while blocked in getch(), no updates to virtual screens can
 * occur.
 */
void do_command() {
    // Quit command: terminates the program cleanly
    set_status("");
    int in = wgetch(main_screen);
    if(in == 'q')
	finalize();
    else if(in == 'n'){
        //curses_init();
        char *path = getenv("SHELL");
        if(path == NULL)
        path = "/bin/bash";

        char *argv[2] = { " (ecran session)", NULL };

        session_init(path, argv);
        vscreen_sync(fg_session->vscreen);
        wrefresh(main_screen);
        wrefresh(split_screen1);
        wrefresh(split_screen2);

    }else if(in == '0'){
        if(sessions[0] != NULL){
            session_setfg(sessions[0]);
            set_status("Current Session: Session 0");
        }else{
            flash();
            set_status("Session 0 does not exist");
        }

    }else if(in == '1'){
        if(sessions[1] != NULL){
            session_setfg(sessions[1]);
            set_status("Current Session: Session 1");
        }else{
            flash();
            set_status("Session 1 does not exist");
        }

    }else if(in == '2'){
        if(sessions[2] != NULL){
            session_setfg(sessions[2]);
            set_status("Current Session: Session 2");
        }else{
            flash();
            set_status("Session 2 does not exist");
        }

    }else if(in == '3'){
        if(sessions[3] != NULL){
            session_setfg(sessions[3]);
            set_status("Current Session: Session 3");
        }else{
            flash();
            set_status("Session 3 does not exist");
        }

    }else if(in == '4'){
        if(sessions[4] != NULL){
            session_setfg(sessions[4]);
            set_status("Current Session: Session 4");
        }else{
            flash();
            set_status("Session 4 does not exist");
        }

    }else if(in == '5'){
        if(sessions[5] != NULL){
            session_setfg(sessions[5]);
            set_status("Current Session: Session 5");
        }else{
            flash();
            set_status("Session 5 does not exist");

        }

    }else if(in == '6'){
        if(sessions[6] != NULL){
            session_setfg(sessions[6]);
            set_status("Current Session: Session 6");
        }else{
            flash();
            set_status("Session 6 does not exist");
        }

    }else if(in == '7'){
        if(sessions[7] != NULL){
            session_setfg(sessions[7]);
            set_status("Current Session: Session 7");
        }else{
            flash();
            set_status("Session 7 does not exist");
        }

    }else if(in == '8'){
        if(sessions[8] != NULL){
            session_setfg(sessions[8]);
            set_status("Current Session: Session 8");
        }else{
            flash();
            set_status("Session 8 does not exist");
        }

    }else if(in == '9'){
        if(sessions[9] != NULL){
            session_setfg(sessions[9]);
            set_status("Current Session: Session 9");
        }else{
            flash();
            set_status("Session 9 does not exist");
        }
    }else if(in == 'k'){
        int sec = wgetch(main_screen);
        if(sec == '0'){
            if(sessions[0] != NULL){
                fg(sessions[0]);
                session_kill(sessions[0]);
                sessions[0] = NULL;
                set_status("Session 0 Killed");

            }else{
                flash();
                set_status("Session 0 does not exist");

            }
        }else if(sec == '1'){
            if(sessions[1] != NULL){
                fg(sessions[1]);
                session_kill(sessions[1]);
                sessions[1] = NULL;
                set_status("Session 1 Killed");


            }else{
                flash();
                set_status("Session 1 does not exist");
            }

        }else if(sec == '2'){
            if(sessions[2] != NULL){
                fg(sessions[2]);
                session_kill(sessions[2]);
                sessions[2] = NULL;
                set_status("Session 2 Killed");

            }else{
                flash();
                set_status("Session 2 does not exist");
            }

        }else if(sec == '3'){
            if(sessions[3] != NULL){
                fg(sessions[3]);
                session_kill(sessions[3]);
                sessions[3] = NULL;
                set_status("Session 3 Killed");

            }else{
                flash();
                set_status("Session 3 does not exist");
            }

        }else if(sec == '4'){
            if(sessions[4] != NULL){
                fg(sessions[4]);
                session_kill(sessions[4]);
                sessions[4] = NULL;
                set_status("Session 4 Killed");

            }else{
                flash();
                set_status("Session 4 does not exist");
            }

        }else if(sec == '5'){
            if(sessions[5] != NULL){
                fg(sessions[5]);
                session_kill(sessions[5]);
                sessions[5] = NULL;
                set_status("Session 5 Killed");

            }else{
                flash();
                set_status("Session 5 does not exist");
            }

        }else if(sec == '6'){
            if(sessions[6] != NULL){
                fg(sessions[6]);
                session_kill(sessions[6]);
                sessions[6] = NULL;
                set_status("Session 6 Killed");

            }else{
                flash();
                set_status("Session 6 does not exist");
            }

        }else if(sec == '7'){
            if(sessions[7] != NULL){
                fg(sessions[7]);
                session_kill(sessions[7]);
                sessions[7] = NULL;
                set_status("Session 7 Killed");


            }else{
                flash();
                set_status("Session 7 does not exist");
            }

        }else if(sec == '8'){
            if(sessions[8] != NULL){
                fg(sessions[8]);
                session_kill(sessions[8]);
                sessions[8] = NULL;
                set_status("Session 8 Killed");

            }else{
                flash();
                set_status("Session 8 does not exist");
            }

        }else if(sec == '9'){
            if(sessions[9] != NULL){
                fg(sessions[9]);
                session_kill(sessions[9]);
                sessions[9] = NULL;
                set_status("Session 9 Killed");
            }else{
                flash();
                set_status("Session 9 does not exist");
            }
        }else flash();
    }else if(in == 's'){
        if(split_screenmode){
            split_screenmode = 0;
            session_setfg(fg_session);
            wrefresh(split_screen1);
        }else{
            split_screenmode = 1;
            session_setfg(fg_session);
            wrefresh(split_screen1);
            wrefresh(split_screen2);
        }
    }else if(in == 'h'){
        if(!helpmode){
            helpmode = 1;
            vscreen_show(helpvscreen);

            fprintf(stderr, "help\n" );

            wprintw(help, "Help Screen\n");
            wprintw(help, "----------------------------\n");
            wprintw(help, "Ecran Can Understand the Following Commands:\n");
            wprintw(help, "CTRL -a n: Start a new Terminal Session\n");
            wprintw(help, "CTRL -a 0-9: Swtich to a specific virtual session, if it is active\n");
            wprintw(help, "CTRL -a k 0-9: Forcibly Terminate an Existing Session\n");
            wprintw(help, "CTRL -a s: Split the Screen, showing current session in both halves of screen\n");
            wprintw(help, "CTRL -a h: Display Help Screen\n");
            wprintw(help, "ESC: Escape from Help Screen\n");
            wprintw(help, "CTRL -a q: QUIT ECRAN\n");
            wprintw(help, "Current Sessions that are active: \n");
            for(int i = 0; i < MAX_SESSIONS; i++){
                if(sessions[i] != NULL){
                    char s[1];
                    sprintf(s,"%i",i);
                    wprintw(help, s);
                }
            }
            wrefresh(help);
            vscreen_show(helpvscreen);
            vscreen_sync(helpvscreen);
        }


    }else if(in == 27){
        vscreen_putc(helpvscreen, 27);

    }else{
        flash();
    }

}



void set_status(char *status){
    wclear(status_screen);
    wprintw(status_screen, status);
    wrefresh(status_screen);
}

void fg(SESSION *session){
    if(session == fg_session){
        for(int i = 0; i < MAX_SESSIONS; i++){
            if(sessions[i] != NULL && sessions[i] != session){
                session_setfg(sessions[i]);
                return;
            }
        }
        finalize();
    }
}


