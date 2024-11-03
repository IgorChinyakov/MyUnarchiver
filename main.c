#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 1024

// Структура для хранения информации о файле
typedef struct {
    char file_name[256];
    long file_size;
} FileHeader;

// Функция для получения текущего времени в формате "YYYY-MM-DD HH:MM:SS"
void get_current_time(char *buffer, size_t size) {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", timeinfo);
}

// Функция для добавления даты архивации в текстовый файл
void append_extract_date(const char *file_path) {
    char date_buffer[64];
    get_current_time(date_buffer, sizeof(date_buffer));

    // Открываем файл для добавления данных в конец
    FILE *file = fopen(file_path, "a");
    if (file == NULL) {
        perror("fopen");
        return;
    }

    // Добавляем строку с текущей датой в конец файла
    fprintf(file, "\n[Extracted on: %s]\n", date_buffer);
    fclose(file);
}

// Функция для создания директорий, если они не существуют
void create_directories(const char *file_path) {
    char path[512];
    strncpy(path, file_path, sizeof(path));

    // Идем по каждому символу пути и создаем директории по мере необходимости
    for (char *p = path; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            mkdir(path, 0755);  // Создаем директорию, если она не существует
            *p = '/';
        }
    }
}

// Функция для извлечения файлов из архива
void extract_archive(const char *archive_path, const char *output_dir) {
    FILE *archive = fopen(archive_path, "rb");
    if (archive == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    FileHeader header;
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    char output_file_path[512];
    FILE *output_file;

    // Читаем заголовки файлов и их содержимое
    while (fread(&header, sizeof(header), 1, archive) == 1) {
        // Создаем полный путь для файла, включая выходную директорию
        snprintf(output_file_path, sizeof(output_file_path), "%s/%s", output_dir, header.file_name);

        // Создаем необходимые директории
        create_directories(output_file_path);

        // Открываем файл для записи
        output_file = fopen(output_file_path, "wb");
        if (output_file == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        // Читаем и записываем содержимое файла
        long remaining_size = header.file_size;
        while (remaining_size > 0) {
            size_t to_read = remaining_size < BUFFER_SIZE ? remaining_size : BUFFER_SIZE;
            bytes_read = fread(buffer, 1, to_read, archive);
            fwrite(buffer, 1, bytes_read, output_file);
            remaining_size -= bytes_read;
        }

        fclose(output_file);

        // Проверяем, является ли файл текстовым (по расширению .txt)
        const char *ext = strrchr(header.file_name, '.');
        if (ext != NULL && strcmp(ext, ".txt") == 0) {
            // Добавляем текущую дату в текстовый файл
            append_extract_date(output_file_path);
        }
    }

    fclose(archive);
    printf("Извлечение завершено. Файлы сохранены в директории %s\n", output_dir);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <archive_file> <output_directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *archive_file = argv[1];
    const char *output_directory = argv[2];

    // Извлекаем архив в указанную директорию
    extract_archive(archive_file, output_directory);

    return EXIT_SUCCESS;
}
