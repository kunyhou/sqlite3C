/*
** 2000-05-29
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Driver template for the LEMON parser generator.
**
** The "lemon" program processes an LALR(1) input grammar file, then uses
** this template to construct a parser.  The "lemon" program inserts text
** at each "%%" line.  Also, any "P-a-r-s-e" identifer prefix (without the
** interstitial "-" characters) contained in this template is changed into
** the value of the %name directive from the grammar.  Otherwise, the content
** of this template is copied straight through into the generate parser
** source file.
**
** The following is the concatenation of all %include directives from the
** input grammar file:
*/
#include <stdio.h>
/************ Begin %include sections from the grammar ************************/
#line 48 "parse.y"

#include "sqliteInt.h"

/*
** Disable all error recovery processing in the parser push-down
** automaton.
*/
#define YYNOERRORRECOVERY 1

/*
** Make yytestcase() the same as testcase()
*/
#define yytestcase(X) testcase(X)

/*
** Indicate that sqlite3ParserFree() will never be called with a null
** pointer.
*/
#define YYPARSEFREENEVERNULL 1

/*
** Alternative datatype for the argument to the malloc() routine passed
** into sqlite3ParserAlloc().  The default is size_t.
*/
#define YYMALLOCARGTYPE  u64

/*
** An instance of this structure holds information about the
** LIMIT clause of a SELECT statement.
*/
struct LimitVal {
  Expr *pLimit;    /* The LIMIT expression.  NULL if there is no limit */
  Expr *pOffset;   /* The OFFSET expression.  NULL if there is none */
};

/*
** An instance of this structure is used to store the LIKE,
** GLOB, NOT LIKE, and NOT GLOB operators.
*/
struct LikeOp {
  Token eOperator;  /* "like" or "glob" or "regexp" */
  int bNot;         /* True if the NOT keyword is present */
};

/*
** An instance of the following structure describes the event of a
** TRIGGER.  "a" is the event type, one of TK_UPDATE, TK_INSERT,
** TK_DELETE, or TK_INSTEAD.  If the event is of the form
**
**      UPDATE ON (a,b,c)
**
** Then the "b" IdList records the list "a,b,c".
*/
struct TrigEvent { int a; IdList * b; };

/*
** An instance of this structure holds the ATTACH key and the key type.
*/
struct AttachKey { int type;  Token key; };

/*
** Disable lookaside memory allocation for objects that might be
** shared across database connections.
*/
static void disableLookaside(Parse *pParse){
  pParse->disableLookaside++;
  pParse->db->lookaside.bDisable++;
}

#line 413 "parse.y"

  /*
  ** For a compound SELECT statement, make sure p->pPrior->pNext==p for
  ** all elements in the list.  And make sure list length does not exceed
  ** SQLITE_LIMIT_COMPOUND_SELECT.
  */
  static void parserDoubleLinkSelect(Parse *pParse, Select *p){
    if( p->pPrior ){
      Select *pNext = 0, *pLoop;
      int mxSelect, cnt = 0;
      for(pLoop=p; pLoop; pNext=pLoop, pLoop=pLoop->pPrior, cnt++){
        pLoop->pNext = pNext;
        pLoop->selFlags |= SF_Compound;
      }
      if( (p->selFlags & SF_MultiValue)==0 && 
        (mxSelect = pParse->db->aLimit[SQLITE_LIMIT_COMPOUND_SELECT])>0 &&
        cnt>mxSelect
      ){
        sqlite3ErrorMsg(pParse, "too many terms in compound SELECT");
      }
    }
  }
#line 830 "parse.y"

  /* This is a utility routine used to set the ExprSpan.zStart and
  ** ExprSpan.zEnd values of pOut so that the span covers the complete
  ** range of text beginning with pStart and going to the end of pEnd.
  */
  static void spanSet(ExprSpan *pOut, Token *pStart, Token *pEnd){
    pOut->zStart = pStart->z;
    pOut->zEnd = &pEnd->z[pEnd->n];
  }

  /* Construct a new Expr object from a single identifier.  Use the
  ** new Expr to populate pOut.  Set the span of pOut to be the identifier
  ** that created the expression.
  */
  static void spanExpr(ExprSpan *pOut, Parse *pParse, int op, Token t){
    pOut->pExpr = sqlite3PExpr(pParse, op, 0, 0, &t);
    pOut->zStart = t.z;
    pOut->zEnd = &t.z[t.n];
  }
#line 921 "parse.y"

  /* This routine constructs a binary expression node out of two ExprSpan
  ** objects and uses the result to populate a new ExprSpan object.
  */
  static void spanBinaryExpr(
    Parse *pParse,      /* The parsing context.  Errors accumulate here */
    int op,             /* The binary operation */
    ExprSpan *pLeft,    /* The left operand, and output */
    ExprSpan *pRight    /* The right operand */
  ){
    pLeft->pExpr = sqlite3PExpr(pParse, op, pLeft->pExpr, pRight->pExpr, 0);
    pLeft->zEnd = pRight->zEnd;
  }

  /* If doNot is true, then add a TK_NOT Expr-node wrapper around the
  ** outside of *ppExpr.
  */
  static void exprNot(Parse *pParse, int doNot, ExprSpan *pSpan){
    if( doNot ){
      pSpan->pExpr = sqlite3PExpr(pParse, TK_NOT, pSpan->pExpr, 0, 0);
    }
  }
#line 980 "parse.y"

  /* Construct an expression node for a unary postfix operator
  */
  static void spanUnaryPostfix(
    Parse *pParse,         /* Parsing context to record errors */
    int op,                /* The operator */
    ExprSpan *pOperand,    /* The operand, and output */
    Token *pPostOp         /* The operand token for setting the span */
  ){
    pOperand->pExpr = sqlite3PExpr(pParse, op, pOperand->pExpr, 0, 0);
    pOperand->zEnd = &pPostOp->z[pPostOp->n];
  }                           
#line 997 "parse.y"

  /* A routine to convert a binary TK_IS or TK_ISNOT expression into a
  ** unary TK_ISNULL or TK_NOTNULL expression. */
  static void binaryToUnaryIfNull(Parse *pParse, Expr *pY, Expr *pA, int op){
    sqlite3 *db = pParse->db;
    if( pA && pY && pY->op==TK_NULL ){
      pA->op = (u8)op;
      sqlite3ExprDelete(db, pA->pRight);
      pA->pRight = 0;
    }
  }
#line 1025 "parse.y"

  /* Construct an expression node for a unary prefix operator
  */
  static void spanUnaryPrefix(
    ExprSpan *pOut,        /* Write the new expression node here */
    Parse *pParse,         /* Parsing context to record errors */
    int op,                /* The operator */
    ExprSpan *pOperand,    /* The operand */
    Token *pPreOp         /* The operand token for setting the span */
  ){
    pOut->zStart = pPreOp->z;
    pOut->pExpr = sqlite3PExpr(pParse, op, pOperand->pExpr, 0, 0);
    pOut->zEnd = pOperand->zEnd;
  }
#line 1250 "parse.y"

  /* Add a single new term to an ExprList that is used to store a
  ** list of identifiers.  Report an error if the ID list contains
  ** a COLLATE clause or an ASC or DESC keyword, except ignore the
  ** error while parsing a legacy schema.
  */
  static ExprList *parserAddExprIdListTerm(
    Parse *pParse,
    ExprList *pPrior,
    Token *pIdToken,
    int hasCollate,
    int sortOrder
  ){
    ExprList *p = sqlite3ExprListAppend(pParse, pPrior, 0);
    if( (hasCollate || sortOrder!=SQLITE_SO_UNDEFINED)
        && pParse->db->init.busy==0
    ){
      sqlite3ErrorMsg(pParse, "syntax error after column name \"%.*s\"",
                         pIdToken->n, pIdToken->z);
    }
    sqlite3ExprListSetName(pParse, p, pIdToken, 1);
    return p;
  }
#line 228 "parse.c"
/**************** End of %include directives **********************************/
/* These constants specify the various numeric values for terminal symbols
** in a format understandable to "makeheaders".  This section is blank unless
** "lemon" is run with the "-m" command-line option.
***************** Begin makeheaders token definitions *************************/
/**************** End makeheaders token definitions ***************************/

