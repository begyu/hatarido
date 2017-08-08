/* $Id: hatarido.c, v.2.0 by begyu 2017/08/08 $
 * Adott ‚v (h˘nap[nap]) napjaihoz adott napok d tumai.
 * Munkanap  thelyez‚sek a "hatarido_20??.cfg" f jlban "mm.dd-mm.dd" form ban.
 * -m munkanappal kezd
 * -i interaktˇv m˘d
 * -c r‚szben interaktˇv
 * na nem munkanapra is eshet (nA u.a. -1 nap)
 * nb csak munkanapot sz mol (nB u.a. -1 nap)
 * nC kezd‹ napot is sz molja (-1 nap a v‚ge)
 * nd 'joger‹'
 * RTF ‚s CSV kimenetet gener l Word ‚s Excel (vagy LibreOffice) sz m ra.
 * kiemeles sargaval
 * TODO: UTF-8-ra Ăˇt kĂ©ne mĂ©g Ă­rni!
 * http://sf.net/p/hati/
 * https://sourceforge.net/projects/libdate/
 * https://github.com/begyu/hatarido/
 */

#define VERSION "2.0"
//Wordpad-hoz hegesztve!

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
/*#include <date.h>*/
#include "date.h"
#include "date.c" /* Nofene */
#include <time.h>
#include <locale.h>
#include <ctype.h>
#include <process.h>
#include <getopt.h>


/*#define csvfn "hati.csv"*/
#define rtffn "hati.rtf"

#define PRG "c:\\windows\\system32\\write.exe"

#define R_CTAB "{\\rtf\\ansi\\deff0{\\fonttbl{\\f0\\fnil Courier New;}" \
               "{\\f1\\froman Times New Roman;}{\\f2\\fdecor Century;}}" \
               "{\\colortbl;\\red0\\green0\\blue0;" \
               "\\red0\\green0\\blue255;" \
               "\\red0\\green255\\blue255;" \
               "\\red0\\green255\\blue0;" \
               "\\red255\\green0\\blue255;" \
               "\\red255\\green0\\blue0;" \
               "\\red255\\green255\\blue0;" \
               "\\red255\\green255\\blue255;" \
               "\\red0\\green0\\blue128;" \
               "\\red0\\green128\\blue128;" \
               "\\red0\\green128\\blue0;" \
               "\\red128\\green0\\blue128;" \
               "\\red128\\green0\\blue0;" \
               "\\red128\\green128\\blue0;" \
               "\\red128\\green128\\blue128;" \
               "\\red192\\green192\\blue192;}"

#define R_HL ("{\\highlight7 ")
#define R_HO ("{\\chcbpat7 ")
#define R_I ("{\\i ")
#define R_B ("{\\b ")
#define R_S ("{\\fs22 \\sa-200 ")
#define R_P ("{\\par ")
#define R_E ("}")
#define R_PG ("{\\page }")
#define R_ROMAN ("{\\f1 ")
#define R_COUR_N ("\\f0 ")

#define TRUE  1
#define FALSE 0

typedef unsigned char bool;

static bool munkanappal_kezd = FALSE;

#define MAXUNN 8
static int unho[MAXUNN] = {1,  3, 5,  8, 10, 11, 12, 12};
static int unap[MAXUNN] = {1, 15, 1, 20, 23,  1, 25, 26};

static int hetvege;

