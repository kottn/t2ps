# t2ps
Convert from text-file to postscript-file.

Fork from :link: http://denki.nara-edu.ac.jp/~yabu/soft/original/index.html

The key advantages are below:  
1. Programs and generated PostScript files are extremely simple, easy to modify and customize.
1. You can set the font size and line spacing to your preferred values.
1. You can set the width of English font like "two half-width characters = a full-width character".
1. The vertical two-column set which is very convenient for viewing the code is the default, and about 170 lines fit. In addition, it supports one vertical column and two horizontal columns.

## Usage
```
$ t2ps file1 file2 ... > example.ps
```

## Requirements
* make
* nkf

## Installation
```
$ git clone https:/github.com/kottn/t2ps
$ cd t2ps; make
```

## Help
```
$ t2ps -h
```

## Lisence
GPL
