#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// windowSize = Размер словаря
// bufferSize = Размер буфера
// Важно: размер буфера < размер окна < 255!
#define windowSize 250
#define bufferSize 200
#define arraySize bufferSize + windowSize

typedef enum { false, true } bool;

// ============================================================================

// Список функций
int findMatch(unsigned char window[], unsigned char str[], int strLen);
int compress(char* inputPath);
int decompress(char* inputPath);
int merge(int f_count, char* file_list);
int separate(char* file_path);

// ============================================================================

// В основной функции определяем, что хочет от программы пользователь
// и перенаправляем на вспомогательные функции

int main() {
    Programm_Start:
    char act_code = 0;
    char* files;
    int file_count = 0;
    int result = 0;
    files = (char*)calloc(30, sizeof(char));
    //printf("Need action code\nc - compress\nd - decompress\nm - create archive\ns - unpack archive\n");
    printf("Need action code\nm - create archive\ns - unpack archive\n");
    scanf("%c", &act_code);
    switch (act_code) {
        case 'c':
            printf("Need target file name\n");
            files = (char*)calloc(30, sizeof(char));
            scanf("%s", files);
            break;
        case 'd':
            printf("Need file name\n");
            files = (char*)calloc(30, sizeof(char));
            scanf("%s", files);
            break;
        case 'm':
            printf("Needs target files count and names\n");
            scanf("%d\n", &file_count);
            files = (char*)calloc(30 * file_count, sizeof(char));
            gets(files);
            break;
        case 's':
            printf("Need name of archive\n");
            files = (char*)calloc(30, sizeof(char));
            scanf("%s", files);
            break;
        default:
            printf("Invalid arguments");

    }

    clock_t begin = clock();

    switch (act_code) {
        case 'c':
            result = compress(files);
            if (result == 0) {
                fprintf(stderr, "\nCompression FAIL\n");
            }
            else if (result == 1) {
                printf("\nCompression OK");
            }
            else if (result == 2) {
                fprintf(stderr, "\nFile too small\n");
            }
            else if (result == 3) {
                fprintf(stderr, "\nFile is EMPTY\n");
            }
            break;

        case 'd':
            result = decompress(files);
            if (result == 0) {
                fprintf(stderr, "\nDecompression FAIL");
            }
            else if (result == 1) {
                printf("\nDecompression OK");
            }
            break;
        
        case 'm':
            merge(file_count, files);
            break;

        case 's':
            separate(files);
            break;
    }
    

    free(files);
    // Время выполнения
    clock_t end = clock();
    printf("\n\nExecution time: ");
    printf("%f", ((double)(end - begin) / CLOCKS_PER_SEC));
    printf(" [seconds]\n\n");
    printf("Do you need anything else? [y/n]\n");
    if (getch() == 'y') {
        getchar();
        system("cls");
        goto Programm_Start;
    }
    return 0;
}

// ============================================================================

// Эта функция ищет совпадение из str[] в window[] длины strLen. 
// Возвращает позицию совпадения, начиная с начала window[], или -1, если
// совпадений не найдено. Вызывается во время каждой итерации алгоритма сжатия.

int findMatch(unsigned char window[], unsigned char str[], int strLen) {
    int j, k, pos = -1;

    for (int i = 0; i <= windowSize - strLen; i++) {
        pos = k = i;

        for (j = 0; j < strLen; j++) {
            if (str[j] == window[k])
                k++;
            else
                break;
        }
        if (j == strLen)
            return pos;
    }

    return -1;
}

// ============================================================================

// Функция содержит логику алгоритма сжатия. Вызывается, когда 
// в команде запуска указана опция "c", за которой следует имя файла.

