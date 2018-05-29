/*
      ページを管理するモジュール

   (1) set_page_basic   を呼び基本的な設定を行う
       set_header_basic

   1 ファイル毎に以下を繰り返す

   (1) start       を呼ぶ
       set_header     
   
   (2) manage_1line ( 1 行印刷せよ ) を繰り返し呼ぶ

   (3) finish を呼ぶ
*/

#include <stdio.h>
#include <string.h>

#include "t2ps.h"

static int max_row,max_block,x_pos[4],y_pos[4];
static int row,page,block,from,to;
static int begin;

static int header_type=0,direction;
static int header_x,header_x2,header_x3,header_x4,header_x5;
static int header_y;
static char fname[100],time_stamp[100];
static double font_size;

/*--------------- 出力は常にこの関数を呼ぶ ------------*/

static int print(char str[])
{
	if ( from <= page && page <= to )  printf("%s",str);
}

static int eprint(char str[])
{
	if ( from <= page && page <= to )  fprintf(stderr,"%s",str);
}

/*----------------- 基本的な設定を行う -----------*/

void set_page_basic(int max_block_, int x_pos_[], int y_pos_[],
					int max_row_, int from_, int to_, int direction_)
{
	int i;

	max_block = max_block_;

	for( i = 0 ; i < max_block ; i++ ){
		x_pos[i] = x_pos_[i];
		y_pos[i]  = y_pos_[i];
	}

	max_row   = max_row_;
	from = from_;
	to = to_;
	direction = direction_;
}

/*----------------- 開始時に呼ぶ -----------------*/

void start()
{
	begin = 0;
	row   = 1;
	page  = 1;
	block = 1;
}

/*---------------- 1 行の処理 -----------------*/

void manage_1line(char instr[])
{
	int i,line;
	char mbuf[MAX_ROW][MAX_COLUMN];
	char str[MAX_COLUMN];
	
	if ( begin == 0 ) {
		fprintf(stderr,"page  :");
		eprint(" 1");
		begin = 1;
		if ( direction == YOKO ){
			sprintf(str,"0 %d translate\n",LONG_SIDE);
			print(str);
			print("-90 rotate\n\n");
		}
		if ( header_type > 0 ) show_title();
		sprintf(str,"/left %d def\n",x_pos[0]);
		print(str);
		sprintf(str,"%d %d moveto\n",x_pos[0],y_pos[0]);
		print(str);
	}

	line = analyze_1line(instr, mbuf);

	for(i=0;i<line;i++){

		sprintf(str,"(%s) show NL\n",&mbuf[i][0]);
		print(str);
		row++;

		if ( row > max_row ){ 
			row = 1;
			block++;
			if ( block > max_block) {
				print("showpage\n");
				page++;
				sprintf(str," %d",page);
				eprint(str);
				if ( direction == YOKO ){
					sprintf(str,"0 %d translate\n",LONG_SIDE);
					print(str);
					print("-90 rotate\n\n");
				}
				if ( header_type > 0 ) show_title();
				block = 1;
			}
			sprintf(str,"/left %d def\n",x_pos[block-1]);
			print(str);
			sprintf(str,"%d %d moveto\n",x_pos[block-1],y_pos[block-1]);
			print(str);
		}
	}
}

/*------------------ 終了時に呼ぶ -------------------*/

void finish()
{
	print("showpage\n");
	fprintf(stderr,"\n");
}


/*----------------- ヘッダ関連 -------------------*/

void set_header_basic(int header_type_, int x1_, int x2_, int x3_,
					  int x4_, int x5_, int y_, double font_size_)
/*
    入力  x1_         印刷領域の左端
          x2_         ファイル名を表示する x 位置
          x3_         時刻を印刷する x 位置
          x4_         ページ番号を印刷する x 位置
          x5_         印刷領域の右端
          y_          ファイル名、時刻を印刷する y 位置
		  font_size_  フォントサイズ
*/
{
	header_type = header_type_;
	header_x  = x1_;
	header_x2 = x2_;
	header_x3 = x3_;
	header_x4 = x4_;
	header_x5 = x5_;
	header_y  = y_;
	font_size = font_size_;
}

void set_header(char fname_[],char time_stamp_[])
/*  
    入力
          fname_      ファイル名
          time_stamp_ 時刻
*/
{
	strcpy(fname,fname_);
	strcpy(time_stamp,time_stamp_);
}


void show_title()
{
	char str[100];

	print("TITLE_BEGIN\n");

	if ( header_type == 2 ){
		print("0.2 setgray\n");
		sprintf(str,"%d %d moveto\n",header_x,header_y);
		print(str);
		sprintf(str,"%f setlinewidth\n",font_size*1.5);
		print(str);
		sprintf(str,"0 %f rmoveto\n",font_size*0.4);
		print(str);
		sprintf(str,"%d 0 rlineto\n",header_x5-header_x);
		print(str);
		print("stroke\n");
		print("1 setgray\n");
	}

	sprintf(str,"%d %d moveto (%s) show\n",
			header_x2,header_y,fname);
	print(str);
	sprintf(str,"%d %d moveto (%s) show\n",
			header_x3,header_y,time_stamp);
	print(str);
	sprintf(str,"%d %d moveto (Page %d) show\n",
			header_x4,header_y,page);
	print(str);

	if ( header_type == 2 ) print("0 setgray\n");

	print("TITLE_END\n");
}

