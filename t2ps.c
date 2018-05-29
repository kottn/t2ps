/*
    テキストファイルを Postscript に変換する
                                              薮 哲郎

              http://denki.nara-edu.ac.jp/~yabu/
              yabu@nara-edu.ac.jp

 (1)  本プログラムは GPL に従います。
 (2)  本プログラムは無保証です。プログラムを使用した
      ことによって生じた損害の責任は負いません。

＜ 使用法 ＞

   % t2ps  を引数なしで実行する

   注意！  本プログラムは nkf を内部で呼び出し、
           入力ファイルを EUC に変換します。

           従って t2ps の実行には nkf が必要です。

           入力ファイルが常に EUC の場合は 'nkf -e' の部分を
		   'cat' として下さい。

＜ コンパイルオプション ＞

#define USE_FILE_DATE 

指定する  場合  ファイルの mtime を表示します。
指定しない場合  現在時刻を表示します。

＜ 製作の動機 ＞

    ひたすらコンパクトに。美しく作りたい・・・

＜ 製作履歴 ＞

  ver 0.0  1999.3.15  製作開始
      1.0       3.16  一応完成
      1.0b      3.17  ページ番号を出力
      1.1       3.18  オプションの追加と整理
                      ヘッダ内文字位置の微調整
                      PS ファイルの整理
                      #define USE_FILE_DATE 追加  
      1.2       3.23  複数ファイル指定時に最上行が 2 行ずつ下へ
                      ずれるバグを修正
      1.3       3.26  /tmp に作成するファイルを消去しないバグを修正
                      break 時に消去するよう signal 処理を追加
      1.4       6.1   tab の処理が正しくないバグを修正
                      ( 謝辞参照 )
      1.4b      8.2   nkf に失敗した時 ( nkf がない or ファイルがない ) に
                      テンポラリファイルを消去しないバグを修正
      1.4c      11.1  -s -e オプション使用時、出力しないページにも
                      TITLE_BEGIN TITLE_END だけを出力していたのを修正
      1.5       12.24 複数ファイルを印刷する時に gsave と grestore の
                      数が対応していなかったバグを修正。
                      ファイルが存在しない場合、以前は nkf でエラーが
                      出ていたが、nkf の手前でファイルの存在をチェックする
                      よう修正
      1.5b 2000.1.7   オプションを指定してファイル名を指定しない場合も
                      usage を表示する
      1.5c      8.11  バッファ溢れを起こした時、警告を出して、終了する
      1.6  2001.5.22  ファイル名 - は標準入力と解釈
                      -N オプションで行番号表示
                      ( 謝辞参照 )
      1.6b      5.23  -m オプションを追加
      1.6c      6.19  -l t1 のときの行間を変更
      1.6d      6.21  -m 処理部のバグを修正 ( 実害は出ていなかった )
      1.6e 2016 12.25 漢字コードを UTF-8 に変更 
      1.7  2017.12.18 ANSI C に対応

＜ 不完全な点 ＞

-N オプション使用時に、複数行にまたがる行が来た場合、
本来ならインデント処理をすべきですが、page.c line.c も
変更する必要があるので、とりあえずこのような簡便な処理に
してあります。

-x オプションでプロポーショナルフォントを使用したとき、
各文字の横幅が異なるので、一行の文字数はその行の内容に
よって、変更する必要があります。それには、PS の Type1 フォントの
各文字の横幅を知ことが必要です。

   1. 各文字の横幅をこのプログラム内にテーブルとして持つ
   2. PS の stringwidth 命令を使う

1. はプログラムするのが繁雑になってしまいます。
2. は本プログラムの理念である「極めて平易なポストスクリプト
ファイルを作る」に反し、t2ps の構造を全く変える必要があるので、
行っていません。

現在は -x オプションを使用するとフォントは切り替わりますが、
一行文字数は Courier の場合に求めた固定数を使用しているので、
-x オプションは事実上、使い物になりません。

＜ 謝辞 ＞

ver 1.3 以前では tab の処理として単純に tab_width だけのスペースに
置換していたため、tab の前に文字がある場合に桁がずれていました。
このバグを、紺野＠アドバンテスト様に修正して頂きました。

ver 1.6 の機能追加は豊田英司さん ( toyoda@gfd-dennou.org ) によって
行われました。

この場を借りて御礼を申し上げます。
*/