int compress(char* inputPath) {
    FILE* fileInput;
    FILE* fileOutput;
    bool last = false;
    int inputLength = 0;
    int outputLength = 0;
    int endOffset = 0;
    int pos = -1;
    int i, size, shift, c_in;
    size_t bytesRead = (size_t)-1;
    unsigned char c;
    unsigned char array[arraySize];
    unsigned char window[windowSize];
    unsigned char buffer[bufferSize];
    unsigned char loadBuffer[bufferSize];
    unsigned char str[bufferSize];
    char arch_file_name[30];

    printf("Need name of archive\n");
    scanf("%s", arch_file_name);
    strcat(arch_file_name, ".uwu");

    // Открытие файлов
    char path[30] = "";
    strcat(path, inputPath);
    fileInput = fopen(path, "rb");
    fileOutput = fopen(arch_file_name, "wb");

    // Если невозможно открыть - возврат ошибки
    if (!fileInput) {
        fprintf(stderr, "Unable to open fileInput %s", inputPath);
        return 0;
    }

    // Определение длины входного файла
    fseek(fileInput, 0, SEEK_END);
    inputLength = ftell(fileInput);
    fseek(fileInput, 0, SEEK_SET);

    fprintf(stdout, "Input file size: %d bytes", inputLength);

    // Если файл пустой - возврат ошибки
    if (inputLength == 0)
        return 3;
    
    // Если длина файла меньше, чем arraySize, не стоит обрабатывать
    if (inputLength < arraySize)
        return 2;
    
    // Загрузка массива с начальными байтами
    fread(array, 1, arraySize, fileInput);
    
    // Запись первых байт в выходной файл
    fwrite(array, 1, windowSize, fileOutput);
    
    // логика LZ77
    while (true) {
        if ((c_in = fgetc(fileInput)) == EOF)
            last = true;
        else
            c = (unsigned char)c_in;

        // Загрузка окна (словарь)
        for (int k = 0; k < windowSize; k++)
            window[k] = array[k];

        // Загрузка буфера (предпросмотр)
        for (int k = windowSize, j = 0; k < arraySize; k++, j++) {
            buffer[j] = array[k];
            str[j] = array[k];
        }

        // Поиск самого длинного совпадения в окне
        if (endOffset != 0) {
            size = bufferSize - endOffset;
            if (endOffset == bufferSize)
                break;
        }
        else {
            size = bufferSize;
        }

        pos = -1;
        for (i = size; i > 0; i--) {
            pos = findMatch(window, str, i);
            if (pos != -1)
                break;
        }

        // Совпадение не найдено
        // Запись только одного байта вместо двух
        // 255 -> offset = 0, match = 0
        if (pos == -1) {
            fputc(255, fileOutput);
            fputc(buffer[0], fileOutput);
            shift = 1;
        }
        // Совпадение найдено
        // offset = windowSize - position of match
        // i = количество байтов соответствия
        // endOffset = количество байтов в опережающем буфере, которое не следует учитывать (EOF)
        else {
            fputc(windowSize - pos, fileOutput);
            fputc(i, fileOutput);
            if (i == bufferSize) {
                shift = bufferSize + 1;
                if (!last)
                    fputc(c, fileOutput);
                else
                    endOffset = 1;
            }
            else {
                if (i + endOffset != bufferSize)
                    fputc(buffer[i], fileOutput);
                else
                    break;
                shift = i + 1;
            }
        }

        // Буферы сдвига
        for (int j = 0; j < arraySize - shift; j++)
            array[j] = array[j + shift];
        if (!last)
            array[arraySize - shift] = c;

        if (shift == 1 && last)
            endOffset++;

        // Если (shift != 1) -> прочитать больше байтов из файла
        if (shift != 1) {
            // Загружаем loadBuffer новыми байтами
            bytesRead = fread(loadBuffer, 1, (size_t)shift - 1, fileInput);

            // Загружаем массив новыми байтами
            // Сдвинуть байты в массиве, а затем разделить на window[] и buffer[] во время следующей итерации
            for (int k = 0, l = arraySize - shift + 1; k < shift - 1; k++, l++)
                array[l] = loadBuffer[k];

            if (last) {
                endOffset += shift;
                continue;
            }

            if (bytesRead < shift - 1)
                endOffset = shift - 1 - bytesRead;
        }
    }

    // Получение длины выходного файла
    fseek(fileOutput, 0, SEEK_END);
    outputLength = ftell(fileOutput);
    fseek(fileOutput, 0, SEEK_SET);

    fprintf(stdout, "\nOutput file size: %d bytes\n", outputLength);

    // Закрытие файлов
    fclose(fileInput);
    fclose(fileOutput);

    return 1;
}

// ============================================================================

// Функция содержит логику обратного алгоритма, используемого для распаковки.
// Вызывается, когда в команде запуска указана опция "d".

