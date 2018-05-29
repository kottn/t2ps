
#define MAX_LINEBUF  4000 /* ファイル読み込み時の 1 行の最大文字数 */
#define MAX_LINEBUF2 5000 /* 上に加えて tab を展開 */

#define MAX_COLUMN 1000 /* page.c <---- line.c 引渡し時の */
#define MAX_ROW    50   /* バッファの大きさ               */

#define CM2POINT  0.035277777

#define LONG_SIDE   840
#define SHORT_SIDE  595

#define TATE  1
#define YOKO  2

#define H_RATIO 0.6  /* Courier のフォントは point × 0.6 が横幅 */

/*--------- 関数プロトタイプ -----------*/
/* t2ps.c */

void set_default(char[], int*, int*, int*, int*, int*, int*, int*,
				 double*, double*, double*);
void usage();

/* line.c */

void set_line_basic(int, int);
int analyze_1line(char[], char[][MAX_COLUMN]);
void write_oct(char[], int);

/* page.c */

void set_page_basic(int, int[], int[], int, int, int, int);
void start();
void manage_1line(char[]);
void finish();
void set_header_basic(int, int, int, int, int, int, int, double);
void set_header(char[],char[]);
void show_title();