/* The next sections is a series of control #defines.
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used to store the integer codes
**                       that represent terminal and non-terminal symbols.
**                       "unsigned char" is used if there are fewer than
**                       256 symbols.  Larger types otherwise.
**    YYNOCODE           is a number of type YYCODETYPE that is not used for
**                       any terminal or nonterminal symbol.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       (also known as: "terminal symbols") have fall-back
**                       values which should be used if the original symbol
**                       would not parse.  This permits keywords to sometimes
**                       be used as identifiers, for example.
**    YYACTIONTYPE       is the data type used for "action codes" - numbers
**                       that indicate what to do in response to the next
**                       token.
**    sqlite3ParserTOKENTYPE     is the data type used for minor type for terminal
**                       symbols.  Background: A "minor type" is a semantic
**                       value associated with a terminal or non-terminal
**                       symbols.  For example, for an "ID" terminal symbol,
**                       the minor type might be the name of the identifier.
**                       Each non-terminal can have a different minor type.
**                       Terminal symbols all have the same minor type, though.
**                       This macros defines the minor type for terminal 
**                       symbols.
**    YYMINORTYPE        is the data type used for all minor types.
**                       This is typically a union of many types, one of
**                       which is sqlite3ParserTOKENTYPE.  The entry in the union
**                       for terminal symbols is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    sqlite3ParserARG_SDECL     A static variable declaration for the %extra_argument
**    sqlite3ParserARG_PDECL     A parameter declaration for the %extra_argument
**    sqlite3ParserARG_STORE     Code to store %extra_argument into yypParser
**    sqlite3ParserARG_FETCH     Code to extract %extra_argument from yypParser
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YY_MAX_SHIFT       Maximum value for shift actions
**    YY_MIN_SHIFTREDUCE Minimum value for shift-reduce actions
**    YY_MAX_SHIFTREDUCE Maximum value for shift-reduce actions
**    YY_MIN_REDUCE      Maximum value for reduce actions
**    YY_ERROR_ACTION    The yy_action[] code for syntax error
**    YY_ACCEPT_ACTION   The yy_action[] code for accept
**    YY_NO_ACTION       The yy_action[] code for no-op
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/************* Begin control #defines *****************************************/
#define YYCODETYPE unsigned char
#define YYNOCODE 251
#define YYACTIONTYPE unsigned short int
#define YYWILDCARD 70
#define sqlite3ParserTOKENTYPE Token
typedef union {
  int yyinit;
  sqlite3ParserTOKENTYPE yy0;
  struct LimitVal yy64;
  Expr* yy122;
  Select* yy159;
  IdList* yy180;
  struct {int value; int mask;} yy207;
  struct LikeOp yy318;
  TriggerStep* yy327;
  With* yy331;
  ExprSpan yy342;
  SrcList* yy347;
  int yy392;
  struct TrigEvent yy410;
  ExprList* yy442;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define sqlite3ParserARG_SDECL Parse *pParse;
#define sqlite3ParserARG_PDECL ,Parse *pParse
#define sqlite3ParserARG_FETCH Parse *pParse = yypParser->pParse
#define sqlite3ParserARG_STORE yypParser->pParse = pParse
#define YYFALLBACK 1
#define YYNSTATE             440
#define YYNRULE              326
#define YY_MAX_SHIFT         439
#define YY_MIN_SHIFTREDUCE   649
#define YY_MAX_SHIFTREDUCE   974
#define YY_MIN_REDUCE        975
#define YY_MAX_REDUCE        1300
#define YY_ERROR_ACTION      1301
#define YY_ACCEPT_ACTION     1302
#define YY_NO_ACTION         1303
/************* End control #defines *******************************************/

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N <= YY_MAX_SHIFT             Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   N between YY_MIN_SHIFTREDUCE       Shift to an arbitrary state then
**     and YY_MAX_SHIFTREDUCE           reduce by rule N-YY_MIN_SHIFTREDUCE.
**
**   N between YY_MIN_REDUCE            Reduce by rule N-YY_MIN_REDUCE
**     and YY_MAX_REDUCE

**   N == YY_ERROR_ACTION               A syntax error has occurred.
**
**   N == YY_ACCEPT_ACTION              The parser accepts its input.
**
**   N == YY_NO_ACTION                  No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.  
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
**
*********** Begin parsing tables **********************************************/
#define YY_ACTTAB_COUNT (1499)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   315, 1302,  146,  921,    2,  194,  922,  342,  952,   91,
 /*    10 */    91,   91,   91,   84,   89,   89,   89,   89,   88,   88,
 /*    20 */    87,   87,   87,   86,  339,   87,   87,   87,   86,  339,
 /*    30 */   331,  819,  819,   91,   91,   91,   91,  339,   89,   89,
 /*    40 */    89,   89,   88,   88,   87,   87,   87,   86,  339,  319,
 /*    50 */   933,  933,   92,   93,   83,  831,  834,  823,  823,   90,
 /*    60 */    90,   91,   91,   91,   91,  123,   89,   89,   89,   89,
 /*    70 */    88,   88,   87,   87,   87,   86,  339,  315,  952,   89,
 /*    80 */    89,   89,   89,   88,   88,   87,   87,   87,   86,  339,
 /*    90 */   365,  772,  360,   24,  933,  933,  947,  694,  933,  933,
 /*   100 */   773,  937,  933,  933,  434,  715,  328,  434,  819,  819,
 /*   110 */   203,  160,  278,  391,  273,  390,  190,  933,  933,  370,
 /*   120 */   934,  935,  367,  271,  953,   48,  679,  953,   48,   92,
 /*   130 */    93,   83,  831,  834,  823,  823,   90,   90,   91,   91,
 /*   140 */    91,   91,  123,   89,   89,   89,   89,   88,   88,   87,
 /*   150 */    87,   87,   86,  339,  315,  682,  337,  336,  218,  412,
 /*   160 */   398,   68,  412,  403,  934,  935,  743,  959,  934,  935,
 /*   170 */   810,  937,  934,  935,  957,  221,  958,   88,   88,   87,
 /*   180 */    87,   87,   86,  339,  291,  819,  819,  934,  935,  185,
 /*   190 */    94,  792,  388,  385,  384, 1240, 1240,  792,  804,  960,
 /*   200 */   960,  290,  798,  383,  123,  315,   92,   93,   83,  831,
 /*   210 */   834,  823,  823,   90,   90,   91,   91,   91,   91,  326,
 /*   220 */    89,   89,   89,   89,   88,   88,   87,   87,   87,   86,
 /*   230 */   339,  681,  741,  803,  803,  803,  819,  819,  944,   56,
 /*   240 */   253,  353,  242,   85,   82,  168,  253,  358,  252,  110,
 /*   250 */    96,  233,  397,  698,  677,  683,  683,   92,   93,   83,
 /*   260 */   831,  834,  823,  823,   90,   90,   91,   91,   91,   91,
 /*   270 */   433,   89,   89,   89,   89,   88,   88,   87,   87,   87,
 /*   280 */    86,  339,  315,  434,  439,  651,  396,   57,  733,  733,
 /*   290 */   234,  291,  107,  287,  395,   86,  339,  810,  427,  728,
 /*   300 */   933,  933,  185,  953,   30,  388,  385,  384,  215,  949,
 /*   310 */   434,  933,  933,  819,  819,  697,  383,  162,  161,  407,
 /*   320 */   400,   85,   82,  168,  677,  804,  335,  113,  771,  798,
 /*   330 */   953,   48,   22,  351,   92,   93,   83,  831,  834,  823,
 /*   340 */   823,   90,   90,   91,   91,   91,   91,  870,   89,   89,
 /*   350 */    89,   89,   88,   88,   87,   87,   87,   86,  339,  315,
 /*   360 */   803,  803,  803,  268,  123,  412,  394,    1,  933,  933,
 /*   370 */   934,  935,  933,  933,   85,   82,  168,  232,    5,  343,
 /*   380 */   194,  934,  935,  952,   85,   82,  168,   54,  956,  434,
 /*   390 */   819,  819,  431,  938,  939,  792,   67,  759,  350,  144,
 /*   400 */   166,  770,  123,  896,  889,  955,  348,  288,  758,  953,
 /*   410 */    47,   92,   93,   83,  831,  834,  823,  823,   90,   90,
 /*   420 */    91,   91,   91,   91,  892,   89,   89,   89,   89,   88,
 /*   430 */    88,   87,   87,   87,   86,  339,  315,  113,  934,  935,
 /*   440 */   687,  893,  934,  935,  253,  358,  252,   85,   82,  168,
 /*   450 */   820,  820,  956,  952,  338,  938,  939,  894,  701,  721,
 /*   460 */   359,  289,  233,  397,  434,  349,  434,  819,  819,  955,
 /*   470 */   866,  722,   23,  389,  832,  835,  692,  357,  904,  667,
 /*   480 */   194,  702,  402,  952,  953,   48,  953,   48,   92,   93,
 /*   490 */    83,  831,  834,  823,  823,   90,   90,   91,   91,   91,
 /*   500 */    91,  824,   89,   89,   89,   89,   88,   88,   87,   87,
 /*   510 */    87,   86,  339,  315,  434,  113,  434,  680,  434,  332,
 /*   520 */   434,  408,  889,  356,  380,  940,  401,  720,  948,  864,
 /*   530 */   191,  165,  329,  689,  953,    9,  953,    9,  953,    9,
 /*   540 */   953,    9,  718,  948,  819,  819,  953,    8,  325,  111,
 /*   550 */   327,  153,  224,  952,  410,  113,  189,  337,  336,  913,
 /*   560 */  1295,  852,   75, 1295,   73,   92,   93,   83,  831,  834,
 /*   570 */   823,  823,   90,   90,   91,   91,   91,   91,  359,   89,
 /*   580 */    89,   89,   89,   88,   88,   87,   87,   87,   86,  339,
 /*   590 */   315,  730,  148,  236,  797,  366,  789,  892, 1179,  434,
 /*   600 */   960,  960,  400,  148,  314,  212,  873,  911,  757,  404,
 /*   610 */   872,  300,  320,  434,  893,  311,  237,  271,  405,  953,
 /*   620 */    34,  819,  819,  225,  371,  945,  360,  913, 1296,  113,
 /*   630 */   894, 1296,  417,  953,   35, 1245,  922,  342,  259,  247,
 /*   640 */   290,  315,   92,   93,   83,  831,  834,  823,  823,   90,
 /*   650 */    90,   91,   91,   91,   91,  148,   89,   89,   89,   89,
 /*   660 */    88,   88,   87,   87,   87,   86,  339,  310,  434,  796,
 /*   670 */   434,  240,  819,  819,  266,  911,  876,  876,  373,  346,
 /*   680 */   167,  654,  655,  656,  259,  244,   19,  246,  953,   11,
 /*   690 */   953,   26,  222,   92,   93,   83,  831,  834,  823,  823,
 /*   700 */    90,   90,   91,   91,   91,   91,  757,   89,   89,   89,
 /*   710 */    89,   88,   88,   87,   87,   87,   86,  339,  315,  434,
 /*   720 */   261,  434,  264,  696,  434,  241,  434,  344,  971,  308,
 /*   730 */   757,  434,  796,  434,  324,  434,  393,  423,  434,  953,
 /*   740 */    36,  953,   37,   20,  953,   38,  953,   27,  434,  819,
 /*   750 */   819,  953,   28,  953,   39,  953,   40,  738,  953,   41,
 /*   760 */    71,  738,  737,  245,  307,  973,  737,  259,  953,   10,
 /*   770 */    92,   93,   83,  831,  834,  823,  823,   90,   90,   91,
 /*   780 */    91,   91,   91,  434,   89,   89,   89,   89,   88,   88,
 /*   790 */    87,   87,   87,   86,  339,  315,  434,  372,  434,  259,
 /*   800 */   149,  434,  167,  953,   42,  188,  187,  186,  219,  434,
 /*   810 */   748,  434,  974,  434,  796,  434,  953,   98,  953,   43,
 /*   820 */   862,  953,   44,  434,  920,    2,  819,  819,  757,  953,
 /*   830 */    31,  953,   45,  953,   46,  953,   32,   74,  307,  912,
 /*   840 */   220,  259,  259,  953,  115,  909,  315,   92,   93,   83,
 /*   850 */   831,  834,  823,  823,   90,   90,   91,   91,   91,   91,
 /*   860 */   434,   89,   89,   89,   89,   88,   88,   87,   87,   87,
 /*   870 */    86,  339,  434,  248,  434,  215,  949,  819,  819,  333,
 /*   880 */   953,  116,  895,  860,  176,  259,  974,  400,  361,  259,
 /*   890 */   951,  887,  953,  117,  953,   52,  884,  315,   92,   93,
 /*   900 */    83,  831,  834,  823,  823,   90,   90,   91,   91,   91,
 /*   910 */    91,  434,   89,   89,   89,   89,   88,   88,   87,   87,
 /*   920 */    87,   86,  339,  434,  113,  434,  258,  883,  819,  819,
 /*   930 */   727,  953,   33,  363,  259,  673,  321,  189,  430,  321,
 /*   940 */   368,  365,  364,  953,   99,  953,   49,  365,  315,   92,
 /*   950 */    81,   83,  831,  834,  823,  823,   90,   90,   91,   91,
 /*   960 */    91,   91,  434,   89,   89,   89,   89,   88,   88,   87,
 /*   970 */    87,   87,   86,  339,  434,  723,  434,  214,  165,  819,
 /*   980 */   819,  772,  953,  100,  322,  124, 1269,  158,   65,  710,
 /*   990 */   773,  700,  699,  320,  953,  101,  953,   97,  255,  315,
 /*  1000 */   216,   93,   83,  831,  834,  823,  823,   90,   90,   91,
 /*  1010 */    91,   91,   91,  434,   89,   89,   89,   89,   88,   88,
 /*  1020 */    87,   87,   87,   86,  339,  434,  251,  434,  707,  708,
 /*  1030 */   819,  819,  223,  953,  114,  908,  794,  254,  309,  193,
 /*  1040 */    67,  381,  869,  869,  199,  953,  112,  953,  105,  269,
 /*  1050 */   726,  260,   67,   83,  831,  834,  823,  823,   90,   90,
 /*  1060 */    91,   91,   91,   91,  263,   89,   89,   89,   89,   88,
 /*  1070 */    88,   87,   87,   87,   86,  339,   79,  429,  690,    3,
 /*  1080 */  1174,  228,  434,  113,  340,  340,  868,  868,  265,   79,
 /*  1090 */   429,  735,    3,  859,   70,  432,  434,  340,  340,  434,
 /*  1100 */  1259,  434,  953,  104,  434,  670,  416,  766,  432,  434,
 /*  1110 */   193,  434,  413,  434,  418,  806,  953,  102,  420,  953,
 /*  1120 */   103,  953,   48,  123,  953,   51,  810,  418,  424,  953,
 /*  1130 */    53,  953,   50,  953,   25,  267,  123,  711,  113,  810,
 /*  1140 */   428,  277,  695,  272,  764,  113,   76,   77,  690,  434,
 /*  1150 */   795,  113,  276,   78,  436,  435,  412,  414,  798,   76,
 /*  1160 */    77,  113,  855,  859,  376,  199,   78,  436,  435,  953,
 /*  1170 */    29,  798,  744,  113,  755,   79,  429,  675,    3,  415,
 /*  1180 */   109,  292,  293,  340,  340,  806,  802,  678,  672,  803,
 /*  1190 */   803,  803,  805,   18,  432,  661,  660,  662,  927,  209,
 /*  1200 */   150,  352,  803,  803,  803,  805,   18,    6,  306,  280,
 /*  1210 */   282,  284,  786,  418,  250,  386,  243,  886,  694,  362,
 /*  1220 */   286,  163,  275,   79,  429,  810,    3,  857,  856,  159,
 /*  1230 */   419,  340,  340,  298,  930,  968,  126,  196,  965,  903,
 /*  1240 */   901,  323,  432,  136,   55,   76,   77,  742,  147,   58,
 /*  1250 */   121,  129,   78,  436,  435,   65,  783,  798,  354,  131,
 /*  1260 */   355,  418,  379,  132,  133,  134,  175,  139,  151,  369,
 /*  1270 */   888,  180,  791,  810,   61,  851,  871,   69,  429,  375,
 /*  1280 */     3,  756,  210,  257,  181,  340,  340,  145,  803,  803,
 /*  1290 */   803,  805,   18,   76,   77,  377,  432,  262,  182,  183,
 /*  1300 */    78,  436,  435,  663,  312,  798,  392,  714,  713,  712,
 /*  1310 */   330,  705,  692,  313,  704,  418,  686,  406,  752,  685,
 /*  1320 */   274,  684,  942,   64,  279,  195,  281,  810,  753,  839,
 /*  1330 */   751,  283,   72,  750,  285,  422,  803,  803,  803,  805,
 /*  1340 */    18,  334,  426,   95,  411,  229,  409,   76,   77,  230,
 /*  1350 */   734,   66,  231,  294,   78,  436,  435,  204,  295,  798,
 /*  1360 */   217,  296,  297,  669,   21,  305,  304,  303,  206,  301,
 /*  1370 */   437,  928,  664,  205,  208,  207,  438,  658,  657,  652,
 /*  1380 */   118,  108,  119,  226,  650,  341,  157,  170,  169,  239,
 /*  1390 */   803,  803,  803,  805,   18,  125,  120,  235,  238,  317,
 /*  1400 */   318,  345,  106,  790,  867,  127,  865,  128,  130,  724,
 /*  1410 */   249,  172,  174,  882,  135,  137,   59,  138,  173,   60,
 /*  1420 */   885,  123,  171,  177,  178,  881,    7,   12,  179,  256,
 /*  1430 */   874,  140,  193,  962,  374,  141,  666,  152,  378,  276,
 /*  1440 */   184,  382,  142,  122,   62,   13,  387,  703,  270,   14,
 /*  1450 */    63,  227,  809,  808,  837,  732,   15,  841,  736,    4,
 /*  1460 */   765,  211,  399,  164,  213,  143,  760,  201,   70,  316,
 /*  1470 */    67,  838,  836,  891,  198,  192,   16,  197,  890,  917,
 /*  1480 */   154,   17,  202,  421,  918,  155,  200,  156,  425,  840,
 /*  1490 */   807, 1261,  676,   80,  302,  299,  347, 1260,  923,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    19,  144,  145,  146,  147,   24,    1,    2,   27,   80,
 /*    10 */    81,   82,   83,   84,   85,   86,   87,   88,   89,   90,
 /*    20 */    91,   92,   93,   94,   95,   91,   92,   93,   94,   95,
 /*    30 */    19,   50,   51,   80,   81,   82,   83,   95,   85,   86,
 /*    40 */    87,   88,   89,   90,   91,   92,   93,   94,   95,  157,
 /*    50 */    27,   28,   71,   72,   73,   74,   75,   76,   77,   78,
 /*    60 */    79,   80,   81,   82,   83,   66,   85,   86,   87,   88,
 /*    70 */    89,   90,   91,   92,   93,   94,   95,   19,   97,   85,
 /*    80 */    86,   87,   88,   89,   90,   91,   92,   93,   94,   95,
 /*    90 */   152,   33,  152,   22,   27,   28,  179,  180,   27,   28,
 /*   100 */    42,   27,   27,   28,  152,  188,   95,  152,   50,   51,
 /*   110 */    99,  100,  101,  102,  103,  104,  105,   27,   28,  227,
 /*   120 */    97,   98,  230,  112,  172,  173,  172,  172,  173,   71,
 /*   130 */    72,   73,   74,   75,   76,   77,   78,   79,   80,   81,
 /*   140 */    82,   83,   66,   85,   86,   87,   88,   89,   90,   91,
 /*   150 */    92,   93,   94,   95,   19,  172,   89,   90,  218,  207,
 /*   160 */   208,   26,  207,  208,   97,   98,   91,  100,   97,   98,
 /*   170 */    69,   97,   97,   98,  107,  237,  109,   89,   90,   91,
 /*   180 */    92,   93,   94,   95,  152,   50,   51,   97,   98,   99,
 /*   190 */    55,   59,  102,  103,  104,  119,  120,   59,   97,  132,
 /*   200 */   133,  152,  101,  113,   66,   19,   71,   72,   73,   74,
 /*   210 */    75,   76,   77,   78,   79,   80,   81,   82,   83,  187,
 /*   220 */    85,   86,   87,   88,   89,   90,   91,   92,   93,   94,
 /*   230 */    95,  172,  210,  132,  133,  134,   50,   51,  185,   53,
 /*   240 */   108,  109,  110,  221,  222,  223,  108,  109,  110,   22,
 /*   250 */    22,  119,  120,  181,   27,   27,   28,   71,   72,   73,
 /*   260 */    74,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   270 */   152,   85,   86,   87,   88,   89,   90,   91,   92,   93,
 /*   280 */    94,   95,   19,  152,  148,  149,  115,   24,  117,  118,
 /*   290 */   154,  152,  156,  152,  163,   94,   95,   69,  249,  163,
 /*   300 */    27,   28,   99,  172,  173,  102,  103,  104,  194,  195,
 /*   310 */   152,   27,   28,   50,   51,  181,  113,   89,   90,  152,
 /*   320 */   206,  221,  222,  223,   97,   97,  187,  196,  175,  101,
 /*   330 */   172,  173,  196,  219,   71,   72,   73,   74,   75,   76,
 /*   340 */    77,   78,   79,   80,   81,   82,   83,   11,   85,   86,
 /*   350 */    87,   88,   89,   90,   91,   92,   93,   94,   95,   19,
 /*   360 */   132,  133,  134,   23,   66,  207,  208,   22,   27,   28,
 /*   370 */    97,   98,   27,   28,  221,  222,  223,  199,   22,  243,
 /*   380 */    24,   97,   98,   27,  221,  222,  223,  209,  152,  152,
 /*   390 */    50,   51,  168,  169,  170,   59,   26,  124,  100,   58,
 /*   400 */   152,  175,   66,  240,  163,  169,  170,  152,  124,  172,
 /*   410 */   173,   71,   72,   73,   74,   75,   76,   77,   78,   79,
 /*   420 */    80,   81,   82,   83,   12,   85,   86,   87,   88,   89,
 /*   430 */    90,   91,   92,   93,   94,   95,   19,  196,   97,   98,
 /*   440 */    23,   29,   97,   98,  108,  109,  110,  221,  222,  223,
 /*   450 */    50,   51,  152,   97,  168,  169,  170,   45,   37,   47,
 /*   460 */   219,  224,  119,  120,  152,  229,  152,   50,   51,  169,
 /*   470 */   170,   59,  231,   52,   74,   75,  106,  236,  152,   21,
 /*   480 */    24,   60,  163,   27,  172,  173,  172,  173,   71,   72,
 /*   490 */    73,   74,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   500 */    83,  101,   85,   86,   87,   88,   89,   90,   91,   92,
 /*   510 */    93,   94,   95,   19,  152,  196,  152,   23,  152,  207,
 /*   520 */   152,  207,  163,   65,   19,  171,  152,  190,  191,  229,
 /*   530 */   211,  212,  111,  179,  172,  173,  172,  173,  172,  173,
 /*   540 */   172,  173,  190,  191,   50,   51,  172,  173,  186,   22,
 /*   550 */   186,   24,  186,   97,  186,  196,   51,   89,   90,   22,
 /*   560 */    23,  103,  137,   26,  139,   71,   72,   73,   74,   75,
 /*   570 */    76,   77,   78,   79,   80,   81,   82,   83,  219,   85,
 /*   580 */    86,   87,   88,   89,   90,   91,   92,   93,   94,   95,
 /*   590 */    19,  195,  152,  152,   23,  236,  163,   12,  140,  152,
 /*   600 */   132,  133,  206,  152,  164,   23,   31,   70,   26,   19,
 /*   610 */    35,  160,  107,  152,   29,  164,  152,  112,   28,  172,
 /*   620 */   173,   50,   51,  183,   49,  185,  152,   22,   23,  196,
 /*   630 */    45,   26,   47,  172,  173,    0,    1,    2,  152,   16,
 /*   640 */   152,   19,   71,   72,   73,   74,   75,   76,   77,   78,
 /*   650 */    79,   80,   81,   82,   83,  152,   85,   86,   87,   88,
 /*   660 */    89,   90,   91,   92,   93,   94,   95,  164,  152,  152,
 /*   670 */   152,  152,   50,   51,   16,   70,  108,  109,  110,  193,
 /*   680 */    98,    7,    8,    9,  152,   62,   22,   64,  172,  173,
 /*   690 */   172,  173,  218,   71,   72,   73,   74,   75,   76,   77,
 /*   700 */    78,   79,   80,   81,   82,   83,  124,   85,   86,   87,
 /*   710 */    88,   89,   90,   91,   92,   93,   94,   95,   19,  152,
 /*   720 */    62,  152,   64,  181,  152,  193,  152,  241,  246,  247,
 /*   730 */    26,  152,  152,  152,  217,  152,   91,  249,  152,  172,
 /*   740 */   173,  172,  173,   79,  172,  173,  172,  173,  152,   50,
 /*   750 */    51,  172,  173,  172,  173,  172,  173,  116,  172,  173,
 /*   760 */   138,  116,  121,  140,   22,   23,  121,  152,  172,  173,
 /*   770 */    71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*   780 */    81,   82,   83,  152,   85,   86,   87,   88,   89,   90,
 /*   790 */    91,   92,   93,   94,   95,   19,  152,  217,  152,  152,
 /*   800 */    24,  152,   98,  172,  173,  108,  109,  110,  193,  152,
 /*   810 */   213,  152,   70,  152,  152,  152,  172,  173,  172,  173,
 /*   820 */   152,  172,  173,  152,  146,  147,   50,   51,  124,  172,
 /*   830 */   173,  172,  173,  172,  173,  172,  173,  138,   22,   23,
 /*   840 */   193,  152,  152,  172,  173,  152,   19,   71,   72,   73,
 /*   850 */    74,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   860 */   152,   85,   86,   87,   88,   89,   90,   91,   92,   93,
 /*   870 */    94,   95,  152,  152,  152,  194,  195,   50,   51,  217,
 /*   880 */   172,  173,  193,  193,   26,  152,   70,  206,  152,  152,
 /*   890 */    26,  163,  172,  173,  172,  173,  152,   19,   71,   72,
 /*   900 */    73,   74,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   910 */    83,  152,   85,   86,   87,   88,   89,   90,   91,   92,
 /*   920 */    93,   94,   95,  152,  196,  152,  193,  152,   50,   51,
 /*   930 */   193,  172,  173,   19,  152,  166,  167,   51,  166,  167,
 /*   940 */   152,  152,   28,  172,  173,  172,  173,  152,   19,   71,
 /*   950 */    72,   73,   74,   75,   76,   77,   78,   79,   80,   81,
 /*   960 */    82,   83,  152,   85,   86,   87,   88,   89,   90,   91,
 /*   970 */    92,   93,   94,   95,  152,  193,  152,  211,  212,   50,
 /*   980 */    51,   33,  172,  173,  244,  245,   23,  123,  130,   26,
 /*   990 */    42,  100,  101,  107,  172,  173,  172,  173,  152,   19,
 /*  1000 */    22,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*  1010 */    81,   82,   83,  152,   85,   86,   87,   88,   89,   90,
 /*  1020 */    91,   92,   93,   94,   95,  152,  237,  152,    7,    8,
 /*  1030 */    50,   51,  237,  172,  173,   23,   23,   23,   26,   26,
 /*  1040 */    26,   23,  132,  133,   26,  172,  173,  172,  173,   23,
 /*  1050 */   163,  152,   26,   73,   74,   75,   76,   77,   78,   79,
 /*  1060 */    80,   81,   82,   83,  152,   85,   86,   87,   88,   89,
 /*  1070 */    90,   91,   92,   93,   94,   95,   19,   20,   27,   22,
 /*  1080 */    23,  210,  152,  196,   27,   28,  132,  133,  152,   19,
 /*  1090 */    20,   23,   22,   27,   26,   38,  152,   27,   28,  152,
 /*  1100 */   122,  152,  172,  173,  152,  163,  191,   23,   38,  152,
 /*  1110 */    26,  152,  163,  152,   57,   27,  172,  173,  163,  172,
 /*  1120 */   173,  172,  173,   66,  172,  173,   69,   57,  163,  172,
 /*  1130 */   173,  172,  173,  172,  173,  152,   66,  152,  196,   69,
 /*  1140 */   163,  101,  152,  152,  152,  196,   89,   90,   97,  152,
 /*  1150 */   152,  196,  112,   96,   97,   98,  207,  208,  101,   89,
 /*  1160 */    90,  196,   23,   97,  233,   26,   96,   97,   98,  172,
 /*  1170 */   173,  101,  152,  196,  152,   19,   20,   23,   22,  152,
 /*  1180 */    26,  152,  152,   27,   28,   97,  152,  152,  152,  132,
 /*  1190 */   133,  134,  135,  136,   38,  152,  152,  152,  152,  232,
 /*  1200 */   197,  214,  132,  133,  134,  135,  136,  198,  150,  210,
 /*  1210 */   210,  210,  201,   57,  238,  176,  214,  201,  180,  238,
 /*  1220 */   214,  184,  175,   19,   20,   69,   22,  175,  175,  198,
 /*  1230 */   226,   27,   28,  200,  155,   39,  242,  122,   41,  159,
 /*  1240 */   159,  159,   38,   22,  239,   89,   90,   91,  220,  239,
 /*  1250 */    71,  189,   96,   97,   98,  130,  201,  101,   18,  192,
 /*  1260 */   159,   57,   18,  192,  192,  192,  158,  189,  220,  159,
 /*  1270 */   201,  158,  189,   69,  137,  201,  235,   19,   20,   46,
 /*  1280 */    22,  159,  159,  234,  158,   27,   28,   22,  132,  133,
 /*  1290 */   134,  135,  136,   89,   90,  177,   38,  159,  158,  158,
 /*  1300 */    96,   97,   98,  159,  177,  101,  107,  174,  174,  174,
 /*  1310 */    48,  182,  106,  177,  182,   57,  174,  125,  216,  176,
 /*  1320 */   174,  174,  174,  107,  215,  159,  215,   69,  216,  159,
 /*  1330 */   216,  215,  137,  216,  215,  177,  132,  133,  134,  135,
 /*  1340 */   136,   95,  177,  129,  126,  225,  127,   89,   90,  228,
 /*  1350 */   205,  128,  228,  204,   96,   97,   98,   25,  203,  101,
 /*  1360 */     5,  202,  201,  162,   26,   10,   11,   12,   13,   14,
 /*  1370 */   161,   13,   17,  153,    6,  153,  151,  151,  151,  151,
 /*  1380 */   165,  178,  165,  178,    4,    3,   22,   32,   15,   34,
 /*  1390 */   132,  133,  134,  135,  136,  245,  165,  142,   43,  248,
 /*  1400 */   248,   68,   16,  120,   23,  131,   23,  111,  123,   20,
 /*  1410 */    16,   56,  125,    1,  123,  131,   79,  111,   63,   79,
 /*  1420 */    28,   66,   67,   36,  122,    1,    5,   22,  107,  140,
 /*  1430 */    54,   54,   26,   61,   44,  107,   20,   24,   19,  112,
 /*  1440 */   105,   53,   22,   40,   22,   22,   53,   30,   23,   22,
 /*  1450 */    22,   53,   23,   23,   23,  116,   22,   11,   23,   22,
 /*  1460 */    28,   23,   26,  122,   23,   22,  124,  122,   26,  114,
 /*  1470 */    26,   23,   23,   23,   22,   36,   36,   26,   23,   23,
 /*  1480 */    22,   36,  122,   24,   23,   22,   26,   22,   24,   23,
 /*  1490 */    23,  122,   23,   22,   15,   23,  141,  122,    1,
};
#define YY_SHIFT_USE_DFLT (-72)
#define YY_SHIFT_COUNT (439)
#define YY_SHIFT_MIN   (-71)
#define YY_SHIFT_MAX   (1497)
static const short yy_shift_ofst[] = {
 /*     0 */     5, 1057, 1355, 1070, 1204, 1204, 1204,  138,  -19,   58,
 /*    10 */    58,  186, 1204, 1204, 1204, 1204, 1204, 1204, 1204,   67,
 /*    20 */    67,   90,  132,  336,   76,  135,  263,  340,  417,  494,
 /*    30 */   571,  622,  699,  776,  827,  827,  827,  827,  827,  827,
 /*    40 */   827,  827,  827,  827,  827,  827,  827,  827,  827,  878,
 /*    50 */   827,  929,  980,  980, 1156, 1204, 1204, 1204, 1204, 1204,
 /*    60 */  1204, 1204, 1204, 1204, 1204, 1204, 1204, 1204, 1204, 1204,
 /*    70 */  1204, 1204, 1204, 1204, 1204, 1204, 1204, 1204, 1204, 1204,
 /*    80 */  1204, 1204, 1204, 1258, 1204, 1204, 1204, 1204, 1204, 1204,
 /*    90 */  1204, 1204, 1204, 1204, 1204, 1204, 1204,  -71,  -47,  -47,
 /*   100 */   -47,  -47,  -47,   -6,   88,  -66,   23,  458,  505,  468,
 /*   110 */   468,   23,  201,  343,  -58,  -72,  -72,  -72,   11,   11,
 /*   120 */    11,  412,  412,  341,  537,  605,   23,   23,   23,   23,
 /*   130 */    23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
 /*   140 */    23,   23,   23,   23,   23,   23,  635,  298,   74,   74,
 /*   150 */   343,   -1,   -1,   -1,   -1,   -1,   -1,  -72,  -72,  -72,
 /*   160 */   228,  101,  101,  203,   75,   71,  273,  284,  345,   23,
 /*   170 */    23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
 /*   180 */    23,   23,   23,   23,   23,   23,  421,  421,  421,   23,
 /*   190 */    23,  582,   23,   23,   23,  356,   23,   23,  585,   23,
 /*   200 */    23,   23,   23,   23,   23,   23,   23,   23,   23,  568,
 /*   210 */   575,  456,  456,  456,  704,  171,  645,  674,  858,  590,
 /*   220 */   590,  914,  858,  914,  370,  963,  886,  948,  590,  425,
 /*   230 */   948,  948,  864,  641,  527, 1196, 1115, 1115, 1197, 1197,
 /*   240 */  1115, 1221, 1179, 1125, 1240, 1240, 1240, 1240, 1115, 1244,
 /*   250 */  1125, 1221, 1179, 1179, 1125, 1115, 1244, 1137, 1233, 1115,
 /*   260 */  1115, 1244, 1265, 1115, 1244, 1115, 1244, 1265, 1199, 1199,
 /*   270 */  1199, 1262, 1265, 1199, 1206, 1199, 1262, 1199, 1199, 1192,
 /*   280 */  1216, 1192, 1216, 1192, 1216, 1192, 1216, 1115, 1115, 1195,
 /*   290 */  1265, 1246, 1246, 1265, 1214, 1218, 1223, 1219, 1125, 1332,
 /*   300 */  1338, 1358, 1358, 1368, 1368, 1368, 1368,  -72,  -72,  -72,
 /*   310 */   -72,  -72,  -72,  -72,  -72,  400,  623,  742,  816,  658,
 /*   320 */   697,  227, 1012,  664, 1013, 1014, 1018, 1026, 1051,  891,
 /*   330 */  1021, 1040, 1068, 1084, 1066, 1139,  910,  954, 1154, 1088,
 /*   340 */   978, 1380, 1382, 1364, 1255, 1373, 1333, 1386, 1381, 1383,
 /*   350 */  1283, 1274, 1296, 1285, 1389, 1287, 1394, 1412, 1291, 1284,
 /*   360 */  1337, 1340, 1306, 1392, 1387, 1302, 1424, 1421, 1405, 1321,
 /*   370 */  1289, 1376, 1406, 1377, 1372, 1390, 1328, 1413, 1416, 1419,
 /*   380 */  1327, 1335, 1420, 1388, 1422, 1423, 1425, 1427, 1393, 1417,
 /*   390 */  1428, 1398, 1403, 1429, 1430, 1431, 1339, 1434, 1435, 1437,
 /*   400 */  1436, 1341, 1438, 1441, 1432, 1439, 1443, 1342, 1442, 1440,
 /*   410 */  1444, 1445, 1442, 1448, 1449, 1450, 1451, 1455, 1452, 1446,
 /*   420 */  1456, 1458, 1459, 1460, 1461, 1463, 1464, 1460, 1466, 1465,
 /*   430 */  1467, 1469, 1471, 1345, 1360, 1369, 1375, 1472, 1479, 1497,
};
#define YY_REDUCE_USE_DFLT (-144)
#define YY_REDUCE_COUNT (314)
#define YY_REDUCE_MIN   (-143)
#define YY_REDUCE_MAX   (1231)
static const short yy_reduce_ofst[] = {
 /*     0 */  -143,  949,  136,  131,  -48,  -45,  158,  241,   22,  153,
 /*    10 */   226,  163,  362,  364,  366,  312,  314,  368,  237,  236,
 /*    20 */   300,  440,  114,  359,  319,  100,  100,  100,  100,  100,
 /*    30 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*    40 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*    50 */   100,  100,  100,  100,  374,  447,  461,  516,  518,  567,
 /*    60 */   569,  572,  574,  579,  581,  583,  586,  596,  631,  644,
 /*    70 */   646,  649,  657,  659,  661,  663,  671,  708,  720,  722,
 /*    80 */   759,  771,  773,  810,  822,  824,  861,  873,  875,  930,
 /*    90 */   944,  947,  952,  957,  959,  961,  997,  100,  100,  100,
 /*   100 */   100,  100,  100,  100,  100,  100,  486, -108,  -83,  224,
 /*   110 */   286,  451,  100,  681,  100,  100,  100,  100,  354,  354,
 /*   120 */   354,  337,  352,   49,  482,  482,  503,  532,  -60,  615,
 /*   130 */   647,  689,  690,  737,  782,  -62,  517,  789,  474,  795,
 /*   140 */   580,  733,   32,  662,  488,  139,  678,  433,  769,  772,
 /*   150 */   396,  728,  887,  942,  955,  965,  977,  740,  766,  178,
 /*   160 */   -46,  -17,   59,   53,  118,  141,  167,  248,  255,  326,
 /*   170 */   441,  464,  519,  668,  693,  721,  736,  744,  775,  788,
 /*   180 */   846,  899,  912,  936,  983,  985,   72,  134,  542,  990,
 /*   190 */   991,  597,  992,  998, 1020,  871, 1022, 1027,  915, 1029,
 /*   200 */  1030, 1034,  118, 1035, 1036, 1043, 1044, 1045, 1046,  931,
 /*   210 */   967,  999, 1000, 1001,  597, 1003, 1009, 1058, 1011,  987,
 /*   220 */  1002,  976, 1016,  981, 1039, 1037, 1038, 1047, 1006, 1004,
 /*   230 */  1052, 1053, 1033, 1031, 1079,  994, 1080, 1081, 1005, 1010,
 /*   240 */  1082, 1028, 1062, 1055, 1067, 1071, 1072, 1073, 1101, 1108,
 /*   250 */  1069, 1048, 1078, 1083, 1074, 1110, 1113, 1041, 1049, 1122,
 /*   260 */  1123, 1126, 1118, 1138, 1140, 1144, 1141, 1127, 1133, 1134,
 /*   270 */  1135, 1129, 1136, 1142, 1143, 1146, 1132, 1147, 1148, 1102,
 /*   280 */  1109, 1112, 1111, 1114, 1116, 1117, 1119, 1166, 1170, 1120,
 /*   290 */  1158, 1121, 1124, 1165, 1145, 1149, 1155, 1159, 1161, 1201,
 /*   300 */  1209, 1220, 1222, 1225, 1226, 1227, 1228, 1151, 1152, 1150,
 /*   310 */  1215, 1217, 1203, 1205, 1231,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */  1250, 1240, 1240, 1240, 1174, 1174, 1174, 1240, 1071, 1100,
 /*    10 */  1100, 1224, 1301, 1301, 1301, 1301, 1301, 1301, 1173, 1301,
 /*    20 */  1301, 1301, 1301, 1240, 1075, 1106, 1301, 1301, 1301, 1301,
 /*    30 */  1301, 1301, 1301, 1301, 1223, 1225, 1114, 1113, 1206, 1087,
 /*    40 */  1111, 1104, 1108, 1175, 1169, 1170, 1168, 1172, 1176, 1301,
 /*    50 */  1107, 1138, 1153, 1137, 1301, 1301, 1301, 1301, 1301, 1301,
 /*    60 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*    70 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*    80 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*    90 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1147, 1152, 1159,
 /*   100 */  1151, 1148, 1140, 1139, 1141, 1142, 1301,  994, 1042, 1301,
 /*   110 */  1301, 1301, 1143, 1301, 1144, 1156, 1155, 1154, 1231, 1258,
 /*   120 */  1257, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   130 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   140 */  1301, 1301, 1301, 1301, 1301, 1301, 1250, 1240, 1000, 1000,
 /*   150 */  1301, 1240, 1240, 1240, 1240, 1240, 1240, 1236, 1075, 1066,
 /*   160 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   170 */  1228, 1226, 1301, 1187, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   180 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   190 */  1301, 1301, 1301, 1301, 1301, 1071, 1301, 1301, 1301, 1301,
 /*   200 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1252, 1301,
 /*   210 */  1201, 1071, 1071, 1071, 1073, 1055, 1065,  979, 1110, 1089,
 /*   220 */  1089, 1290, 1110, 1290, 1017, 1272, 1014, 1100, 1089, 1171,
 /*   230 */  1100, 1100, 1072, 1065, 1301, 1293, 1080, 1080, 1292, 1292,
 /*   240 */  1080, 1119, 1045, 1110, 1051, 1051, 1051, 1051, 1080,  991,
 /*   250 */  1110, 1119, 1045, 1045, 1110, 1080,  991, 1205, 1287, 1080,
 /*   260 */  1080,  991, 1180, 1080,  991, 1080,  991, 1180, 1043, 1043,
 /*   270 */  1043, 1032, 1180, 1043, 1017, 1043, 1032, 1043, 1043, 1093,
 /*   280 */  1088, 1093, 1088, 1093, 1088, 1093, 1088, 1080, 1080, 1301,
 /*   290 */  1180, 1184, 1184, 1180, 1105, 1094, 1103, 1101, 1110,  997,
 /*   300 */  1035, 1255, 1255, 1251, 1251, 1251, 1251, 1298, 1298, 1236,
 /*   310 */  1267, 1267, 1019, 1019, 1267, 1301, 1301, 1301, 1301, 1301,
 /*   320 */  1301, 1262, 1301, 1189, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   330 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   340 */  1125, 1301,  975, 1233, 1301, 1301, 1232, 1301, 1301, 1301,
 /*   350 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   360 */  1301, 1301, 1301, 1301, 1301, 1289, 1301, 1301, 1301, 1301,
 /*   370 */  1301, 1301, 1204, 1203, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   380 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   390 */  1301, 1301, 1301, 1301, 1301, 1301, 1057, 1301, 1301, 1301,
 /*   400 */  1276, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1102, 1301,
 /*   410 */  1095, 1301, 1280, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   420 */  1301, 1301, 1301, 1242, 1301, 1301, 1301, 1241, 1301, 1301,
 /*   430 */  1301, 1301, 1301, 1127, 1301, 1126, 1130, 1301,  985, 1301,
};
/********** End of lemon-generated parsing tables *****************************/

