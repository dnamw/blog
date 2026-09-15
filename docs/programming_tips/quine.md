# Quine

> Quine 是一类计算机程序，运行后会输出自身的源代码。

## 为什么会存在这样的程序

自我创造是一件相当反直觉的事。在现实生活中，创造某物通常需要借助外力。例如，游戏由人创造，它不可能自行开发自身。然而生命却十分神奇：细胞可以一分为二，生命能够自我复制。那么，人类为什么想写一个能打印自身源码的程序呢？或许也源自某种生物本能吧 :)

不过先别急着动手。我们首先要知道：理论上是否存在这样的程序？为此，需要借助可计算理论，快速了解一些基本概念。

### 可计算理论

在可计算理论中，我们把计算机程序所计算的函数称为**可计算函数**。例如，简单的 \(f(x) = x + 1\) 就是一个可计算函数；它对任意输入 \(x\) 都有输出，这样的函数称为**全函数**。而有些函数在某些输入上没有结果（例如程序死循环或出错），这类函数称为**部分函数**。

对计算机程序而言，一切输入输出本质上都可以编码为整数，而程序本身也不过是一串整数。因此，我们可以给每个程序分配一个编号，并用编号代表程序。用 \(\phi_n\) 表示编号为 \(n\) 的程序，那么 \(\phi_n(x)\) 就是程序 \(n\) 在输入 \(x\) 下的输出。

接下来看两个重要定理。

#### 普适性定理

存在一个**通用程序** \(u\)，使得

\[
\phi_u(n, x) = \phi_n(x)
\]

也就是说，把程序编号 \(n\) 和输入 \(x\) 一起交给通用程序 \(u\)，它就能模拟程序 \(n\) 在输入 \(x\) 上的运行结果。

这个通用程序是不是很像现代计算机中的解释器？

#### s-m-n 定理（参数化定理）

如果 \(\phi_n(y, x)\) 是一个接受两个参数的程序，那么对于任意固定的 \(y\)，我们可以**自动构造**一个新程序：它只接受一个参数 \(x\)，行为等同于把 \(y\) 固定后的结果。也就是说，存在可计算函数 \(s(n, y)\)，使得

\[
\phi_{s(n, y)}(x) = \phi_n(y, x)
\]

这其实就是编程里的**柯里化**（Currying）。

了解这两个定理后，再来看最重要的不动点定理。它与 Quine 程序密切相关。

#### 不动点定理

设 \(h: \mathbb{N} \to \mathbb{N}\) 是一个可计算的全函数，那么存在一个自然数 \(n\)，使得

\[
\phi_n = \phi_{h(n)}
\]

即程序 \(n\) 和程序 \(h(n)\) 计算同一个函数。

简证：
对任意 \(t\)，考虑 \(\phi_t(t)\)，也就是以程序自身作为输入。根据 **s-m-n 定理**，我们可以构造一个变换 \(s\)，使得 \(\phi_{s(t,t)} = \phi_t(t)\)。

接着，将变换 \(h\) 作用于 \(s(t,t)\)，得到 \(h(s(t,t))\)。

现在考虑 \(\phi_{h(s(t,t))}\)。它本身不接受输入，但其程序编号是以 \(t\) 为输入、经过 \(s\) 和 \(h\) 变换得到的。对于这样一个程序，根据普适性定理，并结合 \(s\) 和 \(h\) 的可计算性，可以找到一个程序 \(m\)，满足 \(\phi_m(t) = \phi_{h(s(t,t))}\)。现在断言

\[
n = s(m,m)
\]

就是所需的不动点。
验证：

\[
\phi_n = \phi_{s(m,m)} = \phi_m(m)
\]

\[
\phi_{h(n)} = \phi_{h(s(m,m))} = \phi_m(m)
\]

证毕。

#### Quine 的合理性