#define MAXNAP 10
static int rdm[MAXNAP], rdd[MAXNAP];
static int napok[MAXNAP+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int skips[MAXNAP+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
static int kezdo[MAXNAP+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
static int joger[MAXNAP+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 

typedef struct {
	int ho_innen;
	int nap_innen;
	int ho_ide;
	int nap_ide;
} munkanap_atrak_t;

static munkanap_atrak_t m_nap_ath[10];
static int athelyezett_mnap;

int munkanap_ath(int ev)
{
  int i;
  char ys[] = "1234";
  char fn[] = "hatarido_YYYY.cfg";
  char s[] = "mm.dd-mm.dd";
  char s2[] = "12";
  char c;
  FILE *f;

  itoa(ev, ys, 10);
  for (i=0; i<4; i++)
    fn[9+i] = ys[i];
  f = fopen(fn, "rt");
  if (f == NULL)
    	return -1;
  i =0;
  while (!feof(f))
  {
    fgets(s, 12, f);
    c = fgetc(f); /* CRLF miatt! */
    s2[0] = s[0];
    s2[1] = s[1];
    m_nap_ath[i].ho_innen = atoi(s2);
    s2[0] = s[3];
    s2[1] = s[4];
    m_nap_ath[i].nap_innen = atoi(s2);
    s2[0] = s[6];
    s2[1] = s[7];
    m_nap_ath[i].ho_ide = atoi(s2);
    s2[0] = s[9];
    s2[1] = s[10];
    m_nap_ath[i].nap_ide = atoi(s2);
    i++;
  }
  athelyezett_mnap = i;
  fclose(f);
  return 0;
}

int get_year()
{
  time_t now;
  struct tm *t;

  time(&now);
  t = gmtime(&now);
  return(1900 + t->tm_year);
}

int dt_eq(DT_DATE *d1, DT_DATE *d2)
{
  if ((DT_year(d1) == DT_year(d2))
    &&(DT_month(d1) == DT_month(d2))
    &&(DT_day(d1) == DT_day(d2)))
  {
    return 0;
  }
  return -1;
}

int is_unnep(int h, int n)
{
  int i;

  for (i=0; i<MAXUNN; i++)
  {
    if ((h == unho[i]) && (n == unap[i]))
      	return 0;
  }
  return -1;
}

int is_munkanap(DT_DATE *dat)
{
  DT_DATE *husv, *punk, *nagyp;
  int i;

  hetvege = 0;
  i = DT_dow(dat);
  if ((i==0) || (i==6))
    	hetvege = 1;

  for (i=0; i<athelyezett_mnap; i++)
  {
    if ((m_nap_ath[i].ho_innen == DT_month(dat))
    && (m_nap_ath[i].nap_innen == DT_day(dat)))
      return -1;
    if ((m_nap_ath[i].ho_ide == DT_month(dat))
    && (m_nap_ath[i].nap_ide == DT_day(dat)))
      return 0;
  }

  if (is_unnep(DT_month(dat), DT_day(dat)) == 0)
    	return (-1);

  husv = DT_copy_date(dat);
  (void)DT_easter(husv);
  DT_add_days(husv, 1);  //h‚tf“!

  if (dt_eq(dat, husv) == 0)
    	hetvege = 1; //val˘j ban hŁsv‚t!
       
  punk = DT_copy_date(husv);
  DT_add_days(punk, 49);

  nagyp = DT_copy_date(husv);
  DT_add_days(nagyp, -3);

  if (dt_eq(dat, punk) == 0)
    	hetvege = 1; //val˘j ban pnk”sd!
       
  if (dt_eq(dat, nagyp) == 0)
    	hetvege = 1; //nagyp‚ntek!
       
  if (hetvege == 1)
    	return (-1);

  return 0;
}

int proc(int y, int m, int d)
{
  DT_DATE *dat;
  int i, j;

  dat = DT_mkdate(y,m,d);

  if (dat == NULL)
  {
    fprintf(stderr, "*** DT_mkdate_die(): said \"%s\"\n",
     strerror(errno));                                         
    DT_rmdate(dat);
    return -1;
  }
  
  if (m==2 && d==29)
    if (!DT_is_leap_year(dat))
    {
      fprintf(stderr, "%d nem sz”k‹‚v!\n", y);
      DT_rmdate(dat);
      return -1;
    }

  if ((munkanappal_kezd == TRUE)
  && (is_munkanap(dat) == -1))
  {
    DT_rmdate(dat);
    return -1;
  }

  DT_rmdate(dat);

  for (i=0; i<MAXNAP; i++)
  {
    dat = DT_mkdate(y,m,d);
    if (skips[i] != 2)
      DT_add_days(dat, napok[i]);
    else
    {
      for (j=0; j<napok[i]; j++)
      {
        DT_add_days(dat, 1);
        while (is_munkanap(dat) == -1)
          	DT_add_days(dat, 1);
      }
    }
    if (kezdo[i] == 1)
        DT_add_days(dat, -1);
    if (skips[i] != 1)
    {
      while (is_munkanap(dat) == -1)
        	DT_add_days(dat, 1);
    }
    if (joger[i] == 1)
    {
      if (is_munkanap(dat) == -1)
      {
        while (is_munkanap(dat) == -1)
          	DT_add_days(dat, 1);
        DT_add_days(dat, 1);
      }
      else
        	DT_add_days(dat, 1);
      while (is_munkanap(dat) == -1)
        	DT_add_days(dat, 1);
    }
    rdm[i] = DT_month(dat);
    rdd[i] = DT_day(dat);
    DT_rmdate(dat);
  }

  return 0;
}

int cal(int year, int month, FILE *f)
{
  int day;
  int dow;
  int total_days;
  DT_DATE *d1;
  int i;
  bool r = (f != stdout);
  bool szunnap;

  char *month_ascii[] = { "Januar", "Februar", "Marcius",
                          "Aprilis", "Majus", "Junius",
                          "Julius", "Augusztus", "Szeptember",
                          "Oktober", "November", "December" };

  char *honapok[] = { "Janu r", "Febru r", "M rcius",
                      "µprilis", "M jus", "JŁnius",
                      "JŁlius", "Augusztus", "Szeptember",
                      "Okt˘ber", "November", "December" };

  char days[]  = "He Ke Sz Cs Pe Sz Va";
  char napok[] = "H‚ Ke Sz Cs P‚ Sz Va\n";

  d1 = DT_mkdate(year,month,1);

  if (r)
  {
    fprintf(f, R_P);
    fprintf(f, "%s %d\n", month_ascii[month-1], year);
    fprintf(f, R_E);
    fprintf(f, R_P);
    fprintf(f, days);
    fprintf(f, R_E);
    fprintf(f, R_P);
  }
  else
  {
    fprintf(f, "%s %d\n", honapok[month-1], year);
    fprintf(f, napok);
  }

  dow = DT_dow(d1);
  total_days = DT_days_this_month(d1);

  if (dow == 0)
  {
    for (i=18; i>0; i--)
      fprintf(f, " ");
  }

  for (day = -dow; day < total_days; day++)
  {
    if (day < 0)
      fprintf(f, "   ");
    else
    {
      if (r)
      {
          i = is_munkanap(d1);
          szunnap = (i == -1) ? TRUE : FALSE;
      }
      else
          szunnap = FALSE;
      if (szunnap == TRUE)
      {
          fprintf(f, R_B);
          fprintf(f, R_I);
      }
      fprintf(f, "%2d ", day+1);
      if (szunnap == TRUE)
      {
          fprintf(f, R_E);
          fprintf(f, R_E);
      }
      DT_add_days(d1, 1);
    }

    if ((day+dow)%7 == 0)
    {
      fprintf(f, "\n");
      if (r)
      {
        fprintf(f, R_E);
        fprintf(f, R_P);
      }

    }
  }

  fprintf(f, "\n");
  if (r)
  {
    fprintf(f, R_E);
    fprintf(f, R_P);
    fprintf(f, R_E);
  }

  DT_rmdate(d1);
  return 0;
}

void datform(char *s)
{
  int i = strlen(s);

  if ((i == 6)
      && (isdigit(s[4])))
  {
    s[7] = '\0';
    s[6] = s[5];
    s[5] = s[4];
    s[4] = '.';
  }
  else if ((i == 8)
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

void id()
{
  puts("Hat rid‹ sz mˇt˘ v."VERSION" ("__DATE__")");
}

int main(int ac, char **av)
{
  int e,h; /* ‚v,h˘ */
  int n=0; /* nap */
  int i,j,k,m,o,x,r;
  char *s=NULL;
  char *p;
#if 0
  FILE *fc;
#endif
  FILE *fr;
  int first = TRUE;
  int odd = 0;
  char sfn[256];
  char dbuf[12];
  char buf[128];
  char c;
  int ax = 0;
  int mx = 0;

  x = (ac > (MAXNAP+3)) ? (MAXNAP+3) : ac;

  strcpy(sfn, "copy ");
  strcat(sfn, av[0]);
  for (i=strlen(sfn); i; i--)
  {
    if (sfn[i] == '\\')
    {
       sfn[++i] = '\0';
       break;
    }
  }
  if (i == 0)
    	sfn[0] = '\0';
  strcat(sfn, "hatarido_20??.cfg .");

  opterr = 0;
  while ((c=getopt(ac, av, "CcIiMmVvHh?")) != -1)
  {
    switch (c)
    {
      case 'm':
      case 'M':
        munkanappal_kezd = TRUE;
        if (strlen(av[optind]) == 2)
          	mx++;
        ax++;
        break;
      case 'c':
      case 'C':
        x = 88;
        break;
      case 'i':
      case 'I':
        x = 99;
        break;
      case 'h':
      case 'H':
        x = 0;
        break;
      case 'v':
      case 'V':
        printf("\nHatarido v.%s by begyu\n", VERSION);
        exit(0);
        break;
      case '-':
      case '?':
        if (optopt != '?')
        {
           printf("Unknown option %c\n", optopt);
           exit(1);
        }
/*
      case ':':
        if (c != '?')
              printf("Missing argument %c\n", optopt);
*/
      default:
        break;
    }
  }
  if (optind < ac)
      strcpy(buf, av[optind++]);

  if (x > 2) 
  {
  	 system(sfn);
  	 id();
  	 setlocale(LC_ALL, "C");
  	 if ((x == 88) || (x == 99))
  	 {
  	   printf("D tum (‚‚‚‚[.h˘[.nn]]): ");
  	   fgets(dbuf, sizeof(dbuf), stdin);
  	   s = dbuf;
  	   s[strlen(s)-1] = '\0';
  	 }
  	 else
  	 {
  	   strcpy(dbuf, av[1+ax]);
  	   s = dbuf;
  	 }
  	 datform(s);
  	 e = atoi(s);
  	 p = strchr(s, '.');
  	 strtok(s, ".");
  	 m = (p == NULL) ? 0 : atoi(strtok(0, "."));
  	 if (m > 0)
  	 {
  	   p = strtok(0, ".");
  	   if (p != NULL)
  	     	n = atoi(p);
  	   j = m;
  	 }
  	 else
  	 {
  	   m = 12;
  	   j = 1;
  	 }
  	 if (e < 1)
  	   	e = get_year();
  	 else
  	 if (e > 3000)
  	 {
  	   fprintf(stderr, "\n\"%d\" ‚rv‚nytelen d tum!\n", e);
  	   return -1;
  	 }
  	 if ((m < 1) || (m > 12))
  	 {
  	   fprintf(stderr, "\n\"%d\" ‚rv‚nytelen h˘nap!\n", m);
  	   return -1;
  	 }
  	 if ((n < 0) || (n > DT_days_this_month_q(e, m)))
  	 {
  	   fprintf(stderr, "\n\"%d\" ‚rv‚nytelen nap!\n", n);
  	   return -1;
  	 }
  	 if (e < get_year())
  	   	printf("\n*** Az ‚vsz m (%d) kisebb, mint a foly˘ ‚v! ***", e);
  	 if (x == 99)
  	 {
  	   do {
  	     printf("Napok (n1 [n2 [...n10]]): ");
  	     fgets(buf, sizeof(buf), stdin);
  	     s = buf;
  	     strtok(s, " ");
  	     napok[0] = atoi(s);
  	   } while (napok[0] == 0);
  	   i = 0;
  	   while (napok[i] != 0)
  	   {
  	     i++;
  	     p = strtok(0, " ");
  	     if (p != NULL)
  	       	napok[i] = atoi(p);
  	   }
  	   x = i;
  	 }
  	 else
  	 {
  	   if (x == 88)
  	     	x = ac - (2+mx);
  	   else
  	   {
  	     	x = (x>(MAXNAP+2)) ? MAXNAP : (x-2);
  	     	mx = ax;
  	   }
  	   for (i=0; i<x; i++)
  	   {
  	   	 s = av[i+2+mx];
  	   	 if (s != NULL)
  	   	   k = strlen(s) - 1;
  	   	 else
  	   	   k = -1;
  	   	 if (k != -1)
  	   	 {
  	   	   c = s[k];
  	   	   if ((c=='a') || (c=='A'))
  	   	   {
  	   	     skips[i] = 1;
  	   	     if (c=='A')
  	   	        	kezdo[i] = 1;
  	   	   }
  	   	   else if ((c=='b') || (c=='B'))
  	   	   {
  	   	     	skips[i] = 2;
  	   	     if (c=='B')
  	   	        	kezdo[i] = 1;
  	   	   }
  	   	   else if (c=='C')
  	   	     	kezdo[i] = 1;
  	   	   else if (c=='d')
  	   	   {
  	   	     	kezdo[i] = 1;
  	   	     	joger[i] = 1;
  	   	   }
  	   	   if (skips[i] > 0)
  	   	     	s[k] = 0;
  	   	 }
  	   	 if (s == NULL)
  	   	   napok[i] = 0;
  	   	 else
  	   	   napok[i] = atoi(s);
  	   }
  	 }
  	
  	 munkanap_ath(e);
  	 first = TRUE;
  	 fr = fopen(rtffn, "wt");
  	 if (fr != NULL)
  	 {
  	   fprintf(fr, R_CTAB);
  	   fprintf(fr, R_S);
  	   for (h=j; h<=m; h++)
  	   {
  	   	 printf("\n");
  	   	 fprintf(fr, R_P);
  	   	 fprintf(fr, R_COUR_N);
  	   	 if (n > 0)
  	   	 {
  	   	   x = k = n;
  	   	 }
  	   	 else
  	   	 {
  	   	   k = 1;
  	   	   x = DT_days_this_month_q(e, h);
  	   	   cal(e, h, stdout);
  	   	   cal(e, h, fr);
  	   	 }
  	   	 printf("\n%d.%02d h˘\t\t+%d\t", e, h, napok[0]);
  	   	 fprintf(fr, R_P);
  	   	 fprintf(fr, R_ROMAN);
  	   	 fprintf(fr, R_B);
  	   	 fprintf(fr, "\n%d.%02d ho\t\t+%d\t", e, h, napok[0]);
  	   	 for (i=1; napok[i]; i++)
  	   	 {
  	   	   printf("+%d\t", napok[i]);
  	   	   fprintf(fr, "+%d\t", napok[i]);
  	   	 }
  	   	 printf("\n");
  	   	 fprintf(fr, "\n");
  	   	 fprintf(fr, R_E);
  	   	 for (i=k; i<=x; i++)
  	   	 {
  	   	 	 r = proc(e, h, i);
  	   	 	 if (r != -1)
  	   	 	 {
  	   	 	   printf("\n%d.%02d.%02d.\t\t", e,h,i);
  	   	 	   fprintf(fr, R_P);
  	   	 	   if (odd)
  	   	 	   {
  	   	 	     fprintf(fr, R_HL);
  	   	 	     fprintf(fr, R_HO);
  	   	 	   }
  	   	 	   fprintf(fr, "%d.%02d.%02d.\t\t", e,h,i);
  	   	 	   for (o=0; napok[o]; o++)
  	   	 	   {
  	   	 	     printf("%02d.%02d\t", rdm[o], rdd[o]);
  	   	 	     fprintf(fr, "%02d.%02d\t", rdm[o], rdd[o]);
  	   	 	   }
  	   	 	   fprintf(fr, "\n");
  	   	 	   if (odd)
  	   	 	   {
  	   	 	     fprintf(fr, R_E);
  	   	 	     fprintf(fr, R_E);
  	   	 	     odd = 0;
  	   	 	   }
  	   	 	   else
  	   	 	     odd = 1;
  	   	 	   fprintf(fr, R_E);
  	   	 	 }
  	   	 	 else if (n > 0)
  	   	 	   	printf("%d.%d.%d. h‚tv‚ge vagy nnep!\n", e,h,i);
  	   	 }
  	   	 printf("\n");
  	   	 fprintf(fr, "\n");
  	   	 if (h<m)
  	   	 	  	fprintf(fr, R_PG);
  	   	 fprintf(fr, R_E);
  	   	 fprintf(fr, R_E);
  	   	 fprintf(fr, R_E);
#if 0
// beg_wr_csv
  	   	 if (first == TRUE)
  	   	 {
  	   	   fc = fopen(csvfn, "wt");
  	   	   first = FALSE;
  	   	 }
  	   	 else
  	   	   fc = fopen(csvfn, "at");
  	   	
  	   	 if (fc != NULL)
  	   	 {
  	   	   fprintf(fc, "\"%d.%02d ho\";", e, h);
  	   	   for (i=0; napok[i]; i++)
  	   	     	fprintf(fc, "\"'+%d\";", napok[i]);
  	   	   fprintf(fc, "\n");
  	   	   for (i=k; i<=x; i++)
  	   	   {
  	   	     r = proc(e, h, i);
  	   	     if (r != -1)
  	   	     {
  	   	       fprintf(fc, "\"'%02d.%02d.\";", h,i);
  	   	       for (o=0; napok[o]; o++)
  	   	         	fprintf(fc, "\"'%02d.%02d\";", rdm[o], rdd[o]);
  	   	       fprintf(fc, "\n");
  	   	     }
  	   	   }
  	   	   fprintf(fc, "\n");
  	   	   fclose(fc);
  	   	 }
// end_wr_csv
#endif
  	   }
  	   fprintf(fr, R_E);
  	   fprintf(fr, R_E);
  	   fclose(fr);
  	 }
  }
  else
  {
    id();
    puts("\t(munkanap  thelyez‚sek a 'hatarido_20??.cfg' f jlban)");
#define ps "hatarido"
    printf("Haszn lat:\t%s [-m] <‚‚‚‚[.hh[.nn]] n1 [n2 [...n10]]>\n", ps);
    printf("\tvagy:\t%s [-m] -i\n", ps);
    printf("\tvagy:\t%s [-m] -c n1[a|A|b|B|C|d] [n2[a|A|b|B|C|d] ...]\n", ps);
    puts("ahol:");
    puts("\t-m = munkanappal kezd");
    puts("\t-i = interaktˇv m˘d");
    puts("\t-c = r‚szben interaktˇv (csak d tumot k‚r)");
    puts("\t#a = nem munkanapra is eshet");
    puts("\t#A = u.a. de a kezd‹ napot is sz molja");
    puts("\t#b = csak munkanapot sz mol");
    puts("\t#B = u.a. de a kezd‹ napot is sz molja");
    puts("\t#C = a kezd‹ napot is sz molja");
    puts("\t#d = 'joger‹'");
    return -1;
  }
  execl(PRG, " ", rtffn, NULL);
  return 0;
}