/* The next table maps tokens (terminal symbols) into fallback tokens.  
** If a construct like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
**
** This feature can be used, for example, to cause some keywords in a language
** to revert to identifiers if they keyword does not apply in the context where
** it appears.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
    0,  /*          $ => nothing */
    0,  /*       SEMI => nothing */
   27,  /*    EXPLAIN => ID */
   27,  /*      QUERY => ID */
   27,  /*       PLAN => ID */
   27,  /*      BEGIN => ID */
    0,  /* TRANSACTION => nothing */
   27,  /*   DEFERRED => ID */
   27,  /*  IMMEDIATE => ID */
   27,  /*  EXCLUSIVE => ID */
    0,  /*     COMMIT => nothing */
   27,  /*        END => ID */
   27,  /*   ROLLBACK => ID */
   27,  /*  SAVEPOINT => ID */
   27,  /*    RELEASE => ID */
    0,  /*         TO => nothing */
    0,  /*      TABLE => nothing */
    0,  /*     CREATE => nothing */
   27,  /*         IF => ID */
    0,  /*        NOT => nothing */
    0,  /*     EXISTS => nothing */
   27,  /*       TEMP => ID */
    0,  /*         LP => nothing */
    0,  /*         RP => nothing */
    0,  /*         AS => nothing */
   27,  /*    WITHOUT => ID */
    0,  /*      COMMA => nothing */
    0,  /*         ID => nothing */
    0,  /*    INDEXED => nothing */
   27,  /*      ABORT => ID */
   27,  /*     ACTION => ID */
   27,  /*      AFTER => ID */
   27,  /*    ANALYZE => ID */
   27,  /*        ASC => ID */
   27,  /*     ATTACH => ID */
   27,  /*     BEFORE => ID */
   27,  /*         BY => ID */
   27,  /*    CASCADE => ID */
   27,  /*       CAST => ID */
   27,  /*   COLUMNKW => ID */
   27,  /*   CONFLICT => ID */
   27,  /*   DATABASE => ID */
   27,  /*       DESC => ID */
   27,  /*     DETACH => ID */
   27,  /*       EACH => ID */
   27,  /*       FAIL => ID */
   27,  /*        FOR => ID */
   27,  /*     IGNORE => ID */
   27,  /*  INITIALLY => ID */
   27,  /*    INSTEAD => ID */
   27,  /*    LIKE_KW => ID */
   27,  /*      MATCH => ID */
   27,  /*         NO => ID */
   27,  /*        KEY => ID */
   27,  /*         OF => ID */
   27,  /*     OFFSET => ID */
   27,  /*     PRAGMA => ID */
   27,  /*      RAISE => ID */
   27,  /*  RECURSIVE => ID */
   27,  /*    REPLACE => ID */
   27,  /*   RESTRICT => ID */
   27,  /*        ROW => ID */
   27,  /*    TRIGGER => ID */
   27,  /*     VACUUM => ID */
   27,  /*       VIEW => ID */
   27,  /*    VIRTUAL => ID */
   27,  /*       WITH => ID */
   27,  /*    REINDEX => ID */
   27,  /*     RENAME => ID */
   27,  /*   CTIME_KW => ID */
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
**
** After the "shift" half of a SHIFTREDUCE action, the stateno field
** actually contains the reduce action for the second half of the
** SHIFTREDUCE.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number, or reduce action in SHIFTREDUCE */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyidxMax;                 /* Maximum value of yyidx */
#endif
#ifndef YYNOERRORRECOVERY
  int yyerrcnt;                 /* Shifts left before out of the error */
#endif
  sqlite3ParserARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void sqlite3ParserTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "SEMI",          "EXPLAIN",       "QUERY",       
  "PLAN",          "BEGIN",         "TRANSACTION",   "DEFERRED",    
  "IMMEDIATE",     "EXCLUSIVE",     "COMMIT",        "END",         
  "ROLLBACK",      "SAVEPOINT",     "RELEASE",       "TO",          
  "TABLE",         "CREATE",        "IF",            "NOT",         
  "EXISTS",        "TEMP",          "LP",            "RP",          
  "AS",            "WITHOUT",       "COMMA",         "ID",          
  "INDEXED",       "ABORT",         "ACTION",        "AFTER",       
  "ANALYZE",       "ASC",           "ATTACH",        "BEFORE",      
  "BY",            "CASCADE",       "CAST",          "COLUMNKW",    
  "CONFLICT",      "DATABASE",      "DESC",          "DETACH",      
  "EACH",          "FAIL",          "FOR",           "IGNORE",      
  "INITIALLY",     "INSTEAD",       "LIKE_KW",       "MATCH",       
  "NO",            "KEY",           "OF",            "OFFSET",      
  "PRAGMA",        "RAISE",         "RECURSIVE",     "REPLACE",     
  "RESTRICT",      "ROW",           "TRIGGER",       "VACUUM",      
  "VIEW",          "VIRTUAL",       "WITH",          "REINDEX",     
  "RENAME",        "CTIME_KW",      "ANY",           "OR",          
  "AND",           "IS",            "BETWEEN",       "IN",          
  "ISNULL",        "NOTNULL",       "NE",            "EQ",          
  "GT",            "LE",            "LT",            "GE",          
  "ESCAPE",        "BITAND",        "BITOR",         "LSHIFT",      
  "RSHIFT",        "PLUS",          "MINUS",         "STAR",        
  "SLASH",         "REM",           "CONCAT",        "COLLATE",     
  "BITNOT",        "STRING",        "JOIN_KW",       "CONSTRAINT",  
  "DEFAULT",       "NULL",          "PRIMARY",       "UNIQUE",      
  "CHECK",         "REFERENCES",    "AUTOINCR",      "ON",          
  "INSERT",        "DELETE",        "UPDATE",        "SET",         
  "DEFERRABLE",    "FOREIGN",       "DROP",          "UNION",       
  "ALL",           "EXCEPT",        "INTERSECT",     "SELECT",      
  "VALUES",        "DISTINCT",      "DOT",           "FROM",        
  "JOIN",          "USING",         "ORDER",         "GROUP",       
  "HAVING",        "LIMIT",         "WHERE",         "INTO",        
  "INTEGER",       "FLOAT",         "BLOB",          "VARIABLE",    
  "CASE",          "WHEN",          "THEN",          "ELSE",        
  "INDEX",         "ALTER",         "ADD",           "error",       
  "input",         "cmdlist",       "ecmd",          "explain",     
  "cmdx",          "cmd",           "transtype",     "trans_opt",   
  "nm",            "savepoint_opt",  "create_table",  "create_table_args",
  "createkw",      "temp",          "ifnotexists",   "dbnm",        
  "columnlist",    "conslist_opt",  "table_options",  "select",      
  "columnname",    "carglist",      "typetoken",     "typename",    
  "signed",        "plus_num",      "minus_num",     "ccons",       
  "term",          "expr",          "onconf",        "sortorder",   
  "autoinc",       "eidlist_opt",   "refargs",       "defer_subclause",
  "refarg",        "refact",        "init_deferred_pred_opt",  "conslist",    
  "tconscomma",    "tcons",         "sortlist",      "eidlist",     
  "defer_subclause_opt",  "orconf",        "resolvetype",   "raisetype",   
  "ifexists",      "fullname",      "selectnowith",  "oneselect",   
  "with",          "multiselect_op",  "distinct",      "selcollist",  
  "from",          "where_opt",     "groupby_opt",   "having_opt",  
  "orderby_opt",   "limit_opt",     "values",        "nexprlist",   
  "exprlist",      "sclp",          "as",            "seltablist",  
  "stl_prefix",    "joinop",        "indexed_opt",   "on_opt",      
  "using_opt",     "idlist",        "setlist",       "insert_cmd",  
  "idlist_opt",    "likeop",        "between_op",    "in_op",       
  "case_operand",  "case_exprlist",  "case_else",     "uniqueflag",  
  "collate",       "nmnum",         "trigger_decl",  "trigger_cmd_list",
  "trigger_time",  "trigger_event",  "foreach_clause",  "when_clause", 
  "trigger_cmd",   "trnm",          "tridxby",       "database_kw_opt",
  "key_opt",       "add_column_fullname",  "kwcolumn_opt",  "create_vtab", 
  "vtabarglist",   "vtabarg",       "vtabargtoken",  "lp",          
  "anylist",       "wqlist",      
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "explain ::= EXPLAIN",
 /*   1 */ "explain ::= EXPLAIN QUERY PLAN",
 /*   2 */ "cmdx ::= cmd",
 /*   3 */ "cmd ::= BEGIN transtype trans_opt",
 /*   4 */ "transtype ::=",
 /*   5 */ "transtype ::= DEFERRED",
 /*   6 */ "transtype ::= IMMEDIATE",
 /*   7 */ "transtype ::= EXCLUSIVE",
 /*   8 */ "cmd ::= COMMIT trans_opt",
 /*   9 */ "cmd ::= END trans_opt",
 /*  10 */ "cmd ::= ROLLBACK trans_opt",
 /*  11 */ "cmd ::= SAVEPOINT nm",
 /*  12 */ "cmd ::= RELEASE savepoint_opt nm",
 /*  13 */ "cmd ::= ROLLBACK trans_opt TO savepoint_opt nm",
 /*  14 */ "create_table ::= createkw temp TABLE ifnotexists nm dbnm",
 /*  15 */ "createkw ::= CREATE",
 /*  16 */ "ifnotexists ::=",
 /*  17 */ "ifnotexists ::= IF NOT EXISTS",
 /*  18 */ "temp ::= TEMP",
 /*  19 */ "temp ::=",
 /*  20 */ "create_table_args ::= LP columnlist conslist_opt RP table_options",
 /*  21 */ "create_table_args ::= AS select",
 /*  22 */ "table_options ::=",
 /*  23 */ "table_options ::= WITHOUT nm",
 /*  24 */ "columnname ::= nm typetoken",
 /*  25 */ "typetoken ::=",
 /*  26 */ "typetoken ::= typename LP signed RP",
 /*  27 */ "typetoken ::= typename LP signed COMMA signed RP",
 /*  28 */ "typename ::= typename ID|STRING",
 /*  29 */ "ccons ::= CONSTRAINT nm",
 /*  30 */ "ccons ::= DEFAULT term",
 /*  31 */ "ccons ::= DEFAULT LP expr RP",
 /*  32 */ "ccons ::= DEFAULT PLUS term",
 /*  33 */ "ccons ::= DEFAULT MINUS term",
 /*  34 */ "ccons ::= DEFAULT ID|INDEXED",
 /*  35 */ "ccons ::= NOT NULL onconf",
 /*  36 */ "ccons ::= PRIMARY KEY sortorder onconf autoinc",
 /*  37 */ "ccons ::= UNIQUE onconf",
 /*  38 */ "ccons ::= CHECK LP expr RP",
 /*  39 */ "ccons ::= REFERENCES nm eidlist_opt refargs",
 /*  40 */ "ccons ::= defer_subclause",
 /*  41 */ "ccons ::= COLLATE ID|STRING",
 /*  42 */ "autoinc ::=",
 /*  43 */ "autoinc ::= AUTOINCR",
 /*  44 */ "refargs ::=",
 /*  45 */ "refargs ::= refargs refarg",
 /*  46 */ "refarg ::= MATCH nm",
 /*  47 */ "refarg ::= ON INSERT refact",
 /*  48 */ "refarg ::= ON DELETE refact",
 /*  49 */ "refarg ::= ON UPDATE refact",
 /*  50 */ "refact ::= SET NULL",
 /*  51 */ "refact ::= SET DEFAULT",
 /*  52 */ "refact ::= CASCADE",
 /*  53 */ "refact ::= RESTRICT",
 /*  54 */ "refact ::= NO ACTION",
 /*  55 */ "defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt",
 /*  56 */ "defer_subclause ::= DEFERRABLE init_deferred_pred_opt",
 /*  57 */ "init_deferred_pred_opt ::=",
 /*  58 */ "init_deferred_pred_opt ::= INITIALLY DEFERRED",
 /*  59 */ "init_deferred_pred_opt ::= INITIALLY IMMEDIATE",
 /*  60 */ "conslist_opt ::=",
 /*  61 */ "tconscomma ::= COMMA",
 /*  62 */ "tcons ::= CONSTRAINT nm",
 /*  63 */ "tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf",
 /*  64 */ "tcons ::= UNIQUE LP sortlist RP onconf",
 /*  65 */ "tcons ::= CHECK LP expr RP onconf",
 /*  66 */ "tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt",
 /*  67 */ "defer_subclause_opt ::=",
 /*  68 */ "onconf ::=",
 /*  69 */ "onconf ::= ON CONFLICT resolvetype",
 /*  70 */ "orconf ::=",
 /*  71 */ "orconf ::= OR resolvetype",
 /*  72 */ "resolvetype ::= IGNORE",
 /*  73 */ "resolvetype ::= REPLACE",
 /*  74 */ "cmd ::= DROP TABLE ifexists fullname",
 /*  75 */ "ifexists ::= IF EXISTS",
 /*  76 */ "ifexists ::=",
 /*  77 */ "cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select",
 /*  78 */ "cmd ::= DROP VIEW ifexists fullname",
 /*  79 */ "cmd ::= select",
 /*  80 */ "select ::= with selectnowith",
 /*  81 */ "selectnowith ::= selectnowith multiselect_op oneselect",
 /*  82 */ "multiselect_op ::= UNION",
 /*  83 */ "multiselect_op ::= UNION ALL",
 /*  84 */ "multiselect_op ::= EXCEPT|INTERSECT",
 /*  85 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt",
 /*  86 */ "values ::= VALUES LP nexprlist RP",
 /*  87 */ "values ::= values COMMA LP exprlist RP",
 /*  88 */ "distinct ::= DISTINCT",
 /*  89 */ "distinct ::= ALL",
 /*  90 */ "distinct ::=",
 /*  91 */ "sclp ::=",
 /*  92 */ "selcollist ::= sclp expr as",
 /*  93 */ "selcollist ::= sclp STAR",
 /*  94 */ "selcollist ::= sclp nm DOT STAR",
 /*  95 */ "as ::= AS nm",
 /*  96 */ "as ::=",
 /*  97 */ "from ::=",
 /*  98 */ "from ::= FROM seltablist",
 /*  99 */ "stl_prefix ::= seltablist joinop",
 /* 100 */ "stl_prefix ::=",
 /* 101 */ "seltablist ::= stl_prefix nm dbnm as indexed_opt on_opt using_opt",
 /* 102 */ "seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_opt using_opt",
 /* 103 */ "seltablist ::= stl_prefix LP select RP as on_opt using_opt",
 /* 104 */ "seltablist ::= stl_prefix LP seltablist RP as on_opt using_opt",
 /* 105 */ "dbnm ::=",
 /* 106 */ "dbnm ::= DOT nm",
 /* 107 */ "fullname ::= nm dbnm",
 /* 108 */ "joinop ::= COMMA|JOIN",
 /* 109 */ "joinop ::= JOIN_KW JOIN",
 /* 110 */ "joinop ::= JOIN_KW nm JOIN",
 /* 111 */ "joinop ::= JOIN_KW nm nm JOIN",
 /* 112 */ "on_opt ::= ON expr",
 /* 113 */ "on_opt ::=",
 /* 114 */ "indexed_opt ::=",
 /* 115 */ "indexed_opt ::= INDEXED BY nm",
 /* 116 */ "indexed_opt ::= NOT INDEXED",
 /* 117 */ "using_opt ::= USING LP idlist RP",
 /* 118 */ "using_opt ::=",
 /* 119 */ "orderby_opt ::=",
 /* 120 */ "orderby_opt ::= ORDER BY sortlist",
 /* 121 */ "sortlist ::= sortlist COMMA expr sortorder",
 /* 122 */ "sortlist ::= expr sortorder",
 /* 123 */ "sortorder ::= ASC",
 /* 124 */ "sortorder ::= DESC",
 /* 125 */ "sortorder ::=",
 /* 126 */ "groupby_opt ::=",
 /* 127 */ "groupby_opt ::= GROUP BY nexprlist",
 /* 128 */ "having_opt ::=",
 /* 129 */ "having_opt ::= HAVING expr",
 /* 130 */ "limit_opt ::=",
 /* 131 */ "limit_opt ::= LIMIT expr",
 /* 132 */ "limit_opt ::= LIMIT expr OFFSET expr",
 /* 133 */ "limit_opt ::= LIMIT expr COMMA expr",
 /* 134 */ "cmd ::= with DELETE FROM fullname indexed_opt where_opt",
 /* 135 */ "where_opt ::=",
 /* 136 */ "where_opt ::= WHERE expr",
 /* 137 */ "cmd ::= with UPDATE orconf fullname indexed_opt SET setlist where_opt",
 /* 138 */ "setlist ::= setlist COMMA nm EQ expr",
 /* 139 */ "setlist ::= nm EQ expr",
 /* 140 */ "cmd ::= with insert_cmd INTO fullname idlist_opt select",
 /* 141 */ "cmd ::= with insert_cmd INTO fullname idlist_opt DEFAULT VALUES",
 /* 142 */ "insert_cmd ::= INSERT orconf",
 /* 143 */ "insert_cmd ::= REPLACE",
 /* 144 */ "idlist_opt ::=",
 /* 145 */ "idlist_opt ::= LP idlist RP",
 /* 146 */ "idlist ::= idlist COMMA nm",
 /* 147 */ "idlist ::= nm",
 /* 148 */ "expr ::= LP expr RP",
 /* 149 */ "term ::= NULL",
 /* 150 */ "expr ::= ID|INDEXED",
 /* 151 */ "expr ::= JOIN_KW",
 /* 152 */ "expr ::= nm DOT nm",
 /* 153 */ "expr ::= nm DOT nm DOT nm",
 /* 154 */ "term ::= INTEGER|FLOAT|BLOB",
 /* 155 */ "term ::= STRING",
 /* 156 */ "expr ::= VARIABLE",
 /* 157 */ "expr ::= expr COLLATE ID|STRING",
 /* 158 */ "expr ::= CAST LP expr AS typetoken RP",
 /* 159 */ "expr ::= ID|INDEXED LP distinct exprlist RP",
 /* 160 */ "expr ::= ID|INDEXED LP STAR RP",
 /* 161 */ "term ::= CTIME_KW",
 /* 162 */ "expr ::= expr AND expr",
 /* 163 */ "expr ::= expr OR expr",
 /* 164 */ "expr ::= expr LT|GT|GE|LE expr",
 /* 165 */ "expr ::= expr EQ|NE expr",
 /* 166 */ "expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr",
 /* 167 */ "expr ::= expr PLUS|MINUS expr",
 /* 168 */ "expr ::= expr STAR|SLASH|REM expr",
 /* 169 */ "expr ::= expr CONCAT expr",
 /* 170 */ "likeop ::= LIKE_KW|MATCH",
 /* 171 */ "likeop ::= NOT LIKE_KW|MATCH",
 /* 172 */ "expr ::= expr likeop expr",
 /* 173 */ "expr ::= expr likeop expr ESCAPE expr",
 /* 174 */ "expr ::= expr ISNULL|NOTNULL",
 /* 175 */ "expr ::= expr NOT NULL",
 /* 176 */ "expr ::= expr IS expr",
 /* 177 */ "expr ::= expr IS NOT expr",
 /* 178 */ "expr ::= NOT expr",
 /* 179 */ "expr ::= BITNOT expr",
 /* 180 */ "expr ::= MINUS expr",
 /* 181 */ "expr ::= PLUS expr",
 /* 182 */ "between_op ::= BETWEEN",
 /* 183 */ "between_op ::= NOT BETWEEN",
 /* 184 */ "expr ::= expr between_op expr AND expr",
 /* 185 */ "in_op ::= IN",
 /* 186 */ "in_op ::= NOT IN",
 /* 187 */ "expr ::= expr in_op LP exprlist RP",
 /* 188 */ "expr ::= LP select RP",
 /* 189 */ "expr ::= expr in_op LP select RP",
 /* 190 */ "expr ::= expr in_op nm dbnm",
 /* 191 */ "expr ::= EXISTS LP select RP",
 /* 192 */ "expr ::= CASE case_operand case_exprlist case_else END",
 /* 193 */ "case_exprlist ::= case_exprlist WHEN expr THEN expr",
 /* 194 */ "case_exprlist ::= WHEN expr THEN expr",
 /* 195 */ "case_else ::= ELSE expr",
 /* 196 */ "case_else ::=",
 /* 197 */ "case_operand ::= expr",
 /* 198 */ "case_operand ::=",
 /* 199 */ "exprlist ::=",
 /* 200 */ "nexprlist ::= nexprlist COMMA expr",
 /* 201 */ "nexprlist ::= expr",
 /* 202 */ "cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt",
 /* 203 */ "uniqueflag ::= UNIQUE",
 /* 204 */ "uniqueflag ::=",
 /* 205 */ "eidlist_opt ::=",
 /* 206 */ "eidlist_opt ::= LP eidlist RP",
 /* 207 */ "eidlist ::= eidlist COMMA nm collate sortorder",
 /* 208 */ "eidlist ::= nm collate sortorder",
 /* 209 */ "collate ::=",
 /* 210 */ "collate ::= COLLATE ID|STRING",
 /* 211 */ "cmd ::= DROP INDEX ifexists fullname",
 /* 212 */ "cmd ::= VACUUM",
 /* 213 */ "cmd ::= VACUUM nm",
 /* 214 */ "cmd ::= PRAGMA nm dbnm",
 /* 215 */ "cmd ::= PRAGMA nm dbnm EQ nmnum",
 /* 216 */ "cmd ::= PRAGMA nm dbnm LP nmnum RP",
 /* 217 */ "cmd ::= PRAGMA nm dbnm EQ minus_num",
 /* 218 */ "cmd ::= PRAGMA nm dbnm LP minus_num RP",
 /* 219 */ "plus_num ::= PLUS INTEGER|FLOAT",
 /* 220 */ "minus_num ::= MINUS INTEGER|FLOAT",
 /* 221 */ "cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END",
 /* 222 */ "trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause",
 /* 223 */ "trigger_time ::= BEFORE",
 /* 224 */ "trigger_time ::= AFTER",
 /* 225 */ "trigger_time ::= INSTEAD OF",
 /* 226 */ "trigger_time ::=",
 /* 227 */ "trigger_event ::= DELETE|INSERT",
 /* 228 */ "trigger_event ::= UPDATE",
 /* 229 */ "trigger_event ::= UPDATE OF idlist",
 /* 230 */ "when_clause ::=",
 /* 231 */ "when_clause ::= WHEN expr",
 /* 232 */ "trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI",
 /* 233 */ "trigger_cmd_list ::= trigger_cmd SEMI",
 /* 234 */ "trnm ::= nm DOT nm",
 /* 235 */ "tridxby ::= INDEXED BY nm",
 /* 236 */ "tridxby ::= NOT INDEXED",
 /* 237 */ "trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist where_opt",
 /* 238 */ "trigger_cmd ::= insert_cmd INTO trnm idlist_opt select",
 /* 239 */ "trigger_cmd ::= DELETE FROM trnm tridxby where_opt",
 /* 240 */ "trigger_cmd ::= select",
 /* 241 */ "expr ::= RAISE LP IGNORE RP",
 /* 242 */ "expr ::= RAISE LP raisetype COMMA nm RP",
 /* 243 */ "raisetype ::= ROLLBACK",
 /* 244 */ "raisetype ::= ABORT",
 /* 245 */ "raisetype ::= FAIL",
 /* 246 */ "cmd ::= DROP TRIGGER ifexists fullname",
 /* 247 */ "cmd ::= ATTACH database_kw_opt expr AS expr key_opt",
 /* 248 */ "cmd ::= DETACH database_kw_opt expr",
 /* 249 */ "key_opt ::=",
 /* 250 */ "key_opt ::= KEY expr",
 /* 251 */ "cmd ::= REINDEX",
 /* 252 */ "cmd ::= REINDEX nm dbnm",
 /* 253 */ "cmd ::= ANALYZE",
 /* 254 */ "cmd ::= ANALYZE nm dbnm",
 /* 255 */ "cmd ::= ALTER TABLE fullname RENAME TO nm",
 /* 256 */ "cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist",
 /* 257 */ "add_column_fullname ::= fullname",
 /* 258 */ "cmd ::= create_vtab",
 /* 259 */ "cmd ::= create_vtab LP vtabarglist RP",
 /* 260 */ "create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm",
 /* 261 */ "vtabarg ::=",
 /* 262 */ "vtabargtoken ::= ANY",
 /* 263 */ "vtabargtoken ::= lp anylist RP",
 /* 264 */ "lp ::= LP",
 /* 265 */ "with ::=",
 /* 266 */ "with ::= WITH wqlist",
 /* 267 */ "with ::= WITH RECURSIVE wqlist",
 /* 268 */ "wqlist ::= nm eidlist_opt AS LP select RP",
 /* 269 */ "wqlist ::= wqlist COMMA nm eidlist_opt AS LP select RP",
 /* 270 */ "input ::= cmdlist",
 /* 271 */ "cmdlist ::= cmdlist ecmd",
 /* 272 */ "cmdlist ::= ecmd",
 /* 273 */ "ecmd ::= SEMI",
 /* 274 */ "ecmd ::= explain cmdx SEMI",
 /* 275 */ "explain ::=",
 /* 276 */ "trans_opt ::=",
 /* 277 */ "trans_opt ::= TRANSACTION",
 /* 278 */ "trans_opt ::= TRANSACTION nm",
 /* 279 */ "savepoint_opt ::= SAVEPOINT",
 /* 280 */ "savepoint_opt ::=",
 /* 281 */ "cmd ::= create_table create_table_args",
 /* 282 */ "columnlist ::= columnlist COMMA columnname carglist",
 /* 283 */ "columnlist ::= columnname carglist",
 /* 284 */ "nm ::= ID|INDEXED",
 /* 285 */ "nm ::= STRING",
 /* 286 */ "nm ::= JOIN_KW",
 /* 287 */ "typetoken ::= typename",
 /* 288 */ "typename ::= ID|STRING",
 /* 289 */ "signed ::= plus_num",
 /* 290 */ "signed ::= minus_num",
 /* 291 */ "carglist ::= carglist ccons",
 /* 292 */ "carglist ::=",
 /* 293 */ "ccons ::= NULL onconf",
 /* 294 */ "conslist_opt ::= COMMA conslist",
 /* 295 */ "conslist ::= conslist tconscomma tcons",
 /* 296 */ "conslist ::= tcons",
 /* 297 */ "tconscomma ::=",
 /* 298 */ "defer_subclause_opt ::= defer_subclause",
 /* 299 */ "resolvetype ::= raisetype",
 /* 300 */ "selectnowith ::= oneselect",
 /* 301 */ "oneselect ::= values",
 /* 302 */ "sclp ::= selcollist COMMA",
 /* 303 */ "as ::= ID|STRING",
 /* 304 */ "expr ::= term",
 /* 305 */ "exprlist ::= nexprlist",
 /* 306 */ "nmnum ::= plus_num",
 /* 307 */ "nmnum ::= nm",
 /* 308 */ "nmnum ::= ON",
 /* 309 */ "nmnum ::= DELETE",
 /* 310 */ "nmnum ::= DEFAULT",
 /* 311 */ "plus_num ::= INTEGER|FLOAT",
 /* 312 */ "foreach_clause ::=",
 /* 313 */ "foreach_clause ::= FOR EACH ROW",
 /* 314 */ "trnm ::= nm",
 /* 315 */ "tridxby ::=",
 /* 316 */ "database_kw_opt ::= DATABASE",
 /* 317 */ "database_kw_opt ::=",
 /* 318 */ "kwcolumn_opt ::=",
 /* 319 */ "kwcolumn_opt ::= COLUMNKW",
 /* 320 */ "vtabarglist ::= vtabarg",
 /* 321 */ "vtabarglist ::= vtabarglist COMMA vtabarg",
 /* 322 */ "vtabarg ::= vtabarg vtabargtoken",
 /* 323 */ "anylist ::=",
 /* 324 */ "anylist ::= anylist LP anylist RP",
 /* 325 */ "anylist ::= anylist ANY",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(yyParser *p){
  int newSize;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  if( pNew ){
    p->yystack = pNew;
    p->yystksz = newSize;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, p->yystksz);
    }
