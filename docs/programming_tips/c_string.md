# C String and String_View

> Reference: [C Strings are Terrible! By Tsoding](https://www.youtube.com/watch?v=y8PLpDgZc0E)

## C语言并没有字符串

是的，C语言严格来说就是没有字符串。

```c
#include <stdio.h>
int main(){
  char* s = "Hello, World!";
  printf("%s\n", s);
  return 0;
}
```

我们常称`s`为字符串，而事实上，这不过是一个指针，实际的内容被存在内存某处，并且要求`\0`结尾，在内存中，它看起来像这样

```txt
......Hello, World!\0.....
```

由于是指针，因此砍掉字符串左边是容易的，而且我们根本不需要改变实际的字符串内容。

```c
printf("%s\n", s + 1);
```

但是从字符串右边砍掉就不是那么方便了，由于这里的字符串要求`\0`为结尾，我们不能在不改变`s`的情况下把右边砍掉。
于是我们尝试

```c
int n = strlen(s);
s[n - 1] = 0;
printf("%s\n", s);
```

然而这会导致`Segmentation fault`，因为`"Hello, World!"`字面量是一个`static const`的，根本不可变。
因此，我们必须在一个可写内存中存储上述字符串，以及末尾的`\0`，也就是得拷贝字符串，然后做我们想做的事。
我们分配一个动态内存，然后将字面量拷贝，从而能让我们修改。
当然，C语言（**C23**）有相应的库函数`strdup`帮我们做这件事。

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(){
  char* s = strdup("Hello, World!");
  int n = strlen(s);
  s[n - 1] = 0;
  printf("%s\n", s);
  free(s); // remember to free
  return 0;
}
```

好了，花点时间看看C语言的字符串设计：我们用指针和结尾的0来实现。这也是C语言的库函数以及无数操作系统的API要求的规范。

## String_View的设计与使用

现代编程语言中的`String_View`往往是一个结构体，存着字符串起始地址和字符数量。在这种情况下，我们不再需要一个终止符

```c
typedef struct {
  const char* data;
  size_t count;
}String_View;
```

以及一个简单的构造函数

```c
String_View sv(const char* cstr){
  return (String_View){
    .data = cstr,
    .count = strlen(cstr),
  };
}
```

现在，再来看看从左右砍掉字符，简洁许多，也没有额外申请内存的开销

```c
void sv_chop_left(String_View* sv, size_t n){
  if(n > sv->count) n = sv->count;
  if(sv->count == 0) return;
  sv->count -= n;
  sv->data += n;
}

void sv_chop_right(String_View* sv, size_t n){
  if(n > sv->count) n = sv->count;
  if(sv->count == 0) return;
  sv->count -= n;
}
```

然而，现在还有问题，那就是打印一个`String_View`时，如果我们仅采用`%s`来打印，那么`printf`并不会care我们的`count`，没办法，我们得用点`printf`的特性

```c
int main(){
  String_View s = sv("Hello, World!");
  printf("%.*s\n", s.count, s.data);
  return 0;
}
```

`String_View`的一个常见支持是去除左右空格。我们来实现

```c
void sv_trim_left(String_View* sv){
  while(sv->count > 0 && isspace(sv->data[0])){
    sv_chop_left(sv, 1);
  }
}

void sv_trim_right(String_View* sv){
  while(sv->count > 0 && isspace(sv->data[sv->count - 1])){
    sv_chop_right(sv, 1);
  }
}

void sv_trim(String_View* sv){
  sv_trim_left(sv);
  sv_trim_right(sv);
}

int main(){
  String_View s = sv("  Hello, World!  ");
  printf("before: |%.*s|\n", s.count, s.data);
  sv_trim(&s);
  printf("after:  |%.*s|\n", s.count, s.data);
  return 0;
}
```

我们也可以让`String_View`支持字符串分割。

```c
//分割后，返回分隔符前的字符串，sv保留之后的字符串
String_View sv_chop_by_delim(String_View* sv, const char delim){
  size_t i = 0;
  while(i < sv->count && sv->data[i] != delim){
    i++;
  }
  
  // 找到了delim
  if(i < sv->count){
    String_View result = {
      .data = sv->data,
      .count = i,
    };
    sv_chop_left(sv, i + 1);
    return result;
  }
  //没找到delim
  String_View result = *sv;
  sv_chop_left(sv, sv->count);
  return result;
}

int main(){
  String_View s = sv("    Hello, World!    ");
  sv_trim(&s);
  String_View hello = sv_chop_by_delim(&s, ',');
  printf("|%.*s|%.*s|\n", hello.count, hello.data, s.count, s.data);
  return 0;
}
```

然而，这里可以看到，一个`String_View`还好，一多起来打印确实麻烦。所以，我们可以再进一步，再用点C语言的特性，字符串合并。
我们知道，在C语言中，`"hello" "world"`实际等价于`"helloworld"`
那么我们可以用一下预处理。

```c
#define SV_FMT "%.*s"
#define SV_ARGS(sv) (sv).count, (sv).data
```

那么上述打印可以改为

```c
printf("|"SV_FMT"|"SV_FMT"|\n", SV_ARGS(hello), SV_ARGS(s));
```

## 做些有趣的事

让我们用`String_View`做些interesting的事，例如，打印这个源代码。
>仅仅是个demo，让我们忽略错误处理:(

```c
void do_something(){
  FILE* f = fopen(__FILE__, "r");
  if (!f)
    return;
  const size_t capacity = 1024 * 1024;
  char* buffer = malloc(capacity);
  if (!buffer) {
    fclose(f);
    return;
  }
  size_t size = fread(buffer, 1, capacity, f);

  String_View s = {
  .data = buffer,
  .count = size,
  };
  
  while(s.count > 0){
  String_View line = sv_chop_by_delim(&s, '\n');
  printf(SV_FMT"\n", SV_ARGS(line));
  }
  free(buffer);
  fclose(f);
}
```

进一步，我们可以按单词来打印 ~~（伪tokenizer）~~，为了做到这一点，我们需要按照一定规则来分割，而不是再按照单一的`delim`。

```c
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
```

现在，我们来修改`do_something`

```c
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
```
