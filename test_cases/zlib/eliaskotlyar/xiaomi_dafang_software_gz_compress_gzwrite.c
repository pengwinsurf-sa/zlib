#include "zlib.h"
#include <stdio.h>
#include <stdint.h>
/* Sequence selected: gzwrite,gzerror,gzclose */
#include "zconf.h"
#include "zlib.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SET_BINARY_MODE(file)
#define GZ_SUFFIX ".gz"
#define SUFFIX_LEN (sizeof(GZ_SUFFIX) - 1)
#define BUFLEN 16384
#define MAX_NAME_LEN 1024
#define LARGE_LEN 100000
#define local

static const char *prog = "test_gz_compress";
static const char *TEST_DATA = "hello zlib gzwrite test data";
static const char *OUT_NAME = "test_output.gz";
static const char *LARGE_NAME = "test_output_large.gz";

void error(const char *msg)
{
  printf("[DEBUG] error() called with msg='%s' -> exiting with status 1\n", msg);
  fprintf(stderr, "%s: %s\n", prog, msg);
  exit(1);
}

int Test_gz_compress(FILE *in, gzFile out) {
  char buf[BUFLEN];
  int len = 0;
  int written = 0;
  int err = Z_OK;
  int ret = 0;
  int cret = Z_OK;
  int half = 0;
  const char *msg = NULL;

  printf("[DEBUG] Test_gz_compress: entered (in=%p, out=%p)\n", (void *)in, (void *)out);

  memset(buf, 0, sizeof(buf));

  if (in == NULL || out == NULL) {
    printf("invalid arguments: in=%p out=%p\n", (void *)in, (void *)out);
    if (out != NULL) {
      int cr = gzclose(out);
      printf("[DEBUG] Test_gz_compress: gzclose returned %d\n", cr);
    }
    return -1;
  }

  len = (int)fread(buf, 1, sizeof(buf), in);
  printf("fread returned %d bytes\n", len);

  if (ferror(in)) {
    printf("fread error on input\n");
    gzclose(out);
    return -1;
  }

  if (len <= 1) {
    printf("not enough data read from input, gzwrite not executed\n");
    gzclose(out);
    return -1;
  }

  half = len / 2;

  written = gzwrite(out, (voidpc)buf, (unsigned)half);
  printf("gzwrite (chunk 1) executed with return %d (expected %d)\n", written, half);
  if (written != half) {
    printf("ASSERTION FAILED: gzwrite chunk 1 returned %d, expected %d\n", written, half);
    ret = -1;
  }

  msg = gzerror(out, &err);
  printf("gzerror after chunk 1: %s (errnum %d)\n", msg ? msg : "(null)", err);
  if (err != Z_OK) {
    printf("ASSERTION FAILED: gzerror reports error after successful gzwrite\n");
    ret = -1;
  }

  written = gzwrite(out, (voidpc)buf, 0);
  printf("gzwrite (zero length) executed with return %d (expected 0)\n", written);
  if (written != 0) {
    printf("ASSERTION FAILED: zero length gzwrite returned %d, expected 0\n", written);
    ret = -1;
  }
  msg = gzerror(out, &err);
  printf("gzerror after zero-length write: %s (errnum %d)\n", msg ? msg : "(null)", err);
  if (err != Z_OK) {
    printf("ASSERTION FAILED: zero length gzwrite set an error\n");
    ret = -1;
  }

  written = gzwrite(out, (voidpc)(buf + half), (unsigned)(len - half));
  printf("gzwrite (chunk 2) executed with return %d (expected %d)\n", written, len - half);
  if (written != len - half) {
    printf("ASSERTION FAILED: gzwrite chunk 2 returned %d, expected %d\n", written, len - half);
    ret = -1;
  }

  msg = gzerror(out, &err);
  printf("gzerror executed with return %s (errnum %d)\n", msg ? msg : "(null)", err);
  if (err != Z_OK) {
    fprintf(stderr, "gzwrite error (%d): %s\n", err, msg ? msg : "unknown");
    ret = -1;
  }

  cret = gzclose(out);
  printf("gzclose executed with return %d\n", cret);
  if (cret != Z_OK) {
    ret = -1;
  }
  printf("[DEBUG] Test_gz_compress: returning %d\n", ret);
  return ret;
}

