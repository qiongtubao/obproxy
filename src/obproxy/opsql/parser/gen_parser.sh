#!/bin/bash
#
# DATE: 2016-02-01
# DESCRIPTION:
#
set +x
CURDIR="$(dirname $(readlink -f "$0"))"
export PATH=${CURDIR}/../../../..//deps/3rd/usr/local/oceanbase/devtools/bin/:/usr/local/bin:$PATH
export BISON_PKGDATADIR=${CURDIR}/../../../../deps/3rd/usr/local/oceanbase/devtools/share/bison/
# generate oracle utf8 obproxy_parser(support multi_byte_space, multi_byte_comma, multi_byte_left_parenthesis, multi_byte_right_parenthesis)
##1.copy lex and yacc files
cat ob_proxy_parser.y > ob_proxy_parser_utf8.y
cat ob_proxy_parser.l > ob_proxy_parser_utf8.l
##2.replace name
sed  "s/ob_proxy_parser_yy/ob_proxy_parser_utf8_yy/g" -i ob_proxy_parser_utf8.y
sed  "s/ob_proxy_parser_yy/ob_proxy_parser_utf8_yy/g" -i ob_proxy_parser_utf8.l
sed  "s/ob_proxy_parser_lex/ob_proxy_parser_utf8_lex/g" -i ob_proxy_parser_utf8.y
sed  "s/ob_proxy_parser_lex/ob_proxy_parser_utf8_lex/g" -i ob_proxy_parser_utf8.l
sed  "s/ob_proxy_parser_tab/ob_proxy_parser_utf8_tab/g" -i ob_proxy_parser_utf8.l
sed  "s/ob_proxy_parser_fatal_error/ob_proxy_utf8_parser_fatal_error/g" -i ob_proxy_parser_utf8.y
sed  "s/ob_proxy_parser_fatal_error/ob_proxy_utf8_parser_fatal_error/g" -i ob_proxy_parser_utf8.l
sed  "s/obproxy_parse_sql/obproxy_parse_utf8_sql/g" -i ob_proxy_parser_utf8.y
##3.add multi_byte_space, multi_byte_comma, multi_byte_left_parenthesis, multi_byte_right_parenthesis code.
sed  "s/multi_byte_space              \[\\\u3000\]/multi_byte_space              ([\\\xe3\][\\\x80\][\\\x80])/g" -i ob_proxy_parser_utf8.l
sed  "s/multi_byte_comma              \[\\\uff0c\]/multi_byte_comma              ([\\\xef\][\\\xbc\][\\\x8c])/g" -i ob_proxy_parser_utf8.l
sed  "s/multi_byte_left_parenthesis   \[\\\uff08\]/multi_byte_left_parenthesis   ([\\\xef\][\\\xbc\][\\\x88])/g" -i ob_proxy_parser_utf8.l
sed  "s/multi_byte_right_parenthesis  \[\\\uff09\]/multi_byte_right_parenthesis  ([\\\xef\][\\\xbc\][\\\x89])/g" -i ob_proxy_parser_utf8.l
sed 's/space                   \[ \\t\\n\\r\\f\]/space                   (\[ \\t\\n\\r\\f\]|{multi_byte_space})/g' -i ob_proxy_parser_utf8.l
##4.generate oracle utf8 parser files

# run bison
#bison -p obproxy -v -Werror -d ob_proxy_parser.y -o ob_proxy_parser_tab.c
bison -v -Werror -d ob_proxy_parser_utf8.y -o ob_proxy_parser_utf8_tab.c
if [ $? -ne 0 ]
then
    echo Compile error[$?], abort.
    exit 1
fi

