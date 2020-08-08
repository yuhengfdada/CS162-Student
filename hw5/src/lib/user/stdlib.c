#include <stdlib.h>
#include <stdbool.h>
#include <syscall.h>
#include <stdio.h>
#include <string.h>


struct list_elem{
  int size;
  bool free;
  struct list_elem *prev;
  struct list_elem *next;
  struct whole_block* parent;
};

struct whole_block{
  struct list_elem metadata;
  char memories[0];
};

struct whole_block* head = NULL;

void*
malloc (size_t size)
{
  /* Homework 5, Part B: YOUR CODE HERE */
  if (size == 0) return NULL;
  if (head == NULL)
  {
    head = (struct whole_block*) sbrk(size + sizeof(struct whole_block)); //get the heap start address
    if ((int)head == -1) return NULL;
    head->metadata.size = size;
    head->metadata.free = false;
    head->metadata.prev = NULL;
    head->metadata.next = NULL;
    head->metadata.parent = head;
    return head->memories;
  }
  else
  {
    /* Traverse the linked list. */
    bool found = false;
    bool need_split = false;
    struct list_elem* elem = &(head->metadata);
    for (; elem != NULL; elem = elem->next)
    {
      if (elem->size >= size && elem->free == true)
      {
        found = true;
        if (elem->size - size > sizeof(struct list_elem)) need_split = true;
        break;
      }
    }

    /* Case 1: a suitable free block is found. */
    if (found)
    {
      if (need_split)
      {
        /* 1.1: Need to split. Insert into the linked list. */
        //elem->size = size;
        elem->free = false;
        struct whole_block* new_block = (struct whole_block*) (elem->parent->memories + size);
        new_block->metadata.size = elem->size - size - sizeof(struct list_elem);
        new_block->metadata.free = true;
        new_block->metadata.prev = elem;
        new_block->metadata.next = elem->next;
        new_block->metadata.parent = new_block;

        elem->size = size;
        elem->next = &(new_block->metadata);
        return elem->parent->memories;
      }
      else
      {
        /* 1.2: No need to split. */
        elem->free = false;
        return elem->parent->memories;
      }
    }
    else
    {
    /* Case 2: where no suitable free block is found. Must use sbrk() to request more memory. */
    struct whole_block* new_block = (struct whole_block*) sbrk(size + sizeof(struct list_elem));
    
    if ((int)new_block == -1) {
      //printf("%x\n",new_block);
      return NULL;
    }
    new_block->metadata.size = size;
    new_block->metadata.free = false;
    new_block->metadata.prev = NULL;
    new_block->metadata.next = NULL;
    new_block->metadata.parent = new_block;

    elem = &(head->metadata);
    for (; elem->next != NULL; elem = elem->next);
    elem->next = &(new_block->metadata);
    new_block->metadata.prev = elem;
    return new_block->memories;
    }
  }
  

  
}

void merge(struct list_elem* elem)
{
  if (elem->next == NULL || !elem->next->free) return;
  merge(elem->next);
  elem->size += sizeof(struct list_elem) + elem->next->size;
  elem->next = elem->next->next;
  if (elem->next) elem->next->prev = elem;
}

void free (void* ptr)
{
  /* Homework 5, Part B: YOUR CODE HERE */
  struct list_elem* elem = &(head->metadata);
  for (; elem != NULL; elem = elem->next)
  {
    if ((char *)ptr == elem->parent->memories)
    {
      elem->free = true;
      break;
    }
  }
  elem = &(head->metadata);
  for (; elem != NULL; elem = elem->next)
  {
    if (elem->free && elem->next != NULL && elem->next->free) merge(elem);
  }
}

void* calloc (size_t nmemb, size_t size)
{
  /* Homework 5, Part B: YOUR CODE HERE */
  void *res = malloc(nmemb * size);
  if(res) memset(res, 0, nmemb * size);
  return res;
}
void* realloc (void* ptr, size_t size)
{
  /* Homework 5, Part B: YOUR CODE HERE */
  if (ptr == NULL && size == 0)
  {
    return NULL;
  }
  else if (ptr != NULL && size == 0)
  {
    free(ptr);
    return NULL;
  }
  else if (ptr == NULL && size != 0)
  {
    return malloc(size);
  }
  void *new_block = malloc(size);
  if (new_block == NULL) return NULL;
  memcpy (new_block, ptr, size);
  free (ptr);
  return new_block;
}
