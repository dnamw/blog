# Variable Capture

> 上回: [Simple Lambda](./simple_lambda.md)

上回，我们在参考[Tsoding](https://www.youtube.com/watch?v=_dVchhnO_KI)后，实现了简单的lambda表达式，但是他也没做变量捕获。

所以，这一次，我们来做简单的变量捕获。

我们不打算使用类似C#中的闭包对象来实现，而是采用一种朴素的方案：通过全局变量来支持值捕获，不考虑运行时环境的问题。

## 基本思路

我们不引入额外的运行时结构，而是直接在编译期支持变量捕获。

具体来说，我们做三件事：

1. 为每一个被捕获的外层变量，分配一个匿名全局变量。
2. 在 lambda 表达式解析时，将所有外层变量替换为对应的匿名全局变量。
3. 在 lambda 表达式的位置，使用逗号表达式，先把外层变量的值拷贝到对应的匿名全局变量，再返回匿名函数的指针。

整体效果会像这样：

```c
(anon_gvar1 = x, anon_gvar2 = y, &lambda_fn)
```

这样一来，lambda 函数执行时，实际使用的是匿名全局变量，从而实现值捕获。

## 捕获变量链表

为了记录“原变量”和“匿名全局变量”之间的映射，我们在 `parse.c` 里新增了一个简单的链表节点：

```c
typedef struct CapturedVar CapturedVar;
struct CapturedVar {
  CapturedVar *next;
  Obj *src;
  Obj *dst;
};
```

这里的含义很直接：

- `src` 表示 lambda 体内引用到的外层变量
- `dst` 表示我们为它生成的匿名全局变量

同时，我们还增加了两个解析期状态：

```c
static bool lambda_capture_enabled;
static CapturedVar *lambda_captures;
```

`lambda_capture_enabled` 用来标记当前是否处在 lambda 的解析上下文中，`lambda_captures` 则保存当前 lambda 的捕获列表。

## 何时触发捕获

真正的触发点，放在 `primary` 函数里对标识符的解析逻辑中。

当我们读到一个变量后，如果它满足下面这些条件：

1. 它是一个局部变量
2. 它不属于当前正在解析的函数
3. 当前确实处在 lambda 解析上下文

那么就不能直接返回这个变量，而要执行变量捕获，再返回我们生成的匿名全局变量。

对应的逻辑如下：

```c
static Node *primary(Token **rest, Token *tok) {
  // ...
  if(tok->kind == TK_IDENT){
    if(sc){
      if (sc->var && sc->var->is_local && !is_local_in_current_fn(sc->var)) {
      if (lambda_capture_enabled)
        return new_var_node(capture_lambda_var(sc->var, tok), tok);
      }
    }
    // ...
  }
  // ...
}
```

这里最关键的判断，是“它是否属于当前函数”。因为我们的 lambda 是嵌套在普通函数内部的，外层局部变量和当前 lambda 自己的局部变量必须严格区分开。

其实现如下

```c
static bool is_local_in_current_fn(Obj *var) {
  for (Obj *v = locals; v; v = v->next)
    if (v == var)
      return true;
  return false;
}
```

## 如何捕获变量

这一步大致分成：

1. 先查重，保证同一个源变量只捕获一次
2. 做类型检查，我们不支持数组、VLA、函数等类型
3. 通过 `new_anon_gvar` 创建匿名全局变量
4. 创建 `CaptureVar` 节点 `src -> dst`
5. 将其添加到链表

```c
static Obj *capture_lambda_var(Obj *var, Token *tok) {
  for (CapturedVar *cap = lambda_captures; cap; cap = cap->next)
    if (cap->src == var)
      return cap->dst;

  if (var->ty->kind == TY_ARRAY || var->ty->kind == TY_VLA || var->ty->kind == TY_FUNC)
    error_tok(tok, "lambda capture by value does not support this type");

  Obj *slot = new_anon_gvar(var->ty);

  CapturedVar *cap = calloc(1, sizeof(CapturedVar));
  cap->src = var;
  cap->dst = slot;
  cap->next = lambda_captures;
  lambda_captures = cap;

  return slot;
}
```

## 在 lambda 位置赋值

当 lambda 函数体解析完成后，真正有意思的部分才开始。

我们原本希望 lambda 表达式返回的是 `&lambda_fn`，但是在这之前，还需要先把捕获变量的值拷贝到匿名全局变量。这一步我们使用逗号表达式来完成。

??? note "使用逗号表达式的灵感"
    来源于 chibicc 在对 `VLA` 做 `sizeof` 的处理：

    ```c
    static Node *primary(Token **rest, Token *tok) {
      // ...
      if (equal(tok, "sizeof") && equal(tok->next, "(") && is_typename(tok->next->next)) {
        Type *ty = typename(&tok, tok->next->next);
        *rest = skip(tok, ")");  

        if (ty->kind == TY_VLA) {  
          if (ty->vla_size)
            return new_var_node(ty->vla_size, tok);
    
          Node *lhs = compute_vla_size(ty, tok);
          Node *rhs = new_var_node(ty->vla_size, tok);
          return new_binary(ND_COMMA, lhs, rhs, tok);
        }
    
        return new_ulong(ty->size, start);
      }
      // ...
    }
    ```

当然，和前一篇一样，这里仍然要注意全局lambda解析状态的保存与恢复。
那么 lambda 解析部分修改代码如下：

```c
static Node *primary(Token **rest, Token *tok) {
  // ...
  if (equal(tok, "lambda")) {
    // ...

    CapturedVar *saved_lambda_captures = lambda_captures;
    lambda_captures = NULL;
    lambda_capture_enabled = true;

    fn->body = compound_stmt(&tok, tok);
    fn->locals = locals;

    CapturedVar *captures = lambda_captures;
    lambda_captures = saved_lambda_captures;
    lambda_capture_enabled = false;

    // ...

    VarScope *sc = find_var_c_str(name);
    Node *node = new_unary(ND_ADDR, new_var_node(sc->var, start), start);

    for (CapturedVar *cap = captures; cap; cap = cap->next) {
      Node *lhs = new_var_node(cap->dst, start);
      Node *rhs = new_var_node(cap->src, start);
      Node *assign_stmt = new_binary(ND_ASSIGN, lhs, rhs, start);
      node = new_binary(ND_COMMA, assign_stmt, node, start);
    }

    *rest = tok;
    return node;
  }
  // ...
}
```

## 最终代码与示例

完整代码可见[Github](https://github.com/dnamw/chibicc_with_lambda)

这次提交里的 demo，大致是下面这样的：

```c
int m = 1;
print_map_array(arr, n, lambda int(x) {
  m = 2;
  return x + m;
});
printf("after lambda: %d\n", m);
```

由于这里做的是值捕获，lambda 内部拿到的是定义点的值副本，而不是外层变量本身。所以，lambda 里对 `m` 的修改不会直接影响外部的 `m`。

运行

```bash
make && ./chibicc -o demo/demo demo/demo.c && ./demo
```

来确认工作正常。

## The End

再次感谢[chibicc](https://github.com/rui314/chibicc)项目，毫无疑问是一个优秀的、简易而不简单的C语言编译器。

这个项目的代码十分优秀，使得我们不需要了解具体细节就可以完成功能的实现。这体现了代码的可扩展性的重要性。相比于扩展功能本身，学习其代码也是收益颇丰。

在给现有项目添加功能时，阅读代码，熟悉接口，遵循其用法便是最佳实践。当然，在有LLM加持的时代，这些功能可以变得更加简单，更加便利。
