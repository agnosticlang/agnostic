---
title: Структуры
description: Поля, методы и литералы структур.
---

## Объявление

```agn
struct Vector2 {
    x i64,
    y i64
}
```

Поля — это пары `имя тип` через запятую. Наследования и таблиц виртуальных методов нет: структура не может расширять другую структуру, а методы разрешаются статически на этапе компиляции, а не через таблицу диспетчеризации.

## Литералы структур

```agn
var a Vector2 = Vector2{x: 3, y: 4}
```

В литерале каждое поле должно быть названо (`field: value`); позиционной формы нет.

## Методы

Метод — это функция с получателем (receiver) в скобках перед именем:

```agn
func (v Vector2) Length2() i64 {
    return v.x * v.x + v.y * v.y
}

func (v Vector2) Add(other Vector2) Vector2 {
    return Vector2{x: v.x + other.x, y: v.y + other.y}
}
```

Вызов через точку:

```agn
var a Vector2 = Vector2{x: 3, y: 4}
stdio.Println(a.Length2())   // 25
```

Получатель передаётся по ссылке (скрытым указателем на переменную вызывающей стороны), а не копируется. Присваивание полю получателя внутри метода меняет структуру самой вызывающей стороны:

```agn
func (v Vector2) SetX(newX i64) {
    v.x = newX
}

var a Vector2 = Vector2{x: 3, y: 4}
a.SetX(99)
stdio.Println(a.x)   // 99
```

## Поля функционального типа

Поле структуры может хранить значение-функцию:

```agn
struct Platform {
    write func(i64, i64, i64) -> i64
}

var platform Platform
platform.write = linuxWrite
stdio.Println(platform.write(1, 0, 4))
```

Это работает только под `--backend=llvm`, по той же причине, по которой под `--backend=nvm` не работают обычные замыкания: см. [Функции и замыкания](/ru/functions/).

## Доступ к полям и присваивание

```agn
var a Vector2 = Vector2{x: 3, y: 4}
a.x = 10
stdio.Println(a.x)
```
