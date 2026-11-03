#include "HomeSQLite.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
int main(void){
 const char *src="build/account-test.sqlite",*dst="build/account-copy.sqlite";
 unlink(src);unlink(dst);sqlite3 *db=0,*copy=0;assert(sqlite3_open(src,&db)==SQLITE_OK);
 assert(sqlite3_exec(db,"PRAGMA journal_mode=WAL;CREATE TABLE state(value);INSERT INTO state VALUES(42);",0,0,0)==SQLITE_OK);
 assert(sqlite3_exec(db,"BEGIN IMMEDIATE;INSERT INTO state VALUES(99);",0,0,0)==SQLITE_OK);
 assert(HPBackupAccount(src,dst)==SQLITE_OK);assert(sqlite3_open(dst,&copy)==SQLITE_OK);
 sqlite3_stmt *stmt=0;assert(sqlite3_prepare_v2(copy,"SELECT count(*),sum(value) FROM state",-1,&stmt,0)==SQLITE_OK);
 assert(sqlite3_step(stmt)==SQLITE_ROW);assert(sqlite3_column_int(stmt,0)==1&&sqlite3_column_int(stmt,1)==42);sqlite3_finalize(stmt);
 assert(sqlite3_prepare_v2(copy,"PRAGMA integrity_check",-1,&stmt,0)==SQLITE_OK);assert(sqlite3_step(stmt)==SQLITE_ROW);assert(sqlite3_column_text(stmt,0)[0]=='o');sqlite3_finalize(stmt);
 sqlite3_close(copy);sqlite3_exec(db,"ROLLBACK",0,0,0);sqlite3_close(db);
 assert(HPBackupAccount("build/nonexistent-source",dst)!=SQLITE_OK);assert(access("build/nonexistent-source",F_OK)!=0);
 puts("Live WAL snapshot includes committed state, excludes uncommitted writer state, passes integrity, and refuses missing source.");return 0;
}
