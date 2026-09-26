---
title: Стандартная библиотека
description: result, stdio, math, string и os.
---

Каждая функция в этих модулях — интринсик компилятора: тело, написанное в исходнике `.agn`, существует только для того, чтобы файл проходил проверку типов и чтобы человек, читающий его, видел сигнатуру. Реальное поведение генерируется напрямую бэкендом компилятора, а не выполнением этого тела.

## result

`stdlib/result.agn` объявляет `Result<T,E>` (`ok bool`, `value T`, `err E`) и `Option<T>` (`some bool`, `value T`). В отличие от всех остальных модулей здесь, эти две обобщённые структуры всегда в области видимости — `import "result"` не нужен, а их поля — настоящие данные, а не тело, генерируемое компилятором: компилятор парсит этот файл напрямую и вливает его объявления в каждую компилируемую программу. Как работает инстанцирование (`Result<int,string>`) и монолиформизация — см. [Обобщённые структуры](/ru/structs/#обобщённые-структуры-дженерики).

## stdio

| Функция | Сигнатура | Примечания |
|---|---|---|
| `Print` | `(value int)` | пишет десятичное целое, без перевода строки |
| `Println` | `(value int)` | пишет десятичное целое и перевод строки |
| `PrintStr` | `(text string)` | пишет строку, без перевода строки |
| `PrintlnStr` | `(text string)` | пишет строку и перевод строки |
| `PrintBool` | `(value bool)` | пишет `true` или `false`, без перевода строки |
| `PrintlnBool` | `(value bool)` | пишет `true` или `false` и перевод строки |
| `PrintChar` | `(ch int)` | пишет один байт |
| `ReadInt` | `() int` | читает десятичное целое из stdin |
| `ReadChar` | `() int` | читает один байт из stdin |
| `ReadLine` | `(buffer *u8, maxlen int) int` | читает строку в `buffer`, возвращает число байт |
| `Flush` | `()` | ничего не делает; вывод небуферизован |

`Print` и `Println` принимают только `int`; передача `string` или `bool` является ошибкой типов, используйте `PrintStr`/`PrintlnStr` или `PrintBool`/`PrintlnBool`.

## math

Все функции принимают `int`; большинство и возвращают `int`, кроме `IsEven`/`IsOdd`/`IsPrime`, которые возвращают `bool`.

| Функция | Сигнатура | Примечания |
|---|---|---|
| `Max` | `(a int, b int) int` | |
| `Min` | `(a int, b int) int` | |
| `Pow` | `(base int, exp int) int` | |
| `Sqrt` | `(n int) int` | целочисленный квадратный корень, метод Ньютона, только положительный вход |
| `GCD` | `(a int, b int) int` | алгоритм Евклида, только положительный вход |
| `LCM` | `(a int, b int) int` | только положительный вход |
| `Fact` | `(n int) int` | факториал |
| `IsEven` | `(n int) bool` | |
| `IsOdd` | `(n int) bool` | |
| `Sign` | `(x int) int` | 1, если `x > 0`, иначе 0; отрицательные значения не отличаются от нуля |
| `Clamp` | `(value int, min int, max int) int` | |
| `SumRange` | `(n int) int` | сумма `1..n` |
| `IsPrime` | `(n int) bool` | пробное деление |
| `Fib` | `(n int) int` | n-е число Фибоначчи, итеративно |

## string

| Функция | Сигнатура | Примечания |
|---|---|---|
| `len` | `(s string) int` | длина в байтах |
| `compare` | `(s1 string, s2 string) int` | 0, если равны, -1, если `s1 < s2`, 1, если `s1 > s2` |
| `concat` | `(s1 string, s2 string) string` | |
| `is_empty` | `(s string) bool` | |
| `indexOf` | `(s string, sub string) Option<int>` | индекс первого вхождения `sub`; `some=false`, если не найдено |
| `contains` | `(s string, sub string) bool` | |
| `startsWith` | `(s string, prefix string) bool` | |
| `endsWith` | `(s string, suffix string) bool` | |
| `charAt` | `(s string, index int) Option<int>` | код байта по `index`; `some=false`, если вне границ |
| `substr` | `(s string, start int, len int) string` | обрезается по границам строки |
| `toUpper` | `(s string) string` | только ASCII |
| `toLower` | `(s string) string` | только ASCII |

`++` и интерполяция `$(...)` (см. [Синтаксис](/ru/syntax/)) для простых случаев покрывают то же самое, что и `concat`.

## os

Файловые дескрипторы и аргументы командной строки.

| Функция | Сигнатура | Примечания |
|---|---|---|
| `ArgCount` | `() int` | число аргументов командной строки, включая argv[0] (путь к программе) |
| `Arg` | `(index int) string` | аргумент по `index` |
| `OpenRead` | `(path string) Option<int>` | value — файловый дескриптор; `some=false` при ошибке |
| `OpenCreate` | `(path string) Option<int>` | создать/обрезать для записи; value — дескриптор, `some=false` при ошибке |
| `Close` | `(fd int) int` | |
| `ReadFd` | `(fd int, buffer *u8, maxlen int) Option<int>` | читает в `buffer`, как `stdio.ReadLine`; value — число прочитанных байт, `some=false` при ошибке |
| `WriteFd` | `(fd int, data string) Option<int>` | value — число записанных байт, `some=false` при ошибке |
| `Exit` | `(code int)` | немедленно завершает процесс, не выполняя оставшийся код вызывающей функции |

Для массива `buf` типа `[N]u8` передавайте `&buf` в параметр `buffer` у `ReadFd` и `stdio.ReadLine`. См. [Указатели](/ru/types/#указатели).

`OpenRead`, `OpenCreate`, `ReadFd` и `WriteFd` возвращают `Option<int>` (см. [Обобщённые структуры](/ru/structs/#обобщённые-структуры-дженерики)) вместо сырого сентинела: `some` равен `true` при успехе и `false` при ошибке, а `value` содержит дескриптор или число байт только когда `some` равен `true` — проверяйте `some` перед тем, как доверять `value`.
