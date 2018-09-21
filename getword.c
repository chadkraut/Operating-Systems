
/*
 * Programmer: Chad Krauthamer
 * Class: CS570 Operating Systems
 * Professor: John Carroll
 * Semester: Fall 2018
 * Due Date: 09/17/18
 * */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include "getword.h"

/*
* INPUT: a pointer to the beginning of a character string [a character array]
* OUTPUT: the number of characters in the word (or the negative of that number)
* SIDE EFFECTS: bytes beginning at address w will be overwritten.
*   Anyone using this routine should have w pointing to an
*   available area at least STORAGE bytes long before calling getword().
*/

int getword(char *w) {
    //variable declarations
    int iochar = 0;
    int wCount = 0;
    int dollarCheck = 0;
    int temp = 0;


    // Check for leading spaces, we are at the beginning
    iochar = getchar();
    while (iochar == ' ') {
        iochar = getchar();
    }

    // Check for EOF at the beginning
    if (iochar == EOF) {
        *w = '\0';
        return -255;
    }

    // Check for a newline or ';' at the beginning
    if (iochar == '\n' || iochar == ';') {
        *w = '\0';
        return wCount;
    }

    // Check for metacharacters at the beginning then add to *w++
    if (iochar == '>' || iochar == '|' || iochar == '&') {
        wCount++;
        *w++ = (char) iochar;
        *w = '\0';
        return wCount;
    }

    // Check if word starts with ~
    // if it does, add the results of getenv() to *w
    if (iochar == '~') {
        char *env;
        iochar = getchar();
        for(env = getenv("HOME"); *env != '\0'; env++){
            *w++ = *env;
            wCount++;
        }
    }

    // Check if word starts with $
    // set a flag for $
    if (iochar == '$') {
        iochar = getchar();
        dollarCheck = 1;
    }

    // Check for '<<' metacharacter, if true add it to *w
    // then null terminate, if not place back into stdin
    // char is placed back into stdin to get eaten by switch
    if (iochar == '<') {
        temp = getchar();
        if (temp == '<') {
            *w++ = (char) iochar;
            *w++ = (char) temp;
            wCount = wCount + 2;
            *w = '\0';
            return wCount;
        } else {
            ungetc(temp, stdin);
            wCount++;
            *w++ = (char) iochar;
            *w = '\0';
            return wCount;
        }
    }

    // Loop through input placing characters in *w
    // Switch will break the loop by returning when a given case is met
    // chars are placed back into stdin to get eaten up by begining cases
    for (;;) {

        switch (iochar) {
            // Check for spaces
            case (' '):
                *w = '\0';
                if (dollarCheck)
                    return (-1 * wCount);
                return wCount;
            // Check for ';' then place back into stdin so it is accepted
            // as a leading case. Similar with most switch cases
            case (';'):
                ungetc(iochar, stdin);
                *w = '\0';
                if (dollarCheck)
                    return (-1 * wCount);
                return wCount;

            // Check for '>'
            case ('>'):
                ungetc(iochar, stdin);
                wCount++;
                *w = '\0';
                if (dollarCheck)
                    return (-1 * wCount);
                return wCount;

            // Check for '<'
            case ('<'):
                ungetc(iochar, stdin);
                *w = '\0';
                if (dollarCheck)
                    return (-1 * wCount);
                return wCount;

            // Check for '|'
            case ('|'):
                ungetc(iochar, stdin);
                *w = '\0';
                if (dollarCheck)
                    return (-1 * wCount);
                return wCount;

            // Check for '&'
            case ('&'):
                ungetc(iochar, stdin);
                *w = '\0';
                if (dollarCheck)
                    return (-1 * wCount);
                return wCount;

            // Check for newline
            case ('\n'):
                ungetc(iochar, stdin);
                *w = '\0';
                if (dollarCheck)
                    return (wCount * -1);
                return wCount;

            // Check for EOF
            case (EOF):
                if (wCount >= 0) {
                    *w = '\0';
                    if (dollarCheck)
                        return (wCount * -1);
                    return wCount;
                }

            // Check for '\' and handle newline case
            // iochar will contain special characters and
            // be placed in *w after switch
            case ('\\'):
                temp = getchar();
                if (temp == '\n'){
                    *w = '\0';
                    return wCount;
                } else{
                    iochar = temp;
                }

            default:
                break;
        }

        //passed test cases, add char to w and increment.
        *w++ = (char) iochar;
        wCount++;

        //Check for overflow
        if (wCount == STORAGE - 1) {
            *w = '\0';
            return wCount;
        }

        iochar = getchar();
    }
}
