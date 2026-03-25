#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const char *data;
  size_t count;
} String_View;

#define SV_FMT "%.*s"
#define SV_ARGS(sv) (sv).count, (sv).data

String_View sv(const char *cstr) {
  return (String_View){
      .data = cstr,
      .count = strlen(cstr),
  };
}

void sv_chop_left(String_View *sv, size_t n) {
  if (n > sv->count)
    n = sv->count;
  if (sv->count == 0)
    return;
  sv->count -= n;
  sv->data += n;
}

void sv_chop_right(String_View *sv, size_t n) {
  if (n > sv->count)
    n = sv->count;
  if (sv->count == 0)
    return;
  sv->count -= n;
}

void sv_trim_left(String_View *sv) {
  while (sv->count > 0 && isspace(sv->data[0])) {
    sv_chop_left(sv, 1);
  }
}

void sv_trim_right(String_View *sv) {
  while (sv->count > 0 && isspace(sv->data[sv->count - 1])) {
    sv_chop_right(sv, 1);
  }
}

void sv_trim(String_View *sv) {
  sv_trim_left(sv);
  sv_trim_right(sv);
}

// 分割后，返回分隔符前的字符串，sv保留之后的字符串
String_View sv_chop_by_delim(String_View *sv, const char delim) {
  size_t i = 0;
  while (i < sv->count && sv->data[i] != delim) {
    i++;
  }

  // 找到了delim
  if (i < sv->count) {
    String_View result = {
        .data = sv->data,
        .count = i,
    };
    sv_chop_left(sv, i + 1);
    return result;
  }
  // 没找到delim
  String_View result = *sv;
  sv_chop_left(sv, sv->count);
  return result;
}

String_View sv_chop_by_rule(String_View *sv, int (*rule)(int c)) {
  size_t i = 0;
  while (i < sv->count && !rule(sv->data[i])) {
    i++;
  }

  if (i < sv->count) {
    String_View result = {
        .data = sv->data,
        .count = i,
    };
    sv_chop_left(sv, i + 1);
    return result;
  }

  String_View result = *sv;
  sv_chop_left(sv, sv->count);
  return result;
}

int my_rule(int c) { return isspace(c) || c == ';' || c == ','; }

void do_something() {
  FILE *f = fopen(__FILE__, "r");
  if (!f)
    return;
  const size_t capacity = 1024 * 1024;
  char *buffer = malloc(capacity);
  if (!buffer) {
    fclose(f);
    return;
  }
  size_t size = fread(buffer, 1, capacity, f);

  String_View s = {
      .data = buffer,
      .count = size,
  };

  size_t word_count = 0;

  while (s.count > 0) {
    sv_trim_left(&s);
    if (s.count == 0)
      break;
    // 跳过注释行
    if (s.count > 1 && s.data[0] == '/' && s.data[1] == '/')
      sv_chop_by_delim(&s, '\n');
    String_View word = sv_chop_by_rule(&s, my_rule);
    // 过滤掉空字符串
    if(word.count == 0)
      continue;
    printf(SV_FMT "\n", SV_ARGS(word));
    word_count += 1;
  }
  printf("word count = %zu\n", word_count);
  free(buffer);
  fclose(f);
}

int main() {
  String_View s = sv("    Hello, World!    ");
  printf("before: |%.*s|\n", s.count, s.data);
  sv_trim(&s);
  printf("after:  |%.*s|\n", s.count, s.data);
  String_View hello = sv_chop_by_delim(&s, ',');
  printf("|" SV_FMT "|" SV_FMT "|\n", SV_ARGS(hello), SV_ARGS(s));

  printf("-----------------------------------\n");

  do_something();
  return 0;
}