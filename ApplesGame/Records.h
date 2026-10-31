#pragma once
#include <string>
#include <map>

// Таблица рекордов: имя -> лучший счёт
using RecordsTable = std::map<std::string, int>;

// Файл рядом с бинарником (в рабочей директории)
constexpr const char* RECORDS_FILE = "records.dat";

// Пара функций по гайду
bool SerializeGame(const RecordsTable& table);
bool DeserializeGame(RecordsTable& table);
