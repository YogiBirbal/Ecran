/*
 * Virtual screen sessions.
 *
 * A session has a pseudoterminal (pty), a virtual screen,
 * and a process that is the session leader.  Output from the
 * pty goes to the virtual screen, which can be one of several
 * virtual screens multiplexed onto the physical screen.
 * At any given time there is a particular session that is
 * designated the "foreground" session.  The contents of the
 * virtual screen associated with the foreground session is what
 * is shown on the physical screen.  Input from the physical
 * keyboard is directed to the pty for the foreground session.
 */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "ecran.h"
#include <time.h>

#include <errno.h>
#include "session.h"
#include <signal.h>
#include <sys/wait.h>


SESSION *sessions[MAX_SESSIONS];  // Table of existing sessions
SESSION *fg_session;              // Current foreground session
void exit_error();
void sigchld_handler(int sig);
void sigalrm_handler(int sig);
void set(char* s);
VSCREEN *helpvscreen;
int t = 0;
/*
 * Initialize a new session whose session leader runs a specified command.
 * If the command is NULL, then the session leader runs a shell.
 * The new session becomes the foreground session.
 */
SESSION *session_init(char *path, char *argv[]) {
    int error;

    for(int i = 0; i < MAX_SESSIONS; i++) {

	if(sessions[i] == NULL) {
	    int mfd = posix_openpt(O_RDWR | O_NOCTTY);
	    if(mfd == -1)
		      return NULL; // No more ptys
	    if(unlockpt(mfd) == -1)
            exit_error();

	    char *sname = ptsname(mfd);
        if(sname == NULL)
            exit_error();
	    // Set nonblocking I/O on master side of pty
	    if((error = fcntl(mfd, F_SETFL, O_NONBLOCK)) == -1)
            exit_error();

	    SESSION *session = calloc(sizeof(SESSION), 1);
	    sessions[i] = session;
	    session->sid = i;
	    session->vscreen = vscreen_init();
	    session->ptyfd = mfd;

	    // Fork process to be leader of new session.
	    if((session->pid = fork()) == 0) {
		// Open slave side of pty, create new session,
		// and set pty as controlling terminal.
        int sfd = open(sname, O_RDWR);
        if(sfd == -1)
            exit_error();


        if((error = setsid()) == -1)
            exit_error();
        if((error = ioctl(sfd, TIOCSCTTY, 0)) == -1)
            exit_error();

        dup2(sfd,2);

		if((putenv("TERM=dumb")))
            exit_error();

        if((error = dup2(sfd,0)) == -1){
            exit_error();
        }

        if((error = dup2(sfd,1)) == -1){
            exit_error();
        }

        if((error = close(sfd)) == -1){
            exit_error();
        }
        if((error = close(mfd)) == -1){
            exit_error();
        }
        if((error = execv(path, argv)) == -1)
            exit_error();

	    }else if (session->pid <0){
            exit_error();
        }else{
            set_status("New Session Made");
            session_setfg(session);
            return session;
        }

	    }else{
            set_status("No More Sessions Available");
        }
    }
    return NULL;  // Session table full.
}

/*
 * Set a specified session as the foreground session.
 * The current contents of the virtual screen of the new foreground session
 * are displayed in the physical screen, and subsequent input from the
 * physical terminal is directed at the new foreground session.
 */
void session_setfg(SESSION *session) {
    fg_session = session;
    // REST TO BE FILLED IN
    //fprintf(stderr,"SID: %i, %i\n", session->sid, session->error);
    vscreen_show(session ->vscreen);

}

/*
 * Read up to bufsize bytes of available output from the session pty.
 * Returns the number of bytes read, or EOF on error.
 */
int session_read(SESSION *session, char *buf, int bufsize) {
    return read(session->ptyfd, buf, bufsize);
}

/*
 * Write a single byte to the session pty, which will treat it as if
 * typed on the terminal.  The number of bytes written is returned,
 * or EOF in case of an error.
 */
int session_putc(SESSION *session, char c) {
    // TODO: Probably should use non-blocking I/O to avoid the potential
    // for hanging here, but this is ignored for now.
    return write(session->ptyfd, &c, 1);
}

/*
 * Forcibly terminate a session by sending SIGKILL to its process group.
 */
void session_kill(SESSION *session) {

    kill(session->pid, SIGKILL);
    close(session->ptyfd);
    session_fini(session);
}

/*
 * Deallocate a session that has terminated.  This function does not
 * actually deal with marking a session as terminated; that would be done
 * by a signal handler.  Note that when a session does terminate,
 * the session leader must be reaped.  In addition, if the terminating
 * session was the foreground session, then the foreground session must
 * be set to some other session, or to NULL if there is none.
 */
void session_fini(SESSION *session) {
    vscreen_fini(session->vscreen);
    free(session);
}



void exit_error(){
    if(errno){
        set_status(strerror(errno));
    }

    for(int i = 0; i < MAX_SESSIONS; i++){
        if(sessions[i]== NULL)
            continue;
        session_kill(sessions[i]);
        session_fini(sessions[i]);


    }

    exit(EXIT_FAILURE);

}

void sigchld_handler(int sig){
    //int i;
    pid_t pid;
    int old = errno;
    if(sig == SIGCHLD){
        while((pid = waitpid(-1,NULL,WNOHANG)) > 0){
        }
    }

    errno = old;
}

void sigalrm_handler(int sig){
    time_t mytime = time(NULL);
    char * time_str = ctime(&mytime);
    time_str[strlen(time_str)-6] = '\0';
    set(time_str + 11);
    alarm(1);

}

void set(char* s){
    mvwprintw(status_screen, 0,COLS - 8,s);
    wrefresh(status_screen);
}


void help_init(){
    if(help!= NULL){
        helpvscreen = vscreen_init();
    }
}

void help_fini(){
    vscreen_fini(helpvscreen);
}