# Lambda for C

> Reference: [A small C compiler By Tsoding](https://www.youtube.com/watch?v=_dVchhnO_KI)

在这个小项目中，我们将以[chibicc](https://github.com/rui314/chibicc)为基础，为C语言增加lambda表达式，当然，考虑到实现的难度，我们这里的lambda表达式不支持变量捕获与闭包。

## 基本思路

考虑到我们不准备实现闭包，所以我们打算将lambda表达式解析成一个函数，然后我们在原来lambda表达式的地方放一个指向那个函数的函数指针即可。

同时，我们引入`lambda`关键字，用于标明对lambda函数的解析。

具体来说，我们的工作是：在词法分析阶段，引入`lambda`关键字，并在语法分析阶段，将其作为一个`primary`对象进行解析，解析定义的函数，并返回一个指向该函数的函数指针的`AST Node`。

## 词法分析

我们在`tokenize.c`中找到定义关键字的数组，然后加入`"lambda"`字段即可。

## 语法分析

我们要在`parse.c`中的`primary`函数中添加对`lambda`语法的支持。

### 准备工作

我们快速了解两个重要的变量`rest`和`tok`。具体而言，`tok`是“当前读取位置”，我们可以移动它来进行语法分析；`rest`是“输出参数”，用于把剩余 token 的起点回传给调用者。我们可以看看`stmt`函数中对`if`语句的解析：

```c
if (equal(tok, "if")) {
    Node *node = new_node(ND_IF, tok);
    tok = skip(tok->next, "(");
    node->cond = expr(&tok, tok);
    tok = skip(tok, ")");
    node->then = stmt(&tok, tok);
    if (equal(tok, "else"))
      node->els = stmt(&tok, tok->next);
    *rest = tok;
    return node;
  }
```

可以看到，最后`tok`变量指向`else`语句之后的token，所以我们用`tok`来赋值`rest`。

所以，我们先操作`tok`，跳过`lambda`关键字。

```c
if(equal(tok, "lambda")){
    tok = tok->next;
}
```

### 解析函数返回值与参数

我们知道，`lambda`之后的代码和正常函数定义唯一的区别在于没有函数名。因此，我们对`lambda`函数体的解析应当和正常的函数解析类似。

翻阅代码，我们可以看到`function`函数，实现了对函数的解析。

```c
static Token *function(Token *tok, Type *basety, VarAttr *attr) {
  Type *ty = declarator(&tok, tok, basety);
  // ...
}
```

这里的`basety`，其实就是函数的返回值类型，我们可以从调用`function`函数的地方看到

```c
static Node *compound_stmt(Token **rest, Token *tok) {
    // ...
    while (!equal(tok, "}")) {
        VarAttr attr = {};
        Type *basety = declspec(&tok, tok, &attr);
        // ...
        if (is_function(tok)) {
        tok = function(tok, basety, &attr);
        continue;
        }
        // ...
    }
    // ...
}
```

而`basety`的解析，则使用了`declspec`函数，翻阅代码，我们发现第三个参数`VarAttr *attr`可以为`NULL`。

于是，我们写出如下代码

```c
if(equal(tok, "lambda")){
    tok = tok->next;
    // parse return type and parameters
    Type* basety = declspec(&tok, tok, NULL);
    Type* ty = declarator(&tok, tok, basety);
}
```

然而，我们尝试逐行 debug 运行时，会在调用`declarator`函数时报错。分析`declarator`函数源码，我们发现：

```c
static Type *declarator(Token **rest, Token *tok, Type *ty) {
  ty = pointers(&tok, tok, ty);

  if (equal(tok, "(")) {
    Token *start = tok;
    Type dummy = {};
    declarator(&tok, start->next, &dummy);
    tok = skip(tok, ")");
    ty = type_suffix(rest, tok, ty);
    return declarator(&tok, start->next, ty);
  }
  // ...
  ty = type_suffix(rest, tok, ty);
  // ...
}
```

我们确认我们的`tok`匹配上了`(`，然而在这里，我们有一个递归过程，这个过程实际是为了处理C语言中的复杂声明。

例如，在解析`int (*f)(int)`时，这里的`tok`一进来就是`(`，所以递归进行`declarator`的解析，解析内容是`*f`，所以完成后`tok`就是`)`，正好可以skip掉，然后我们调用`type_suffix`来解析后缀内容，也就是`(int)`。我们再看函数`type_suffix`，发现它会对后缀的字符做解析，判断是函数`(`还是数组`[`，如果是函数，它会调用`func_params`来解析。翻看`func_params`，我们看到了一些意料之外，情理之中的内容：

```c
static Type *func_params(Token **rest, Token *tok, Type *ty) {
  //...
  while (!equal(tok, ")")) {
    if (cur != &head)
      tok = skip(tok, ",");

    if (equal(tok, "...")) {
      is_variadic = true;
      tok = tok->next;
      skip(tok, ")");
      break;
    }

    Type *ty2 = declspec(&tok, tok, NULL);
    ty2 = declarator(&tok, tok, ty2);
    // ...
}
```

是的，在这个函数内部，我们又递归调用了`declarator`函数来对参数做解析。

也就是说，对于函数的声明，我们使用两个函数`declarator`和`func_params`的双重递归来完成。

例如，对于函数

```c
int print_map_array(int arr[], int n, int(*f)(int));
```

我们用`declarator`解析`print_map_array`，再用`type_suffix`也即是`func_params`来解析函数参数，`func_params`函数解析过程中便会递归调用`declarator`来解析每一个参数，在进行第三个参数解析时，`declarator`会递归调用来解析这一函数指针。

搞清楚了`declarator`函数中递归的作用，那么为什么我们的代码会报错呢？原因是：这里`declarator`遇到`(`后递归，是为了解析函数指针这类复杂声明；而我们这里的`(`是函数参数列表起始位置，它自然无法按预期工作。之所以一上来就遇到`(`，是因为我们的`lambda`函数根本没有名字。

因此，我们在这里不能用`declarator`来解析函数声明。但是从上面的分析中，我们可以发现：我们实际上只需要解析函数参数就行，即，我们已经解析完了`basety`，按照`chibicc`的工作流程，我们只需要用`type_suffix`来解析后缀就行。而我们这里要求后缀一定是一个`(`，因此，我们只需要跳过`(`并调用`func_params`即可。

```c
if(equal(tok, "lambda")){
    tok = tok->next;
    Type *basety = declspec(&tok, tok, NULL);
    Type *ty = func_params(&tok, skip(tok, "("), basety);
}
```

### 给匿名函数名字

在解析函数体之前，我们还要注意一个问题，尽管我们的`lambda`是匿名函数，但是在后期生成函数对象时，我们必须赋予名字，否则链接和调用都会出现问题。

为了解决这个问题，我们使用计数器生成形如`_lambda_id`的名字。当然，它不必是全局变量，声明为静态变量即可。

```c
// create a unique name for the lambda function
static size_t lambda_count = 0;
int len = snprintf(NULL, 0, "_lambda_%zu", lambda_count);
char* name = malloc(len + 1); // don't care heap memory
snprintf(name, len + 1, "_lambda_%zu", lambda_count++);
```

> 这里有一个小技巧：第一次调用`snprintf`函数时，我们传入`NULL, 0`，虽然不会写入内容，但`snprintf`的返回值会告诉我们目标字符串格式化后的长度。

### 解析函数体


`function`函数内部已经实现了对函数体的解析，如下

```c
static Token *function(Token *tok, Type *basety, VarAttr *attr) {
  // ...

  Obj *fn = find_func(name_str);
  if (fn) {
    // Redeclaration
    if (!fn->is_function)
      error_tok(tok, "redeclared as a different kind of symbol");
    if (fn->is_definition && equal(tok, "{"))
      error_tok(tok, "redefinition of %s", name_str);
    if (!fn->is_static && attr->is_static)
      error_tok(tok, "static declaration follows a non-static declaration");
    fn->is_definition = fn->is_definition || equal(tok, "{");
  } else {
    fn = new_gvar(name_str, ty);
    fn->is_function = true;
    fn->is_definition = equal(tok, "{");
    fn->is_static = attr->is_static || (attr->is_inline && !attr->is_extern);
    fn->is_inline = attr->is_inline;
  }

  fn->is_root = !(fn->is_static && fn->is_inline);

  if (consume(&tok, tok, ";"))
    return tok;

  current_fn = fn;
  locals = NULL;
  enter_scope();
  create_param_lvars(ty->params);

  // A buffer for a struct/union return value is passed
  // as the hidden first parameter.
  Type *rty = ty->return_ty;
  if ((rty->kind == TY_STRUCT || rty->kind == TY_UNION) && rty->size > 16)
    new_lvar("", pointer_to(rty));

  fn->params = locals;

  if (ty->is_variadic)
    fn->va_area = new_lvar("__va_area__", array_of(ty_char, 136));
  fn->alloca_bottom = new_lvar("__alloca_size__", pointer_to(ty_char));

  tok = skip(tok, "{");

  // [https://www.sigbus.info/n1570#6.4.2.2p1] "__func__" is
  // automatically defined as a local variable containing the
  // current function name.
  push_scope("__func__")->var =
    new_string_literal(fn->name, array_of(ty_char, strlen(fn->name) + 1));

  // [GNU] __FUNCTION__ is yet another name of __func__.
  push_scope("__FUNCTION__")->var =
    new_string_literal(fn->name, array_of(ty_char, strlen(fn->name) + 1));

  fn->body = compound_stmt(&tok, tok);
  fn->locals = locals;
  leave_scope();
  resolve_goto_labels();
  return tok;
}
```

可以看到，它进行了对函数是否重定义、是否有定义、静态、内联的检查，我们不需要做这些，直接按照`lambda`函数的实际情况来实现。

当然，在这里我们需要注意一个问题：源代码中我们修改的`current_fn`和`locals`是全局变量。在标准 C 中，不存在函数内再定义函数的语法，因此在普通函数解析时，把这两个做成全局变量是可行的。但是我们的`lambda`函数会出现在函数内部，如果直接覆盖这两个全局变量，会导致解析完`lambda`后，外层函数的解析状态出错。

所以，我们需要提前保存这两个全局变量，在`lambda`解析完成后恢复。~~正如我们在汇编时保存寄存器那样~~

```c
if (equal(tok, "lambda")) {
    tok = tok->next;
    // parse return type and parameters
    Type *basety = declspec(&tok, tok, NULL);
    Type *ty = func_params(&tok, skip(tok, "("), basety);
    // create a unique name for the lambda function
    static size_t lambda_count = 0;
    int len = snprintf(NULL, 0, "_lambda_%zu", lambda_count);
    char* name = malloc(len + 1); // don't care heap memory
    snprintf(name, len + 1, "_lambda_%zu", lambda_count++);
    // parse function body
    Obj *fn = new_gvar(name, ty);
    fn->is_function = true;
    fn->is_definition = true;
    fn->is_static = false;
    fn->is_inline = false;
    fn->is_root = true;

    Obj *saved_current_fn = current_fn;
    Obj *saved_locals = locals;
    
    current_fn = fn;
    locals = NULL;
    enter_scope();
    create_param_lvars(ty->params);

    // A buffer for a struct/union return value is passed
    // as the hidden first parameter.
    Type *rty = ty->return_ty;
    if ((rty->kind == TY_STRUCT || rty->kind == TY_UNION) && rty->size > 16)
      new_lvar("", pointer_to(rty));

    fn->params = locals;

    if (ty->is_variadic)
      fn->va_area = new_lvar("__va_area__", array_of(ty_char, 136));
    fn->alloca_bottom = new_lvar("__alloca_size__", pointer_to(ty_char));

    tok = skip(tok, "{");

    // [https://www.sigbus.info/n1570#6.4.2.2p1] "__func__" is
    // automatically defined as a local variable containing the
    // current function name.
    push_scope("__func__")->var =
        new_string_literal(fn->name, array_of(ty_char, strlen(fn->name) + 1));

    // [GNU] __FUNCTION__ is yet another name of __func__.
    push_scope("__FUNCTION__")->var =
        new_string_literal(fn->name, array_of(ty_char, strlen(fn->name) + 1));

    fn->body = compound_stmt(&tok, tok);
    fn->locals = locals;
    leave_scope();
    resolve_goto_labels();

    current_fn = saved_current_fn;
    locals = saved_locals;
}
```

### 返回AST Node

此时，我们已经完成了对`lambda`函数体的解析，只需要返回一个指向该函数的函数指针 AST Node 即可。翻阅`primary`的代码，我们很快就能找到根据标识符 token 创建 Node 的代码：

```c
if (tok->kind == TK_IDENT) {
    // Variable or enum constant
    VarScope *sc = find_var(tok);
    *rest = tok->next;

    // For "static inline" function
    if (sc && sc->var && sc->var->is_function) {
      if (current_fn)
        strarray_push(&current_fn->refs, sc->var->name);
      else
        sc->var->is_root = true;
    }

    if (sc) {
      if (sc->var)
        return new_var_node(sc->var, tok);
      if (sc->enum_ty)
        return new_num(sc->enum_val, tok);
    }

    if (equal(tok->next, "("))
      error_tok(tok, "implicit declaration of a function");
    error_tok(tok, "undefined variable");
  }
```

这里我们调用`find_var`来找到变量对应的作用域。它的原理是根据`tok`中的标识符名字来搜索；然而问题在于，我们这里的`tok`不可能包含该标识符名字，因为这个名字是额外生成的。

因此，我们不如直接改写函数逻辑，直接按照标识符名字来搜索，不必依赖token。我们基于`find_var`函数来修改。

```c
// Find a variable by c_str name.
static VarScope *find_var_c_str(char *name) {
  int len = strlen(name);
  for (Scope *sc = scope; sc; sc = sc->next) {
    VarScope *sc2 = hashmap_get2(&sc->vars, name, len);
    if (sc2)
      return sc2;
  }
  return NULL;
}
```

所以我们生成Node的方法如下：

```c
VarScope *sc = find_var_c_str(name);
Node* node = new_var_node(sc->var, start);
```

然而，我们还需要把它转成函数指针，翻阅代码中有关取地址符`&`的解析，我们发现要调用`new_unary`函数，并传入`ND_ADDR`参数。

因此，我们最终的返回代码如下

```c
VarScope *sc = find_var_c_str(name);
return new_unary(ND_ADDR, new_var_node(sc->var, start), start);
```

### 最终代码

当然，别忘了我们还要设置`rest`的值。解析完`lambda`之后，`tok`指向`}`之后的内容，直接用`tok`设置即可。

```c
*rest = tok;
```

完整代码可见[Github](https://github.com/dnamw/chibicc_with_lambda)

先`make`，然后取消`demo.c`里的注释，再运行

```bash
./chibicc -o demo demo.c && ./demo
```

来确认工作正常。

## The End

再次感谢[chibicc](https://github.com/rui314/chibicc)项目，毫无疑问是一个优秀的、简易而不简单的C语言编译器。

这个项目的代码十分优秀，使得我们不需要了解具体细节就可以完成功能的实现。这体现了代码的可扩展性的重要性。相比于扩展功能本身，学习其代码也是收益颇丰。

在给现有项目添加功能时，阅读代码，熟悉接口，遵循其用法便是最佳实践。当然，在有LLM加持的时代，这些功能可以变得更加简单，更加便利。
