#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "ecran.h"
#include "vscreen.h"

/*
 * Functions to implement a virtual screen that can be multiplexed
 * onto a physical screen.  The current contents of a virtual screen
 * are maintained in an in-memory buffer, from which they can be
 * used to initialize the physical screen to correspond.
 */

WINDOW *main_screen;
WINDOW *status_screen;
WINDOW *split_screen1;
WINDOW *split_screen2;
WINDOW *help;

struct vscreen {
    int num_lines;
    int num_cols;
    int cur_line;
    int cur_col;
    char **lines;
    char *line_changed;
};

static void update_line(VSCREEN *vscreen, int l);

/*
 * Create a new virtual screen of the same size as the physical screen.
 */
VSCREEN *vscreen_init() {

    VSCREEN *vscreen = calloc(sizeof(VSCREEN), 1);
    vscreen->num_lines = LINES-1;
    vscreen->num_cols = COLS;

    vscreen->cur_line = 0;
    vscreen->cur_col = 0;
    vscreen->lines = calloc(sizeof(char *), vscreen->num_lines);
    vscreen->line_changed = calloc(sizeof(char), vscreen->num_lines);
    for(int i = 0; i < vscreen->num_lines; i++)
	vscreen->lines[i] = calloc(sizeof(char), vscreen->num_cols);
    //box(main_screen,0,0);
    //box(status_screen,0,0);



    return vscreen;
}

/*
 * Erase the physical screen and show the current contents of a
 * specified virtual screen.
 */
void vscreen_show(VSCREEN *vscreen) {
    if(helpmode){
        wclear(help);
        wmove(help,0,0);
        return;
    }
    if(split_screenmode){
        wclear(split_screen1);
        wclear(split_screen2);
        for(int l = 0; l < vscreen->num_lines; l++) {
            update_line(vscreen, l);
            vscreen->line_changed[l] = 0;
        }
        wmove(split_screen1, vscreen->cur_line, vscreen->cur_col);
        wmove(split_screen2, vscreen->cur_line, vscreen->cur_col);
        refresh();
    }else{
        if(wclear(main_screen) == ERR)
            set_status("NCurses Function Failed");


        for(int l = 0; l < vscreen->num_lines; l++) {
            update_line(vscreen, l);
            vscreen->line_changed[l] = 0;
        }

        if(wmove(main_screen, vscreen->cur_line, vscreen->cur_col) == ERR)
            set_status("NCurses Function Failed");
        refresh();
    }



}





/*
 * Function to be called after a series of state changes,
 * to cause changed lines on the physical screen to be refreshed
 * and the cursor position to be updated.
 * Although the same effect could be achieved by calling vscreen_show(),
 * the present function tries to be more economical about what is displayed,
 * by only rewriting the contents of lines that have changed.
 */
void vscreen_sync(VSCREEN *vscreen) {
    if(helpmode){
        wclear(help);
        wmove(help, vscreen->cur_line, vscreen->cur_col);
        return;
    }
    if(split_screenmode){
        for(int l = 0; l < vscreen->num_lines; l++) {
        if(vscreen->line_changed[l]) {
            update_line(vscreen, l);
            vscreen->line_changed[l] = 0;
            }
        }
        wmove(split_screen1, vscreen->cur_line, vscreen->cur_col);
        wmove(split_screen2, vscreen->cur_line, vscreen->cur_col);
        wrefresh(split_screen1);
        wrefresh(split_screen2);
        refresh();
    }else{
        for(int l = 0; l < vscreen->num_lines; l++) {
            update_line(vscreen, l);
            vscreen->line_changed[l] = 0;
        }

        if(wmove(main_screen, vscreen->cur_line, vscreen->cur_col) ==ERR)
            set_status("NCurses Function Failed");

        refresh();
    }
}



/*
 * Helper function to clear and rewrite a specified line of the screen.
 */
