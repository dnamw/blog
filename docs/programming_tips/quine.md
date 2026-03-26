# Quine

> Quine 是一种计算机程序，执行后能够打印出自身的源码。

## 为什么会存在这样的程序

自己创造自己，是一件很反直觉的事。在现实生活中，创造某物往往需要借助外力。例如，游戏由人创造出来，游戏不可能自己开发自己。然而，生命却是一种神奇的东西：细胞可以一分为二，生命可以自己创造自己。那么，人类为什么想写一个能打印自身源码的程序呢？或许也是生物的本能吧 :)

先别急着写这样一个程序，我们得先知道在理论上，是否存在这样的程序。因此，我们需要借助可计算理论的力量，快速入门一下可计算理论。

### 可计算理论

我们给计算机程序起个名字，叫**可计算函数**。例如，简单的 \(f(x) = x + 1\) 就是一个可计算函数，它对任意输入 \(x\) 都有输出，这样的函数我们称为**全函数**。而有些函数在某些输入上会没有结果（例如程序死循环或发生错误），这种函数叫做**部分函数**。

对于计算机程序而言，一切输入输出本质上都是整数，而计算机程序本身也不过是一串整数。因此，我们可以给每个程序一个编号，用这个编号来代表程序。
我们用 \(\phi_n\) 来表示编号为 \(n\) 的程序，那么 \(\phi_n(x)\) 就是程序 \(n\) 在输入 \(x\) 下的输出。

接下来我们看两个重要定理。

#### 普适性定理

存在一个**通用程序** \(u\)，使得

\[
\phi_u(n, x) = \phi_n(x)
\]

也就是说，把程序编号 \(n\) 和它的输入 \(x\) 一起交给通用程序 \(u\)，它就能模拟程序 \(n\) 在 \(x\) 上的运行结果。

这个通用程序，是不是感觉和我们现在计算机里的解释器很相似呢？

#### s-m-n 定理（参数化定理）

如果 \(\phi_n(y, x)\) 是一个接受两个参数的程序，那么对于任意固定的 \(y\)，我们可以**自动构造**一个新程序，它只接受一个参数 \(x\)，并且行为等同于把 \(y\) 固定后的结果。即存在一个可计算函数 \(s(n, y)\)，使得

\[
\phi_{s(n, y)}(x) = \phi_n(y, x)
\]

这其实就是编程里的**柯里化**（Currying）。

了解了上述两个定理，我们来看最重要的不动点定理，它与我们的 Quine 程序息息相关。

#### 不动点定理

设 \(h: \mathbb{N} \to \mathbb{N}\) 是一个可计算的全函数，那么存在一个自然数 \(n\)，使得

\[
\phi_n = \phi_{h(n)}
\]

即程序 \(n\) 和程序 \(h(n)\) 计算同一个函数。

简易证明：
\(\forall t\)，考虑 \(\phi_t(t)\)，即以程序自身作为输入。根据 **s-m-n 定理**，我们可以构造一个变换 \(s\)，使得 \(\phi_{s(t,t)} = \phi_t(t)\)。

进一步，我们将变换 \(h\) 作用于 \(s(t,t)\) 上，得到 \(h(s(t,t))\)。

现在我们考虑 \(\phi_{h(s(t,t))}\)，它是一个没有输入的函数，它的程序却是以 \(t\) 为输入、经过 \(s\) 和 \(h\) 变换得到的。那么，对于这样一个程序，由普适性定理，结合 \(s\) 和 \(h\) 的可计算性，我们可以找到一个程序 \(m\)，满足 \(\phi_m(t) = \phi_{h(s(t,t))}\)。现在，我们声称

\[
n = s(m,m)
\]

就是我们要找的不动点。
验证：

\[
\phi_n = \phi_{s(m,m)} = \phi_m(m)
\]

\[
\phi_{h(n)} = \phi_{h(s(m,m))} = \phi_m(m)
\]

即证。

#### Quine 的合理性

现在，我们假设 \(h(t)\) 是这样的程序：它打印 \(t\) 的源代码。
那么，由不动点定理，存在一个程序 \(n\) 为 \(h\) 的不动点，那么程序 \(n\) 就是一个打印自身源代码的程序。
于是，我们就有了 **Quine** 程序。

