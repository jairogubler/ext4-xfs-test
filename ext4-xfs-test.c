#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

#define True 1
#define False 0

unsigned long getMicrotime(){
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}

void gravaArquivo(const char *fileName, const char *text, unsigned int createNew, unsigned int renameToOld, unsigned int sync) {
  unsigned long t1, t2;
  t1 = getMicrotime();
  int fd;
  char newfileName[255], oldfileName[255];
  if (createNew) {
    sprintf(newfileName, "%s.new", fileName);
  }
  else {
    strcpy(newfileName, fileName);
  }
  sprintf(oldfileName, "%s.old", fileName);
  FILE *f=fopen(newfileName,"w");
  if (f) {
    fprintf(f, "%s\n", text );
    printf("fflush %s\n", newfileName);
    fflush(f);

    if (sync) {
      fd = fileno(f);
      if (fd) fdatasync(fd); 
      printf("sync file %s\n", newfileName);
    }
    fclose(f);
    if (createNew && renameToOld) {
      printf("rename %s to %s\n", fileName, oldfileName);
      rename(fileName, oldfileName);
    } 

    if (createNew) {
      printf("rename %s to %s\n", newfileName, fileName);
      rename(newfileName, fileName);
    }
    if (sync) {
      fd = open(".",O_RDONLY|O_CLOEXEC);
      if (fd) {
        fsync(fd);
        close(fd);
        printf("sync dir .\n");
      }
    } 
  }
  t2 = getMicrotime();
  printf("tempo total (%s)= %zu\n", fileName, t2 - t1);
}

main(int argc, char **argv) {
  printf("opções: [--sleep=tempo]\n");
  printf("fazendo sync\n");
  system("sync");
  sleep(3);
  unsigned int sync = 0;
  int sleepTime = 5;
  int arg = 1;
  while (arg < argc) {
    if (strncmp(argv[arg], "--sleep=",8) == 0) {
      sleepTime = atol(argv[arg]+8);
      if (sleepTime == 0) sleepTime = 5;
    }
    arg++;
  }
  time_t rawtime;
  struct tm * timeinfo;
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  char *text = asctime(timeinfo);

  gravaArquivo("arquivo-rename-sync.cfg", text, True, True, True);
  gravaArquivo("arquivo-sync.cfg", text, False, False, True);
  gravaArquivo("arquivo-rename.cfg", text, True, True, False);
  gravaArquivo("arquivo-sem-rename.cfg", text, False, False, False);
    

  printf("sleep ...%d\n", sleepTime);
  system("ls -l\n");
  int i = 0;
  while (i++ < sleepTime) {
    system("grep Dirty /proc/meminfo");
    sleep(1);
  }
  system("cat arquivo-*.cfg");
  printf("reboot...\n");
  system("echo 1 > /proc/sys/kernel/sysrq &&  echo b > /proc/sysrq-trigger");
}