#define VERSION  "1.7"
#define REL_DATE "2017/11/18"
#define CREATOR  "Tetsuro Yabu"

/*#define USE_FILE_DATE*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include "t2ps.h"

extern int optind;
extern char *optarg;

static int  exist_tmp_file = 0;
char tmp_fname[1000];

void signal_handler()
{
	if ( exist_tmp_file == 1 ) {
		fprintf(stderr,"terminated. remove tmp-file.\n");
		unlink(tmp_fname);
	}
	exit(1);
}

int main(int argc, char *argv[])
{
	int  ii,i,j,ifile,len,time_len,fd;
	int  max_column,max_row,max_block,tab_width;
	int  column_set_forced,column_buf;
	int  direction,header_type,from,to;
	int  x_pos[4],y_pos[4];
	double font_size,font_aspect,vskip,fbuf;
	int left_margin,right_margin,top_margin,bottom_margin;
	int interval,height,width,width2;
	int number_yn,nshift,line_count;
	int ret;
	char fontname[20],layout[20];
	int c;

	mode_t mode;
	time_t now_time;
	struct stat s_buf;
	
	char fname[100],command[100],time_stamp[100];
	char linebuf[MAX_LINEBUF];

	FILE *fp;

	if ( signal(SIGTERM,signal_handler) == SIG_ERR ){
		perror("signal");
		exit(1);
	}
	if ( signal(SIGINT,signal_handler) == SIG_ERR ){
		perror("signal");
		exit(1);
	}

     /* デフォルト値 */

	column_set_forced = 0;
	from = 1;
	to   = 9999;
	strcpy(layout,"t2");
	header_type = 1;
	tab_width = 4;
	number_yn = 0;
	strcpy(fontname,"Courier");

	set_default(layout,
				&max_block,&direction,
				&left_margin,&right_margin,&top_margin,&bottom_margin,
				&interval,&font_size,&font_aspect,&vskip);
	
	   /* オプション処理

          getopt から return 直後の optind は次に
          読み込むべき argv[i] 位置を示している。
       */

	while((c = getopt(argc,argv,"l:f:a:v:n:t:s:e:k:r:x:hi:c:Nm:"))!= EOF ){
		switch (c){
		case 'l':
			sscanf(optarg,"%s",layout);
			set_default(layout,
						&max_block,&direction,
						&left_margin,&right_margin,&top_margin,&bottom_margin,
						&interval,&font_size,&font_aspect,&vskip);
			break;
		case 'c':
			sscanf(optarg,"%d",&column_buf);
			column_set_forced = 1;
			break;
		case 'f':
			sscanf(optarg,"%lf",&font_size);
			break;
		case 'a':
			sscanf(optarg,"%lf",&font_aspect);
			break;
		case 'v':
			sscanf(optarg,"%lf",&vskip);
			break;
		case 'N':
			number_yn = 1;
			break;
		case 'n':
			sscanf(optarg,"%d",&header_type);
			break;
		case 't':
			sscanf(optarg,"%d",&tab_width);
			break;
		case 's':
			sscanf(optarg,"%d",&from);
			break;
		case 'e':
			sscanf(optarg,"%d",&to);
			break;
		case 'x':
			sscanf(optarg,"%s",fontname);
			break;
		case 'h':
			usage();
			exit(0);
			break;
		case 'i':
			sscanf(optarg,"%lf",&fbuf);
			interval = fbuf / CM2POINT;
			break;
		case 'm':
			sscanf(optarg,"%s",linebuf);
			if ( strncmp(linebuf,"top=",4) == 0 ){
				sscanf(&linebuf[4],"%lf",&fbuf);
				top_margin = fbuf / CM2POINT;
			} else if ( strncmp(linebuf,"bottom=",7) == 0 ){
				sscanf(&linebuf[7],"%lf",&fbuf);
				bottom_margin = fbuf / CM2POINT;
			} else if ( strncmp(linebuf,"left=",5) == 0 ){
				sscanf(&linebuf[5],"%lf",&fbuf);
				left_margin = fbuf / CM2POINT;
			} else if ( strncmp(linebuf,"right=",6) == 0 ){
				sscanf(&linebuf[6],"%lf",&fbuf);
				right_margin = fbuf / CM2POINT;
			}
			break;
		default:
			usage();
			exit(1);
			break;
		}
	}

	if ( optind == argc ) {
		usage();
		exit(1);
	}

       /* x_pos y_pos max_row max_column を求める */	

	if ( direction == TATE ) {
		height = LONG_SIDE - top_margin - bottom_margin;
		width  = SHORT_SIDE - left_margin - right_margin;
		if ( max_block == 1 ) {
			width2 = width;
			x_pos[0] = left_margin;
			y_pos[0] = LONG_SIDE - top_margin - font_size;
		} else {
			width2 = ( width - interval ) / 2;
			x_pos[0] = left_margin;
			y_pos[0] = LONG_SIDE - top_margin - font_size;
			x_pos[1] = x_pos[0] + width2 + interval;
			y_pos[1] = y_pos[0];
		}
	} else if ( direction == YOKO ){
		height = SHORT_SIDE - top_margin - bottom_margin;
		width  = LONG_SIDE - left_margin - right_margin;
		width2 = ( width - interval ) / 2;
		x_pos[0] = left_margin;
		y_pos[0] = SHORT_SIDE - top_margin - font_size;
		x_pos[1] = x_pos[0] + width2 + interval;
		y_pos[1] = y_pos[0];
	} else {
		fprintf(stderr,"direction is not correct. Stop.\n");
		exit(1);
	}

	max_row = ( height-font_size )/( font_size*vskip ) + 1;

	if ( column_set_forced == 0 ){
		max_column = width2 / ( font_size * font_aspect * H_RATIO );
	} else {
		max_column = column_buf;
	}

	fprintf(stderr,"max column : %d\n",max_column);
	fprintf(stderr,"max row    : %d\n",max_row);

	     /* 内部モジュールの初期設定 */

	if ( header_type > 0 ) {

		if ( header_type == 1 ){
			set_header_basic(header_type,
							 x_pos[0],
							 x_pos[0],
							 (int)(x_pos[0]+width-font_size*20),
							 (int)(x_pos[0]+width-font_size*5),
							 x_pos[0]+width,
							 y_pos[0],
							 font_size);
		} else if ( header_type == 2 ) {
			set_header_basic(header_type,
							 x_pos[0],
							 (int)(x_pos[0]+font_size*2*H_RATIO),
							 (int)(x_pos[0]+width-font_size*21),
							 (int)(x_pos[0]+width-font_size*6),
							 x_pos[0]+width,
							 y_pos[0],
							 font_size);
		} else {
			fprintf(stderr,"header_type is not correct. stop.\n");
			exit(1);
		}
		
		/* 2 行分下へずらす */
			
		max_row = max_row - 2;
		for(i=0;i<max_block;i++){
			y_pos[i] = y_pos[i] - font_size * vskip * 2;
		}
	}
	
	set_page_basic(max_block,x_pos,y_pos,max_row,from,to,direction);
	set_line_basic(tab_width,max_column);