#endif
  }
}
#endif

/* Datatype of the argument to the memory allocated passed as the
** second argument to sqlite3ParserAlloc() below.  This can be changed by
** putting an appropriate #define in the %include section of the input
** grammar.
*/
#ifndef YYMALLOCARGTYPE
# define YYMALLOCARGTYPE size_t
#endif

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to sqlite3Parser and sqlite3ParserFree.
*/
void *sqlite3ParserAlloc(void *(*mallocProc)(YYMALLOCARGTYPE)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (YYMALLOCARGTYPE)sizeof(yyParser) );
  if( pParser ){
    pParser->yyidx = -1;
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyidxMax = 0;
#endif
#if YYSTACKDEPTH<=0
    pParser->yystack = NULL;
    pParser->yystksz = 0;
    yyGrowStack(pParser);
#endif
  }
  return pParser;
}

/* The following function deletes the "minor type" or semantic value
** associated with a symbol.  The symbol can be either a terminal
** or nonterminal. "yymajor" is the symbol code, and "yypminor" is
** a pointer to the value to be deleted.  The code used to do the 
** deletions is derived from the %destructor and/or %token_destructor
** directives of the input grammar.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  sqlite3ParserARG_FETCH;
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are *not* used
    ** inside the C code.
    */
/********* Begin destructor definitions ***************************************/
    case 163: /* select */
    case 194: /* selectnowith */
    case 195: /* oneselect */
    case 206: /* values */
{
#line 407 "parse.y"
sqlite3SelectDelete(pParse->db, (yypminor->yy159));
#line 1498 "parse.c"
}
      break;
    case 172: /* term */
    case 173: /* expr */
{
#line 828 "parse.y"
sqlite3ExprDelete(pParse->db, (yypminor->yy342).pExpr);
#line 1506 "parse.c"
}
      break;
    case 177: /* eidlist_opt */
    case 186: /* sortlist */
    case 187: /* eidlist */
    case 199: /* selcollist */
    case 202: /* groupby_opt */
    case 204: /* orderby_opt */
    case 207: /* nexprlist */
    case 208: /* exprlist */
    case 209: /* sclp */
    case 218: /* setlist */
    case 225: /* case_exprlist */
{
#line 1248 "parse.y"
sqlite3ExprListDelete(pParse->db, (yypminor->yy442));
#line 1523 "parse.c"
}
      break;
    case 193: /* fullname */
    case 200: /* from */
    case 211: /* seltablist */
    case 212: /* stl_prefix */
{
#line 639 "parse.y"
sqlite3SrcListDelete(pParse->db, (yypminor->yy347));
#line 1533 "parse.c"
}
      break;
    case 196: /* with */
    case 249: /* wqlist */
{
#line 1525 "parse.y"
sqlite3WithDelete(pParse->db, (yypminor->yy331));
#line 1541 "parse.c"
}
      break;
    case 201: /* where_opt */
    case 203: /* having_opt */
    case 215: /* on_opt */
    case 224: /* case_operand */
    case 226: /* case_else */
    case 235: /* when_clause */
    case 240: /* key_opt */
{
#line 755 "parse.y"
sqlite3ExprDelete(pParse->db, (yypminor->yy122));
#line 1554 "parse.c"
}
      break;
    case 216: /* using_opt */
    case 217: /* idlist */
    case 220: /* idlist_opt */
{
#line 673 "parse.y"
sqlite3IdListDelete(pParse->db, (yypminor->yy180));
#line 1563 "parse.c"
}
      break;
    case 231: /* trigger_cmd_list */
    case 236: /* trigger_cmd */
{
#line 1362 "parse.y"
sqlite3DeleteTriggerStep(pParse->db, (yypminor->yy327));
#line 1571 "parse.c"
}
      break;
    case 233: /* trigger_event */
{
#line 1348 "parse.y"
sqlite3IdListDelete(pParse->db, (yypminor->yy410).b);
#line 1578 "parse.c"
}
      break;
/********* End destructor definitions *****************************************/
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
*/
static void yy_pop_parser_stack(yyParser *pParser){
  yyStackEntry *yytos;
  assert( pParser->yyidx>=0 );
  yytos = &pParser->yystack[pParser->yyidx--];
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yy_destructor(pParser, yytos->major, &yytos->minor);
}

/* 
** Deallocate and destroy a parser.  Destructors are called for
** all stack elements before shutting the parser down.
**
** If the YYPARSEFREENEVERNULL macro exists (for example because it
** is defined in a %include section of the input grammar) then it is
** assumed that the input pointer is never NULL.
*/
void sqlite3ParserFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
#ifndef YYPARSEFREENEVERNULL
  if( pParser==0 ) return;
#endif
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int sqlite3ParserStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyidxMax;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
*/
static unsigned int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;
 
  if( stateno>=YY_MIN_REDUCE ) return stateno;
  assert( stateno <= YY_SHIFT_COUNT );
  do{
    i = yy_shift_ofst[stateno];
    if( i==YY_SHIFT_USE_DFLT ) return yy_default[stateno];
    assert( iLookAhead!=YYNOCODE );
    i += iLookAhead;
    if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
      if( iLookAhead>0 ){
#ifdef YYFALLBACK
        YYCODETYPE iFallback;            /* Fallback token */
        if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
               && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
          }
#endif
          assert( yyFallback[iFallback]==0 ); /* Fallback loop must terminate */
          iLookAhead = iFallback;
          continue;
        }
#endif
#ifdef YYWILDCARD
        {
          int j = i - iLookAhead + YYWILDCARD;
          if( 
#if YY_SHIFT_MIN+YYWILDCARD<0
            j>=0 &&
#endif
#if YY_SHIFT_MAX+YYWILDCARD>=YY_ACTTAB_COUNT
            j<YY_ACTTAB_COUNT &&
#endif
            yy_lookahead[j]==YYWILDCARD
          ){
#ifndef NDEBUG
            if( yyTraceFILE ){
              fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
                 yyTracePrompt, yyTokenName[iLookAhead],
                 yyTokenName[YYWILDCARD]);
            }
#endif /* NDEBUG */
            return yy_action[j];
          }
        }
