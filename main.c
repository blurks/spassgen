/****************************************************************
 *		spassgen - a simple password generator          *
 *                                                              *
 * This is a very simple password generator that uses the linux *
 * kernel's random number generator.                            *
 *                                                              *
 * TODO: check if the gen_pass and l_shift_array methods work   *
 *       perfectly correct.                                     *
 *                                                              *
 * written by trk                                               *
 *                                                              *
 ****************************************************************/

#include <stdio.h> /* fopen, fread, fclose, printf, fprintf ... */
#include <stdlib.h> /* size_t, strtoul, malloc, free */
#include <errno.h> /* errno */
#include <string.h> /* strlen, strcmp, strerror*/
#include <math.h> /* ceill, log2l */
#include <getopt.h> /* getopt */
#include <assert.h> /* assert */

/* following chars will be printed bold */
#define BOLD "\033[1m"
/* following chars won't be printed bold */
#define NOBOLD "\033[0m"

/* Character sets */
char const* const ascii = " !\"#$%&'()*+,-./0123456789:;<=>?@"
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
  "abcdefghijklmnopqrstuvwxyz{|}~";
char const* const alpha = "abcdefghijklmnopqrstuvwxyz"
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char const* const num = "1234567890";
char const* const alphanum ="abcdefghijklmnopqrstuvwxyz"
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

/* linux kernel's random number generator interfaces */
char const* const dev_random = "/dev/random";
char const* const dev_urandom = "/dev/urandom";

/* function declarations */
char* gen_pass(char const* rnd_dev, size_t len, char const* symbols);
void l_shift_array(void* arr, size_t arr_len, size_t bits);
void print_help(FILE* stream);

