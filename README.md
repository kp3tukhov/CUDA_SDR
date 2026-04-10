# CUDA SDR — Обработка сигналов GPS

Проект **GPU-ускоренного SDR-приёмника** для обработки сигналов GPS L1 C/A. Написан на **C** и **CUDA**, реализует пайплайн захвата GPS-сигнала: генерация синтетического сигнала для тестирования, корреляция и захват спутников. Источник данных, на данный момент, только бинарный файл записанного сигнала

Планируется развитие функционала приёмника: модули трекинга и демодуляции, получение данных с USB SDR устройства

## Особенности

- Генерация синтетических сигналов GPS L1 C/A с произвольными настройками спутников
- Три метода корреляции: временная область, параллельно по частоте (XF), параллельно по коду (FX)
- Аппаратное ускорение корреляции с помощью CUDA
- Захват спутников с обнаружением пиков, уточнением допплеровского сдвига параболической интерполяцией и оценкой C/N0
- Поддержка форматов I-only и IQ данных (в настоящее время поддерживаются только uint8 значения)
- Настраиваемые параметры СВЧ-тракта приёмника

## Структура проекта

```
CUDA_SDR/
├── app/                  # Исходный код приложений
│   ├── gen_sig.c         # Генератор тестовых сигналов
│   ├── corr_sig.c        # Коррелятор сигналов
│   └── acq_sig.c         # Полноценный модуль захвата
├── src/                  # Основные библиотеки
│   ├── core/             # Утилиты, типы, параметры, инициализация GPU
│   ├── dsp/              # Генерация PRN-кодов, операции над комплекснозначным сигналом (CPU + CUDA)
│   ├── correlator/       # Интерфейс и функции корреляции
│   ├── acquisition/      # Обнаружение пиков, оценка C/N0
│   └── io/               # Ввод/вывод, парсинг конфигурации
├── build/                # Директория сборки CMake
├── CMakeLists.txt        # Конфигурация сборки
├── recv.cfg              # Конфигурация CВЧ-тракта приёмника
├── testsig.cfg           # Конфигурация спутников для тестовых сигналов
└── Plots.ipynb           # Jupyter Notebook для визуализации карты корреляции
```

## Зависимости

- **CMake** >= 3.16
- **CUDA: NVHPC**
- **FFTW3** (одинарная точность: `libfftw3f`)
- GCC с поддержкой C11
- GPU NVIDIA с поддержкой CUDA

## Сборка

В ОС Linux


```bash
mkdir -p build && cd build
cmake ..
make
```

Исполняемые файлы создаются в **корневой директории проекта**: `gen_sig`, `corr_sig`, `acq_sig`.

## CLI-утилиты

### 1. Генератор сигналов (`gen_sig`)

Генерирует синтетические сигналы GPS L1 C/A с настраиваемыми параметрами спутников.

```bash
$ ./gen_sig --help
GPS L1 C/A Synthetic Signal Generator (Multi-Satellite)

Usage: ./gen_sig -f <config> -d <duration_ms> -o <output> [-m <mode>]

Required arguments:
  -f, --config <file>      Path to the satellite configuration file.
  -d, --duration <ms>      Signal duration in milliseconds (integer).
  -o, --output <file>      Path to the output binary file.

Optional arguments:
  -m, --mode <mode>        Output mode: 'i' (I-channel only, default) or 'iq'.
  -h, --help               Show this help message.

Config file format (space-separated, one satellite per line):
  PRN  DOPPLER_HZ  CODE_PHASE_CHIPS  CARRIER_PHASE_RAD  CN0_DB_HZ
  Lines starting with '#' are treated as comments.

Example:
  ./gen_sig -f testsig.cfg -d 100 -o test_iq.bin -m iq

```

### 2. Коррелятор сигналов (`corr_sig`)

Выполняет корреляцию записанных сигналов с локальными PRN-кодами GPS L1CA, записывает в файл полученную карту корреляции.