/*----------------- ファイル毎の処理 -----------------*/

	for( ii = optind ; ii < argc ; ii++ ){

		    /* ファイル名を取得し、存在をチェック */

		strcpy(fname,argv[ii]);
		fprintf(stderr,"fname : %s\n",fname);
		if ( strcmp(fname,"-") == 0 ){
			fp = stdin;
		} else if ( ( fp = fopen(fname,"r") ) == NULL ){
			fprintf(stderr,"Cannot open '%s'. Skip this file.\n",fname);
			continue;
		}

            /* 日付を得る */

#ifdef USE_FILE_DATE
		if ( fp == stdin ){
			time(&now_time);
			strcpy(time_stamp,ctime(&now_time));
		} else if ( ( fd = open(fname,O_RDONLY,mode) ) != -1 ){
			if ( fstat(fd,&s_buf) == 0 ){
				strcpy(time_stamp,ctime(&s_buf.st_mtime));
			} else {
				fprintf(stderr,"Error in fstat. Stop.\n");
				exit(1);
			}
		} else {
			fprintf(stderr,"Error in open. Stop. fname %s\n",fname);
			exit(1);
		}
#else
		time(&now_time);
		strcpy(time_stamp,ctime(&now_time));
#endif
		time_len = strlen(time_stamp);
		if ( time_stamp[time_len-1] == '\n' ) time_stamp[time_len-1] = '\0';

            /* EUC コードに変換する */

		strcpy(tmp_fname,"/tmp/t2psXXXXXX");
		mkstemp(tmp_fname);
		sprintf(command,"nkf -e %s > %s",fname,tmp_fname);
		ret = system(command);
		if ( ret != 0 ){
			fprintf(stderr,"Cannot execute '%s'. Stop.\n",command);
			unlink(tmp_fname);
			exit(1);
		}
		exist_tmp_file = 1;

		    /* ファイルをオープンする */

		if ( ( fp = fopen(tmp_fname,"r") ) == NULL ){
			fprintf(stderr,"Cannot open '%s'. Stop.\n",tmp_fname);
			exit(1);
		}

            /* ヘッダ情報の書き出し */

		printf("%%!\n");
		printf("%%%%Creator: t2ps ver %s  %s  <by %s>\n",
			   VERSION,REL_DATE,CREATOR);
		printf("%%%%EndComments\n");
		printf("\n");
		printf("gsave\n");
		printf("\n");
		printf("/NL { currentpoint /y exch def pop left y \n");
		printf("      %f sub moveto } def\n",vskip*font_size);
		printf("\n");
		printf("/titlefont /Helvetica-Bold findfont %f scalefont def\n",
			   font_size);
		printf("\n");
		printf("/TITLE_BEGIN {\n");
		printf("   /savefont currentfont def\n");
		printf("   titlefont setfont } def\n");
		printf("\n");
		printf("/TITLE_END {\n");
		printf("   savefont setfont } def\n");
		printf("\n");
		printf("12 dict begin\n");
		printf("/FontName /Myfont def\n");
		printf("/FontType 0 def\n");
		printf("/WMode 0 def\n");
		printf("/FMapType 3 def\n");
		printf("/FontMatrix matrix def\n");
		printf("/Encoding [0 1] def\n");
		printf("/FDepVector\n");
		printf("[ /%s findfont [ %f 0 0 1 0 0 ] makefont\n",fontname,
			   font_aspect);
		printf("  /Ryumin-Light-H findfont [1 0 0 1 0 0 ] makefont\n");
		printf("] def\n");
		printf("FontName currentdict\n");
		printf("end\n");
		printf("\n");
		printf("definefont pop\n");
		printf("\n");
		printf("/Myfont findfont %f scalefont setfont\n",font_size);
		printf("\n");
		printf("%%%%\n");
		printf("%%%%EndProlog\n");
		printf("%%%%\n");
		printf("\n");

		if ( header_type > 0 ) set_header(fname,time_stamp);
		start();

		       /* 行番号をつける場合、行数を求める */

		if ( number_yn == 1 ){
			line_count = 0;
			while(1){
				if ( fgets(linebuf,MAX_LINEBUF,fp) == NULL ) break;
				line_count++;
			}
			nshift = log10(line_count) + 2;
			rewind(fp);
		} else {
			nshift = 0;
		}

               /* 1 ファイルの処理 */

		line_count = 0;
		while(1){
			line_count++;
			if ( number_yn == 1 ) {
				sprintf(linebuf, "%*d ",nshift-1,line_count);
			}
			if ( fgets(&linebuf[nshift],MAX_LINEBUF-nshift,fp) == NULL ) break;
			len = strlen(linebuf);
			if ( len >= MAX_LINEBUF-1 && linebuf[MAX_LINEBUF-2] != '\n' ) {
				fprintf(stderr,"\nMAX_LINEBUF in t2ps.h is too short to manage this file.\n");
				fprintf(stderr,"Unexpected newline occur. Please set MAX_LINEBUF larger and compile t2ps again.\n");
			}
			if ( linebuf[len-1] == '\n' ){
				linebuf[len-1] = '\0';
				len--;
			}
			if ( linebuf[len-1] == '\r' ){  /* MS-DOS ファイルなど \r\n を */
				linebuf[len-1] = '\0';      /* 行末に持つファイルの処理    */ 
			}
			manage_1line(linebuf);
		}

		finish();

		fclose(fp);
		unlink(tmp_fname);
		exist_tmp_file = 0;

		printf("grestore\n");
	}
	return 0;
}

