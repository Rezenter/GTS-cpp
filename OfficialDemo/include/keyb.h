/*
kbhit() and getch() for Linux/UNIX
Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
*/

#ifdef LINUX

/*****************************************************************************/
/*  SLEEP  */
/*****************************************************************************/
void Sleep(int t);

/*****************************************************************************/
/*  GETCH  */
/*****************************************************************************/
int getch(void);


/*****************************************************************************/
/*  KBHIT  */
/*****************************************************************************/
int kbhit();

/*****************************************************************************/
/*  _SCANF  */
/*****************************************************************************/
int _scanf(char *fmt, ...);

#endif