#endif /* YYWILDCARD */
      }
      return yy_default[stateno];
    }else{
      return yy_action[i];
    }
  }while(1);
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_COUNT ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_COUNT );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_ACTTAB_COUNT );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser){
   sqlite3ParserARG_FETCH;
   yypParser->yyidx--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
/******** Begin %stack_overflow code ******************************************/
#line 37 "parse.y"

  sqlite3ErrorMsg(pParse, "parser stack overflow");
#line 1754 "parse.c"
/******** End %stack_overflow code ********************************************/
   sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Print tracing information for a SHIFT action
*/
#ifndef NDEBUG
static void yyTraceShift(yyParser *yypParser, int yyNewState){
  if( yyTraceFILE ){
    if( yyNewState<YYNSTATE ){
      fprintf(yyTraceFILE,"%sShift '%s', go to state %d\n",
         yyTracePrompt,yyTokenName[yypParser->yystack[yypParser->yyidx].major],
         yyNewState);
    }else{
      fprintf(yyTraceFILE,"%sShift '%s'\n",
         yyTracePrompt,yyTokenName[yypParser->yystack[yypParser->yyidx].major]);
    }
  }
}
#else
# define yyTraceShift(X,Y)
#endif

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  sqlite3ParserTOKENTYPE yyMinor        /* The minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( yypParser->yyidx>yypParser->yyidxMax ){
    yypParser->yyidxMax = yypParser->yyidx;
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yyidx>=YYSTACKDEPTH ){
    yyStackOverflow(yypParser);
    return;
  }
#else
  if( yypParser->yyidx>=yypParser->yystksz ){
    yyGrowStack(yypParser);
    if( yypParser->yyidx>=yypParser->yystksz ){
      yyStackOverflow(yypParser);
      return;
    }
  }
#endif
  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  yytos->minor.yy0 = yyMinor;
  yyTraceShift(yypParser, yyNewState);
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 147, 1 },
  { 147, 3 },
  { 148, 1 },
  { 149, 3 },
  { 150, 0 },
  { 150, 1 },
  { 150, 1 },
  { 150, 1 },
  { 149, 2 },
  { 149, 2 },
  { 149, 2 },
  { 149, 2 },
  { 149, 3 },
  { 149, 5 },
  { 154, 6 },
  { 156, 1 },
  { 158, 0 },
  { 158, 3 },
  { 157, 1 },
  { 157, 0 },
  { 155, 5 },
  { 155, 2 },
  { 162, 0 },
  { 162, 2 },
  { 164, 2 },
  { 166, 0 },
  { 166, 4 },
  { 166, 6 },
  { 167, 2 },
  { 171, 2 },
  { 171, 2 },
  { 171, 4 },
  { 171, 3 },
  { 171, 3 },
  { 171, 2 },
  { 171, 3 },
  { 171, 5 },
  { 171, 2 },
  { 171, 4 },
  { 171, 4 },
  { 171, 1 },
  { 171, 2 },
  { 176, 0 },
  { 176, 1 },
  { 178, 0 },
  { 178, 2 },
  { 180, 2 },
  { 180, 3 },
  { 180, 3 },
  { 180, 3 },
  { 181, 2 },
  { 181, 2 },
  { 181, 1 },
  { 181, 1 },
  { 181, 2 },
  { 179, 3 },
  { 179, 2 },
  { 182, 0 },
  { 182, 2 },
  { 182, 2 },
  { 161, 0 },
  { 184, 1 },
  { 185, 2 },
  { 185, 7 },
  { 185, 5 },
  { 185, 5 },
  { 185, 10 },
  { 188, 0 },
  { 174, 0 },
  { 174, 3 },
  { 189, 0 },
  { 189, 2 },
  { 190, 1 },
  { 190, 1 },
  { 149, 4 },
  { 192, 2 },
  { 192, 0 },
  { 149, 9 },
  { 149, 4 },
  { 149, 1 },
  { 163, 2 },
  { 194, 3 },
  { 197, 1 },
  { 197, 2 },
  { 197, 1 },
  { 195, 9 },
  { 206, 4 },
  { 206, 5 },
  { 198, 1 },
  { 198, 1 },
  { 198, 0 },
  { 209, 0 },
  { 199, 3 },
  { 199, 2 },
  { 199, 4 },
  { 210, 2 },
  { 210, 0 },
  { 200, 0 },
  { 200, 2 },
  { 212, 2 },
  { 212, 0 },
  { 211, 7 },
  { 211, 9 },
  { 211, 7 },
  { 211, 7 },
  { 159, 0 },
  { 159, 2 },
  { 193, 2 },
  { 213, 1 },
  { 213, 2 },
  { 213, 3 },
  { 213, 4 },
  { 215, 2 },
  { 215, 0 },
  { 214, 0 },
  { 214, 3 },
  { 214, 2 },
  { 216, 4 },
  { 216, 0 },
  { 204, 0 },
  { 204, 3 },
  { 186, 4 },
  { 186, 2 },
  { 175, 1 },
  { 175, 1 },
  { 175, 0 },
  { 202, 0 },
  { 202, 3 },
  { 203, 0 },
  { 203, 2 },
  { 205, 0 },
  { 205, 2 },
  { 205, 4 },
  { 205, 4 },
  { 149, 6 },
  { 201, 0 },
  { 201, 2 },
  { 149, 8 },
  { 218, 5 },
  { 218, 3 },
  { 149, 6 },
  { 149, 7 },
  { 219, 2 },
  { 219, 1 },
  { 220, 0 },
  { 220, 3 },
  { 217, 3 },
  { 217, 1 },
  { 173, 3 },
  { 172, 1 },
  { 173, 1 },
  { 173, 1 },
  { 173, 3 },
  { 173, 5 },
  { 172, 1 },
  { 172, 1 },
  { 173, 1 },
  { 173, 3 },
  { 173, 6 },
  { 173, 5 },
  { 173, 4 },
  { 172, 1 },
  { 173, 3 },
  { 173, 3 },
  { 173, 3 },
  { 173, 3 },
  { 173, 3 },
  { 173, 3 },
  { 173, 3 },
  { 173, 3 },
  { 221, 1 },
  { 221, 2 },
  { 173, 3 },
  { 173, 5 },
  { 173, 2 },
  { 173, 3 },
  { 173, 3 },
  { 173, 4 },
  { 173, 2 },
  { 173, 2 },
  { 173, 2 },
  { 173, 2 },
  { 222, 1 },
  { 222, 2 },
  { 173, 5 },
  { 223, 1 },
  { 223, 2 },
  { 173, 5 },
  { 173, 3 },
  { 173, 5 },
  { 173, 4 },
  { 173, 4 },
  { 173, 5 },
  { 225, 5 },
  { 225, 4 },
  { 226, 2 },
  { 226, 0 },
  { 224, 1 },
  { 224, 0 },
  { 208, 0 },
  { 207, 3 },
  { 207, 1 },
  { 149, 12 },
  { 227, 1 },
  { 227, 0 },
  { 177, 0 },
  { 177, 3 },
  { 187, 5 },
  { 187, 3 },
  { 228, 0 },
  { 228, 2 },
  { 149, 4 },
  { 149, 1 },
  { 149, 2 },
  { 149, 3 },
  { 149, 5 },
  { 149, 6 },
  { 149, 5 },
  { 149, 6 },
  { 169, 2 },
  { 170, 2 },
  { 149, 5 },
  { 230, 11 },
  { 232, 1 },
  { 232, 1 },
  { 232, 2 },
  { 232, 0 },
  { 233, 1 },
  { 233, 1 },
  { 233, 3 },
  { 235, 0 },
  { 235, 2 },
  { 231, 3 },
  { 231, 2 },
  { 237, 3 },
  { 238, 3 },
  { 238, 2 },
  { 236, 7 },
  { 236, 5 },
  { 236, 5 },
  { 236, 1 },
  { 173, 4 },
  { 173, 6 },
  { 191, 1 },
  { 191, 1 },
  { 191, 1 },
  { 149, 4 },
  { 149, 6 },
  { 149, 3 },
  { 240, 0 },
  { 240, 2 },
  { 149, 1 },
  { 149, 3 },
  { 149, 1 },
  { 149, 3 },
  { 149, 6 },
  { 149, 7 },
  { 241, 1 },
  { 149, 1 },
  { 149, 4 },
  { 243, 8 },
  { 245, 0 },
  { 246, 1 },
  { 246, 3 },
  { 247, 1 },
  { 196, 0 },
  { 196, 2 },
  { 196, 3 },
  { 249, 6 },
  { 249, 8 },
  { 144, 1 },
  { 145, 2 },
  { 145, 1 },
  { 146, 1 },
  { 146, 3 },
  { 147, 0 },
  { 151, 0 },
  { 151, 1 },
  { 151, 2 },
  { 153, 1 },
  { 153, 0 },
  { 149, 2 },
  { 160, 4 },
  { 160, 2 },
  { 152, 1 },
  { 152, 1 },
  { 152, 1 },
  { 166, 1 },
  { 167, 1 },
  { 168, 1 },
  { 168, 1 },
  { 165, 2 },
  { 165, 0 },
  { 171, 2 },
  { 161, 2 },
  { 183, 3 },
  { 183, 1 },
  { 184, 0 },
  { 188, 1 },
  { 190, 1 },
  { 194, 1 },
  { 195, 1 },
  { 209, 2 },
  { 210, 1 },
  { 173, 1 },
  { 208, 1 },
  { 229, 1 },
  { 229, 1 },
  { 229, 1 },
  { 229, 1 },
  { 229, 1 },
  { 169, 1 },
  { 234, 0 },
  { 234, 3 },
  { 237, 1 },
  { 238, 0 },
  { 239, 1 },
  { 239, 0 },
  { 242, 0 },
  { 242, 1 },
  { 244, 1 },
  { 244, 3 },
  { 245, 2 },
  { 248, 0 },
  { 248, 4 },
  { 248, 2 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  unsigned int yyruleno        /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  sqlite3ParserARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    yysize = yyRuleInfo[yyruleno].nrhs;
    fprintf(yyTraceFILE, "%sReduce [%s], go to state %d.\n", yyTracePrompt,
      yyRuleName[yyruleno], yymsp[-yysize].stateno);
  }
#endif /* NDEBUG */

  /* Check that the stack is large enough to grow by a single entry
  ** if the RHS of the rule is empty.  This ensures that there is room
  ** enough on the stack to push the LHS value */
  if( yyRuleInfo[yyruleno].nrhs==0 ){
#ifdef YYTRACKMAXSTACKDEPTH
    if( yypParser->yyidx>yypParser->yyidxMax ){
      yypParser->yyidxMax = yypParser->yyidx;
    }
#endif
#if YYSTACKDEPTH>0 
    if( yypParser->yyidx>=YYSTACKDEPTH-1 ){
      yyStackOverflow(yypParser);
      return;
    }
#else
    if( yypParser->yyidx>=yypParser->yystksz-1 ){
      yyGrowStack(yypParser);
      if( yypParser->yyidx>=yypParser->yystksz-1 ){
        yyStackOverflow(yypParser);
        return;
      }
    }
#endif
  }

  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
/********** Begin reduce actions **********************************************/
        YYMINORTYPE yylhsminor;
      case 0: /* explain ::= EXPLAIN */
#line 127 "parse.y"
{ pParse->explain = 1; }
#line 2214 "parse.c"
        break;
      case 1: /* explain ::= EXPLAIN QUERY PLAN */
#line 128 "parse.y"
{ pParse->explain = 2; }
#line 2219 "parse.c"
        break;
      case 2: /* cmdx ::= cmd */
#line 130 "parse.y"
{ sqlite3FinishCoding(pParse); }
#line 2224 "parse.c"
        break;
      case 3: /* cmd ::= BEGIN transtype trans_opt */
#line 135 "parse.y"
{sqlite3BeginTransaction(pParse, yymsp[-1].minor.yy392);}
#line 2229 "parse.c"
        break;
      case 4: /* transtype ::= */
#line 140 "parse.y"
{yymsp[1].minor.yy392 = TK_DEFERRED;}
#line 2234 "parse.c"
        break;
      case 5: /* transtype ::= DEFERRED */
      case 6: /* transtype ::= IMMEDIATE */ yytestcase(yyruleno==6);
      case 7: /* transtype ::= EXCLUSIVE */ yytestcase(yyruleno==7);
#line 141 "parse.y"
{yymsp[0].minor.yy392 = yymsp[0].major; /*A-overwrites-X*/}
#line 2241 "parse.c"
        break;
      case 8: /* cmd ::= COMMIT trans_opt */
      case 9: /* cmd ::= END trans_opt */ yytestcase(yyruleno==9);
#line 144 "parse.y"
{sqlite3CommitTransaction(pParse);}
#line 2247 "parse.c"
        break;
      case 10: /* cmd ::= ROLLBACK trans_opt */
#line 146 "parse.y"
{sqlite3RollbackTransaction(pParse);}
#line 2252 "parse.c"
        break;
      case 11: /* cmd ::= SAVEPOINT nm */
#line 150 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_BEGIN, &yymsp[0].minor.yy0);
}
#line 2259 "parse.c"
        break;
      case 12: /* cmd ::= RELEASE savepoint_opt nm */
#line 153 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_RELEASE, &yymsp[0].minor.yy0);
}
#line 2266 "parse.c"
        break;
      case 13: /* cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
#line 156 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_ROLLBACK, &yymsp[0].minor.yy0);
}
#line 2273 "parse.c"
        break;
      case 14: /* create_table ::= createkw temp TABLE ifnotexists nm dbnm */
#line 163 "parse.y"
{
   sqlite3StartTable(pParse,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0,yymsp[-4].minor.yy392,0,0,yymsp[-2].minor.yy392);
}
#line 2280 "parse.c"
        break;
      case 15: /* createkw ::= CREATE */
#line 166 "parse.y"
{disableLookaside(pParse);}
#line 2285 "parse.c"
        break;
      case 16: /* ifnotexists ::= */
      case 19: /* temp ::= */ yytestcase(yyruleno==19);
      case 22: /* table_options ::= */ yytestcase(yyruleno==22);
      case 42: /* autoinc ::= */ yytestcase(yyruleno==42);
      case 57: /* init_deferred_pred_opt ::= */ yytestcase(yyruleno==57);
      case 67: /* defer_subclause_opt ::= */ yytestcase(yyruleno==67);
      case 76: /* ifexists ::= */ yytestcase(yyruleno==76);
      case 90: /* distinct ::= */ yytestcase(yyruleno==90);
      case 209: /* collate ::= */ yytestcase(yyruleno==209);
#line 169 "parse.y"
{yymsp[1].minor.yy392 = 0;}
#line 2298 "parse.c"
        break;
      case 17: /* ifnotexists ::= IF NOT EXISTS */
#line 170 "parse.y"
{yymsp[-2].minor.yy392 = 1;}
#line 2303 "parse.c"
        break;
      case 18: /* temp ::= TEMP */
      case 43: /* autoinc ::= AUTOINCR */ yytestcase(yyruleno==43);
#line 173 "parse.y"
{yymsp[0].minor.yy392 = 1;}
#line 2309 "parse.c"
        break;
      case 20: /* create_table_args ::= LP columnlist conslist_opt RP table_options */
#line 176 "parse.y"
{
  sqlite3EndTable(pParse,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0,yymsp[0].minor.yy392,0);
}
#line 2316 "parse.c"
        break;
      case 21: /* create_table_args ::= AS select */
#line 179 "parse.y"
{
  sqlite3EndTable(pParse,0,0,0,yymsp[0].minor.yy159);
  sqlite3SelectDelete(pParse->db, yymsp[0].minor.yy159);
}
#line 2324 "parse.c"
        break;
      case 23: /* table_options ::= WITHOUT nm */
#line 185 "parse.y"
{
  if( yymsp[0].minor.yy0.n==5 && sqlite3_strnicmp(yymsp[0].minor.yy0.z,"rowid",5)==0 ){
    yymsp[-1].minor.yy392 = TF_WithoutRowid | TF_NoVisibleRowid;
  }else{
    yymsp[-1].minor.yy392 = 0;
    sqlite3ErrorMsg(pParse, "unknown table option: %.*s", yymsp[0].minor.yy0.n, yymsp[0].minor.yy0.z);
  }
}
#line 2336 "parse.c"
        break;
      case 24: /* columnname ::= nm typetoken */
#line 195 "parse.y"
{sqlite3AddColumn(pParse,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0);}
#line 2341 "parse.c"
        break;
      case 25: /* typetoken ::= */
      case 60: /* conslist_opt ::= */ yytestcase(yyruleno==60);
      case 96: /* as ::= */ yytestcase(yyruleno==96);
#line 259 "parse.y"
{yymsp[1].minor.yy0.n = 0; yymsp[1].minor.yy0.z = 0;}
#line 2348 "parse.c"
        break;
      case 26: /* typetoken ::= typename LP signed RP */
#line 261 "parse.y"
{
  yymsp[-3].minor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-3].minor.yy0.z);
}
#line 2355 "parse.c"
        break;
      case 27: /* typetoken ::= typename LP signed COMMA signed RP */
#line 264 "parse.y"
{
  yymsp[-5].minor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-5].minor.yy0.z);
}
#line 2362 "parse.c"
        break;
      case 28: /* typename ::= typename ID|STRING */
#line 269 "parse.y"
{yymsp[-1].minor.yy0.n=yymsp[0].minor.yy0.n+(int)(yymsp[0].minor.yy0.z-yymsp[-1].minor.yy0.z);}
#line 2367 "parse.c"
        break;
      case 29: /* ccons ::= CONSTRAINT nm */
      case 62: /* tcons ::= CONSTRAINT nm */ yytestcase(yyruleno==62);
#line 278 "parse.y"
{pParse->constraintName = yymsp[0].minor.yy0;}
#line 2373 "parse.c"
        break;
      case 30: /* ccons ::= DEFAULT term */
      case 32: /* ccons ::= DEFAULT PLUS term */ yytestcase(yyruleno==32);
#line 279 "parse.y"
{sqlite3AddDefaultValue(pParse,&yymsp[0].minor.yy342);}
#line 2379 "parse.c"
        break;
      case 31: /* ccons ::= DEFAULT LP expr RP */
#line 280 "parse.y"
{sqlite3AddDefaultValue(pParse,&yymsp[-1].minor.yy342);}
#line 2384 "parse.c"
        break;
      case 33: /* ccons ::= DEFAULT MINUS term */
#line 282 "parse.y"
{
  ExprSpan v;
  v.pExpr = sqlite3PExpr(pParse, TK_UMINUS, yymsp[0].minor.yy342.pExpr, 0, 0);
  v.zStart = yymsp[-1].minor.yy0.z;
  v.zEnd = yymsp[0].minor.yy342.zEnd;
  sqlite3AddDefaultValue(pParse,&v);
}
#line 2395 "parse.c"
        break;
      case 34: /* ccons ::= DEFAULT ID|INDEXED */
#line 289 "parse.y"
{
  ExprSpan v;
  spanExpr(&v, pParse, TK_STRING, yymsp[0].minor.yy0);
  sqlite3AddDefaultValue(pParse,&v);
}
#line 2404 "parse.c"
        break;
      case 35: /* ccons ::= NOT NULL onconf */
#line 299 "parse.y"
{sqlite3AddNotNull(pParse, yymsp[0].minor.yy392);}
#line 2409 "parse.c"
        break;
      case 36: /* ccons ::= PRIMARY KEY sortorder onconf autoinc */
#line 301 "parse.y"
{sqlite3AddPrimaryKey(pParse,0,yymsp[-1].minor.yy392,yymsp[0].minor.yy392,yymsp[-2].minor.yy392);}
#line 2414 "parse.c"
        break;
      case 37: /* ccons ::= UNIQUE onconf */
#line 302 "parse.y"
{sqlite3CreateIndex(pParse,0,0,0,0,yymsp[0].minor.yy392,0,0,0,0);}
#line 2419 "parse.c"
        break;
      case 38: /* ccons ::= CHECK LP expr RP */
#line 303 "parse.y"
{sqlite3AddCheckConstraint(pParse,yymsp[-1].minor.yy342.pExpr);}
#line 2424 "parse.c"
        break;
      case 39: /* ccons ::= REFERENCES nm eidlist_opt refargs */
#line 305 "parse.y"
{sqlite3CreateForeignKey(pParse,0,&yymsp[-2].minor.yy0,yymsp[-1].minor.yy442,yymsp[0].minor.yy392);}
#line 2429 "parse.c"
        break;
      case 40: /* ccons ::= defer_subclause */
#line 306 "parse.y"
{sqlite3DeferForeignKey(pParse,yymsp[0].minor.yy392);}
#line 2434 "parse.c"
        break;
      case 41: /* ccons ::= COLLATE ID|STRING */
#line 307 "parse.y"
{sqlite3AddCollateType(pParse, &yymsp[0].minor.yy0);}
#line 2439 "parse.c"
        break;
      case 44: /* refargs ::= */
#line 320 "parse.y"
{ yymsp[1].minor.yy392 = OE_None*0x0101; /* EV: R-19803-45884 */}
#line 2444 "parse.c"
        break;
      case 45: /* refargs ::= refargs refarg */
#line 321 "parse.y"
{ yymsp[-1].minor.yy392 = (yymsp[-1].minor.yy392 & ~yymsp[0].minor.yy207.mask) | yymsp[0].minor.yy207.value; }
#line 2449 "parse.c"
        break;
      case 46: /* refarg ::= MATCH nm */
#line 323 "parse.y"
{ yymsp[-1].minor.yy207.value = 0;     yymsp[-1].minor.yy207.mask = 0x000000; }
#line 2454 "parse.c"
        break;
      case 47: /* refarg ::= ON INSERT refact */
#line 324 "parse.y"
{ yymsp[-2].minor.yy207.value = 0;     yymsp[-2].minor.yy207.mask = 0x000000; }
#line 2459 "parse.c"
        break;
      case 48: /* refarg ::= ON DELETE refact */
#line 325 "parse.y"
{ yymsp[-2].minor.yy207.value = yymsp[0].minor.yy392;     yymsp[-2].minor.yy207.mask = 0x0000ff; }
#line 2464 "parse.c"
        break;
      case 49: /* refarg ::= ON UPDATE refact */
#line 326 "parse.y"
{ yymsp[-2].minor.yy207.value = yymsp[0].minor.yy392<<8;  yymsp[-2].minor.yy207.mask = 0x00ff00; }
#line 2469 "parse.c"
        break;
      case 50: /* refact ::= SET NULL */
#line 328 "parse.y"
{ yymsp[-1].minor.yy392 = OE_SetNull;  /* EV: R-33326-45252 */}
#line 2474 "parse.c"
        break;
      case 51: /* refact ::= SET DEFAULT */
#line 329 "parse.y"
{ yymsp[-1].minor.yy392 = OE_SetDflt;  /* EV: R-33326-45252 */}
#line 2479 "parse.c"
        break;
      case 52: /* refact ::= CASCADE */
#line 330 "parse.y"
{ yymsp[0].minor.yy392 = OE_Cascade;  /* EV: R-33326-45252 */}
#line 2484 "parse.c"
        break;
      case 53: /* refact ::= RESTRICT */
#line 331 "parse.y"
{ yymsp[0].minor.yy392 = OE_Restrict; /* EV: R-33326-45252 */}
#line 2489 "parse.c"
        break;
      case 54: /* refact ::= NO ACTION */
#line 332 "parse.y"
{ yymsp[-1].minor.yy392 = OE_None;     /* EV: R-33326-45252 */}
#line 2494 "parse.c"
        break;
      case 55: /* defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
#line 334 "parse.y"
{yymsp[-2].minor.yy392 = 0;}
#line 2499 "parse.c"
        break;
      case 56: /* defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
      case 71: /* orconf ::= OR resolvetype */ yytestcase(yyruleno==71);
      case 142: /* insert_cmd ::= INSERT orconf */ yytestcase(yyruleno==142);
#line 335 "parse.y"
{yymsp[-1].minor.yy392 = yymsp[0].minor.yy392;}
#line 2506 "parse.c"
        break;
      case 58: /* init_deferred_pred_opt ::= INITIALLY DEFERRED */
      case 75: /* ifexists ::= IF EXISTS */ yytestcase(yyruleno==75);
      case 183: /* between_op ::= NOT BETWEEN */ yytestcase(yyruleno==183);
      case 186: /* in_op ::= NOT IN */ yytestcase(yyruleno==186);
      case 210: /* collate ::= COLLATE ID|STRING */ yytestcase(yyruleno==210);
#line 338 "parse.y"
{yymsp[-1].minor.yy392 = 1;}
#line 2515 "parse.c"
        break;
      case 59: /* init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
#line 339 "parse.y"
{yymsp[-1].minor.yy392 = 0;}
#line 2520 "parse.c"
        break;
      case 61: /* tconscomma ::= COMMA */
#line 345 "parse.y"
{pParse->constraintName.n = 0;}
#line 2525 "parse.c"
        break;
      case 63: /* tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
#line 349 "parse.y"
{sqlite3AddPrimaryKey(pParse,yymsp[-3].minor.yy442,yymsp[0].minor.yy392,yymsp[-2].minor.yy392,0);}
#line 2530 "parse.c"
        break;
      case 64: /* tcons ::= UNIQUE LP sortlist RP onconf */
#line 351 "parse.y"
{sqlite3CreateIndex(pParse,0,0,0,yymsp[-2].minor.yy442,yymsp[0].minor.yy392,0,0,0,0);}
#line 2535 "parse.c"
        break;
      case 65: /* tcons ::= CHECK LP expr RP onconf */
#line 353 "parse.y"
{sqlite3AddCheckConstraint(pParse,yymsp[-2].minor.yy342.pExpr);}
#line 2540 "parse.c"
        break;
      case 66: /* tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
#line 355 "parse.y"
{
    sqlite3CreateForeignKey(pParse, yymsp[-6].minor.yy442, &yymsp[-3].minor.yy0, yymsp[-2].minor.yy442, yymsp[-1].minor.yy392);
    sqlite3DeferForeignKey(pParse, yymsp[0].minor.yy392);
}
#line 2548 "parse.c"
        break;
      case 68: /* onconf ::= */
      case 70: /* orconf ::= */ yytestcase(yyruleno==70);
#line 369 "parse.y"
{yymsp[1].minor.yy392 = OE_Default;}
#line 2554 "parse.c"
        break;
      case 69: /* onconf ::= ON CONFLICT resolvetype */
#line 370 "parse.y"
{yymsp[-2].minor.yy392 = yymsp[0].minor.yy392;}
#line 2559 "parse.c"
        break;
      case 72: /* resolvetype ::= IGNORE */
#line 374 "parse.y"
{yymsp[0].minor.yy392 = OE_Ignore;}
#line 2564 "parse.c"
        break;
      case 73: /* resolvetype ::= REPLACE */
      case 143: /* insert_cmd ::= REPLACE */ yytestcase(yyruleno==143);
#line 375 "parse.y"
{yymsp[0].minor.yy392 = OE_Replace;}
#line 2570 "parse.c"
        break;
      case 74: /* cmd ::= DROP TABLE ifexists fullname */
