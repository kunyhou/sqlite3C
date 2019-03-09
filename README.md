# sqlite3C

为了方便学习sqlite3加密 将目前可编译的文件保存，及记录。

sqlite3源文件在编译时

剔除rijndael.c、codec.c、codecext.c、extensionfunctions.c

以上文件在个别头文件中已包含。

sqlite3使用加密时 在sqlite3.h头文件中加入 SQLITE_HAS_CODEC

源文件来自 sqlite3 version 3.12.1

sqlite3 :<https://sqlite.org/src/info/fe7d3b75fe1bde41>

wxSqlite3:<http://wxcode.sourceforge.net/components/wxsqlite3/>