/*
 * Word count application with one thread per input file.
 *
 * You may modify this file in any way you like, and are expected to modify it.
 * Your solution must read each input file from a separate thread. We encourage
 * you to make as few changes as necessary.
 */

/*
 * Copyright © 2019 University of California, Berkeley
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <pthread.h>

#include "word_count.h"
#include "word_helpers.h"

word_count_list_t word_counts;

void *threadfun(void *argv_t) {
  char *fname = (char *)argv_t;
    //printf("haha\n");
      //fflush(stdout);
  FILE *infile = fopen(fname, "r");
  if (infile == NULL) {
    //printf("fuck %s \n",fname);
    //fflush(stdout);
    pthread_exit(NULL);
  }
  //printf("counting...\n");
  //fflush(stdout);
  count_words(&word_counts, infile);

  fclose(infile);
  
  pthread_exit(NULL);
}


/*
 * main - handle command line, spawning one thread per file.
 */
int main(int argc, char *argv[]) {
  /* Create the empty data structure. */
  
  init_words(&word_counts);
  int t;
  int rc;
  int nthreads = argc - 1;
  pthread_t threads[nthreads];
  if (argc <= 1) {
    /* Process stdin in a single thread. */
    count_words(&word_counts, stdin);
  } else {
    /* TODO */
    for(t = 0; t < nthreads; t++) {
      char * temp = argv[t+1];
      //printf("%s\n",temp);
      //fflush(stdout);
      rc = pthread_create(&threads[t], NULL, threadfun, (void *)temp);
      //printf("%d\n",rc);
      //fflush(stdout);
    }

  }
  for(t = 0; t < nthreads; t++){
    pthread_join(threads[t],NULL);
  }
  /* Output final result of all threads' work. */
  //must wait for all other threads to finish!!
  wordcount_sort(&word_counts, less_count);
  fprint_words(&word_counts, stdout);
  pthread_exit(NULL);
}
