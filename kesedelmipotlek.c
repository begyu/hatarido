/* $Id: kesedelmipotlek.c, v.0.1 by begyu 2017/09/22 $
 * Kesedelmi potlek szamitas.
 * Csak fix alapkamat eseten mukodik helyesen!
 * Datumok a XXI. szazadra korlatozva.
 */

#define VER "2017.09.22"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "date.h"
#include "date.c" /* Nofene */
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <windows.h>

#define NEV "Késedelmi pótlék számítás"
#define PRG "notepad.exe"
#define PAR "kes_potl.txt"

static char default_login[] = "";

char *getlogin(void)
{
  char *p;

  p = getenv("USERNAME");
  if (!p)
    p = getenv("LOGNAME");
  if (!p)
    p = getenv("USER");
  if (!p)
    p = default_login;
  return p;
}

char *gethomepath(char *p)
{
  char *drive;
  char *path;

  path = getenv("TEMP");
  if (path != NULL)
  {
    	strcpy(p, path);
    	return p;
  }
  drive = getenv("HOMEDRIVE");
  path = getenv("HOMEPATH");
  if (!drive || !path)
    	p[0] = '\0';
  else
  {
    	strcpy(p, drive);
    	strcat(p, path);
  }
  return p;
}

int get_year()
{
  time_t now;
  struct tm *t;

  time(&now);
  t = gmtime(&now);
  return(1900 + t->tm_year);
}

void pause(unsigned int secs)
{
  time_t rettime = time(0) + secs;
  while (time(0) < rettime);
}

char *ektelen(char *str)
{
  char in[] = "áéíóöőúüűÁÉÍÓÖŐÚÜŰ";
  char ou[] = "aeiooouuuAEIOOOUUU";
#define MAXCHS 18
  register int i, j;
  unsigned char c;
  int len=strlen(str);

  for (i=0; i<len; i++)
  {
      if ((c=str[i]) > 0x7F)
      {
         for (j=0; j<MAXCHS; j++)
         {
             if (c == (unsigned char)in[j])
             {
                str[i] = ou[j];
                j = MAXCHS;
             }
             else
                str[i] = '?';
         }
      }
      str[i] = toupper(str[i]);
  }
  return str;
}

void datform(char *s)
{
  int i = strlen(s);

  if ((i == 8)
      && (isdigit(s[4]))
      && (isdigit(s[6])))
  {
      s[10] = '\0';
      s[9] = s[7];
      s[8] = s[6];
      s[7] = '.';
      s[6] = s[5];
      s[5] = s[4];
      s[4] = '.';
  }
  else
      for ( ; i>3; i--)
      {
         if (s[i] == ',')
             s[i] = '.';
      }
}

int validdate(char *s, int *ev, int *ho, int *nap)
{
  int e,h; /* ev,ho */
  int n=0; /* nap */
  char *p;

  n = strlen(s);
  if (n < 8)
      return -1;

  e = atoi(s);
  p = strchr(s, '.');
  strtok(s, ".");
  h = (p == NULL) ? 0 : atoi(strtok(0, "."));
  if (h > 0)
  {
      p = strtok(0, ".");
      if (p != NULL)
         	n = atoi(p);
  }
  else
  {
      h = 12;
  }
  if (e < 1)
     	e = get_year();
  else
  if ((e > 3000) || (e < 2000))
  {
      fprintf(stderr, "\n\"%d\" érvénytelen dátum!\n", e);
      return -1;
  }
  if ((h < 1) || (h > 12))
  {
      fprintf(stderr, "\n\"%d\" érvénytelen hónap!\n", h);
      return -1;
  }
  if ((n < 0) || (n > DT_days_this_month_q(e, h)))
  {
      fprintf(stderr, "\n\"%d\" érvénytelen nap!\n", n);
      return -1;
  }
  if (e > get_year())
  {
      printf("\n Az évszám (%d) nagyobb, mint a folyó év!\n", e);
      return -1;
  }
  *ev = e;
  *ho = h;
  *nap= n;
  return 0;
}

