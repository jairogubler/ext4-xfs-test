#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>

#define True 1
#define False 0

unsigned long getMicrotime(){
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}

void gravaArquivo(const char *pasta, const char *fileName, const char *text, unsigned int renameToOld, unsigned int sync) {
  unsigned long t1, t2;
  t1 = getMicrotime();
  int fd;
  char newfileName[255], oldfileName[255], fullFileName[255];

  sprintf(fullFileName, "%s%s", pasta, fileName);
  if (renameToOld) {
    sprintf(newfileName, "%s%s.new", pasta, fileName);
    remove(newfileName);
  }
  else {
    sprintf(newfileName, "%s%s", pasta, fileName);
  }
  sprintf(oldfileName, "%s%s.old", pasta, fileName);
  FILE *f=fopen(newfileName,"w");
  if (f) {
    fprintf(f, "%s\n", text );
    printf("fflush %s\n", newfileName);

    if (sync) {
      fflush(f);
      fd = fileno(f);
      if (fd) fdatasync(fd); 
      printf("sync file %s\n", newfileName);
    }
    fclose(f);
    if (renameToOld) {
      remove(oldfileName);
      printf("rename %s to %s\n", fullFileName, oldfileName);
      rename(fullFileName, oldfileName);
    } 

    if (renameToOld) {
      printf("rename %s to %s\n", newfileName, fullFileName);
      rename(newfileName, fullFileName);
    }
    if (sync) {
      char *copiaNewFileName = strdup(newfileName);
      char *dir = dirname(copiaNewFileName);
      if (dir) {
	fd = open(dir,O_RDONLY|O_CLOEXEC);
	if (fd) {
	  fsync(fd);
	  close(fd);
	  printf("sync dir %s\n",dir);
	}
      }
      free(copiaNewFileName);
    } 
  }
  t2 = getMicrotime();
  printf("tempo total (%s)= %zu\n", fullFileName, t2 - t1);
}

int tamanhoArquivo(const char *fileName) {
  struct stat fileStat;
  if(stat(fileName,&fileStat) < 0)  return -1;
  return fileStat.st_size;
}

struct st_contadores {
  int ok;
  int zerados;
  int sumidos;
  int erros;
} contadores[3];

void verificaArquivo(const char *pasta, const char *tipo, const char *arquivo, const char *ref, struct st_contadores *contador) {
  char filename[255];
  sprintf(filename, "%s%s%s", pasta, tipo, arquivo);
  int tamanho;
  tamanho = tamanhoArquivo(filename);
  FILE *f = fopen(filename, "r");
  char conteudo[201];
  *conteudo = 0;
  int conteudo_ok = 0;
  if (f) { 
    if (fread(conteudo, 1, 200, f) > 0) {
      if (strcmp(conteudo, ref) == 0) {
        conteudo_ok = 1;
      }
    }
    fclose(f);
  }
   
  if (tamanho == 0) {
    contador->zerados++; 
  }
  else if (tamanho < 0) {
    contador->sumidos++; 
  }
  else {
    if (conteudo_ok) {
      contador->ok++;
    }
    else {
      contador->erros++;
    }
  }
}