## 用 C 语言来写这样一个程序

> Reference: [Self-Reproducing Programs By Tsoding](https://www.youtube.com/watch?v=QGm-d5Ch5JM)

### 初试牛刀

一个简单但不容易想到的思路是，我们有一个能指向源代码的字符串，然后输出这个字符串就行了。

> 正如上述证明的开头一样，我们得把自己包含在自己当中。尽管这很奇怪，但我们感觉这应该是有效的。

```c
#include <stdio.h>

int main() {
  const char *self = "";
  printf("%s", self);
  return 0;
}
```

当然，这段程序输出的是空白内容，因为 `self` 什么都没有。

现在，我们手动让 `self` 包含程序的源代码，但直接复制是不行的。我们需要处理换行符和双引号。具体而言，我们要把换行符手动替换为 `\n`，把双引号手动替换为 `\"`。

```c
#include <stdio.h>

int main() {
  const char *self = "#include <stdio.h>\n\nint main() {\n  const char *self = \"\";\n  printf(\"%s\", self);\n  return 0;\n}\n";
  printf("%s", self);
  return 0;
}
```

### 包含自己？

现在，运行这段代码，我们得到如下输出：

```txt
#include <stdio.h>

int main() {
  const char *self = "";
  printf("%s", self);
  return 0;
}
```

不错，有输出了，但问题是这里 `self` 的内容并没有被输出出来。此时，一个常见的想法是：我修改 `self` 包含的内容，但这显然会导致无限递归。

所以，让我们重新审视 `self` 变量。正是因为 `self` 是一个变量，所以我们能做的事情更多：一个变量可以反复使用，我们也可以反复打印它。

我们的思路如下：我们在 `self` 变量中设置一个 **锚点**（anchor），这个字符是我们的代码不会用到的。然后我们逐字打印 `self`，如果遇到了这个锚点，就意味着我们需要再次打印一遍 `self` 变量，我们再次打印即可。在这里，我们不妨取锚点为 `?`。

```c
#include <stdio.h>
#include <string.h>

int main() {
  const char *self = "?";
  int n = strlen(self);
  for (int i = 0; i < n; i++) {
    if (self[i] == '?') {
        printf("%s", self);
    } else {
      printf("%c", self[i]);
    }
  }
  return 0;
}
```

我们再让 `self` 重新包含源代码，这样得到我们的第二版程序：

```c
#include <stdio.h>
#include <string.h>

int main() {
  const char *self = "#include <stdio.h>\n#include <string.h>\n\nint main() {\n  const char *self = \"?\";\n  int n = strlen(self);\n  for (int i = 0; i < n; i++) {\n    if (self[i] == '?') {\n        printf(\"%s\", self);\n    } else {\n      printf(\"%c\", self[i]);\n    }\n  }\n  \n  return 0;\n}\n";
  int n = strlen(self);
  for (int i = 0; i < n; i++) {
    if (self[i] == '?') {
        printf("%s", self);
    } else {
      printf("%c", self[i]);
    }
  }
  return 0;
}
```

### 处理转义

现在，我们运行第二版程序：

```txt
#include <stdio.h>
#include <string.h>

int main() {
  const char *self = "#include <stdio.h>
#include <string.h>

int main() {
  const char *self = "?";
  int n = strlen(self);
  for (int i = 0; i < n; i++) {
    if (self[i] == '?') {
        printf("%s", self);
    } else {
      printf("%c", self[i]);
    }
  }

  return 0;
}
";
  int n = strlen(self);
  for (int i = 0; i < n; i++) {
    if (self[i] == '#include <stdio.h>
#include <string.h>

int main() {
  const char *self = "?";
  int n = strlen(self);
  for (int i = 0; i < n; i++) {
    if (self[i] == '?') {
        printf("%s", self);
    } else {
      printf("%c", self[i]);
    }
  }

  return 0;
}
') {
        printf("%s", self);
    } else {
      printf("%c", self[i]);
    }
  }

  return 0;
}
```

不错，这次 `self` 可以输出自己包含的内容了，但还是有点不对。一个明显的问题是，`self` 输出的 `\n`、`\"` 被转义了，所以打印的是转义后的字符，而原来的转义符号 `\` 没了。

因此，我们在输出 `self` 时，需要对其中的转义字符做特殊处理，所以我们得再一次遍历 `self`，这次是为了处理转义字符。

```c
#include <stdio.h>
#include <string.h>

int main() {
  const char *self = "?";
  int n = strlen(self);
  for (int i = 0; i < n; i++) {
    if (self[i] == '?') {
      for (int j = 0; j < n; j++) {
        switch (self[j]) {
        case '\n':
          printf("\\n");
          break;
        case '"':
          printf("\\\"");
          break;
        default:
          printf("%c", self[j]);
        }
      }
    } else {
      printf("%c", self[i]);
    }
  }
  return 0;
}
```

当然，这里同时引出一个新的问题：我们在源代码中使用了 `\\`，这个东西在 `self` 中也会被转义成单独的 `\`，所以我们还需要特殊处理：

```c
case '\\':
  printf("\\\\");
  break;
```

好了，我们整合上面的修改，再让 `self` 包含源代码。注意，此时 `\` 也要被我们处理成 `\\`，而且要先于 `\n` 和 `"` 的处理。我们得到第三版程序：

```c
#include <stdio.h>
#include <string.h>

int main() {
  const char *self = "#include <stdio.h>\n#include <string.h>\n\nint main() {\n  const char *self = \"?\";\n  int n = strlen(self);\n  for (int i = 0; i < n; i++) {\n    if (self[i] == '?') {\n      for (int j = 0; j < n; j++) {\n        switch (self[j]) {\n        case '\\n':\n          printf(\"\\\\n\");\n          break;\n        case '\"':\n          printf(\"\\\\\\\"\");\n          break;\n        case '\\\\':\n          printf(\"\\\\\\\\\");\n          break;\n        default:\n          printf(\"%c\", self[j]);\n        }\n      }\n    } else {\n      printf(\"%c\", self[i]);\n    }\n  }\n  return 0;\n}\n";
  int n = strlen(self);
  for (int i = 0; i < n; i++) {
    if (self[i] == '?') {
      for (int j = 0; j < n; j++) {
        switch (self[j]) {
        case '\n':
          printf("\\n");
          break;
        case '"':
          printf("\\\"");
          break;
        case '\\':
          printf("\\\\");
          break;
        default:
          printf("%c", self[j]);
        }
      }
    } else {
      printf("%c", self[i]);
    }
  }
  return 0;
}
```

### 最后一问？

好了，现在，我们运行第三版程序：

```txt
#include <stdio.h>
#include <string.h>

int main() {
  const char *self = "#include <stdio.h>\n#include <string.h>\n\nint main() {\n  const char *self = \"?\";\n  int n = strlen(self);\n  for (int i = 0; i < n; i++) {\n    if (self[i] == '?') {\n      for (int j = 0; j < n; j++) {\n        switch (self[j]) {\n        case '\\n':\n          printf(\"\\\\n\");\n          break;\n        case '\"':\n          printf(\"\\\\\\\"\");\n          break;\n        case '\\\\':\n          printf(\"\\\\\\\\\");\n          break;\n        default:\n          printf(\"%c\", self[j]);\n        }\n      }\n    } else {\n      printf(\"%c\", self[i]);\n    }\n  }\n  return 0;\n}\n";
  int n = strlen(self);
  for (int i = 0; i < n; i++) {
    if (self[i] == '#include <stdio.h>\n#include <string.h>\n\nint main() {\n  const char *self = \"?\";\n  int n = strlen(self);\n  for (int i = 0; i < n; i++) {\n    if (self[i] == '?') {\n      for (int j = 0; j < n; j++) {\n        switch (self[j]) {\n        case '\\n':\n          printf(\"\\\\n\");\n          break;\n        case '\"':\n          printf(\"\\\\\\\"\");\n          break;\n        case '\\\\':\n          printf(\"\\\\\\\\\");\n          break;\n        default:\n          printf(\"%c\", self[j]);\n        }\n      }\n    } else {\n      printf(\"%c\", self[i]);\n    }\n  }\n  return 0;\n}\n') {
      for (int j = 0; j < n; j++) {
        switch (self[j]) {
        case '\n':
          printf("\\n");
          break;
        case '"':
          printf("\\\"");
          break;
        case '\\':
          printf("\\\\");
          break;
        default:
          printf("%c", self[j]);
        }
      }
    } else {
      printf("%c", self[i]);
    }
  }
  return 0;
}
```

很好，现在 `self` 的内容打印没有任何问题了，但是还有一个明显的问题：在判断条件那一行，我们设置的 `?` 本来是作为锚点，标志着什么时候要输出 `self`，结果在这里被错误地展开了。想想也是，我们设置锚点的原则是，我们不会在代码中用到这个字符，然而我们注定要把这个字符写进代码，这不是矛盾了吗？

实则不然。我们注意到我们的原则是不能**直接用到**这个字符，但我们可以间接使用，那就是用 **ASCII** 码。

> `?` 的 ASCII 码是 63。

所以，让我们做最后一次修改，并把修改后的代码重新处理并包含到 `self` 中，得到我们的最终版本代码：

```c
#include <stdio.h>
#include <string.h>

int main() {
  const char *self = "#include <stdio.h>\n#include <string.h>\n\nint main() {\n  const char *self = \"?\";\n  int n = strlen(self);\n  for (int i = 0; i < n; i++) {\n    if (self[i] == 63) {\n      for (int j = 0; j < n; j++) {\n        switch (self[j]) {\n        case '\\n':\n          printf(\"\\\\n\");\n          break;\n        case '\"':\n          printf(\"\\\\\\\"\");\n          break;\n        case '\\\\':\n          printf(\"\\\\\\\\\");\n          break;\n        default:\n          printf(\"%c\", self[j]);\n        }\n      }\n    } else {\n      printf(\"%c\", self[i]);\n    }\n  }\n  return 0;\n}\n";
  int n = strlen(self);
  for (int i = 0; i < n; i++) {
    if (self[i] == 63) {
      for (int j = 0; j < n; j++) {
        switch (self[j]) {
        case '\n':
          printf("\\n");
          break;
        case '"':
          printf("\\\"");
          break;
        case '\\':
          printf("\\\\");
          break;
        default:
          printf("%c", self[j]);
        }
      }
    } else {
      printf("%c", self[i]);
    }
  }
  return 0;
}
```

现在，我们运行，得到如下结果：

```txt
#include <stdio.h>
#include <string.h>

int main() {
  const char *self = "#include <stdio.h>\n#include <string.h>\n\nint main() {\n  const char *self = \"?\";\n  int n = strlen(self);\n  for (int i = 0; i < n; i++) {\n    if (self[i] == 63) {\n      for (int j = 0; j < n; j++) {\n        switch (self[j]) {\n        case '\\n':\n          printf(\"\\\\n\");\n          break;\n        case '\"':\n          printf(\"\\\\\\\"\");\n          break;\n        case '\\\\':\n          printf(\"\\\\\\\\\");\n          break;\n        default:\n          printf(\"%c\", self[j]);\n        }\n      }\n    } else {\n      printf(\"%c\", self[i]);\n    }\n  }\n  return 0;\n}\n";
  int n = strlen(self);
  for (int i = 0; i < n; i++) {
    if (self[i] == 63) {
      for (int j = 0; j < n; j++) {
        switch (self[j]) {
        case '\n':
          printf("\\n");
          break;
        case '"':
          printf("\\\"");
          break;
        case '\\':
          printf("\\\\");
          break;
        default:
          printf("%c", self[j]);
        }
      }
    } else {
      printf("%c", self[i]);
    }
  }
  return 0;
}
```

应该是一模一样的了。我们将输出重定向到文件，再 `diff` 一下：

```bash
gcc quine.c -o quine && ./quine > q.c && diff -u quine.c q.c
```

```bash
$
```

没有任何输出，完全一致！

本文完结~