#line 379 "parse.y"
{
  sqlite3DropTable(pParse, yymsp[0].minor.yy347, 0, yymsp[-1].minor.yy392);
}
#line 2577 "parse.c"
        break;
      case 77: /* cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
#line 390 "parse.y"
{
  sqlite3CreateView(pParse, &yymsp[-8].minor.yy0, &yymsp[-4].minor.yy0, &yymsp[-3].minor.yy0, yymsp[-2].minor.yy442, yymsp[0].minor.yy159, yymsp[-7].minor.yy392, yymsp[-5].minor.yy392);
}
#line 2584 "parse.c"
        break;
      case 78: /* cmd ::= DROP VIEW ifexists fullname */
#line 393 "parse.y"
{
  sqlite3DropTable(pParse, yymsp[0].minor.yy347, 1, yymsp[-1].minor.yy392);
}
#line 2591 "parse.c"
        break;
      case 79: /* cmd ::= select */
#line 400 "parse.y"
{
  SelectDest dest = {SRT_Output, 0, 0, 0, 0, 0};
  sqlite3Select(pParse, yymsp[0].minor.yy159, &dest);
  sqlite3SelectDelete(pParse->db, yymsp[0].minor.yy159);
}
#line 2600 "parse.c"
        break;
      case 80: /* select ::= with selectnowith */
#line 437 "parse.y"
{
  Select *p = yymsp[0].minor.yy159;
  if( p ){
    p->pWith = yymsp[-1].minor.yy331;
    parserDoubleLinkSelect(pParse, p);
  }else{
    sqlite3WithDelete(pParse->db, yymsp[-1].minor.yy331);
  }
  yymsp[-1].minor.yy159 = p; /*A-overwrites-W*/
}
#line 2614 "parse.c"
        break;
      case 81: /* selectnowith ::= selectnowith multiselect_op oneselect */
#line 450 "parse.y"
{
  Select *pRhs = yymsp[0].minor.yy159;
  Select *pLhs = yymsp[-2].minor.yy159;
  if( pRhs && pRhs->pPrior ){
    SrcList *pFrom;
    Token x;
    x.n = 0;
    parserDoubleLinkSelect(pParse, pRhs);
    pFrom = sqlite3SrcListAppendFromTerm(pParse,0,0,0,&x,pRhs,0,0);
    pRhs = sqlite3SelectNew(pParse,0,pFrom,0,0,0,0,0,0,0);
  }
  if( pRhs ){
    pRhs->op = (u8)yymsp[-1].minor.yy392;
    pRhs->pPrior = pLhs;
    if( ALWAYS(pLhs) ) pLhs->selFlags &= ~SF_MultiValue;
    pRhs->selFlags &= ~SF_MultiValue;
    if( yymsp[-1].minor.yy392!=TK_ALL ) pParse->hasCompound = 1;
  }else{
    sqlite3SelectDelete(pParse->db, pLhs);
  }
  yymsp[-2].minor.yy159 = pRhs;
}
#line 2640 "parse.c"
        break;
      case 82: /* multiselect_op ::= UNION */
      case 84: /* multiselect_op ::= EXCEPT|INTERSECT */ yytestcase(yyruleno==84);
#line 473 "parse.y"
{yymsp[0].minor.yy392 = yymsp[0].major; /*A-overwrites-OP*/}
#line 2646 "parse.c"
        break;
      case 83: /* multiselect_op ::= UNION ALL */
#line 474 "parse.y"
{yymsp[-1].minor.yy392 = TK_ALL;}
#line 2651 "parse.c"
        break;
      case 85: /* oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
#line 478 "parse.y"
{
#if SELECTTRACE_ENABLED
  Token s = yymsp[-8].minor.yy0; /*A-overwrites-S*/
#endif
  yymsp[-8].minor.yy159 = sqlite3SelectNew(pParse,yymsp[-6].minor.yy442,yymsp[-5].minor.yy347,yymsp[-4].minor.yy122,yymsp[-3].minor.yy442,yymsp[-2].minor.yy122,yymsp[-1].minor.yy442,yymsp[-7].minor.yy392,yymsp[0].minor.yy64.pLimit,yymsp[0].minor.yy64.pOffset);
#if SELECTTRACE_ENABLED
  /* Populate the Select.zSelName[] string that is used to help with
  ** query planner debugging, to differentiate between multiple Select
  ** objects in a complex query.
  **
  ** If the SELECT keyword is immediately followed by a C-style comment
  ** then extract the first few alphanumeric characters from within that
  ** comment to be the zSelName value.  Otherwise, the label is #N where
  ** is an integer that is incremented with each SELECT statement seen.
  */
  if( yymsp[-8].minor.yy159!=0 ){
    const char *z = s.z+6;
    int i;
    sqlite3_snprintf(sizeof(yymsp[-8].minor.yy159->zSelName), yymsp[-8].minor.yy159->zSelName, "#%d",
                     ++pParse->nSelect);
    while( z[0]==' ' ) z++;
    if( z[0]=='/' && z[1]=='*' ){
      z += 2;
      while( z[0]==' ' ) z++;
      for(i=0; sqlite3Isalnum(z[i]); i++){}
      sqlite3_snprintf(sizeof(yymsp[-8].minor.yy159->zSelName), yymsp[-8].minor.yy159->zSelName, "%.*s", i, z);
    }
  }
#endif /* SELECTRACE_ENABLED */
}
#line 2685 "parse.c"
        break;
      case 86: /* values ::= VALUES LP nexprlist RP */
#line 512 "parse.y"
{
  yymsp[-3].minor.yy159 = sqlite3SelectNew(pParse,yymsp[-1].minor.yy442,0,0,0,0,0,SF_Values,0,0);
}
#line 2692 "parse.c"
        break;
      case 87: /* values ::= values COMMA LP exprlist RP */
#line 515 "parse.y"
{
  Select *pRight, *pLeft = yymsp[-4].minor.yy159;
  pRight = sqlite3SelectNew(pParse,yymsp[-1].minor.yy442,0,0,0,0,0,SF_Values|SF_MultiValue,0,0);
  if( ALWAYS(pLeft) ) pLeft->selFlags &= ~SF_MultiValue;
  if( pRight ){
    pRight->op = TK_ALL;
    pRight->pPrior = pLeft;
    yymsp[-4].minor.yy159 = pRight;
  }else{
    yymsp[-4].minor.yy159 = pLeft;
  }
}
#line 2708 "parse.c"
        break;
      case 88: /* distinct ::= DISTINCT */
#line 532 "parse.y"
{yymsp[0].minor.yy392 = SF_Distinct;}
#line 2713 "parse.c"
        break;
      case 89: /* distinct ::= ALL */
#line 533 "parse.y"
{yymsp[0].minor.yy392 = SF_All;}
#line 2718 "parse.c"
        break;
      case 91: /* sclp ::= */
      case 119: /* orderby_opt ::= */ yytestcase(yyruleno==119);
      case 126: /* groupby_opt ::= */ yytestcase(yyruleno==126);
      case 199: /* exprlist ::= */ yytestcase(yyruleno==199);
      case 205: /* eidlist_opt ::= */ yytestcase(yyruleno==205);
#line 546 "parse.y"
{yymsp[1].minor.yy442 = 0;}
#line 2727 "parse.c"
        break;
      case 92: /* selcollist ::= sclp expr as */
#line 547 "parse.y"
{
   yymsp[-2].minor.yy442 = sqlite3ExprListAppend(pParse, yymsp[-2].minor.yy442, yymsp[-1].minor.yy342.pExpr);
   if( yymsp[0].minor.yy0.n>0 ) sqlite3ExprListSetName(pParse, yymsp[-2].minor.yy442, &yymsp[0].minor.yy0, 1);
   sqlite3ExprListSetSpan(pParse,yymsp[-2].minor.yy442,&yymsp[-1].minor.yy342);
}
#line 2736 "parse.c"
        break;
      case 93: /* selcollist ::= sclp STAR */
#line 552 "parse.y"
{
  Expr *p = sqlite3Expr(pParse->db, TK_ASTERISK, 0);
  yymsp[-1].minor.yy442 = sqlite3ExprListAppend(pParse, yymsp[-1].minor.yy442, p);
}
#line 2744 "parse.c"
        break;
      case 94: /* selcollist ::= sclp nm DOT STAR */
#line 556 "parse.y"
{
  Expr *pRight = sqlite3PExpr(pParse, TK_ASTERISK, 0, 0, &yymsp[0].minor.yy0);
  Expr *pLeft = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[-2].minor.yy0);
  Expr *pDot = sqlite3PExpr(pParse, TK_DOT, pLeft, pRight, 0);
  yymsp[-3].minor.yy442 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy442, pDot);
}
#line 2754 "parse.c"
        break;
      case 95: /* as ::= AS nm */
      case 106: /* dbnm ::= DOT nm */ yytestcase(yyruleno==106);
      case 219: /* plus_num ::= PLUS INTEGER|FLOAT */ yytestcase(yyruleno==219);
      case 220: /* minus_num ::= MINUS INTEGER|FLOAT */ yytestcase(yyruleno==220);
#line 567 "parse.y"
{yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;}
#line 2762 "parse.c"
        break;
      case 97: /* from ::= */
#line 581 "parse.y"
{yymsp[1].minor.yy347 = sqlite3DbMallocZero(pParse->db, sizeof(*yymsp[1].minor.yy347));}
#line 2767 "parse.c"
        break;
      case 98: /* from ::= FROM seltablist */
#line 582 "parse.y"
{
  yymsp[-1].minor.yy347 = yymsp[0].minor.yy347;
  sqlite3SrcListShiftJoinType(yymsp[-1].minor.yy347);
}
#line 2775 "parse.c"
        break;
      case 99: /* stl_prefix ::= seltablist joinop */
#line 590 "parse.y"
{
   if( ALWAYS(yymsp[-1].minor.yy347 && yymsp[-1].minor.yy347->nSrc>0) ) yymsp[-1].minor.yy347->a[yymsp[-1].minor.yy347->nSrc-1].fg.jointype = (u8)yymsp[0].minor.yy392;
}
#line 2782 "parse.c"
        break;
      case 100: /* stl_prefix ::= */
#line 593 "parse.y"
{yymsp[1].minor.yy347 = 0;}
#line 2787 "parse.c"
        break;
      case 101: /* seltablist ::= stl_prefix nm dbnm as indexed_opt on_opt using_opt */
#line 595 "parse.y"
{
  yymsp[-6].minor.yy347 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy347,&yymsp[-5].minor.yy0,&yymsp[-4].minor.yy0,&yymsp[-3].minor.yy0,0,yymsp[-1].minor.yy122,yymsp[0].minor.yy180);
  sqlite3SrcListIndexedBy(pParse, yymsp[-6].minor.yy347, &yymsp[-2].minor.yy0);
}
#line 2795 "parse.c"
        break;
      case 102: /* seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_opt using_opt */
#line 600 "parse.y"
{
  yymsp[-8].minor.yy347 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-8].minor.yy347,&yymsp[-7].minor.yy0,&yymsp[-6].minor.yy0,&yymsp[-2].minor.yy0,0,yymsp[-1].minor.yy122,yymsp[0].minor.yy180);
  sqlite3SrcListFuncArgs(pParse, yymsp[-8].minor.yy347, yymsp[-4].minor.yy442);
}
#line 2803 "parse.c"
        break;
      case 103: /* seltablist ::= stl_prefix LP select RP as on_opt using_opt */
#line 606 "parse.y"
{
    yymsp[-6].minor.yy347 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy347,0,0,&yymsp[-2].minor.yy0,yymsp[-4].minor.yy159,yymsp[-1].minor.yy122,yymsp[0].minor.yy180);
  }
#line 2810 "parse.c"
        break;
      case 104: /* seltablist ::= stl_prefix LP seltablist RP as on_opt using_opt */
#line 610 "parse.y"
{
    if( yymsp[-6].minor.yy347==0 && yymsp[-2].minor.yy0.n==0 && yymsp[-1].minor.yy122==0 && yymsp[0].minor.yy180==0 ){
      yymsp[-6].minor.yy347 = yymsp[-4].minor.yy347;
    }else if( yymsp[-4].minor.yy347->nSrc==1 ){
      yymsp[-6].minor.yy347 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy347,0,0,&yymsp[-2].minor.yy0,0,yymsp[-1].minor.yy122,yymsp[0].minor.yy180);
      if( yymsp[-6].minor.yy347 ){
        struct SrcList_item *pNew = &yymsp[-6].minor.yy347->a[yymsp[-6].minor.yy347->nSrc-1];
        struct SrcList_item *pOld = yymsp[-4].minor.yy347->a;
        pNew->zName = pOld->zName;
        pNew->zDatabase = pOld->zDatabase;
        pNew->pSelect = pOld->pSelect;
        pOld->zName = pOld->zDatabase = 0;
        pOld->pSelect = 0;
      }
      sqlite3SrcListDelete(pParse->db, yymsp[-4].minor.yy347);
    }else{
      Select *pSubquery;
      sqlite3SrcListShiftJoinType(yymsp[-4].minor.yy347);
      pSubquery = sqlite3SelectNew(pParse,0,yymsp[-4].minor.yy347,0,0,0,0,SF_NestedFrom,0,0);
      yymsp[-6].minor.yy347 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy347,0,0,&yymsp[-2].minor.yy0,pSubquery,yymsp[-1].minor.yy122,yymsp[0].minor.yy180);
    }
  }
#line 2836 "parse.c"
        break;
      case 105: /* dbnm ::= */
      case 114: /* indexed_opt ::= */ yytestcase(yyruleno==114);
#line 635 "parse.y"
{yymsp[1].minor.yy0.z=0; yymsp[1].minor.yy0.n=0;}
#line 2842 "parse.c"
        break;
      case 107: /* fullname ::= nm dbnm */
#line 641 "parse.y"
{yymsp[-1].minor.yy347 = sqlite3SrcListAppend(pParse->db,0,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/}
#line 2847 "parse.c"
        break;
      case 108: /* joinop ::= COMMA|JOIN */
#line 644 "parse.y"
{ yymsp[0].minor.yy392 = JT_INNER; }
#line 2852 "parse.c"
        break;
      case 109: /* joinop ::= JOIN_KW JOIN */
#line 646 "parse.y"
{yymsp[-1].minor.yy392 = sqlite3JoinType(pParse,&yymsp[-1].minor.yy0,0,0);  /*X-overwrites-A*/}
#line 2857 "parse.c"
        break;
      case 110: /* joinop ::= JOIN_KW nm JOIN */
#line 648 "parse.y"
{yymsp[-2].minor.yy392 = sqlite3JoinType(pParse,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0,0); /*X-overwrites-A*/}
#line 2862 "parse.c"
        break;
      case 111: /* joinop ::= JOIN_KW nm nm JOIN */
#line 650 "parse.y"
{yymsp[-3].minor.yy392 = sqlite3JoinType(pParse,&yymsp[-3].minor.yy0,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0);/*X-overwrites-A*/}
#line 2867 "parse.c"
        break;
      case 112: /* on_opt ::= ON expr */
      case 129: /* having_opt ::= HAVING expr */ yytestcase(yyruleno==129);
      case 136: /* where_opt ::= WHERE expr */ yytestcase(yyruleno==136);
      case 195: /* case_else ::= ELSE expr */ yytestcase(yyruleno==195);
#line 654 "parse.y"
{yymsp[-1].minor.yy122 = yymsp[0].minor.yy342.pExpr;}
#line 2875 "parse.c"
        break;
      case 113: /* on_opt ::= */
      case 128: /* having_opt ::= */ yytestcase(yyruleno==128);
      case 135: /* where_opt ::= */ yytestcase(yyruleno==135);
      case 196: /* case_else ::= */ yytestcase(yyruleno==196);
      case 198: /* case_operand ::= */ yytestcase(yyruleno==198);
#line 655 "parse.y"
{yymsp[1].minor.yy122 = 0;}
#line 2884 "parse.c"
        break;
      case 115: /* indexed_opt ::= INDEXED BY nm */
#line 669 "parse.y"
{yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;}
#line 2889 "parse.c"
        break;
      case 116: /* indexed_opt ::= NOT INDEXED */
#line 670 "parse.y"
{yymsp[-1].minor.yy0.z=0; yymsp[-1].minor.yy0.n=1;}
#line 2894 "parse.c"
        break;
      case 117: /* using_opt ::= USING LP idlist RP */
#line 674 "parse.y"
{yymsp[-3].minor.yy180 = yymsp[-1].minor.yy180;}
#line 2899 "parse.c"
        break;
      case 118: /* using_opt ::= */
      case 144: /* idlist_opt ::= */ yytestcase(yyruleno==144);
#line 675 "parse.y"
{yymsp[1].minor.yy180 = 0;}
#line 2905 "parse.c"
        break;
      case 120: /* orderby_opt ::= ORDER BY sortlist */
      case 127: /* groupby_opt ::= GROUP BY nexprlist */ yytestcase(yyruleno==127);
#line 689 "parse.y"
{yymsp[-2].minor.yy442 = yymsp[0].minor.yy442;}
#line 2911 "parse.c"
        break;
      case 121: /* sortlist ::= sortlist COMMA expr sortorder */
#line 690 "parse.y"
{
  yymsp[-3].minor.yy442 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy442,yymsp[-1].minor.yy342.pExpr);
  sqlite3ExprListSetSortOrder(yymsp[-3].minor.yy442,yymsp[0].minor.yy392);
}
#line 2919 "parse.c"
        break;
      case 122: /* sortlist ::= expr sortorder */
#line 694 "parse.y"
{
  yymsp[-1].minor.yy442 = sqlite3ExprListAppend(pParse,0,yymsp[-1].minor.yy342.pExpr); /*A-overwrites-Y*/
  sqlite3ExprListSetSortOrder(yymsp[-1].minor.yy442,yymsp[0].minor.yy392);
}
#line 2927 "parse.c"
        break;
      case 123: /* sortorder ::= ASC */
#line 701 "parse.y"
{yymsp[0].minor.yy392 = SQLITE_SO_ASC;}
#line 2932 "parse.c"
        break;
      case 124: /* sortorder ::= DESC */
#line 702 "parse.y"
{yymsp[0].minor.yy392 = SQLITE_SO_DESC;}
#line 2937 "parse.c"
        break;
      case 125: /* sortorder ::= */
#line 703 "parse.y"
{yymsp[1].minor.yy392 = SQLITE_SO_UNDEFINED;}
#line 2942 "parse.c"
        break;
      case 130: /* limit_opt ::= */
#line 728 "parse.y"
{yymsp[1].minor.yy64.pLimit = 0; yymsp[1].minor.yy64.pOffset = 0;}
#line 2947 "parse.c"
        break;
      case 131: /* limit_opt ::= LIMIT expr */
#line 729 "parse.y"
{yymsp[-1].minor.yy64.pLimit = yymsp[0].minor.yy342.pExpr; yymsp[-1].minor.yy64.pOffset = 0;}
#line 2952 "parse.c"
        break;
      case 132: /* limit_opt ::= LIMIT expr OFFSET expr */
#line 731 "parse.y"
{yymsp[-3].minor.yy64.pLimit = yymsp[-2].minor.yy342.pExpr; yymsp[-3].minor.yy64.pOffset = yymsp[0].minor.yy342.pExpr;}
#line 2957 "parse.c"
        break;
      case 133: /* limit_opt ::= LIMIT expr COMMA expr */
#line 733 "parse.y"
{yymsp[-3].minor.yy64.pOffset = yymsp[-2].minor.yy342.pExpr; yymsp[-3].minor.yy64.pLimit = yymsp[0].minor.yy342.pExpr;}
#line 2962 "parse.c"
        break;
      case 134: /* cmd ::= with DELETE FROM fullname indexed_opt where_opt */
#line 747 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-5].minor.yy331, 1);
  sqlite3SrcListIndexedBy(pParse, yymsp[-2].minor.yy347, &yymsp[-1].minor.yy0);
  sqlite3DeleteFrom(pParse,yymsp[-2].minor.yy347,yymsp[0].minor.yy122);
}
#line 2971 "parse.c"
        break;
      case 137: /* cmd ::= with UPDATE orconf fullname indexed_opt SET setlist where_opt */
#line 774 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-7].minor.yy331, 1);
  sqlite3SrcListIndexedBy(pParse, yymsp[-4].minor.yy347, &yymsp[-3].minor.yy0);
  sqlite3ExprListCheckLength(pParse,yymsp[-1].minor.yy442,"set list"); 
  sqlite3Update(pParse,yymsp[-4].minor.yy347,yymsp[-1].minor.yy442,yymsp[0].minor.yy122,yymsp[-5].minor.yy392);
}
#line 2981 "parse.c"
        break;
      case 138: /* setlist ::= setlist COMMA nm EQ expr */
#line 785 "parse.y"
{
  yymsp[-4].minor.yy442 = sqlite3ExprListAppend(pParse, yymsp[-4].minor.yy442, yymsp[0].minor.yy342.pExpr);
  sqlite3ExprListSetName(pParse, yymsp[-4].minor.yy442, &yymsp[-2].minor.yy0, 1);
}
#line 2989 "parse.c"
        break;
      case 139: /* setlist ::= nm EQ expr */
#line 789 "parse.y"
{
  yylhsminor.yy442 = sqlite3ExprListAppend(pParse, 0, yymsp[0].minor.yy342.pExpr);
  sqlite3ExprListSetName(pParse, yylhsminor.yy442, &yymsp[-2].minor.yy0, 1);
}
#line 2997 "parse.c"
  yymsp[-2].minor.yy442 = yylhsminor.yy442;
        break;
      case 140: /* cmd ::= with insert_cmd INTO fullname idlist_opt select */
#line 796 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-5].minor.yy331, 1);
  sqlite3Insert(pParse, yymsp[-2].minor.yy347, yymsp[0].minor.yy159, yymsp[-1].minor.yy180, yymsp[-4].minor.yy392);
}
#line 3006 "parse.c"
        break;
      case 141: /* cmd ::= with insert_cmd INTO fullname idlist_opt DEFAULT VALUES */
#line 801 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-6].minor.yy331, 1);
  sqlite3Insert(pParse, yymsp[-3].minor.yy347, 0, yymsp[-2].minor.yy180, yymsp[-5].minor.yy392);
}
#line 3014 "parse.c"
        break;
      case 145: /* idlist_opt ::= LP idlist RP */
#line 816 "parse.y"
{yymsp[-2].minor.yy180 = yymsp[-1].minor.yy180;}
#line 3019 "parse.c"
        break;
      case 146: /* idlist ::= idlist COMMA nm */
#line 818 "parse.y"
{yymsp[-2].minor.yy180 = sqlite3IdListAppend(pParse->db,yymsp[-2].minor.yy180,&yymsp[0].minor.yy0);}
#line 3024 "parse.c"
        break;
      case 147: /* idlist ::= nm */
