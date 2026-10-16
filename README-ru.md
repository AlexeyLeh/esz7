# ESZ7 Блок наземки

## 1. Назначение проекта

Замена блока наземки с датчиками давления и веса.  
Данный проект работает на  Xilinx Zynq 2023.2 Linux ARMv7.  
(Теоретически может работать на любом Linux и Windows)

## 2. esz7-siu

```
ESZ7-SIU Ver. 2.1.1 Oct 16 2025 (built Oct 24 2025 11:28:22)

Usage: esz7-siu  \\\[options] \\\[-d <max\\\_npnts>] \\\[-v] \\\[-h]
Where options are
    --mode <type>       - work mode, where type is Atlas, ADS, SureShot
    --dump <type>       - save obj to files. type: 1..3
    -d <1-5>            - debug printout
    -h                  - help
    -v                  - version

see esz7-siu.cfg to setup work configuration
```

Примеры:

```
$ ./esz7-siu
```

#### Рабочие порты

Порты работы с APS ADS (декодер) и SureShot

* 8220 - ADS команды
* 8110 - данные



* 7111 - SureShot команды
* 8111 - данные



* 7110 - Depth Tracker команды
* 8210 - данные

## 3. Недоделки (текущие особенности)

Пункты меню в SureShot с которыми работает esz7-siu:

#### MWD - MWD Selection

* ADS	- работает нормально
* SUI 1 - не поддерживается
* SIU 2 - подключение обрабатывается, но данные не передаются (формат передаваемых данных неизвестет пока)
* WITS  - подключение обрабытывается, но передаваемые данные игнорируются (нужны доп-исследования)

#### Depth - Depth Selection

* APS SIU2 - работает нормально
* WITS from APS SIU2 - алгоритм работ непонятен на данный момент

#### Wits

* алгоритм работ непонятен. Данные принимает, в окне "SHOW WITS" показывает,
  парсинг "молчит", никаких откликов не наблюдается.


## 4. Варианты сборки

Проект написан в среде CLion.

Сборка выполняется с использованием CMake.

Так как целевое использование программы - это модуль Xilinx XME0724 (ARMv7 Linux),  
то сборка выполняется на Linux машине.  
Также сборку можно выполнять на Windows и запускать в терминале.

* gcc компиляторы на Linux устанавливается так (возможны варианты):

```
sudo apt install -y flex bison
sudo apt install -y make
sudo apt install -y gcc g++
sudo apt install -y gcc-arm-linux-gnueabi   g++-arm-linux-gnueabi   binutils-arm-linux-gnueabi
sudo apt install -y gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf binutils-arm-linux-gnueabihf
```

* gcc-компилятор для Windows устанавливается как-то иначе:

* ```
  ищем в интернете инструкции...
  :)

## 5. Порядок сборки

* CMake
* Makefile

Примечание: Порядок сборки так-же описан в файле **HowToBuild.txt** для cmake и Makefile

#### При использовании CMake

* /usr/bin/cmake
* cmake.exe

1. Подготовка сборки

    `cd /.../esz7-siu/ cmake.exe -S . -B build`

2. Сборка проекта

```
   cmake.exe --build debug -v --target esz7-siu -j4
   ```

3. Очистка проекта

    `cmake.exe --build debug -v --target clean -j4`

#### При использовании Makefile

* /usr/bin/make
* mingw64-make.exe

1. Подготовка сборки

    `$ mkdir build $ cmake -G "MinGW Makefiles" -B build .`

or

    `$ mkdir build $ cd build $ cmake -G "MinGW Makefiles" ..`

2. Сборка проекта

       `$ make -C build`

   or

       `$ cd build $ make`

3. Очистка проекта

       `$ make -C build clean`

## 6. Место, где находится готовая программа

   ```
      $ ls -l build\esz7-siu\*
      ```