/* --- Main Program --- */
int main(int argc, char** argv)
{
  /* settings and with default values */
  char const* rnd_dev = dev_random;
  char const* charset = ascii;
  size_t length = 10;

  /* parse command line arguments */
  int opt_char;
  do {
    opt_char = getopt(argc, argv, "hruc:");

    switch(opt_char) {
    case 'h':
      print_help(stdout);
      exit(EXIT_SUCCESS);
      break;
    case 'r':
      rnd_dev = dev_random;
      break;
    case 'u':
      rnd_dev = dev_urandom;
      break;
    case 'c':
      if(strcmp(optarg, "ascii") == 0) {
	charset = ascii;
      }
      else if(strcmp(optarg, "alphanum") == 0) {
	charset = alphanum;
      }
      else if(strcmp(optarg, "alpha") == 0) {
	charset = alpha;
      }
      else if(strcmp(optarg, "num") == 0) {
	charset = num;
      }
      else {
	fprintf(stderr, "Unknown character set: %s\n", optarg);
	print_help(stderr);
	exit(EXIT_FAILURE);
      }
      break;
    case '?':
      /* unknown command line option. getopt autonomously prints
	 default error message, so we only have to print out the help
	 text. */
      print_help(stderr);
      exit(EXIT_FAILURE);
    }
  } while(opt_char > 0);

  /* parse <length> argument or use default, if not given */
  if(optind < argc - 1) {
    fprintf(stderr, "Too many arguments.\n");
    print_help(stderr);
    exit(EXIT_FAILURE);
  }
  else if (optind == argc - 1) { /* <length> is the last element  of argv */
    char* endptr;
    length = strtoul(argv[optind], &endptr, 0);
    if(*endptr != '\0') {
      fprintf(stderr, "Invalid argument: length has to be a number!\n");
      print_help(stderr);
      exit(EXIT_FAILURE);
    }
  }

  /* generate password */
  char* password = gen_pass(rnd_dev, length, charset);
  if(password == NULL) {
    fprintf(stderr, "Error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);    
  }
  
  printf("%s\n", password);
  
  free(password);
  return 0;
}

/* print a friendly help text to given stream. */
void print_help(FILE* stream)
{
  fprintf(stream, "Usage: spassgen [-h|-r|-u|-c <charset>] <length>\n"
	  "Generate a password of given length using the linux kernel's random\n"
	  "number genebrator.\n\n"
	  "  -h \tprint that help text\n"
	  "  -r \tuse /dev/random to generated randomness (default). This is the\n"
	  "     \tsaver, but slower option\n"
	  "  -u \tuse /dev/urandom to generated randomness. This is faster, but\n"
	  "     \tless secure.\n"
	  "  -c <charset> \tuse only characters of the specified character\n"
	  "               \tset. Available sets are ascii (default), alphanum,\n"
	  "               \talpha, num.\n");
}

/* return a random string of given lenght (len), containing only the
   given characters (symbols). symbols has to be a null-terminated
   string. rnd_dev is the source of randomness (either /dev/random or
   /dev/urandom). */
char* gen_pass(char const* rnd_dev, size_t password_len, char const* symbols)
{
  /* if this pointer is still NULL at the end of the function, it
     indicates, that an error occured. */
  char* return_val = NULL;
  char* password = NULL;
  char* rnd_bytes = NULL;
  FILE* rnd_dev_fd = NULL;

  size_t symbols_len = strlen(symbols);
  /* number of bits needed to encode one symbol */
  size_t bits_per_symbol = (size_t) ceill(log2l((long double) symbols_len));
  /* number of bits needed to encode the whole password */
  size_t needed_bits = password_len * bits_per_symbol;
  /* number of bytes needed to store needed_bits */
  size_t rnd_bytes_len = (size_t) ceill((long double) needed_bits / 8.0);

  /* mask for leftmost bits in a byte */
  unsigned char bitmask = ~(0xff >> bits_per_symbol);

  /* Allocate buffers and open /dev/(u)random */
  rnd_bytes = malloc(rnd_bytes_len);
  if(rnd_bytes == NULL) {
    goto fail;
  }
  /* password_len + 1 to hold a trailing '\0' */
  password = malloc(password_len + 1);
  if(password == NULL) {
    goto fail;
  }
  rnd_dev_fd = fopen(rnd_dev, "r");
  if(rnd_dev_fd == NULL) {
    goto fail;
  }

  /* Number of symbols in the rnd_bytes */
  size_t avail_rnd_symbols = 0;
  size_t password_index = 0;
  while(password_index < password_len) {
    /* If we run out of random Data, reread some. */
    if(avail_rnd_symbols == 0) {
      needed_bits = bits_per_symbol * (password_len - password_index);
      size_t needed_bytes = (size_t) ceill((long double) needed_bits / 8.0);
      assert(needed_bytes <= rnd_bytes_len);
      /* A short number of read bytes might indicate an error. TODO: check
	 feof and ferror */
      if(fread(rnd_bytes, 1, needed_bytes, rnd_dev_fd) < needed_bytes) {
	goto fail;
      }
      avail_rnd_symbols = needed_bytes;
    }
    /* read all needed bits for the current symbol from rnd_bytes */
    unsigned char sym_index = (rnd_bytes[0] & bitmask);
    /* they come from the left, but must go to the right edge of the byte */
    sym_index = sym_index >> (8 - bits_per_symbol);
    /* shift the next symbol encoding into place */
    l_shift_array(rnd_bytes, rnd_bytes_len, bits_per_symbol);
    avail_rnd_symbols -= 1;
    /* we use sym_index only if it is in range of the number of
       symbols. otherwise we discard the bits and use the folowing
       ones. */
    if(sym_index < symbols_len) {
      password[password_index] = symbols[sym_index];
      /* if we got a symbol, increase password-symbol-index by one */
      password_index += 1;
    }
  }
  password[password_len] = '\0';
  return_val = password;

 fail:
  /* if an error occured, return_val is set to NULL */
  if(return_val == NULL && password != NULL) {
      free(password);
  }
  /* clean up */
  if(rnd_dev_fd != NULL) {
    fclose(rnd_dev_fd);
  }
  if(rnd_bytes != NULL) {
    free(rnd_bytes);
  }
  return return_val;
}

/* shift the given array <bits> bits to the left. */
void l_shift_array(void* arr, size_t arr_len, size_t bits)
{
  size_t i;
  /* mask of the first <bits> bits from left to rigth.
     eg. for bits=3 , bitmask=0b11100000 */
  unsigned char bitmask = ~(0xff >> bits);

  /* begin with the very left byte */
  for(i = 0; i < arr_len; i++) {
    unsigned char c = *((unsigned char*) arr + i);
    /* shift it to the left. insert 0s from the right */
    c = c << bits;
    /* if there is byte to right of the current one, take the first
       <bits> bits from the left of it and put them into the last
       <bits> bits of the current one. */
    if(i + 1 < arr_len) {
      unsigned char c2 = *((unsigned char*) arr + i + 1);
      c2 &= bitmask;
      c2 = c2 >> (8 - bits);
      c |= c2;
    }
    /* save byte to memory */
    *((unsigned char*) arr + i) = c;
  }
}