现在假设 \(h(t)\) 是一个打印 \(t\) 的源代码的程序。那么由不动点定理，存在程序 \(n\) 是 \(h\) 的不动点，因此程序 \(n\) 会打印自身的源代码。于是，我们就得到了 **Quine** 程序。

## 用 C 语言来写这样一个程序

> Reference: [Self-Reproducing Programs By Tsoding](https://www.youtube.com/watch?v=QGm-d5Ch5JM)

### 初试牛刀

一个简单却不容易想到的思路是：用一个字符串保存源代码，然后输出这个字符串。

> 正如上述证明的开头那样，程序必须把自身包含在自身之中。这听起来很奇怪，但直觉上应该是可行的。

```c
#include <stdio.h>

int main() {
  const char *self = "";
  printf("%s", self);
  return 0;
}
```

当然，这段程序只会输出空白，因为 `self` 中没有任何内容。

现在，手动让 `self` 包含程序源代码。但不能直接复制，因为需要处理换行符和双引号：把换行符替换为 `\n`，把双引号替换为 `\"`。

```c
#include <stdio.h>

int main() {
  const char *self = "#include <stdio.h>\n\nint main() {\n  const char *self = \"\";\n  printf(\"%s\", self);\n  return 0;\n}\n";
  printf("%s", self);
  return 0;
}
```

### 包含自己？

运行这段代码，得到如下输出：

```txt
#include <stdio.h>

int main() {
  const char *self = "";
  printf("%s", self);
  return 0;
}
```

输出是有了，但问题在于 `self` 的内容本身并没有被打印出来。一个常见的想法是修改 `self` 包含的内容，但这显然会导致无限递归。

因此，让我们重新审视 `self` 变量。正因为 `self` 是一个变量，我们才有更多操作空间：变量可以被反复使用，也可以被反复打印。

思路如下：在 `self` 中设置一个 **锚点**（anchor），并确保代码中不会直接使用这个字符。然后逐字符打印 `self`；如果遇到锚点，就说明需要再次打印整个 `self`。这里不妨取锚点为 `?`。

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

再让 `self` 重新包含源代码，就得到第二版程序：

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

现在运行第二版程序：

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

不错，这次 `self` 可以输出自身包含的内容了，但仍有问题。一个明显的问题是：`self` 中写成 `\n`、`\"` 的片段在 C 字符串里已经被解释为换行和双引号，因此直接打印时只会得到实际换行和双引号，而源代码中需要的反斜杠却丢失了。

因此，在输出 `self` 时，需要对其中的转义字符做特殊处理。也就是说，要再遍历一次 `self`，专门处理转义字符。

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

当然，这又引出一个新问题：源代码中使用了 `\\`，它在 `self` 中会被转义成单个 `\`，所以还需要特殊处理：

```c
case '\\':
  printf("\\\\");
  break;
```

整合上述修改，再让 `self` 包含源代码。注意，此时 `\` 也要处理成 `\\`，并且要放在 `\n` 和 `"` 之前处理。于是得到第三版程序：

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

现在运行第三版程序：

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

很好，现在 `self` 的内容打印已经没问题了，但还有一个明显的问题：在判断条件那一行，我们设置的 `?` 原本是作为锚点，用来标记何时输出 `self`，结果这里也被错误地展开了。细想之下也合理：锚点的原则是代码中不直接使用该字符，但我们又必须把这个字符写进代码，这岂不是矛盾？

其实并不矛盾。原则是不**直接使用**这个字符，但可以间接使用它，例如使用它的 **ASCII** 码。

> `?` 的 ASCII 码是 63。

因此，做最后一次修改：把判断条件中的 `'?'` 改为 `63`，再把修改后的代码重新转义并放入 `self`，就得到最终版本：

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

现在运行程序，得到如下结果：

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

应当已经完全一致。将输出重定向到文件，再用 `diff` 比较：

```bash
gcc quine.c -o quine && ./quine > q.c && diff -u quine.c q.c
```

```bash
$
```

没有任何输出，完全一致！

本文完。