/*------ 紙の方向とブロック数  より デフォルト値を設定する -----*/

void set_default(char layout[],
			int *max_block, int *direction,
			int *left_margin, int *right_margin, int *top_margin, int *bottom_margin,
			int *interval,
			double *font_size, double *font_aspect, double *vskip)
{
	if ( layout[0] == 't' ) {  /* 縦置き */

		*direction = TATE;

		if ( layout[1] == '1' ) {  /* 1 段組 */

			*max_block     = 1;
			*left_margin   = 2.3 / CM2POINT;
			*right_margin  = 1.9 / CM2POINT;
			*top_margin    = 1.1 / CM2POINT;
			*bottom_margin = 1.5 / CM2POINT;
			*font_size     = 9;
			*font_aspect   = 1;
			*vskip         = 1.67;

		} else if ( layout[1] == '2' ) {  /* 2 段組 */

			*max_block     = 2;
			*left_margin   = 1.63 / CM2POINT;
			*right_margin  = 0.49 / CM2POINT;
			*top_margin    = 1.00 / CM2POINT;
			*bottom_margin = 1.4  / CM2POINT;
			*interval      = 0.1  / CM2POINT;
			*font_size     = 7;
			*font_aspect   = 0.88;
			*vskip         = 1.2;

		} else {

			fprintf(stderr,"layout is not correct. Stop.\n");
			exit(1);
			
		}

	} else if ( layout[0] == 'y' ) { /* 横置き */

		if ( layout[1] == '2' ){     /* 2 段組 */

			*direction     = YOKO;
			*max_block     = 2;
			*left_margin   = 1.6 / CM2POINT;
			*right_margin  = 1.6 / CM2POINT;
			*top_margin    = 1.3 / CM2POINT;
			*bottom_margin = 1.0 / CM2POINT;
			*interval      = 0.5 / CM2POINT;
			*font_size     = 8.5;
			*font_aspect   = 1;
			*vskip         = 1.2;

		} else {

			fprintf(stderr,"layout is not correct. Stop.\n");
			exit(1);
			
		}

	} else {

		fprintf(stderr,"layout is not correct. Stop.\n");
		exit(1);

	}
}

