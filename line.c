/*
      1 行を処理するモジュール

  (1) set_line_basic を呼んで基本的な設定を行う

  (2) analyze_1line ( 1 行を解析せよ )  を繰り返し呼ぶ

*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "t2ps.h"

#define ROMAN  1
#define KANJI  2

static int tab_width,max_column;
static int mode = ROMAN;

/*------------- タブ幅と一行の長さを設定する --------------*/

void set_line_basic(int tab_width_, int max_column_)
{
	tab_width = tab_width_;
	max_column = max_column_;
}


/*-------------  一行を処理する ----------------*/

int analyze_1line(char line[], char mbuf[][MAX_COLUMN])
/*
    入力  line        一行の文字列
    出力  mbuf[][]    一行に納まりきらなかった場合は
                      分割する。
                      漢字は 8 進数で表し、エスケープシーケンス
                      などを挿入する。
*/
{
	int i,j,k,l,len,iline,high,low;
	char line2[MAX_LINEBUF2];
	unsigned char cha,cha2;

	len = strlen(line);

       /* tab を空白に置換する */

	j = 0;
	for(i=0;i<len;i++){
		if ( line[i] != '\t' ){
			line2[j] = line[i];
			j++;
		} else {
			l = tab_width - ( j % tab_width );
			for(k=0;k<l;k++){
				line2[j] = ' ';
				j++;
			}
		}
	}
	line2[j] = '\0';

	if ( j >= MAX_LINEBUF2 ) {
		fprintf(stderr,"MAX_LINEBUF2 is too small. Stop.\n");
		exit(1);
	}

       /* 処理を行う */

	i = 0;  /* line2[i] のポインタ */
	j = 0;  /* mbuf[][j] のポインタ */
	k = 0;  /* 現在処理している行の何文字目か */
	iline = 0; /* 今、何行目か */
	mode = ROMAN;  /* show を実行すると ROMAN になってしまう */

	len = strlen(line2);

     /*  1 行文字数が 80 字で、80,81 文字目が
         漢字の場合、入れることにしている
     */

	while(1){

		if ( i >= len ) {
			mbuf[iline][j] = '\0';
			break;
		}
		if ( k >= max_column ) {
			if ( j >= MAX_COLUMN ) {
				fprintf(stderr,"MAX_COLUMN is too small. Stop\n");
				exit(1);
			}
			mbuf[iline][j] = '\0';
			iline++;
			if ( iline >= MAX_ROW ) {
				fprintf(stderr,"MAX_ROW is too small. Stop.\n");
				exit(1);
			}
			j = 0;
			k = 0;
			mode = ROMAN;/* show を実行すると ROMAN になってしまう */
		}
			
		cha = line2[i];
		i++;k++;
		if ( cha <= 127 ) {  /* ローマ字の処理 */
			if ( mode == KANJI ){
				mode = ROMAN;
				mbuf[iline][j  ] = '\\';
				mbuf[iline][j+1] = '3';
				mbuf[iline][j+2] = '7';
				mbuf[iline][j+3] = '7';
				mbuf[iline][j+4] = '\\';
				mbuf[iline][j+5] = '0';
				mbuf[iline][j+6] = '0';
				mbuf[iline][j+7] = '0';
				j = j + 8;
			}
			if ( cha == '\\' || cha == '(' || 
				 cha == ')'  || cha == '%' ) {
				mbuf[iline][j] = '\\';
				mbuf[iline][j+1] = cha;
				j = j + 2;
			} else {
				mbuf[iline][j] = cha;
				j++;
			}
		} else {     /* 漢字の処理 */
			cha2 = line2[i];
			i++;k++;
			if ( mode == ROMAN ){
				mode = KANJI;
				mbuf[iline][j  ] = '\\';
				mbuf[iline][j+1] = '3';
				mbuf[iline][j+2] = '7';
				mbuf[iline][j+3] = '7';
				mbuf[iline][j+4] = '\\';
				mbuf[iline][j+5] = '0';
				mbuf[iline][j+6] = '0';
				mbuf[iline][j+7] = '1';
				j = j + 8;
			}
			high = cha  & 0x7f;
			low  = cha2 & 0x7f;

			write_oct(&mbuf[iline][j],high);
			write_oct(&mbuf[iline][j+4],low);
			j = j + 8;
		}
	}
	return(iline+1);
}


/*--------- 8 進数を書き込む --------*/

void write_oct(char moji[], int value)
{
	int i1,i2,i3;

	i1 = value / 64;
	value = value - 64 * i1;
	i2 = value / 8;
	value = value - 8 * i2;
	i3 = value;

	moji[0] = '\\';
	moji[1] = '0'+i1;
	moji[2] = '0'+i2;
	moji[3] = '0'+i3;
}