int decompress(char* inputPath) {
    FILE* fileInput;
    FILE* fileOutput;
    int shift = 0, offset = 0, match = 0, c_in = 0;
    bool done = false;
    char path[30] = "";
    unsigned char c = 0;
    unsigned char window[windowSize];
    unsigned char writeBuffer[windowSize];
    unsigned char readBuffer[2];

    // Открытие файлов
    strcat(path, inputPath);
    strcat(path, ".uwu");
    fileInput = fopen(path, "rb");
    fileOutput = fopen("file.utmp", "wb");

    if (!fileInput) {
        fprintf(stderr, "Unable to open fileInput %s", "archive.uwu");
        return 0;
    }

    // Загрузить массив с начальными байтами и записать в файл
    fread(window, 1, windowSize, fileInput);
    fwrite(window, 1, windowSize, fileOutput);

    // Инвертированный алгоритм
    while (true) {
        // Чтение файла парами/триадами для восстановления исходного файла
        size_t bytesRead = fread(readBuffer, 1, 2, fileInput);

        if (bytesRead >= 2) {
            offset = (int)readBuffer[0];
            match = (int)readBuffer[1];

            // Если первый байт readBuffer равен 255 -> offset = 0, match = 0
            if (offset == 255) {
                offset = 0;
                c = (unsigned char)match;
                match = 0;
                shift = match + 1;
            }
            else {
                shift = match + 1;
                c_in = fgetc(fileInput);
                if (c_in == EOF)
                    done = true;
                else
                    c = (unsigned char)c_in;
            }

            // Загружаем и записываем вхождение в файл
            for (int i = 0, j = windowSize - offset; i < match; i++, j++)
                writeBuffer[i] = window[j];
            fwrite(writeBuffer, 1, (size_t)match, fileOutput);

            if (!done)
                fputc(c, fileOutput);

            // Сдвиг окна
            for (int i = 0; i < windowSize - shift; i++)
                window[i] = window[i + shift];

            for (int i = 0, j = windowSize - shift; i < match; i++, j++)
                window[j] = writeBuffer[i];
            window[windowSize - 1] = c;
        }
        else {
            break;
        }
    }

    // Закрытие файлов
    fclose(fileInput);
    fclose(fileOutput);
    return 1;
}

// ============================================================================

// Функция содержит логику упаковки файлов и информации о них в один для
// последующего сжатия. Вызывается, когда в команде запуска указана опция "m".

int merge(int f_count, char* file_list) {
    FILE* fileInput;
    FILE* fileOutput;
    char elem;
    char* filename;
    char slc[3] = " \n";
    long int inputLength = 0;
    //временный файл, для последующего сжатия
    fileOutput = fopen("temp.mrg", "wb");

    //первая информация - кол-во файлов в архиве
    fwrite(&f_count, sizeof(int), 1, fileOutput);

    //поочерёдно отделяем имена фалов от строки
    filename = strtok(file_list, slc);
    while (filename != NULL) {
        char path[30] = "";
        char* file_buffer;
        strcat(path, filename);
        fileInput = fopen(filename, "rb");
        //запись имени файла в совмещенный
        fwrite(path, sizeof(path), 1, fileOutput);
        // Определение длины входного файла
        fseek(fileInput, 0, SEEK_END);
        inputLength = ftell(fileInput);
        fseek(fileInput, 0, SEEK_SET);
        //запись размера файла в совмещенный
        fwrite(&inputLength, sizeof(long int), 1, fileOutput);
        //запись данных файла в совмещенный
        file_buffer = (char*)calloc(inputLength, sizeof(char));
        fread(file_buffer, inputLength, 1, fileInput);
        fwrite(file_buffer, inputLength, 1, fileOutput);

        free(file_buffer);
        fclose(fileInput);
        filename = strtok(NULL, slc);
    }
    
    fclose(fileOutput);
    //превращаем временный файл в архив а затем удаляем его
    compress("temp.mrg");
    remove("temp.mrg");
}

// ============================================================================

// Функция содержит логику разделения декодированного архива на составляющие.
// Вызывается, когда в команде запуска указана опция "s".

int separate(char* file_path) {
    FILE* fileInput;
    FILE* fileOutput;
    char filename[30];
    long int inputLength = 0;
    int f_count;
    char path[30] = "";

    strcat(path, file_path);
    strcat(path, ".uwu");
    //декодирование архива
    decompress(file_path);

    //чтение кол-ва файлов
    fileInput = fopen("file.utmp", "rb");
    fread(&f_count, sizeof(int), 1, fileInput);

    for (int i = 0; i < f_count; i++) {
        char* file_buffer;
        //чтение имени и размера
        fread(&filename, 30, 1, fileInput);
        fread(&inputLength, sizeof(long int), 1, fileInput);
        file_buffer = (char*)calloc(inputLength, sizeof(char));
        //чтение данных в буфер
        fread(file_buffer, inputLength, 1, fileInput);
        fileOutput = fopen(filename, "wb");
        //запись даннх в файл заданного имени
        fwrite(file_buffer, inputLength, 1, fileOutput);
        fclose(fileOutput);
        free(file_buffer);
    }

    fclose(fileInput);
    //удаление лишних файлов
    remove("file.utmp");
    remove(path);
}