#line 820 "parse.y"
{yymsp[0].minor.yy180 = sqlite3IdListAppend(pParse->db,0,&yymsp[0].minor.yy0); /*A-overwrites-Y*/}
#line 3029 "parse.c"
        break;
      case 148: /* expr ::= LP expr RP */
#line 853 "parse.y"
{spanSet(&yymsp[-2].minor.yy342,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/  yymsp[-2].minor.yy342.pExpr = yymsp[-1].minor.yy342.pExpr;}
#line 3034 "parse.c"
        break;
      case 149: /* term ::= NULL */
      case 154: /* term ::= INTEGER|FLOAT|BLOB */ yytestcase(yyruleno==154);
      case 155: /* term ::= STRING */ yytestcase(yyruleno==155);
#line 854 "parse.y"
{spanExpr(&yymsp[0].minor.yy342,pParse,yymsp[0].major,yymsp[0].minor.yy0);/*A-overwrites-X*/}
#line 3041 "parse.c"
        break;
      case 150: /* expr ::= ID|INDEXED */
      case 151: /* expr ::= JOIN_KW */ yytestcase(yyruleno==151);
#line 855 "parse.y"
{spanExpr(&yymsp[0].minor.yy342,pParse,TK_ID,yymsp[0].minor.yy0); /*A-overwrites-X*/}
#line 3047 "parse.c"
        break;
      case 152: /* expr ::= nm DOT nm */
#line 857 "parse.y"
{
  Expr *temp1 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[-2].minor.yy0);
  Expr *temp2 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[0].minor.yy0);
  spanSet(&yymsp[-2].minor.yy342,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/
  yymsp[-2].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_DOT, temp1, temp2, 0);
}
#line 3057 "parse.c"
        break;
      case 153: /* expr ::= nm DOT nm DOT nm */
#line 863 "parse.y"
{
  Expr *temp1 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[-4].minor.yy0);
  Expr *temp2 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[-2].minor.yy0);
  Expr *temp3 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[0].minor.yy0);
  Expr *temp4 = sqlite3PExpr(pParse, TK_DOT, temp2, temp3, 0);
  spanSet(&yymsp[-4].minor.yy342,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/
  yymsp[-4].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_DOT, temp1, temp4, 0);
}
#line 3069 "parse.c"
        break;
      case 156: /* expr ::= VARIABLE */
#line 873 "parse.y"
{
  Token t = yymsp[0].minor.yy0; /*A-overwrites-X*/
  if( t.n>=2 && t.z[0]=='#' && sqlite3Isdigit(t.z[1]) ){
    /* When doing a nested parse, one can include terms in an expression
    ** that look like this:   #1 #2 ...  These terms refer to registers
    ** in the virtual machine.  #N is the N-th register. */
    spanSet(&yymsp[0].minor.yy342, &t, &t);
    if( pParse->nested==0 ){
      sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", &t);
      yymsp[0].minor.yy342.pExpr = 0;
    }else{
      yymsp[0].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_REGISTER, 0, 0, &t);
      if( yymsp[0].minor.yy342.pExpr ) sqlite3GetInt32(&t.z[1], &yymsp[0].minor.yy342.pExpr->iTable);
    }
  }else{
    spanExpr(&yymsp[0].minor.yy342, pParse, TK_VARIABLE, t);
    sqlite3ExprAssignVarNumber(pParse, yymsp[0].minor.yy342.pExpr);
  }
}
#line 3092 "parse.c"
        break;
      case 157: /* expr ::= expr COLLATE ID|STRING */
#line 892 "parse.y"
{
  yymsp[-2].minor.yy342.pExpr = sqlite3ExprAddCollateToken(pParse, yymsp[-2].minor.yy342.pExpr, &yymsp[0].minor.yy0, 1);
  yymsp[-2].minor.yy342.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
}
#line 3100 "parse.c"
        break;
      case 158: /* expr ::= CAST LP expr AS typetoken RP */
#line 897 "parse.y"
{
  spanSet(&yymsp[-5].minor.yy342,&yymsp[-5].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/
  yymsp[-5].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_CAST, yymsp[-3].minor.yy342.pExpr, 0, &yymsp[-1].minor.yy0);
}
#line 3108 "parse.c"
        break;
      case 159: /* expr ::= ID|INDEXED LP distinct exprlist RP */
#line 902 "parse.y"
{
  if( yymsp[-1].minor.yy442 && yymsp[-1].minor.yy442->nExpr>pParse->db->aLimit[SQLITE_LIMIT_FUNCTION_ARG] ){
    sqlite3ErrorMsg(pParse, "too many arguments on function %T", &yymsp[-4].minor.yy0);
  }
  yylhsminor.yy342.pExpr = sqlite3ExprFunction(pParse, yymsp[-1].minor.yy442, &yymsp[-4].minor.yy0);
  spanSet(&yylhsminor.yy342,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0);
  if( yymsp[-2].minor.yy392==SF_Distinct && yylhsminor.yy342.pExpr ){
    yylhsminor.yy342.pExpr->flags |= EP_Distinct;
  }
}
#line 3122 "parse.c"
  yymsp[-4].minor.yy342 = yylhsminor.yy342;
        break;
      case 160: /* expr ::= ID|INDEXED LP STAR RP */
#line 912 "parse.y"
{
  yylhsminor.yy342.pExpr = sqlite3ExprFunction(pParse, 0, &yymsp[-3].minor.yy0);
  spanSet(&yylhsminor.yy342,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);
}
#line 3131 "parse.c"
  yymsp[-3].minor.yy342 = yylhsminor.yy342;
        break;
      case 161: /* term ::= CTIME_KW */
#line 916 "parse.y"
{
  yylhsminor.yy342.pExpr = sqlite3ExprFunction(pParse, 0, &yymsp[0].minor.yy0);
  spanSet(&yylhsminor.yy342, &yymsp[0].minor.yy0, &yymsp[0].minor.yy0);
}
#line 3140 "parse.c"
  yymsp[0].minor.yy342 = yylhsminor.yy342;
        break;
      case 162: /* expr ::= expr AND expr */
      case 163: /* expr ::= expr OR expr */ yytestcase(yyruleno==163);
      case 164: /* expr ::= expr LT|GT|GE|LE expr */ yytestcase(yyruleno==164);
      case 165: /* expr ::= expr EQ|NE expr */ yytestcase(yyruleno==165);
      case 166: /* expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */ yytestcase(yyruleno==166);
      case 167: /* expr ::= expr PLUS|MINUS expr */ yytestcase(yyruleno==167);
      case 168: /* expr ::= expr STAR|SLASH|REM expr */ yytestcase(yyruleno==168);
      case 169: /* expr ::= expr CONCAT expr */ yytestcase(yyruleno==169);
#line 945 "parse.y"
{spanBinaryExpr(pParse,yymsp[-1].major,&yymsp[-2].minor.yy342,&yymsp[0].minor.yy342);}
#line 3153 "parse.c"
        break;
      case 170: /* likeop ::= LIKE_KW|MATCH */
#line 958 "parse.y"
{yymsp[0].minor.yy318.eOperator = yymsp[0].minor.yy0; yymsp[0].minor.yy318.bNot = 0;/*A-overwrites-X*/}
#line 3158 "parse.c"
        break;
      case 171: /* likeop ::= NOT LIKE_KW|MATCH */
#line 959 "parse.y"
{yymsp[-1].minor.yy318.eOperator = yymsp[0].minor.yy0; yymsp[-1].minor.yy318.bNot = 1;}
#line 3163 "parse.c"
        break;
      case 172: /* expr ::= expr likeop expr */
#line 960 "parse.y"
{
  ExprList *pList;
  pList = sqlite3ExprListAppend(pParse,0, yymsp[0].minor.yy342.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[-2].minor.yy342.pExpr);
  yymsp[-2].minor.yy342.pExpr = sqlite3ExprFunction(pParse, pList, &yymsp[-1].minor.yy318.eOperator);
  exprNot(pParse, yymsp[-1].minor.yy318.bNot, &yymsp[-2].minor.yy342);
  yymsp[-2].minor.yy342.zEnd = yymsp[0].minor.yy342.zEnd;
  if( yymsp[-2].minor.yy342.pExpr ) yymsp[-2].minor.yy342.pExpr->flags |= EP_InfixFunc;
}
#line 3176 "parse.c"
        break;
      case 173: /* expr ::= expr likeop expr ESCAPE expr */
#line 969 "parse.y"
{
  ExprList *pList;
  pList = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy342.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[-4].minor.yy342.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[0].minor.yy342.pExpr);
  yymsp[-4].minor.yy342.pExpr = sqlite3ExprFunction(pParse, pList, &yymsp[-3].minor.yy318.eOperator);
  exprNot(pParse, yymsp[-3].minor.yy318.bNot, &yymsp[-4].minor.yy342);
  yymsp[-4].minor.yy342.zEnd = yymsp[0].minor.yy342.zEnd;
  if( yymsp[-4].minor.yy342.pExpr ) yymsp[-4].minor.yy342.pExpr->flags |= EP_InfixFunc;
}
#line 3190 "parse.c"
        break;
      case 174: /* expr ::= expr ISNULL|NOTNULL */
#line 994 "parse.y"
{spanUnaryPostfix(pParse,yymsp[0].major,&yymsp[-1].minor.yy342,&yymsp[0].minor.yy0);}
#line 3195 "parse.c"
        break;
      case 175: /* expr ::= expr NOT NULL */
#line 995 "parse.y"
{spanUnaryPostfix(pParse,TK_NOTNULL,&yymsp[-2].minor.yy342,&yymsp[0].minor.yy0);}
#line 3200 "parse.c"
        break;
      case 176: /* expr ::= expr IS expr */
#line 1016 "parse.y"
{
  spanBinaryExpr(pParse,TK_IS,&yymsp[-2].minor.yy342,&yymsp[0].minor.yy342);
  binaryToUnaryIfNull(pParse, yymsp[0].minor.yy342.pExpr, yymsp[-2].minor.yy342.pExpr, TK_ISNULL);
}
#line 3208 "parse.c"
        break;
      case 177: /* expr ::= expr IS NOT expr */
#line 1020 "parse.y"
{
  spanBinaryExpr(pParse,TK_ISNOT,&yymsp[-3].minor.yy342,&yymsp[0].minor.yy342);
  binaryToUnaryIfNull(pParse, yymsp[0].minor.yy342.pExpr, yymsp[-3].minor.yy342.pExpr, TK_NOTNULL);
}
#line 3216 "parse.c"
        break;
      case 178: /* expr ::= NOT expr */
      case 179: /* expr ::= BITNOT expr */ yytestcase(yyruleno==179);
#line 1044 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy342,pParse,yymsp[-1].major,&yymsp[0].minor.yy342,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 3222 "parse.c"
        break;
      case 180: /* expr ::= MINUS expr */
#line 1048 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy342,pParse,TK_UMINUS,&yymsp[0].minor.yy342,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 3227 "parse.c"
        break;
      case 181: /* expr ::= PLUS expr */
#line 1050 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy342,pParse,TK_UPLUS,&yymsp[0].minor.yy342,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 3232 "parse.c"
        break;
      case 182: /* between_op ::= BETWEEN */
      case 185: /* in_op ::= IN */ yytestcase(yyruleno==185);
#line 1053 "parse.y"
{yymsp[0].minor.yy392 = 0;}
#line 3238 "parse.c"
        break;
      case 184: /* expr ::= expr between_op expr AND expr */
#line 1055 "parse.y"
{
  ExprList *pList = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy342.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[0].minor.yy342.pExpr);
  yymsp[-4].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_BETWEEN, yymsp[-4].minor.yy342.pExpr, 0, 0);
  if( yymsp[-4].minor.yy342.pExpr ){
    yymsp[-4].minor.yy342.pExpr->x.pList = pList;
  }else{
    sqlite3ExprListDelete(pParse->db, pList);
  } 
  exprNot(pParse, yymsp[-3].minor.yy392, &yymsp[-4].minor.yy342);
  yymsp[-4].minor.yy342.zEnd = yymsp[0].minor.yy342.zEnd;
}
#line 3254 "parse.c"
        break;
      case 187: /* expr ::= expr in_op LP exprlist RP */
#line 1071 "parse.y"
{
    if( yymsp[-1].minor.yy442==0 ){
      /* Expressions of the form
      **
      **      expr1 IN ()
      **      expr1 NOT IN ()
      **
      ** simplify to constants 0 (false) and 1 (true), respectively,
      ** regardless of the value of expr1.
      */
      sqlite3ExprDelete(pParse->db, yymsp[-4].minor.yy342.pExpr);
      yymsp[-4].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_INTEGER, 0, 0, &sqlite3IntTokens[yymsp[-3].minor.yy392]);
    }else if( yymsp[-1].minor.yy442->nExpr==1 ){
      /* Expressions of the form:
      **
      **      expr1 IN (?1)
      **      expr1 NOT IN (?2)
      **
      ** with exactly one value on the RHS can be simplified to something
      ** like this:
      **
      **      expr1 == ?1
      **      expr1 <> ?2
      **
      ** But, the RHS of the == or <> is marked with the EP_Generic flag
      ** so that it may not contribute to the computation of comparison
      ** affinity or the collating sequence to use for comparison.  Otherwise,
      ** the semantics would be subtly different from IN or NOT IN.
      */
      Expr *pRHS = yymsp[-1].minor.yy442->a[0].pExpr;
      yymsp[-1].minor.yy442->a[0].pExpr = 0;
      sqlite3ExprListDelete(pParse->db, yymsp[-1].minor.yy442);
      /* pRHS cannot be NULL because a malloc error would have been detected
      ** before now and control would have never reached this point */
      if( ALWAYS(pRHS) ){
        pRHS->flags &= ~EP_Collate;
        pRHS->flags |= EP_Generic;
      }
      yymsp[-4].minor.yy342.pExpr = sqlite3PExpr(pParse, yymsp[-3].minor.yy392 ? TK_NE : TK_EQ, yymsp[-4].minor.yy342.pExpr, pRHS, 0);
    }else{
      yymsp[-4].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-4].minor.yy342.pExpr, 0, 0);
      if( yymsp[-4].minor.yy342.pExpr ){
        yymsp[-4].minor.yy342.pExpr->x.pList = yymsp[-1].minor.yy442;
        sqlite3ExprSetHeightAndFlags(pParse, yymsp[-4].minor.yy342.pExpr);
      }else{
        sqlite3ExprListDelete(pParse->db, yymsp[-1].minor.yy442);
      }
      exprNot(pParse, yymsp[-3].minor.yy392, &yymsp[-4].minor.yy342);
    }
    yymsp[-4].minor.yy342.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
  }
#line 3309 "parse.c"
        break;
      case 188: /* expr ::= LP select RP */
#line 1122 "parse.y"
{
    spanSet(&yymsp[-2].minor.yy342,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/
    yymsp[-2].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_SELECT, 0, 0, 0);
    if( yymsp[-2].minor.yy342.pExpr ){
      yymsp[-2].minor.yy342.pExpr->x.pSelect = yymsp[-1].minor.yy159;
      ExprSetProperty(yymsp[-2].minor.yy342.pExpr, EP_xIsSelect|EP_Subquery);
      sqlite3ExprSetHeightAndFlags(pParse, yymsp[-2].minor.yy342.pExpr);
    }else{
      sqlite3SelectDelete(pParse->db, yymsp[-1].minor.yy159);
    }
  }
#line 3324 "parse.c"
        break;
      case 189: /* expr ::= expr in_op LP select RP */
#line 1133 "parse.y"
{
    yymsp[-4].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-4].minor.yy342.pExpr, 0, 0);
    if( yymsp[-4].minor.yy342.pExpr ){
      yymsp[-4].minor.yy342.pExpr->x.pSelect = yymsp[-1].minor.yy159;
      ExprSetProperty(yymsp[-4].minor.yy342.pExpr, EP_xIsSelect|EP_Subquery);
      sqlite3ExprSetHeightAndFlags(pParse, yymsp[-4].minor.yy342.pExpr);
    }else{
      sqlite3SelectDelete(pParse->db, yymsp[-1].minor.yy159);
    }
    exprNot(pParse, yymsp[-3].minor.yy392, &yymsp[-4].minor.yy342);
    yymsp[-4].minor.yy342.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
  }
#line 3340 "parse.c"
        break;
      case 190: /* expr ::= expr in_op nm dbnm */
#line 1145 "parse.y"
{
    SrcList *pSrc = sqlite3SrcListAppend(pParse->db, 0,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0);
    yymsp[-3].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-3].minor.yy342.pExpr, 0, 0);
    if( yymsp[-3].minor.yy342.pExpr ){
      yymsp[-3].minor.yy342.pExpr->x.pSelect = sqlite3SelectNew(pParse, 0,pSrc,0,0,0,0,0,0,0);
      ExprSetProperty(yymsp[-3].minor.yy342.pExpr, EP_xIsSelect|EP_Subquery);
      sqlite3ExprSetHeightAndFlags(pParse, yymsp[-3].minor.yy342.pExpr);
    }else{
      sqlite3SrcListDelete(pParse->db, pSrc);
    }
    exprNot(pParse, yymsp[-2].minor.yy392, &yymsp[-3].minor.yy342);
    yymsp[-3].minor.yy342.zEnd = yymsp[0].minor.yy0.z ? &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] : &yymsp[-1].minor.yy0.z[yymsp[-1].minor.yy0.n];
  }
#line 3357 "parse.c"
        break;
      case 191: /* expr ::= EXISTS LP select RP */
#line 1158 "parse.y"
{
    Expr *p;
    spanSet(&yymsp[-3].minor.yy342,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/
    p = yymsp[-3].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_EXISTS, 0, 0, 0);
    if( p ){
      p->x.pSelect = yymsp[-1].minor.yy159;
      ExprSetProperty(p, EP_xIsSelect|EP_Subquery);
      sqlite3ExprSetHeightAndFlags(pParse, p);
    }else{
      sqlite3SelectDelete(pParse->db, yymsp[-1].minor.yy159);
    }
  }
#line 3373 "parse.c"
        break;
      case 192: /* expr ::= CASE case_operand case_exprlist case_else END */
#line 1173 "parse.y"
{
  spanSet(&yymsp[-4].minor.yy342,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0);  /*A-overwrites-C*/
  yymsp[-4].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_CASE, yymsp[-3].minor.yy122, 0, 0);
  if( yymsp[-4].minor.yy342.pExpr ){
    yymsp[-4].minor.yy342.pExpr->x.pList = yymsp[-1].minor.yy122 ? sqlite3ExprListAppend(pParse,yymsp[-2].minor.yy442,yymsp[-1].minor.yy122) : yymsp[-2].minor.yy442;
    sqlite3ExprSetHeightAndFlags(pParse, yymsp[-4].minor.yy342.pExpr);
  }else{
    sqlite3ExprListDelete(pParse->db, yymsp[-2].minor.yy442);
    sqlite3ExprDelete(pParse->db, yymsp[-1].minor.yy122);
  }
}
#line 3388 "parse.c"
        break;
      case 193: /* case_exprlist ::= case_exprlist WHEN expr THEN expr */
#line 1186 "parse.y"
{
  yymsp[-4].minor.yy442 = sqlite3ExprListAppend(pParse,yymsp[-4].minor.yy442, yymsp[-2].minor.yy342.pExpr);
  yymsp[-4].minor.yy442 = sqlite3ExprListAppend(pParse,yymsp[-4].minor.yy442, yymsp[0].minor.yy342.pExpr);
}
#line 3396 "parse.c"
        break;
      case 194: /* case_exprlist ::= WHEN expr THEN expr */
#line 1190 "parse.y"
{
  yymsp[-3].minor.yy442 = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy342.pExpr);
  yymsp[-3].minor.yy442 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy442, yymsp[0].minor.yy342.pExpr);
}
#line 3404 "parse.c"
        break;
      case 197: /* case_operand ::= expr */
#line 1200 "parse.y"
{yymsp[0].minor.yy122 = yymsp[0].minor.yy342.pExpr; /*A-overwrites-X*/}
#line 3409 "parse.c"
        break;
      case 200: /* nexprlist ::= nexprlist COMMA expr */
#line 1211 "parse.y"
{yymsp[-2].minor.yy442 = sqlite3ExprListAppend(pParse,yymsp[-2].minor.yy442,yymsp[0].minor.yy342.pExpr);}
#line 3414 "parse.c"
        break;
      case 201: /* nexprlist ::= expr */
#line 1213 "parse.y"
{yymsp[0].minor.yy442 = sqlite3ExprListAppend(pParse,0,yymsp[0].minor.yy342.pExpr); /*A-overwrites-Y*/}
#line 3419 "parse.c"
        break;
      case 202: /* cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
#line 1219 "parse.y"
{
  sqlite3CreateIndex(pParse, &yymsp[-7].minor.yy0, &yymsp[-6].minor.yy0, 
                     sqlite3SrcListAppend(pParse->db,0,&yymsp[-4].minor.yy0,0), yymsp[-2].minor.yy442, yymsp[-10].minor.yy392,
                      &yymsp[-11].minor.yy0, yymsp[0].minor.yy122, SQLITE_SO_ASC, yymsp[-8].minor.yy392);
}
#line 3428 "parse.c"
        break;
      case 203: /* uniqueflag ::= UNIQUE */
      case 244: /* raisetype ::= ABORT */ yytestcase(yyruleno==244);
#line 1226 "parse.y"
{yymsp[0].minor.yy392 = OE_Abort;}
#line 3434 "parse.c"
        break;
      case 204: /* uniqueflag ::= */
#line 1227 "parse.y"
{yymsp[1].minor.yy392 = OE_None;}
#line 3439 "parse.c"
        break;
      case 206: /* eidlist_opt ::= LP eidlist RP */
#line 1276 "parse.y"
{yymsp[-2].minor.yy442 = yymsp[-1].minor.yy442;}
#line 3444 "parse.c"
        break;
      case 207: /* eidlist ::= eidlist COMMA nm collate sortorder */
#line 1277 "parse.y"
{
  yymsp[-4].minor.yy442 = parserAddExprIdListTerm(pParse, yymsp[-4].minor.yy442, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy392, yymsp[0].minor.yy392);
}
#line 3451 "parse.c"
        break;
      case 208: /* eidlist ::= nm collate sortorder */
#line 1280 "parse.y"
{
  yymsp[-2].minor.yy442 = parserAddExprIdListTerm(pParse, 0, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy392, yymsp[0].minor.yy392); /*A-overwrites-Y*/
}
#line 3458 "parse.c"
        break;
      case 211: /* cmd ::= DROP INDEX ifexists fullname */
#line 1291 "parse.y"
{sqlite3DropIndex(pParse, yymsp[0].minor.yy347, yymsp[-1].minor.yy392);}
#line 3463 "parse.c"
        break;
      case 212: /* cmd ::= VACUUM */
      case 213: /* cmd ::= VACUUM nm */ yytestcase(yyruleno==213);