```bash
$ ./corr_sig --help
GPS Signal Correlator - Correlation Map Generator

Usage: ./corr_sig -f <input_file> --prn <num> [options] -o <output_file>

Required arguments:
  -f, --file <path>       Input binary file (int8_t samples).
  --prn <num>             PRN number of the satellite (1-37).
  -o, --output <path>     Output text file for 3D plot data.

Optional arguments:
  --dop_min <freq>        Minimum Doppler search frequency in Hz. (Default: -5000)
  --dop_max <freq>        Maximum Doppler search frequency in Hz. (Default: 5000)
  --dop_step <freq>       Doppler search step in Hz. (Default: 500.0)
  --tintegration <ms>     Integration time in ms. (Default: 10.0)
  --iq                    Specify if input file contains IQ data (default is I).
  --method <1|2|3>        Correlation method:
                          1: Time Domain (Brute Force)
                          2: Parallel Frequency Search
                          3: Parallel Code Search (FFT, Default)
  --device <cpu|gpu>      Execution device (default: cpu).
                          Note: GPU is only supported for method 3.
                          If method 1 or 2 is selected with --device gpu,
                          a warning will be printed and CPU will be used.
  -h, --help              Show this help message.
Example:
  ./corr_sig -f test_iq.bin --prn=5 --tint=10 --iq --method=3 --device=gpu -o test.dat

```


### 3. Захват спутников (`acq_sig`)

Выполняет поиск спутников по диапазону Доплера и кодовых фаз. Выводит в stdout таблицу параметров всех спутников. Обнаруженные помечаются статусом FOUND

```bash
$ ./acq_sig --help
GPS Signal Acquisition

Usage: ./acq_sig -f <input_file> [options]

Required arguments:
  -f, --file <path>           Input binary file (int8_t samples).

Optional arguments:
  --dop_min <freq>            Minimum Doppler search frequency in Hz. (Default: -5000)
  --dop_max <freq>            Maximum Doppler search frequency in Hz. (Default: 5000)
  --dop_step <freq>           Doppler search step in Hz. (Default: 500.0)
  --tintegration <ms>         Integration time in ms. (Default: 10.0)
  --iq                        Specify if input file contains IQ data (default is I-only).
  --method <1|2|3>            Correlation method:
                              1: Time Domain (Brute Force)
                              2: Parallel Frequency Search
                              3: Parallel Code Search (FFT, Default)
  --device <cpu|gpu>          Execution device (default: cpu).
                              Note: GPU is only supported for method 3.
                              If method 1 or 2 is selected with --device gpu,
                              a warning will be printed and CPU will be used.
  --prn <num>                 Search only specified PRN (1-37). If not specified, scan all.
  --threshold <cn0>           Detection threshold (C/N0 in dB-Hz). (Default: 35.0)
  -h, --help                  Show this help message.

Output:
  Table of detected satellites with parameters:
  PRN | Status | C/N0 (dB-Hz) | Doppler (Hz) | Code Phase (chips)
Example:
  ./acq_sig -f test_iq.bin --tint=10 --iq --method=3 --device=gpu --threshold=35
```

## Конфигурационные файлы

**`recv.cfg`** — Настройки СВЧ-тракта приёмника:
```ini
F_ADC = 8.0e6       # Частота дискретизации АЦП (Гц)
F_BW  = 2.5e6       # Полоса пропускания ВЧ-фильтра (Гц)
F_LO  = 1573.420e6  # Частота гетеродина (Гц)
IQ    = 0           # Режим: 0=только I, 1=IQ
```

**`testsig.cfg`** — Конфигурация спутников (по одной на строку):
```
# PRN  Доплер  ФазаКода  ФазаНесущей  C/N0
10   0.0       0.0        0.0        45.0
5    150.0    900.0       0.0        50.0
22   -200.0   511.5      3.14        40.0
```

## Методы корреляции

1. **Временная область (метод 1)** — последовательная корреляция полным перебором, CPU
2. **Параллельно по частоте (метод 2)** — на основе БПФ, параллелизм по ячейкам Доплера, CPU
3. **Параллельно по коду (метод 3)** — на основе БПФ, параллелизм по кодовым фазам (по умолчанию, самый быстрый), CPU/GPU

## Визуализация

Ноутбук `Plots.ipynb` предоставляет инструменты визуализации на Python 3D-карты корреляции

## Работа с GPU

В настоящий момент реализован функционал корреляции методом параллельного поиска по коду на GPU. За один запуск происходит обработка всего батча: все периоды сигнала (их количество в одном батче ограничено 10), все Допплеровские сдвиги, все спутники

Планируется тестирование и при необходимости доработка CUDA-функционала для device=Jetson (особенность в единой для CPU и GPU ОЗУ, что исключает узкое место пересылки данных)

## Тестовые данные

| Файл | Описание |
|------|----------|
| `test_i.bin` | Тестовый сигнал I-only |
| `test_iq.bin` | Тестовый сигнал IQ |
| `test_pocketsdr.bin` | Тестовые данные из [PocketSDR](https://github.com/tomojitakasu/PocketSDR/tree/master/sample)|
| `test.dat` | Результат работы corr_sig, карта корреляции |
