/*
	efunctions.c: this file contains the efunctions called from
	inside eval_instruction() in interpret.c.  Note: if you are adding
    local efunctions that are specific to your driver, you would be better
    off adding them to a separate source file.  Doing so will make it much
    easier for you to upgrade (won't have to patch this file).  Be sure
    to #include "efuns.h" in that separate source file.
*/

#include "efuns.h"
#include "stralloc.h"
#if defined(__386BSD__) || defined(SunOS_5)
#include <unistd.h>
#endif

/* get a value for CLK_TCK for use by times() */
#if (defined(TIMES) && !defined(RUSAGE))
/* this may need #ifdef'd to handle different types of machines */
#include <limits.h>
#endif

#ifdef F_CRYPT
void
f_crypt(num_arg, instruction)
int num_arg, instruction;
{
  char *res, salt[2];
  char *choice =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ./";
	
  if (sp->type == T_STRING && SVALUE_STRLEN(sp) >= 2)
    {
      salt[0] = sp->u.string[0];
      salt[1] = sp->u.string[1];
    }
  else
    {
      salt[0] = choice[random_number(strlen(choice))];
      salt[1] = choice[random_number(strlen(choice))];
    }
#if defined(sun) && !defined(SunOS_5)
  res = string_copy(_crypt((sp-1)->u.string, salt));
#else
  res = string_copy(crypt((sp-1)->u.string, salt));
#endif
  pop_n_elems(2);
  push_malloced_string(res);
}
#endif

#ifdef F_LOCALTIME
void
f_localtime(num_arg, instruction)
    int num_arg, instruction;
{
    struct tm *tm;
    struct vector *vec;
    time_t lt;
#ifdef sequent
    struct timezone tz;
#endif

    lt = sp->u.number;
    tm = localtime(&lt);

    vec = allocate_array(10);
    vec->item[LT_SEC].type = T_NUMBER;
    vec->item[LT_SEC].u.number = tm->tm_sec;
    vec->item[LT_MIN].type = T_NUMBER;
    vec->item[LT_MIN].u.number = tm->tm_min;
    vec->item[LT_HOUR].type = T_NUMBER;
    vec->item[LT_HOUR].u.number = tm->tm_hour;
    vec->item[LT_MDAY].type = T_NUMBER;
    vec->item[LT_MDAY].u.number = tm->tm_mday;
    vec->item[LT_MON].type = T_NUMBER;
    vec->item[LT_MON].u.number = tm->tm_mon;
    vec->item[LT_YEAR].type = T_NUMBER;
    vec->item[LT_YEAR].u.number = tm->tm_year + 1900;
    vec->item[LT_WDAY].type = T_NUMBER;
    vec->item[LT_WDAY].u.number = tm->tm_wday;
    vec->item[LT_YDAY].type = T_NUMBER;
    vec->item[LT_YDAY].u.number = tm->tm_yday;
    vec->item[LT_GMTOFF].type = T_NUMBER;
    vec->item[LT_ZONE].type = T_STRING;
    vec->item[LT_ZONE].subtype = STRING_MALLOC;
#if defined(BSD42) || defined(apollo) || defined(_AUX_SOURCE) \
	|| defined(OLD_ULTRIX)
	/* 4.2 BSD doesn't seem to provide any way to get these last two values */
    vec->item[LT_GMTOFF].type = T_NUMBER;
	vec->item[LT_GMTOFF].u.number = 0;
    vec->item[LT_ZONE].type = T_NUMBER;
	vec->item[LT_ZONE].u.number = 0;
#else /* BSD42 */
#if defined(sequent)
    gettimeofday(NULL, &tz);
    vec->item[LT_GMTOFF].u.number = tz.tz_minuteswest;
    vec->item[LT_ZONE].u.string =
	string_copy(timezone(tz.tz_minuteswest, tm->tm_isdst));
#else /* sequent */
#if (defined(hpux) || defined(_SEQUENT_) || defined(_AIX) || defined(SunOS_5) \
	|| defined(SVR4) || defined(sgi) || defined(linux) || defined(cray) \
	|| defined(LATTICE)) 
    if (!tm->tm_isdst) {
        vec->item[LT_GMTOFF].u.number = timezone;
        vec->item[LT_ZONE].u.string = string_copy(tzname[0]);
    } else {
#if (defined(_AIX) || defined(hpux) || defined(linux) || defined(cray) \
	|| defined(LATTICE))
        vec->item[LT_GMTOFF].u.number = timezone;
#else
        vec->item[LT_GMTOFF].u.number = altzone;
#endif
        vec->item[LT_ZONE].u.string = string_copy(tzname[1]);
    }
#else
    vec->item[LT_GMTOFF].u.number = tm->tm_gmtoff;
    vec->item[LT_ZONE].u.string = string_copy(tm->tm_zone);
#endif
#endif /* sequent */
#endif /* BSD42 */
    pop_stack();
    push_vector(vec);
    vec->ref--;
}
#endif

