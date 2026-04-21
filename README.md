# CUDA SDR — Обработка сигналов GNSS

Проект **GPU-ускоренного SDR-приёмника** для обработки сигналов GNSS (В настоящий момент: GPS L1C/A). Написан на **C** и **CUDA**, реализует пайплайн захвата GNSS-сигнала: корреляция и захват спутников. Источник данных, на данный момент, только бинарный файл записанного сигнала

Планируется развитие функционала приёмника: модули трекинга и демодуляции, получение данных с USB SDR устройства

## Особенности

- Три метода корреляции: временная область, параллельно по частоте (XF), параллельно по коду (FX)
- Аппаратное ускорение параллельной по коду корреляции с помощью CUDA
- Захват спутников с обнаружением пиков, уточнением допплеровского сдвига параболической интерполяцией и оценкой C/N0
- Поддержка форматов I-only и IQ данных (в настоящее время поддерживается только чтение uint8 значений из бинарного файла)
- Настраиваемые параметры СВЧ-тракта приёмника
- Объектно-ориентированная архитектура: структура приёмника, включающая в себя имеющийся в этом приёмнике радиоканал, а также набор блоков спутников для массивно-параллельной обработки на GPU

## TODOs

- Реализация потокового чтения данных в буфер радиоканала
- Реализация возможности работы с каким-нибудь фронтендом (PocketSDR, RTL-SDR, etc.)
- Расширение списка поддерживаемых спутниковых систем и сигналов: Galileo E1-B,C; GLONASS L1C/A, L1OC...
- Реализация универсальной работы с сигналами на разных частотах в рамках одного блока (GLONASS FDMA)
- Обобщение использования структуры радиоканала до работы с несколькими радиоканалами в рамках одного приёмника
- Реализация асинхронной обработки блока спутников: pthread+cudaStream на каждый блок
- Реализация универсальной работы с разными периодами кода в рамках одного блока (?)
- Разработка алгоритма оптимального распределения спутников по блокам (да и вообще критериев такой оптимальности...)
- Трекинг... 

## Структура проекта

```
CUDA_SDR/
├── app/                  # Исходный код приложений
│   ├── corr_sig.c        # Коррелятор сигналов
│   └── acq_sig.c         # Полноценный модуль захвата
├── src/                  # Основные библиотеки
│   ├── core/             # Утилиты, типы, параметры, инициализация GPU, высокоуровневый интерфейс приёмника целиком
|   ├── code/             # Генерация PRN-кодов, получение характеристик конкретного сигнала конкретной спутниковой системы
│   ├── dsp/              # Операции над PRN-кодами, и комплекснозначным сигналом (CPU + CUDA)
│   ├── correlator/       # Интерфейс и функции корреляции
│   ├── acquisition/      # Обнаружение пиков, оценка C/N0
│   └── io/               # Ввод/вывод, парсинг конфигурации
├── build/                # Директория сборки CMake
├── CMakeLists.txt        # Конфигурация сборки
├── recv.cfg              # Конфигурация CВЧ-тракта приёмника
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

Исполняемые файлы создаются в **корневой директории проекта**: `corr_sig`, `acq_sig`.

## CLI-утилиты

### 1. Коррелятор сигналов (`corr_sig`)

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
  --verbose               Print detailed information.
  --method <1|2|3>        Correlation method:
                          1: Time Domain (Brute Force)
                          2: Parallel Frequency Search
                          3: Parallel Code Search (FFT, Default)
  --device <cpu|gpu>      Execution device (default: cpu).
                          Note: GPU is only supported for method 3.
                          If method 1 or 2 is selected with --device gpu,
                          a warning will be printed and method 3 will be used.
  -h, --help              Show this help message.
Example:
  ./corr_sig -f test_iq.bin --prn=5 --tint=10 --verbose --method=3 --device=gpu -o test.dat
```


### 2. Захват спутников (`acq_sig`)

Выполняет поиск спутников по диапазону Доплера и кодовых фаз. Выводит в stdout таблицу параметров всех спутников. Обнаруженные помечаются статусом FOUND

```bash
$ ./acq_sig --help
GPS Signal Acquisition

Usage: ./acq_sig -f <input_file> [options]

Required arguments:
  -f, --file <path>           Input binary file (int8_t samples).

Optional arguments:
  --sys <system>              Satellite system. (Default: GPS)
  --sig <signal>              Satellite signal. (Default: L1CA)
  --dop_min <freq>            Minimum Doppler search frequency in Hz. (Default: -5000)
  --dop_max <freq>            Maximum Doppler search frequency in Hz. (Default: 5000)
  --dop_step <freq>           Doppler search step in Hz. (Default: 500.0)
  --tintegration <ms>         Integration time in ms. (Default: 10.0)
  --device <cpu|gpu>          Execution device (default: cpu).
                              Note: GPU is only supported for method 3.
                              If method 1 or 2 is selected with --device gpu,
                              a warning will be printed and method 3 will be used.
  --prn <num>                 Search only specified PRN (1-37). If not specified, scan all.
  --threshold <cn0>           Detection threshold (C/N0 in dB-Hz). (Default: 35.0)
  -h, --help                  Show this help message.

Output:
  Table of detected satellites with parameters:
  PRN | Status | C/N0 (dB-Hz) | Doppler (Hz) | Code Phase (chips)
Example:
  ./acq_sig -f test_iq.bin --tint=10 --device=gpu --threshold=35
```

## Конфигурационные файлы

**`recv.cfg`** — Настройки радиоканала приёмника (в настоящий момент поддерживается только один):
```ini
F_ADC = 8.0e6       # Частота дискретизации АЦП (Гц)
F_BW  = 2.5e6       # Полоса пропускания ВЧ-фильтра (Гц)
F_LO  = 1573.420e6  # Частота гетеродина (Гц)
IQ    = 0           # Режим: 0=только I, 1=IQ
```

## Методы корреляции

1. **Временная область (метод 1)** — последовательная корреляция полным перебором, CPU
2. **Параллельно по частоте (метод 2)** — на основе БПФ, параллелизм по ячейкам Доплера, CPU
3. **Параллельно по коду (метод 3)** — на основе БПФ, параллелизм по кодовым фазам (по умолчанию, самый быстрый), CPU/GPU

## Визуализация

Ноутбук `Plots.ipynb` предоставляет инструменты визуализации на Python 3D-карты корреляции

## Работа с GPU

В настоящий момент реализован функционал корреляции методом параллельного поиска по коду на GPU. За один запуск происходит обработка всего батча: все периоды сигнала, все Допплеровские сдвиги, все спутники. При запуске происходит инициализация всех буферов в блоке спутников, в дальнейшем эти данные (FFT всех кодов, шаги фазы для каждого допплеровского сдвига) переиспользуются

## Тестовые данные

| Файл | Описание |
|------|----------|
| `test_i.bin` | Тестовый сигнал I-only |
| `test_iq.bin` | Тестовый сигнал IQ |
| `test.dat` | Результат работы corr_sig, карта корреляции |
