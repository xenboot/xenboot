#include "xs_sub.h"

//domain *latest;
pthread_mutex_t lock_xssub = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;

domain * _add_domain(libxl_domid id, char *name, int stubdom);
domain * search_domain(libxl_domid id);
void save_data(void);
domain * get_domain(libxl_domid id, bool add_new);
void _set_stubdom_attr(domain *dom, int stubdom);


void init_memory()
{
  //printf("initialising memory\n");
  TAILQ_INIT(&domains_head);
  //  print_l("initialising memory\n", NULL);
  // Add domain 0
  // Temporary hack (Need to find a hook into Domain zero's boot process instead)
  // _add_domain(0, "Domain-0", false);
  
  /*
  domain *dom0 = malloc(sizeof(domain));
  dom0->id = 0;
  dom0->name = "Domain-0";
  dom0->stubdom = false;
  LIST_INSERT_HEAD(&domains_head, dom0, domain_list);
  */
}


// assumes checks have already been performed
domain * _add_domain(libxl_domid id, char *name, int stubdom)
{
  //print_l("Trying to add %s\n", name);
  
  //  pthread_mutex_lock(&lock_mem);
  domain *newdom = malloc(sizeof(domain));
  newdom->id = id;
  //  printf("before copy");
  int l = strlen(name) + 1;
  newdom->name = malloc(l);
  strncpy(newdom->name, name, l);
  //printf("after copy");
  newdom->stubdom = stubdom;
  newdom->exists = false;

  TAILQ_INSERT_TAIL(&domains_head, newdom, domain_list);

  /*
  //pthread_mutex_lock(&lock_mem);
  domain *last = TAILQ_LAST(&domains_head, DomainsHead);

  if(!last)
  {
    //pthread_mutex_lock(&lock_mem);
    TAILQ_INSERT_HEAD(&domains_head, newdom, domain_list);
    //    pthread_mutex_unlock(&lock_mem);
    //    return newdom;
  }    
  
  else if(last->id < id)   // this is the most likely scenario
  {
    //    pthread_mutex_lock(&lock_mem);
    TAILQ_INSERT_TAIL(&domains_head, newdom, domain_list);
    //print_l("inserting at tail %s\n", name);
    //    pthread_mutex_unlock(&lock_mem);
  }

  else
  {
    // loop from beginning and search for right position
    //    pthread_mutex_lock(&lock_mem);
    domain *curr, *prev = NULL;
    TAILQ_FOREACH(curr, &domains_head, domain_list)
    {
      if (prev && prev->id < id && curr->id > id)
      {
	TAILQ_INSERT_BEFORE(curr, newdom, domain_list);
      }
      prev = curr;
    }
    //pthread_mutex_unlock(&lock_mem);
  }
  */
  //  pthread_mutex_unlock(&lock_mem);
  //print_l("added %s\n", name);
  return newdom;
}


void xssub_remove_domain(libxl_domid id)
{
  domain *to_delete = get_domain(id, false);

  if (to_delete)
  {
    //    pthread_mutex_lock(&lock_mem);
    TAILQ_REMOVE(&domains_head, to_delete, domain_list);
    //pthread_mutex_unlock(&lock_mem);

    free(to_delete);
  }
}


domain * get_domain(libxl_domid id, bool add_new)
{
  //  pthread_mutex_lock(&lock_mem);
  if(TAILQ_EMPTY(&domains_head)) //!(domains_head) ||
  {
    init_memory();
  }
  //  pthread_mutex_unlock(&lock_mem);
  
  domain *curr = NULL;

  /*
  //pthread_mutex_lock(&lock_mem);
  if (id == 0)
  {
    curr = TAILQ_FIRST(&domains_head);
  }
  else if (latest)
  {
    if (latest->id == id)
    {
      curr = latest;
    }
    else
    {
      curr = TAILQ_NEXT(latest, domain_list);
    }
  }
  
  if (!(curr && curr->id == id && curr->exists)) */
  {
    curr = search_domain(id);
  }
    
  /*
  if (add_new)
  {
    curr = _add_domain(id, NULL, -1);
  }
  */
  
  //  latest = curr;
  //  pthread_mutex_unlock(&lock_mem);
  return curr;

}


domain * search_domain(libxl_domid id)
{
  
  domain *currdom;
  TAILQ_FOREACH(currdom, &domains_head, domain_list)
  {
    if (currdom->id == id)
    {
      return currdom;
    }
  }
  return NULL;
}


char * xssub_get_domain_name(libxl_ctx *ctx, libxl_domid id)
{
  char *ret;
  //char *reqname;
  //print_l("getting name for %d\n", id);
  //  pthread_mutex_lock(&lock_mem);
  
  domain *currdom = get_domain(id, false);
  
  if(currdom)
  {
    //    print_l("Found name of %s\n", currdom->name);
    ret = malloc((strlen(currdom->name) + 1) * sizeof(char));
    strcpy(ret, currdom->name);
    return ret;
  }
  
  char *newname = libxl_domid_to_name(ctx, id);  // Returns a new string. No need to malloc

  if (newname) // if null, domain is still in the process of being created.
  {
    _add_domain(id, newname, -1);
  }

  //  pthread_mutex_unlock(&lock_mem);
  return newname;
}

// assume that this will be called only after a get_domain_name??
int xssub_is_stubdom(libxl_ctx *ctx, libxl_domid id)
{
  domain *currdom = get_domain(id, false);

  if(currdom && currdom->stubdom != -1)
  {
    return currdom->stubdom;
  }

  int stub = libxl_is_stubdom(ctx, id, NULL);

  _set_stubdom_attr(currdom, stub);
  
  return stub;
}


void xssub_set_domain_name(libxl_domid id, const char *name)
{
  domain *dom = get_domain(id, false);

  if(dom)    //need to decide action when null
  {
    int l = strlen(name) + 1;
    dom->name = realloc(dom->name, l);
    strncpy(dom->name, name, l);
  }
  
  //save_data();
}

void xssub_set_stubdom_attr(libxl_domid id, int stubdom)
{
  domain *dom = get_domain(id, false);
  _set_stubdom_attr(dom, stubdom);
}

void _set_stubdom_attr(domain *dom, int stubdom)
{
  if(dom)    //need to decide action when null
  {
    //    pthread_mutex_lock(&lock_mem);
    dom->stubdom = stubdom;
    //pthread_mutex_unlock(&lock_mem);
  }      

  //save_data();  
}


//Just for debugging
void xssub_print_all_domains()
{
  if(TAILQ_EMPTY(&domains_head))
  {
    printf("Cache is still empty\n");
    return;
  }

  printf("%-8s %-30s %-11s\n", "Id", "Name", "Stub Domain");

  domain *currdom;  
  TAILQ_FOREACH(currdom, &domains_head, domain_list)
  {
    printf("%-8d %-30s %-11d\n", currdom->id, currdom->name, currdom->stubdom);
  }
}

/*
void make_valid(libxl_domid id)
{
  domain *dom = get_domain(id, false);

  if(dom)    //need to decide action when null
  {
    dom->exists = true;
  }      

  save_data();
}


int remove_invalid_domains()
{
  domain *currdom;
  int count = 0;
  
  TAILQ_FOREACH(currdom, &domains_head, domain_list)
  {
    if (currdom->exists == false)
    {
      remove_domain(currdom->id);
      count++;
    }
  }

  return count;
}


void save_data()
{
  // flush entire list to a file or other data store
}
*/
