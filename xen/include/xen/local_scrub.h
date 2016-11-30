#ifndef LOCAL_SCRUB_H
#define LOCAL_SCRUB_H

extern struct page_list_head scrub_list;
extern struct spinlock scrub_list_lock;

void scrub_one_page_wrap(void *param);

#endif /* LOCAL_SCRUB_H */