#ifdef F_RUSAGE
#ifdef RUSAGE
void
f_rusage(num_arg, instruction)
int num_arg, instruction;
{
  struct rusage rus;
  struct mapping *m;
  long usertime, stime;
  int maxrss;

  if (getrusage(RUSAGE_SELF, &rus) < 0) {
    m = allocate_mapping(0);
  } else {
#ifndef SunOS_5
    usertime = rus.ru_utime.tv_sec * 1000 + rus.ru_utime.tv_usec / 1000;
    stime = rus.ru_stime.tv_sec * 1000 + rus.ru_stime.tv_usec / 1000;
#else
    usertime = rus.ru_utime.tv_sec * 1000 + rus.ru_utime.tv_nsec / 1000000;
    stime = rus.ru_stime.tv_sec * 1000 + rus.ru_stime.tv_nsec / 1000000;
#endif
    maxrss = rus.ru_maxrss;
#ifdef sun
    maxrss *= getpagesize() / 1024;
#endif
    m = allocate_mapping(16);
    add_mapping_pair(m, "utime", usertime);
    add_mapping_pair(m, "stime", stime);
    add_mapping_pair(m, "maxrss", maxrss);
    add_mapping_pair(m, "ixrss", rus.ru_ixrss);
    add_mapping_pair(m, "idrss", rus.ru_idrss);
    add_mapping_pair(m, "isrss", rus.ru_isrss);
    add_mapping_pair(m, "minflt", rus.ru_minflt);
    add_mapping_pair(m, "majflt", rus.ru_majflt);
    add_mapping_pair(m, "nswap", rus.ru_nswap);
    add_mapping_pair(m, "inblock", rus.ru_inblock);
    add_mapping_pair(m, "oublock", rus.ru_oublock);
    add_mapping_pair(m, "msgsnd", rus.ru_msgsnd);
    add_mapping_pair(m, "msgrcv", rus.ru_msgrcv);
    add_mapping_pair(m, "nsignals", rus.ru_nsignals);
    add_mapping_pair(m, "nvcsw", rus.ru_nvcsw);
    add_mapping_pair(m, "nivcsw", rus.ru_nivcsw);
  }
  m->ref--;
  push_mapping(m);
}
#else

#ifdef GET_PROCESS_STATS
void
f_rusage(num_arg, instruction)
int num_arg, instruction;
{
    struct process_stats ps;
    struct mapping *m;
    int utime, stime, maxrss;

    if (get_process_stats(NULL, PS_SELF, &ps, NULL) == -1)
	m = allocate_mapping(0);
    else {
	utime = ps.ps_utime.tv_sec * 1000 + ps.ps_utime.tv_usec / 1000;
	stime = ps.ps_stime.tv_sec * 1000 + ps.ps_stime.tv_usec / 1000;
	maxrss = ps.ps_maxrss * getpagesize() / 1024;

	m = allocate_mapping(19);
	add_mapping_pair(m, "utime", utime);
	add_mapping_pair(m, "stime", stime);
	add_mapping_pair(m, "maxrss", maxrss);
	add_mapping_pair(m, "pagein", ps.ps_pagein);
	add_mapping_pair(m, "reclaim", ps.ps_reclaim);
	add_mapping_pair(m, "zerofill", ps.ps_zerofill);
	add_mapping_pair(m, "pffincr", ps.ps_pffincr);
	add_mapping_pair(m, "pffdecr", ps.ps_pffdecr);
	add_mapping_pair(m, "swap", ps.ps_swap);
	add_mapping_pair(m, "syscall", ps.ps_syscall);
	add_mapping_pair(m, "volcsw", ps.ps_volcsw);
	add_mapping_pair(m, "involcsw", ps.ps_involcsw);
	add_mapping_pair(m, "signal", ps.ps_signal);
	add_mapping_pair(m, "lread", ps.ps_lread);
	add_mapping_pair(m, "lwrite", ps.ps_lwrite);
	add_mapping_pair(m, "bread", ps.ps_bread);
	add_mapping_pair(m, "bwrite", ps.ps_bwrite);
	add_mapping_pair(m, "phread", ps.ps_phread);
	add_mapping_pair(m, "phwrite", ps.ps_phwrite);
    }
    m->ref--;
    push_mapping(m);
}
#else

#ifdef TIMES /* has times() but not getrusage() */

/*
  warning times are reported in processor dependent units of time.
  see man pages for 'times' to figure out how long a tick is on your system.
*/

void
f_rusage(num_arg, instruction)
int num_arg, instruction;
{
	struct mapping *m;
	struct tms t;

	times(&t);
	m = allocate_mapping(2);
    add_mapping_pair(m, "utime", t.tms_utime * 1000 / CLK_TCK);
    add_mapping_pair(m, "stime", t.tms_stime * 1000 / CLK_TCK);
	m->ref--;
	push_mapping(m);
}

#endif /* TIMES */

#endif /* GET_PROCESS_STATS */

#endif /* RUSAGE */

#endif