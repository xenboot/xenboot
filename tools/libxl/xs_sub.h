#ifndef XS_SUB_H
#define XS_SUB_H

#include<sys/queue.h>
#include<stdlib.h>
#include<stdio.h>
#include<stdbool.h>
#include<stdint.h>
#include<pthread.h>

#include "libxl_utils.h"
#include "libxl.h"


//#define ENABLE_XS_SUB
//#define ENABLE_XSSUB_VERBOSE   //can be used for debugging if needed

typedef struct domain
{
  libxl_domid id; //uint32_t
  int stubdom;
  bool exists;
  char *name;
  TAILQ_ENTRY(domain) domain_list;
} domain;

TAILQ_HEAD(DomainsHead, domain) domains_head;// = TAILQ_HEAD_INITIALIZER(&domains_head);

extern pthread_mutex_t lock_xssub;// = PTHREAD_MUTEX_INITIALIZER;
				
void init_memory(void);
//void _add_domain(libxl_domid id, char *name, int stubdom);
void xssub_remove_domain(libxl_domid id);
char * xssub_get_domain_name(libxl_ctx *ctx, libxl_domid id);
int xssub_is_stubdom(libxl_ctx *ctx, libxl_domid id);
//domain * get_domain(libxl_domid id, bool add_new);
void xssub_set_domain_name(libxl_domid id, const char *name);
void xssub_set_stubdom_attr(libxl_domid id, int stubdom);
//void make_valid(libxl_domid id);
//int remove_invalid_domains(void);
void xssub_print_all_domains(void);


#endif