#line 1297 "parse.y"
{sqlite3Vacuum(pParse);}
#line 3469 "parse.c"
        break;
      case 214: /* cmd ::= PRAGMA nm dbnm */
#line 1305 "parse.y"
{sqlite3Pragma(pParse,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0,0,0);}
#line 3474 "parse.c"
        break;
      case 215: /* cmd ::= PRAGMA nm dbnm EQ nmnum */
#line 1306 "parse.y"
{sqlite3Pragma(pParse,&yymsp[-3].minor.yy0,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0,0);}
#line 3479 "parse.c"
        break;
      case 216: /* cmd ::= PRAGMA nm dbnm LP nmnum RP */
#line 1307 "parse.y"
{sqlite3Pragma(pParse,&yymsp[-4].minor.yy0,&yymsp[-3].minor.yy0,&yymsp[-1].minor.yy0,0);}
#line 3484 "parse.c"
        break;
      case 217: /* cmd ::= PRAGMA nm dbnm EQ minus_num */
#line 1309 "parse.y"
{sqlite3Pragma(pParse,&yymsp[-3].minor.yy0,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0,1);}
#line 3489 "parse.c"
        break;
      case 218: /* cmd ::= PRAGMA nm dbnm LP minus_num RP */
#line 1311 "parse.y"
{sqlite3Pragma(pParse,&yymsp[-4].minor.yy0,&yymsp[-3].minor.yy0,&yymsp[-1].minor.yy0,1);}
#line 3494 "parse.c"
        break;
      case 221: /* cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
#line 1327 "parse.y"
{
  Token all;
  all.z = yymsp[-3].minor.yy0.z;
  all.n = (int)(yymsp[0].minor.yy0.z - yymsp[-3].minor.yy0.z) + yymsp[0].minor.yy0.n;
  sqlite3FinishTrigger(pParse, yymsp[-1].minor.yy327, &all);
}
#line 3504 "parse.c"
        break;
      case 222: /* trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
#line 1336 "parse.y"
{
  sqlite3BeginTrigger(pParse, &yymsp[-7].minor.yy0, &yymsp[-6].minor.yy0, yymsp[-5].minor.yy392, yymsp[-4].minor.yy410.a, yymsp[-4].minor.yy410.b, yymsp[-2].minor.yy347, yymsp[0].minor.yy122, yymsp[-10].minor.yy392, yymsp[-8].minor.yy392);
  yymsp[-10].minor.yy0 = (yymsp[-6].minor.yy0.n==0?yymsp[-7].minor.yy0:yymsp[-6].minor.yy0); /*A-overwrites-T*/
}
#line 3512 "parse.c"
        break;
      case 223: /* trigger_time ::= BEFORE */
#line 1342 "parse.y"
{ yymsp[0].minor.yy392 = TK_BEFORE; }
#line 3517 "parse.c"
        break;
      case 224: /* trigger_time ::= AFTER */
#line 1343 "parse.y"
{ yymsp[0].minor.yy392 = TK_AFTER;  }
#line 3522 "parse.c"
        break;
      case 225: /* trigger_time ::= INSTEAD OF */
#line 1344 "parse.y"
{ yymsp[-1].minor.yy392 = TK_INSTEAD;}
#line 3527 "parse.c"
        break;
      case 226: /* trigger_time ::= */
#line 1345 "parse.y"
{ yymsp[1].minor.yy392 = TK_BEFORE; }
#line 3532 "parse.c"
        break;
      case 227: /* trigger_event ::= DELETE|INSERT */
      case 228: /* trigger_event ::= UPDATE */ yytestcase(yyruleno==228);
#line 1349 "parse.y"
{yymsp[0].minor.yy410.a = yymsp[0].major; /*A-overwrites-X*/ yymsp[0].minor.yy410.b = 0;}
#line 3538 "parse.c"
        break;
      case 229: /* trigger_event ::= UPDATE OF idlist */
#line 1351 "parse.y"
{yymsp[-2].minor.yy410.a = TK_UPDATE; yymsp[-2].minor.yy410.b = yymsp[0].minor.yy180;}
#line 3543 "parse.c"
        break;
      case 230: /* when_clause ::= */
      case 249: /* key_opt ::= */ yytestcase(yyruleno==249);
#line 1358 "parse.y"
{ yymsp[1].minor.yy122 = 0; }
#line 3549 "parse.c"
        break;
      case 231: /* when_clause ::= WHEN expr */
      case 250: /* key_opt ::= KEY expr */ yytestcase(yyruleno==250);
#line 1359 "parse.y"
{ yymsp[-1].minor.yy122 = yymsp[0].minor.yy342.pExpr; }
#line 3555 "parse.c"
        break;
      case 232: /* trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
#line 1363 "parse.y"
{
  assert( yymsp[-2].minor.yy327!=0 );
  yymsp[-2].minor.yy327->pLast->pNext = yymsp[-1].minor.yy327;
  yymsp[-2].minor.yy327->pLast = yymsp[-1].minor.yy327;
}
#line 3564 "parse.c"
        break;
      case 233: /* trigger_cmd_list ::= trigger_cmd SEMI */
#line 1368 "parse.y"
{ 
  assert( yymsp[-1].minor.yy327!=0 );
  yymsp[-1].minor.yy327->pLast = yymsp[-1].minor.yy327;
}
#line 3572 "parse.c"
        break;
      case 234: /* trnm ::= nm DOT nm */
#line 1379 "parse.y"
{
  yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;
  sqlite3ErrorMsg(pParse, 
        "qualified table names are not allowed on INSERT, UPDATE, and DELETE "
        "statements within triggers");
}
#line 3582 "parse.c"
        break;
      case 235: /* tridxby ::= INDEXED BY nm */
#line 1391 "parse.y"
{
  sqlite3ErrorMsg(pParse,
        "the INDEXED BY clause is not allowed on UPDATE or DELETE statements "
        "within triggers");
}
#line 3591 "parse.c"
        break;
      case 236: /* tridxby ::= NOT INDEXED */
#line 1396 "parse.y"
{
  sqlite3ErrorMsg(pParse,
        "the NOT INDEXED clause is not allowed on UPDATE or DELETE statements "
        "within triggers");
}
#line 3600 "parse.c"
        break;
      case 237: /* trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist where_opt */
#line 1409 "parse.y"
{yymsp[-6].minor.yy327 = sqlite3TriggerUpdateStep(pParse->db, &yymsp[-4].minor.yy0, yymsp[-1].minor.yy442, yymsp[0].minor.yy122, yymsp[-5].minor.yy392);}
#line 3605 "parse.c"
        break;
      case 238: /* trigger_cmd ::= insert_cmd INTO trnm idlist_opt select */
#line 1413 "parse.y"
{yymsp[-4].minor.yy327 = sqlite3TriggerInsertStep(pParse->db, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy180, yymsp[0].minor.yy159, yymsp[-4].minor.yy392);/*A-overwrites-R*/}
#line 3610 "parse.c"
        break;
      case 239: /* trigger_cmd ::= DELETE FROM trnm tridxby where_opt */
#line 1417 "parse.y"
{yymsp[-4].minor.yy327 = sqlite3TriggerDeleteStep(pParse->db, &yymsp[-2].minor.yy0, yymsp[0].minor.yy122);}
#line 3615 "parse.c"
        break;
      case 240: /* trigger_cmd ::= select */
#line 1421 "parse.y"
{yymsp[0].minor.yy327 = sqlite3TriggerSelectStep(pParse->db, yymsp[0].minor.yy159); /*A-overwrites-X*/}
#line 3620 "parse.c"
        break;
      case 241: /* expr ::= RAISE LP IGNORE RP */
#line 1424 "parse.y"
{
  spanSet(&yymsp[-3].minor.yy342,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);  /*A-overwrites-X*/
  yymsp[-3].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_RAISE, 0, 0, 0); 
  if( yymsp[-3].minor.yy342.pExpr ){
    yymsp[-3].minor.yy342.pExpr->affinity = OE_Ignore;
  }
}
#line 3631 "parse.c"
        break;
      case 242: /* expr ::= RAISE LP raisetype COMMA nm RP */
#line 1431 "parse.y"
{
  spanSet(&yymsp[-5].minor.yy342,&yymsp[-5].minor.yy0,&yymsp[0].minor.yy0);  /*A-overwrites-X*/
  yymsp[-5].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_RAISE, 0, 0, &yymsp[-1].minor.yy0); 
  if( yymsp[-5].minor.yy342.pExpr ) {
    yymsp[-5].minor.yy342.pExpr->affinity = (char)yymsp[-3].minor.yy392;
  }
}
#line 3642 "parse.c"
        break;
      case 243: /* raisetype ::= ROLLBACK */
#line 1441 "parse.y"
{yymsp[0].minor.yy392 = OE_Rollback;}
#line 3647 "parse.c"
        break;
      case 245: /* raisetype ::= FAIL */
#line 1443 "parse.y"
{yymsp[0].minor.yy392 = OE_Fail;}
#line 3652 "parse.c"
        break;
      case 246: /* cmd ::= DROP TRIGGER ifexists fullname */
#line 1448 "parse.y"
{
  sqlite3DropTrigger(pParse,yymsp[0].minor.yy347,yymsp[-1].minor.yy392);
}
#line 3659 "parse.c"
        break;
      case 247: /* cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
#line 1455 "parse.y"
{
  sqlite3Attach(pParse, yymsp[-3].minor.yy342.pExpr, yymsp[-1].minor.yy342.pExpr, yymsp[0].minor.yy122);
}
#line 3666 "parse.c"
        break;
      case 248: /* cmd ::= DETACH database_kw_opt expr */
#line 1458 "parse.y"
{
  sqlite3Detach(pParse, yymsp[0].minor.yy342.pExpr);
}
#line 3673 "parse.c"
        break;
      case 251: /* cmd ::= REINDEX */
#line 1473 "parse.y"
{sqlite3Reindex(pParse, 0, 0);}
#line 3678 "parse.c"
        break;
      case 252: /* cmd ::= REINDEX nm dbnm */
#line 1474 "parse.y"
{sqlite3Reindex(pParse, &yymsp[-1].minor.yy0, &yymsp[0].minor.yy0);}
#line 3683 "parse.c"
        break;
      case 253: /* cmd ::= ANALYZE */
#line 1479 "parse.y"
{sqlite3Analyze(pParse, 0, 0);}
#line 3688 "parse.c"
        break;
      case 254: /* cmd ::= ANALYZE nm dbnm */
#line 1480 "parse.y"
{sqlite3Analyze(pParse, &yymsp[-1].minor.yy0, &yymsp[0].minor.yy0);}
#line 3693 "parse.c"
        break;
      case 255: /* cmd ::= ALTER TABLE fullname RENAME TO nm */
#line 1485 "parse.y"
{
  sqlite3AlterRenameTable(pParse,yymsp[-3].minor.yy347,&yymsp[0].minor.yy0);
}
#line 3700 "parse.c"
        break;
      case 256: /* cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
#line 1489 "parse.y"
{
  yymsp[-1].minor.yy0.n = (int)(pParse->sLastToken.z-yymsp[-1].minor.yy0.z) + pParse->sLastToken.n;
  sqlite3AlterFinishAddColumn(pParse, &yymsp[-1].minor.yy0);
}
#line 3708 "parse.c"
        break;
      case 257: /* add_column_fullname ::= fullname */
#line 1493 "parse.y"
{
  disableLookaside(pParse);
  sqlite3AlterBeginAddColumn(pParse, yymsp[0].minor.yy347);
}
#line 3716 "parse.c"
        break;
      case 258: /* cmd ::= create_vtab */
#line 1503 "parse.y"
{sqlite3VtabFinishParse(pParse,0);}
#line 3721 "parse.c"
        break;
      case 259: /* cmd ::= create_vtab LP vtabarglist RP */
#line 1504 "parse.y"
{sqlite3VtabFinishParse(pParse,&yymsp[0].minor.yy0);}
#line 3726 "parse.c"
        break;
      case 260: /* create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */
#line 1506 "parse.y"
{
    sqlite3VtabBeginParse(pParse, &yymsp[-3].minor.yy0, &yymsp[-2].minor.yy0, &yymsp[0].minor.yy0, yymsp[-4].minor.yy392);
}
#line 3733 "parse.c"
        break;
      case 261: /* vtabarg ::= */
#line 1511 "parse.y"
{sqlite3VtabArgInit(pParse);}
#line 3738 "parse.c"
        break;
      case 262: /* vtabargtoken ::= ANY */
      case 263: /* vtabargtoken ::= lp anylist RP */ yytestcase(yyruleno==263);
      case 264: /* lp ::= LP */ yytestcase(yyruleno==264);
#line 1513 "parse.y"
{sqlite3VtabArgExtend(pParse,&yymsp[0].minor.yy0);}
#line 3745 "parse.c"
        break;
      case 265: /* with ::= */
#line 1528 "parse.y"
{yymsp[1].minor.yy331 = 0;}
#line 3750 "parse.c"
        break;
      case 266: /* with ::= WITH wqlist */
#line 1530 "parse.y"
{ yymsp[-1].minor.yy331 = yymsp[0].minor.yy331; }
#line 3755 "parse.c"
        break;
      case 267: /* with ::= WITH RECURSIVE wqlist */
#line 1531 "parse.y"
{ yymsp[-2].minor.yy331 = yymsp[0].minor.yy331; }
#line 3760 "parse.c"
        break;
      case 268: /* wqlist ::= nm eidlist_opt AS LP select RP */
#line 1533 "parse.y"
{
  yymsp[-5].minor.yy331 = sqlite3WithAdd(pParse, 0, &yymsp[-5].minor.yy0, yymsp[-4].minor.yy442, yymsp[-1].minor.yy159); /*A-overwrites-X*/
}
#line 3767 "parse.c"
        break;
      case 269: /* wqlist ::= wqlist COMMA nm eidlist_opt AS LP select RP */
#line 1536 "parse.y"
{
  yymsp[-7].minor.yy331 = sqlite3WithAdd(pParse, yymsp[-7].minor.yy331, &yymsp[-5].minor.yy0, yymsp[-4].minor.yy442, yymsp[-1].minor.yy159);
}
#line 3774 "parse.c"
        break;
      default:
      /* (270) input ::= cmdlist */ yytestcase(yyruleno==270);
      /* (271) cmdlist ::= cmdlist ecmd */ yytestcase(yyruleno==271);
      /* (272) cmdlist ::= ecmd */ yytestcase(yyruleno==272);
      /* (273) ecmd ::= SEMI */ yytestcase(yyruleno==273);
      /* (274) ecmd ::= explain cmdx SEMI */ yytestcase(yyruleno==274);
      /* (275) explain ::= */ yytestcase(yyruleno==275);
      /* (276) trans_opt ::= */ yytestcase(yyruleno==276);
      /* (277) trans_opt ::= TRANSACTION */ yytestcase(yyruleno==277);
      /* (278) trans_opt ::= TRANSACTION nm */ yytestcase(yyruleno==278);
      /* (279) savepoint_opt ::= SAVEPOINT */ yytestcase(yyruleno==279);
      /* (280) savepoint_opt ::= */ yytestcase(yyruleno==280);
      /* (281) cmd ::= create_table create_table_args */ yytestcase(yyruleno==281);
      /* (282) columnlist ::= columnlist COMMA columnname carglist */ yytestcase(yyruleno==282);
      /* (283) columnlist ::= columnname carglist */ yytestcase(yyruleno==283);
      /* (284) nm ::= ID|INDEXED */ yytestcase(yyruleno==284);
      /* (285) nm ::= STRING */ yytestcase(yyruleno==285);
      /* (286) nm ::= JOIN_KW */ yytestcase(yyruleno==286);
      /* (287) typetoken ::= typename */ yytestcase(yyruleno==287);
      /* (288) typename ::= ID|STRING */ yytestcase(yyruleno==288);
      /* (289) signed ::= plus_num */ yytestcase(yyruleno==289);
      /* (290) signed ::= minus_num */ yytestcase(yyruleno==290);
      /* (291) carglist ::= carglist ccons */ yytestcase(yyruleno==291);
      /* (292) carglist ::= */ yytestcase(yyruleno==292);
      /* (293) ccons ::= NULL onconf */ yytestcase(yyruleno==293);
      /* (294) conslist_opt ::= COMMA conslist */ yytestcase(yyruleno==294);
      /* (295) conslist ::= conslist tconscomma tcons */ yytestcase(yyruleno==295);
      /* (296) conslist ::= tcons */ yytestcase(yyruleno==296);
      /* (297) tconscomma ::= */ yytestcase(yyruleno==297);
      /* (298) defer_subclause_opt ::= defer_subclause */ yytestcase(yyruleno==298);
      /* (299) resolvetype ::= raisetype */ yytestcase(yyruleno==299);
      /* (300) selectnowith ::= oneselect */ yytestcase(yyruleno==300);
      /* (301) oneselect ::= values */ yytestcase(yyruleno==301);
      /* (302) sclp ::= selcollist COMMA */ yytestcase(yyruleno==302);
      /* (303) as ::= ID|STRING */ yytestcase(yyruleno==303);
      /* (304) expr ::= term */ yytestcase(yyruleno==304);
      /* (305) exprlist ::= nexprlist */ yytestcase(yyruleno==305);
      /* (306) nmnum ::= plus_num */ yytestcase(yyruleno==306);
      /* (307) nmnum ::= nm */ yytestcase(yyruleno==307);
      /* (308) nmnum ::= ON */ yytestcase(yyruleno==308);
      /* (309) nmnum ::= DELETE */ yytestcase(yyruleno==309);
      /* (310) nmnum ::= DEFAULT */ yytestcase(yyruleno==310);
      /* (311) plus_num ::= INTEGER|FLOAT */ yytestcase(yyruleno==311);
      /* (312) foreach_clause ::= */ yytestcase(yyruleno==312);
      /* (313) foreach_clause ::= FOR EACH ROW */ yytestcase(yyruleno==313);
      /* (314) trnm ::= nm */ yytestcase(yyruleno==314);
      /* (315) tridxby ::= */ yytestcase(yyruleno==315);
      /* (316) database_kw_opt ::= DATABASE */ yytestcase(yyruleno==316);
      /* (317) database_kw_opt ::= */ yytestcase(yyruleno==317);
      /* (318) kwcolumn_opt ::= */ yytestcase(yyruleno==318);
      /* (319) kwcolumn_opt ::= COLUMNKW */ yytestcase(yyruleno==319);
      /* (320) vtabarglist ::= vtabarg */ yytestcase(yyruleno==320);
      /* (321) vtabarglist ::= vtabarglist COMMA vtabarg */ yytestcase(yyruleno==321);
      /* (322) vtabarg ::= vtabarg vtabargtoken */ yytestcase(yyruleno==322);
      /* (323) anylist ::= */ yytestcase(yyruleno==323);
      /* (324) anylist ::= anylist LP anylist RP */ yytestcase(yyruleno==324);
      /* (325) anylist ::= anylist ANY */ yytestcase(yyruleno==325);
        break;
/********** End reduce actions ************************************************/
  };
  assert( yyruleno<sizeof(yyRuleInfo)/sizeof(yyRuleInfo[0]) );
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact <= YY_MAX_SHIFTREDUCE ){
    if( yyact>YY_MAX_SHIFT ) yyact += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
    yypParser->yyidx -= yysize - 1;
    yymsp -= yysize-1;
    yymsp->stateno = (YYACTIONTYPE)yyact;
    yymsp->major = (YYCODETYPE)yygoto;
    yyTraceShift(yypParser, yyact);
  }else{
    assert( yyact == YY_ACCEPT_ACTION );
    yypParser->yyidx -= yysize;
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  sqlite3ParserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
/************ Begin %parse_failure code ***************************************/
/************ End %parse_failure code *****************************************/
  sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  sqlite3ParserTOKENTYPE yyminor         /* The minor type of the error token */
){
  sqlite3ParserARG_FETCH;
#define TOKEN yyminor
/************ Begin %syntax_error code ****************************************/
#line 32 "parse.y"

  UNUSED_PARAMETER(yymajor);  /* Silence some compiler warnings */
  assert( TOKEN.z[0] );  /* The tokenizer always gives us a token */
  sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", &TOKEN);
#line 3892 "parse.c"
/************ End %syntax_error code ******************************************/
  sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  sqlite3ParserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
/*********** Begin %parse_accept code *****************************************/
/*********** End %parse_accept code *******************************************/
  sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "sqlite3ParserAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void sqlite3Parser(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  sqlite3ParserTOKENTYPE yyminor       /* The value for the token */
  sqlite3ParserARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  unsigned int yyact;   /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  int yyendofinput;     /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (yyParser*)yyp;
  if( yypParser->yyidx<0 ){
#if YYSTACKDEPTH<=0
    if( yypParser->yystksz <=0 ){
      yyStackOverflow(yypParser);
      return;
    }
#endif
    yypParser->yyidx = 0;
#ifndef YYNOERRORRECOVERY
    yypParser->yyerrcnt = -1;
#endif
    yypParser->yystack[0].stateno = 0;
    yypParser->yystack[0].major = 0;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sInitialize. Empty stack. State 0\n",
              yyTracePrompt);
    }
#endif
  }
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  yyendofinput = (yymajor==0);
#endif
  sqlite3ParserARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput '%s'\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
    if( yyact <= YY_MAX_SHIFTREDUCE ){
      if( yyact > YY_MAX_SHIFT ) yyact += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
      yy_shift(yypParser,yyact,yymajor,yyminor);
#ifndef YYNOERRORRECOVERY
      yypParser->yyerrcnt--;
#endif
      yymajor = YYNOCODE;
    }else if( yyact <= YY_MAX_REDUCE ){
      yy_reduce(yypParser,yyact-YY_MIN_REDUCE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
      yyminorunion.yy0 = yyminor;
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminor);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor, &yyminorunion);
        yymajor = YYNOCODE;
      }else{
        while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        yypParser->yystack[yypParser->yyidx].stateno,
                        YYERRORSYMBOL)) >= YY_MIN_REDUCE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          yy_shift(yypParser,yyact,YYERRORSYMBOL,yyminor);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor, yyminor);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      yymajor = YYNOCODE;
      
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor, yyminor);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
#ifndef NDEBUG
  if( yyTraceFILE ){
    int i;
    fprintf(yyTraceFILE,"%sReturn. Stack=",yyTracePrompt);
    for(i=1; i<=yypParser->yyidx; i++)
      fprintf(yyTraceFILE,"%c%s", i==1 ? '[' : ' ', 
              yyTokenName[yypParser->yystack[i].major]);
    fprintf(yyTraceFILE,"]\n");
  }
#endif
  return;
}