static void update_line(VSCREEN *vscreen, int l) {
    if(helpmode){
        wclear(help);
        wmove(help, vscreen->cur_line, vscreen->cur_col);
        return;
    }
    if(split_screenmode){
        char *line = vscreen->lines[l];
        wmove(split_screen1, l, 0);
        wclrtoeol(split_screen1);
        wmove(split_screen2, l, 0);
        wclrtoeol(split_screen2);
        for(int c = 0; c < vscreen->num_cols; c++) {
            char ch = line[c];
            if(isprint(ch)){
                waddch(split_screen1, line[c]);
                waddch(split_screen2, line[c]);
            }
        }
        wmove(main_screen, vscreen->cur_line, vscreen->cur_col);
        refresh();
    }else{
        char *line = vscreen->lines[l];
        if(wmove(main_screen, l, 0)==ERR)
            set_status("NCurses Function Failed");
        if(wclrtoeol(main_screen) == ERR)
            set_status("NCurses Function Failed");
        for(int c = 0; c < vscreen->num_cols; c++) {
            char ch = line[c];
            if(isprint(ch)){
                if(waddch(main_screen, line[c]) == ERR)
                    set_status("NCurses Function Failed");
            }
        }
        if(wmove(main_screen, vscreen->cur_line, vscreen->cur_col) ==ERR)
            set_status("NCurses Function Failed");
        refresh();
    }


}



/*
 * Output a character to a virtual screen, updating the cursor position
 * accordingly.  Changes are not reflected to the physical screen until
 * vscreen_show() or vscreen_sync() is called.  The current version of
 * this function emulates a "very dumb" terminal.  Each printing character
 * output is placed at the cursor position and the cursor position is
 * advanced by one column.  If the cursor advances beyond the last column,
 * it is reset to the first column.  The only non-printing characters
 * handled are carriage return, which causes the cursor to return to the
 * beginning of the current line, and line feed, which causes the cursor
 * to advance to the next line and clear from the current column position
 * to the end of the line.  There is currently no scrolling: if the cursor
 * advances beyond the last line, it wraps to the first line.
 */
void vscreen_putc(VSCREEN *vscreen, char ch) {
    if(helpmode){
        if(ch == 27){
            helpmode = 0;
            vscreen_show(fg_session->vscreen);
        }
        return;
    }
    int l = vscreen->cur_line;
    int c = vscreen->cur_col;
    if(isprint(ch)) {
	vscreen->lines[l][c] = ch;



	if(vscreen->cur_col + 1 < vscreen->num_cols)
	    vscreen->cur_col++;
    } else if(ch == '\n') {
        if( l >= vscreen->num_lines -1){
            int i;

            for(i = 1; i <  vscreen->num_lines; i++){
                memset(vscreen->lines[i-1], 0, vscreen->num_cols);
                strcpy(vscreen->lines[i - 1],vscreen->lines[i]);
                vscreen->line_changed[i-1] = 1;
            }

            memset(vscreen->lines[vscreen->num_lines-1], 0, vscreen->num_cols);
            vscreen->line_changed[vscreen->num_lines-1]=1;
        }else{
            vscreen->cur_col = 0;
            l = vscreen->cur_line = (vscreen->cur_line + 1) ;
            memset(vscreen->lines[l], 0, vscreen->num_cols);
        }


    } else if(ch == '\r') {
	vscreen->cur_col = 0;

    } else if(ch == '\a'){
        flash();
    }else if(ch == '\b'){
        if(c != 0){
            vscreen->cur_col = vscreen->cur_col -1;

        }
    }else if(ch == '\t'){
        if(c < vscreen->num_cols){
            if(c % 8 == 0)
                c++;
            while(c % 8 != 0)
                c++;
            vscreen->cur_col = c;
        }
    }else if(ch == '\f'){
        for(int i = 0; i < vscreen->num_lines; i++){
            memset(vscreen->lines[i], 0, vscreen->num_cols);
        }
        vscreen->cur_line = 0;
        vscreen->cur_col = 0;


    }else {}
    l = vscreen->cur_line;
    c = vscreen->cur_col;

    vscreen->line_changed[l] = 1;
}

/*
 * Deallocate a virtual screen that is no longer in use.
 */
void vscreen_fini(VSCREEN *vscreen) {
    for(int i = 0; i < vscreen->num_lines; i++){
        free(vscreen->lines[i]);
    }
    free(vscreen -> lines);
    free(vscreen -> line_changed);
    free(vscreen);
}