void usage()
{
	fprintf(stderr,"\nt2ps ver %s  %s <by %s>\n\n",VERSION,REL_DATE,CREATOR);
	fprintf(stderr,"Usage : %% t2ps [options] filename1 filename2 ....\n\n");
	fprintf(stderr,"        ファイル名として - を指定した場合は標準入力から読み込み\n\n");
	fprintf(stderr,"オプション例\n\n");
	fprintf(stderr,"-l y2        レイアウト t2:縦置き 2 段   t1:縦置き 1 段\n");
	fprintf(stderr,"                        y2:横置き 2 段   デフォルトは t2\n");
	fprintf(stderr,"             フォントサイズ、縦横比、行間隔、ブロック間隔が再定義されます\n");
	fprintf(stderr,"             t2: 7pt 0.88 1.2   t1: 9pt 1.0 1.67  y2: 8.5pt 1.0 1.2\n");
	fprintf(stderr,"-f 10        フォントサイズ [point]\n");
	fprintf(stderr,"-a 0.835     ローマ字フォントの縦横比  縦を 1 としたときの横幅\n");
	fprintf(stderr,"             半角文字 2 文字 = 全角文字 1 文字にするには 0.835\n");
	fprintf(stderr,"-v 1.4       行間隔    デフォルトは -l に依存する\n");
	fprintf(stderr,"-i 1         2 段組のときブロックの間隔 [cm]\n");
	fprintf(stderr,"-c 60        1 行文字数を指定  デフォルトは自動計算\n");
	fprintf(stderr,"-n 0         ヘッダのタイプ 0:なし  1:黒字  2:白抜き文字\n");
	fprintf(stderr,"             デフォルトは 1\n");
	fprintf(stderr,"-t 8         タブの幅  デフォルト 4\n");
	fprintf(stderr,"-s 2         開始ページ\n");
	fprintf(stderr,"-e 5         終了ページ\n");
	fprintf(stderr,"-x Helvetica ローマ字フォント名を指定\n");
	fprintf(stderr,"-N           通しの行番号をつける\n");
	fprintf(stderr,"-m top=2     マージン指定 単位は cm。'=' の前後に空白を入れないで下さい\n");
	fprintf(stderr,"-m bottom=2  マージン指定 単位は cm\n");
	fprintf(stderr,"-m left=2    マージン指定 単位は cm\n");
	fprintf(stderr,"-m right=2   マージン指定 単位は cm\n");
/*	fprintf(stderr,"-r 80        1 ページ行数  デフォルトは自動計算\n");*/
	fprintf(stderr,"\n");
}
