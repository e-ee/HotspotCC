#ifndef HP_HOME_SQLITE_H
#define HP_HOME_SQLITE_H
#include <sqlite3.h>
#include <time.h>
// A separate connection may need to recover/create WAL shared memory. Opening
// the existing file read/write permits this without creating a missing database.
static int HPBackupAccount(const char *sourcePath,const char *targetPath){
 sqlite3 *source=0,*target=0;sqlite3_backup *backup=0;
 int rc=sqlite3_open_v2(sourcePath,&source,SQLITE_OPEN_READWRITE,0);
 if(rc!=SQLITE_OK)goto done;
 sqlite3_busy_timeout(source,500);
 rc=sqlite3_open_v2(targetPath,&target,SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE,0);
 if(rc!=SQLITE_OK)goto done;
 sqlite3_busy_timeout(target,500);
 backup=sqlite3_backup_init(target,"main",source,"main");
 if(!backup){rc=sqlite3_errcode(target);goto done;}
 struct timespec start,now;clock_gettime(CLOCK_MONOTONIC,&start);
 do{
  rc=sqlite3_backup_step(backup,128);
  if(rc!=SQLITE_OK&&rc!=SQLITE_BUSY&&rc!=SQLITE_LOCKED)break;
  clock_gettime(CLOCK_MONOTONIC,&now);
  if(now.tv_sec-start.tv_sec>=5){if(rc==SQLITE_OK)rc=SQLITE_BUSY;break;}
  if(rc!=SQLITE_OK)sqlite3_sleep(50);
 }while(1);
 {int finish=sqlite3_backup_finish(backup);backup=0;if(rc==SQLITE_DONE)rc=finish;}
 done:
 if(backup)sqlite3_backup_finish(backup);
 if(source)sqlite3_close(source);
 if(target){int close=sqlite3_close(target);if(rc==SQLITE_OK)rc=close;}
 return rc;
}
#endif