int main()
{
  char kezd[12];
  char vege[12];
  char sbuf[12];
  DT_DATE *d_kezd = NULL;
  DT_DATE *d_vege = NULL;
  unsigned int ev, ho, nap;
  long int napok;
  double kamat;
  double napipotlek;
  int osszeg;
  int potlek;
  char *p;
  int i, j;
  FILE *f;
  char buf[128];
  char txt[256];

#define TABS "\t\t\t\t\t"

  SetConsoleTitle(NEV);

  (void)gethomepath(txt);
  strcat(txt, "\\"PAR);
  f = fopen(txt, "wt");

  if (f == NULL)
     	return -1;

  printf("*** "NEV" ("VER") ***\n\n");
  fprintf(f, "*** KÚsedelmi pˇtlÚk szßmÝtßs ("VER") ***\n\n");

  printf("\nNév: ");
  fgets(buf, sizeof(buf), stdin);
  if (buf[0])
  {
      fprintf(f, "%s\n", ektelen(buf));
  }

  do {
      printf("Dátum kezdete (éééé.hó.nn): ");
      fgets(kezd, sizeof(kezd), stdin);
      i = strlen(kezd);
      if (i != 0)
          i--;
      kezd[i] = '\0';
      datform(kezd);
      strcpy(sbuf, kezd);
      i = validdate(sbuf, &ev, &ho, &nap);
  } while (i != 0);
  d_kezd = DT_mkdate(ev, ho, nap);

  p = DT_ascii(d_kezd, DT_UNIVERSAL_FORMAT);
  printf(TABS"(%s)\n", p);
  fprintf(f, "\t* kezdet: \t%s\n", p);

  do {
      printf("      -- vége (éééé.hó.nn): ");
      fgets(vege, sizeof(vege), stdin);
      i = strlen(vege);
      if (i != 0)
          i--;
      vege[i] = '\0';
      datform(vege);
      strcpy(sbuf, vege);
      i = validdate(sbuf, &ev, &ho, &nap);
  } while (i != 0);
  d_vege = DT_mkdate(ev, ho, nap);

  p = DT_ascii(d_vege, DT_UNIVERSAL_FORMAT);
  printf(TABS"(%s)\n", p);
  fprintf(f, "\t* vÚge: \t%s\n", p);

  DT_add_days(d_vege, 1);
  napok = DT_days_between(d_vege, d_kezd);
  
  if (napok < 1)
  {
      fprintf(stderr, "\nA vége kisebb, mint a kezdet!\n");
      return -1;
  }

  printf(TABS"(napok: %d)\n", napok);
  fprintf(f, "\t* napok: \t%d\n", napok);

  DT_rmdate(d_kezd);
  DT_rmdate(d_vege);

  do {
      printf("Alapkamat (%%): ");
      fgets(sbuf, sizeof(sbuf), stdin);
      i = strlen(sbuf)-1;
      sbuf[i] = '\0';
      for (j=0; j<i; j++)
      {
          if (sbuf[j] == ',')
              sbuf[j] = '.';
      }
      kamat = atof(sbuf);
  } while (kamat == 0.0);

  napipotlek = (2*kamat)/365;
  napipotlek *= 1000;
  napipotlek = floor(napipotlek);
  napipotlek /= 1000;
  

  printf(TABS"(%.2f %%)\n", kamat);
  printf(TABS"(napi pótlék: %.3f)\n", napipotlek);
  fprintf(f, "\t* alapkamat: \t%.2f %%\n", kamat);
  fprintf(f, "\t* napi pˇtlÚk: \t%.3f\n", napipotlek);

  do {
      printf("Összeg (Ft.): ");
      fgets(sbuf, sizeof(sbuf), stdin);
      i = strlen(sbuf)-1;
      sbuf[i] = '\0';
      for (j=0; j<i; j++)
      {
          if (sbuf[j] == ',')
              sbuf[j] = '.';
      }
      osszeg = atoi(sbuf);
  } while (osszeg == 0);

  printf(TABS"(%d,- Ft.)\n", osszeg);
  fprintf(f, "\t* Ísszeg: \t%d,- Ft.\n", osszeg);

  potlek = (int)floor((napok * napipotlek * osszeg) / 100);

  printf("\nPótlék: %d,- Ft.\n", potlek);
  fprintf(f, "\nPˇtlÚk: %d,- Ft.\n", potlek);
  pause(1);
  printf("        ==========\n");
  fprintf(f, "        ==========\n");

  pause(1);
  do ; while ((i=getc(stdin)) == 0);

  fprintf(f, "\n\n(%s)\n", getlogin());

  fclose(f);

  strcpy(buf, PRG);
  strcat(buf, " ");
  strcat(buf, txt);
  WinExec(buf, SW_SHOWNORMAL);

  return 0;
}