int main() {
  FILE *in = NULL;
  gzFile out = NULL;
  gzFile chk = NULL;
  int r = 0;
  int n = 0;
  int w = 0;
  int err = Z_OK;
  char rbuf[BUFLEN];
  size_t expected_len = strlen(TEST_DATA);
  unsigned char *big = NULL;
  unsigned char *bigr = NULL;
  int i;

  setvbuf(stdout, NULL, _IONBF, 0);
  printf("[DEBUG] main: START of test (zlib version %s)\n", zlibVersion());

  memset(rbuf, 0, sizeof(rbuf));

  in = tmpfile();
  if (in == NULL) {
    printf("tmpfile failed\n");
    return -1;
  }
  if (fputs(TEST_DATA, in) == EOF) {
    printf("fputs failed\n");
    fclose(in);
    return -1;
  }
  fflush(in);
  rewind(in);
  printf("input temp file prepared with %lu bytes\n", (unsigned long)expected_len);

  out = gzopen(OUT_NAME, "wb");
  if (out == NULL) {
    printf("gzopen for write failed\n");
    fclose(in);
    return -1;
  }
  printf("gzopen for write succeeded\n");

  r = Test_gz_compress(in, out);
  printf("Test_gz_compress returned %d\n", r);
  fclose(in);
  if (r != 0) {
    printf("[DEBUG] main: Test_gz_compress returned non-zero (%d)\n", r);
    return -1;
  }

  chk = gzopen(OUT_NAME, "rb");
  if (chk == NULL) {
    printf("gzopen for read failed\n");
    return -1;
  }
  n = gzread(chk, rbuf, sizeof(rbuf) - 1);
  printf("gzread returned %d\n", n);
  printf("[DEBUG] main: rbuf after gzread = '%s'\n", rbuf);

  /* gzwrite on a file opened for reading must fail and return 0 */
  w = gzwrite(chk, (voidpc)TEST_DATA, (unsigned)expected_len);
  printf("gzwrite on read-mode file executed with return %d (expected 0)\n", w);
  if (w != 0) {
    printf("ASSERTION FAILED: gzwrite on read-only file returned %d, expected 0\n", w);
    gzclose(chk);
    return -1;
  }
  {
    const char *emsg = gzerror(chk, &err);
    printf("[DEBUG] main: gzerror(chk) returned msg='%s'\n", emsg ? emsg : "(null)");
  }
  printf("gzerror after invalid gzwrite: errnum %d\n", err);
  /* zlib's gzwrite returns 0 for read-mode files without setting an error;
     the data stream state must remain intact (no hard errors). */
  if (err != Z_OK && err != Z_BUF_ERROR && err != Z_STREAM_END) {
    printf("ASSERTION FAILED: unexpected error state %d after invalid gzwrite\n", err);
    gzclose(chk);
    return -1;
  }
  {
    int cr = gzclose(chk);
    printf("[DEBUG] main: gzclose(chk) returned %d\n", cr);
  }

  if (n != (int)expected_len || memcmp(rbuf, TEST_DATA, expected_len) != 0) {
    printf("verification failed\n");
    return -1;
  }
  printf("verification succeeded: %s\n", rbuf);

  big = (unsigned char *)malloc(LARGE_LEN);
  bigr = (unsigned char *)malloc(LARGE_LEN);
  if (big == NULL || bigr == NULL) {
    printf("malloc failed\n");
    free(big);
    free(bigr);
    return -1;
  }
  for (i = 0; i < LARGE_LEN; i++) big[i] = (unsigned char)((i * 31 + (i >> 8)) & 0xFF);

  out = gzopen(LARGE_NAME, "wb");
  if (out == NULL) {
    printf("gzopen for large write failed\n");
    free(big);
    free(bigr);
    return -1;
  }
  w = gzwrite(out, (voidpc)big, LARGE_LEN);
  printf("gzwrite (large) executed with return %d (expected %d)\n", w, LARGE_LEN);
  if (w != LARGE_LEN) {
    printf("ASSERTION FAILED: large gzwrite returned %d, expected %d\n", w, LARGE_LEN);
    gzclose(out);
    free(big);
    free(bigr);
    return -1;
  }
  {
    int cr = gzclose(out);
    printf("[DEBUG] main: gzclose(out) after large write returned %d\n", cr);
    if (cr != Z_OK) {
      printf("gzclose after large write failed\n");
      free(big);
      free(bigr);
      return -1;
    }
  }

  chk = gzopen(LARGE_NAME, "rb");
  if (chk == NULL) {
    printf("gzopen for large read failed\n");
    free(big);
    free(bigr);
    return -1;
  }
  n = gzread(chk, bigr, LARGE_LEN);
  printf("gzread (large) returned %d\n", n);
  gzclose(chk);
  if (n != LARGE_LEN || memcmp(big, bigr, LARGE_LEN) != 0) {
    printf("large verification failed\n");
    free(big);
    free(bigr);
    return -1;
  }
  printf("large verification succeeded\n");
  free(big);
  free(bigr);
  printf("[DEBUG] main: END of test reached successfully, returning 0\n");
  return 0;
}