void verificaArquivos(const char *time, const char *pastas[3]) {
  int i;
  FILE *f = fopen("/root/arquivo-ref.cfg", "r");
  char referencia[201];
  *referencia = 0;
  if (f) { 
    fread(referencia, 1, 200, f);
    fclose(f);
  }
   
  printf("%-8.8s %5s %5s %5s %5s\n", "Pasta", "OK", "Zero", "Sumiu", "Erro");
  for (i = 0; i < 3; i++) {
     contadores[i].ok = 0;
     contadores[i].zerados = 0;
     contadores[i].sumidos = 0;
     contadores[i].erros = 0;
     verificaArquivo(pastas[i], "sync/", "arquivo-rename.cfg", referencia, &contadores[i]);
     verificaArquivo(pastas[i], "sync/", "arquivo-sem-rename.cfg", referencia, &contadores[i]); 
     verificaArquivo(pastas[i], "nosync/", "arquivo-rename.cfg", referencia, &contadores[i]);
     verificaArquivo(pastas[i], "nosync/", "arquivo-sem-rename.cfg", referencia, &contadores[i]);
     printf("%-8s %5d %5d %5d %5d\n", pastas[i], contadores[i].ok, contadores[i].zerados, contadores[i].sumidos, contadores[i].erros);
  }
  f = fopen("/root/resultado.txt", "a"); 
  if (f) {
    int ok = 0;
    int sumidos = 0;
    int zerados = 0;
    int erros = 0;
    fprintf(f, "Hora: %s\n", time);
    fprintf(f,"%-8.8s %5s %5s %5s %5s\n", "Pasta", "OK", "Zero", "Sumiu", "Erro");
    for (i = 0; i < 3; i++) {
      ok += contadores[i].ok;
      zerados += contadores[i].zerados;
      sumidos += contadores[i].sumidos;
      erros += contadores[i].erros;
 
      fprintf(f, "%-8s %5d %5d %5d %5d\n", pastas[i], contadores[i].ok, contadores[i].zerados, contadores[i].sumidos, contadores[i].erros);
    }
    printf("-------- ----- ----- ----- -----\n");
    printf("%-8s %5d %5d %5d %5d\n\n", "Total", ok, zerados, sumidos, erros);

    fprintf(f, "-------- ----- ----- ----- -----\n");
    fprintf(f,"%-8s %5d %5d %5d %5d\n\n", "Total", ok, zerados, sumidos, erros);
    fflush(f);
    int fd = fileno(f);
    if (fd) fsync(fd);
    fclose(f);
    system("tail -n 40 /root/resultado.txt > /dev/console");
  }
}

int main(int argc, char **argv) {
  printf("opções: [--sleep=tempo]\n");
  unsigned int sync = 0;
  unsigned int reboot = 1;
  int sleepTime = 5;
  int arg = 1;
  while (arg < argc) {
    if (strncmp(argv[arg], "--sleep=",8) == 0) {
      sleepTime = atol(argv[arg]+8);
      if (sleepTime == 0) sleepTime = 5;
    }
    else if (strcmp(argv[arg], "noreboot") == 0) {
      reboot=0;
    }
    arg++;
  }
  time_t rawtime;
  struct tm * timeinfo;
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  char *text = strdup(asctime(timeinfo));
  int len = strlen(text);
  if (text[len-1] == '\n') {
    text[len-1] = 0;
  }
  const char *pastas[3];
  pastas[0] = "/ext3/";
  pastas[1] = "/ext4/";
  pastas[2] = "/xfs/";

  verificaArquivos(text, pastas);

  printf("\n\n%s\n\n", text);
  gravaArquivo("/root/","arquivo-ref.cfg", text, False, True);
  sleep(2);

  int i;
  char pastasync[255], pastanosync[255];
  for (i = 0; i < 3; i++) {
    sprintf(pastasync, "%ssync/", pastas[i]);
    sprintf(pastanosync, "%snosync/", pastas[i]);
    gravaArquivo(pastasync,"arquivo-rename.cfg", text, True, True);
    gravaArquivo(pastasync,"arquivo-sem-rename.cfg", text, False, True);
    gravaArquivo(pastanosync,"arquivo-rename.cfg", text, True, False);
    gravaArquivo(pastanosync,"arquivo-sem-rename.cfg", text, False, False);
  }
    

  printf("sleep ...%d\n", sleepTime);
  system("find {/ext3,/ext4/,/xfs} -name arquivo-*.* -exec ls -l {} \\;");
  i = 0;
  while (i++ < sleepTime) {
    system("grep Dirty /proc/meminfo");
    sleep(1);
  }
  system("for filename in $(find {/ext3,/ext4,/xfs} -name '*.cfg' -print ); do echo -e \"$filename - \\t$(cat $filename)\"; done");
  if (reboot) {
    printf("reboot...\n");
    sleep(1);
    system("echo 1 > /proc/sys/kernel/sysrq &&  echo b > /proc/sysrq-trigger");
  }
  free(text);
  exit(0);
}