# format tab.h
sed "s/YY\([a-zA-Z_]*\)/OBPROXY\1/g" -i ob_proxy_parser_utf8_tab.h
sed "s/yy\([a-zA-Z_]*\)/obproxy\1/g" -i ob_proxy_parser_utf8_tab.h
sed "/Tokens/i #ifndef YY_OBPROXY_OB_PROXY_PARSER_TAB_H_INCLUDED\n\
# define YY_OBPROXY_OB_PROXY_PARSER_TAB_H_INCLUDED\n\
/* Debug traces.  */\n\
#ifndef OBPROXY_UTF8_DEBUG\n\
# if defined YYDEBUG\n\
#if YYDEBUG\n\
#   define OBPROXY_UTF8_DEBUG 1\n\
#  else\n\
#   define OBPROXY_UTF8_DEBUG 0\n\
#  endif\n\
# else /* ! defined YYDEBUG */\n\
#  define OBPROXY_UTF8_DEBUG 0\n\
# endif /* ! defined YYDEBUG */\n\
#endif  /* ! defined OBPROXY_UTF8_DEBUG */\n\
#if OBPROXY_UTF8_DEBUG\n\
extern int ob_proxy_parser_utf8_yydebug;\n\
#endif" -i ob_proxy_parser_utf8_tab.h
echo "#endif" >> ob_proxy_parser_utf8_tab.h

# formart tab.c
sed "/#define yyparse/i #define YYSTYPE         OBPROXYSTYPE\n#define YYLTYPE         OBPROXYLTYPE" -i ob_proxy_parser_utf8_tab.c
sed "/Tokens/,/Copy the second/{s/YY\([a-zA-Z_]\)/OBPROXY\1/g}" -i ob_proxy_parser_utf8_tab.c
sed "/Tokens/,/Copy the second/{s/yy\([a-zA-Z_]\)/obproxy\1/g}" -i ob_proxy_parser_utf8_tab.c
sed "s/yylex (\&yylval, \&yylloc)/yylex (\&yylval, \&yylloc, YYLEX_PARAM)/g" -i ob_proxy_parser_utf8_tab.c
sed "/Tokens/i #ifndef YY_OBPROXY_OB_PROXY_PARSER_TAB_H_INCLUDED\n\
# define YY_OBPROXY_OB_PROXY_PARSER_TAB_H_INCLUDED\n\
/* Debug traces.  */\n\
#ifndef OBPROXY_UTF8_DEBUG\n\
# if defined YYDEBUG\n\
#if YYDEBUG\n\
#   define OBPROXY_UTF8_DEBUG 1\n\
#  else\n\
#   define OBPROXY_UTF8_DEBUG 0\n\
#  endif\n\
# else /* ! defined YYDEBUG */\n\
#  define OBPROXY_UTF8_DEBUG 0\n\
# endif /* ! defined YYDEBUG */\n\
#endif  /* ! defined OBPROXY_UTF8_DEBUG */\n\
#if OBPROXY_UTF8_DEBUG\n\
extern int ob_proxy_parser_utf8_yydebug;\n\
#endif" -i ob_proxy_parser_utf8_tab.c
sed "/Copy the second/i #endif" -i ob_proxy_parser_utf8_tab.c

# run flex
#flex -P obproxy -Cfea -o ob_proxy_parser_lex.c ob_proxy_parser.l ob_proxy_parser_tab.h
flex -o ob_proxy_parser_utf8_lex.c ob_proxy_parser_utf8.l ob_proxy_parser_utf8_tab.h

# format lex.h
sed "s/YYSTYPE/OBPROXYSTYPE/g" -i ob_proxy_parser_utf8_lex.h
sed "s/YYLTYPE/OBPROXYLTYPE/g" -i ob_proxy_parser_utf8_lex.h
sed "/static int yy_top_state (yyscan_t yyscanner );/d" -i ob_proxy_parser_utf8_lex.c
sed "/static int yy_top_state/,/\}/d" -i ob_proxy_parser_utf8_lex.c
sed "/\*yy_cp = '\\\0';/d" -i ob_proxy_parser_utf8_lex.c
sed "/Setup the input buffer state to scan the given bytes/,/}/{/int i/d}" -i ob_proxy_parser_utf8_lex.c
sed "/Setup the input buffer state to scan the given bytes/,/}/{/for ( i = 0; i < _yybytes_len; ++i )/d}" -i ob_proxy_parser_utf8_lex.c
sed "/Setup the input buffer state to scan the given bytes/,/}/{s/\tbuf\[i\] = yybytes\[i\]/memcpy(buf, yybytes, _yybytes_len)/g}" -i ob_proxy_parser_utf8_lex.c
##5.clean useless files
rm -f ob_proxy_parser_utf8.l
rm -f ob_proxy_parser_utf8.y
rm -f ob_proxy_parser_utf8.output
