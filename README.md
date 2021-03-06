# Matrix transpose tool

## Usage 
```
Usage: tool <src> <dst>

<src> - file with matrix in strict text format:
3 45 5\n
24 5 4\n
4 55 3

<dst> - file for transposed matrix:
3                    24                   4\n
45                   5                    55\n
5                    4                    3\n

"\n" is LF (0x0A)
```

## Description

Утилита принимает на вход матрицу в текстовом, но строгом формате (LF в качестве разделителя строк, последняя строка без переноса).

Результатом является файл с транспонированной матрицей в том же формате, однако с фиксированным размером элементов (моноширинность нужна для записи по offset-ам из независимых потоков).

## Notes

Т.к. по условию формат не бинарный, а размерность нужно определять в процессе работы, плюс валидировать на ошибки, процесс парсинга формата убивает все намёки на производительность. Поэтому многопоточная обработка не дает в данных условиях никакого выигрыша, сделана только из-за требования реализовать многопоточность.

Сама задача транспонирования матрицы имеет большой потенциал для оптимизаций, но только при работе с бинарным видом данных и определенной заданной заранее размерностью.

Утилита работает с потоком данных, не храня копию в памяти (чтобы не упереться в потолок по доступной памяти). std::bad_alloc от мелких операций std::string не ловится, предполагается что в системе не настолько мало памяти.